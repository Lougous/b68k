//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// RFS remote server
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>

#define PAGE_SIZE 64

struct termios tio;
int tty_fd;

int serial_init(char *dev) {

  tty_fd=open(dev, O_RDWR | O_NOCTTY);

  if (tty_fd == -1) {
    printf("failed to open device: %s\n", dev);
    return -1;
  }
  
  memset(&tio,0,sizeof(tio));
  tio.c_iflag=0;  //IGNBRK; //
  tio.c_oflag=0;
  tio.c_cflag=CS8|CREAD|CLOCAL;
  tio.c_lflag=0;
  tio.c_cc[VMIN]=0;
  tio.c_cc[VTIME]=5;
  
  cfsetospeed(&tio,B115200);            // 115200 baud
  cfsetispeed(&tio,B115200);            // 115200 baud
  //cfsetospeed(&tio,B9600);
  //cfsetispeed(&tio,B9600);
  
  if (tcsetattr(tty_fd,TCSANOW,&tio) == -1) {
    printf("failed to configure serial port\n");
  }

  return tty_fd;
}

void serial_send(char c, int fd) {
  int nb = write(fd, &c, 1);

  if (nb != 1) {
    printf("\nerror: nb sent is not 1 (%d)\n", nb);
  }
}

void send_msg(unsigned char *msg, int len)
{
  unsigned char sum = 0;
  
  serial_send(0x55, tty_fd);
  serial_send((unsigned char) len, tty_fd);

  while (len--) {
    serial_send(*msg, tty_fd);
    sum += *msg++;
  }

  serial_send(sum, tty_fd);
  serial_send(0xAA, tty_fd);
}

int serial_recv(int fd, char *c) {
  int nb = read(fd, c, 1);
  
  if (nb < 0) {
    printf("error: read returned %d\n", nb);
    return -1;
  }

  return nb;
}

void usage()
{
  printf("rfsd [-s dev] [-d] path\n");
  printf("options:\n");
  printf("  -s dev    select device for serial\n");
  printf("  -v[v[v]]  enable verbose mode\n");
}

typedef enum { F_SOF,
	       F_LEN,
	       F_CMD,
	       F_CKS,
	       F_EOF
} fsm_t;

#define FILE_CNT  256

#define F_RDONLY        00000001
#define F_WRONLY        00000002
#define F_RDWR          00000003
#define F_DIRECTORY     00000004

typedef struct {
  char flags;
  
  union desc {
    FILE *file;
    DIR *dir;
  } desc;
} desc_t;

desc_t _open_files[FILE_CNT];
char *_path = 0;

int DEBUG = 0;

void do_cmd (unsigned char *cmd, int len)
{
  int f;
  unsigned char resp[256];
  char filename[256];
  ssize_t rcnt;
  off_t offset;
  char flags;
  
  switch (cmd[0]) {
  case 'I':
    /* init */
    if (DEBUG) printf("I: (re)initialize\n");
    
    for (f = 0; f < FILE_CNT; f++) {
      if (_open_files[f].flags == F_DIRECTORY) {
	closedir(_open_files[f].desc.dir);
      } else if (_open_files[f].flags) {
	fclose(_open_files[f].desc.file);
      }
      
      _open_files[f].flags = 0;
      _open_files[f].desc.file = NULL;
    }
    
    resp[0] = 1;
    send_msg(resp, 1);
    break;

  case 'D':
    /* getdents */
    cmd++; len--;

    {
      int fd = cmd[0];
      int count = cmd[1];
      
      if (DEBUG) printf("I: getdents fd-%d (%i bytes)\n", fd, count);

      struct dirent *de = readdir(_open_files[fd].desc.dir);

      if (de) {
	resp[0] = 0;
	if (de->d_type == DT_REG) resp[0] = 1;
	if (de->d_type == DT_DIR) resp[0] = 2;

	strncpy((char *)&resp[1], de->d_name, count-2);
	
      } else {
	// last entry
	resp[0] = 0;
	resp[1] = 0;
      }

      send_msg(resp, count+1);
    }
      
    break;
    
  case 'O':
    /* open */
    cmd++; len--;

    flags = *cmd;

    cmd++; len--;

    cmd[len] = 0;

    sprintf(filename, "%s/%s", _path, cmd);

    if (DEBUG) printf("I: open '%s', flags %Xh\n", filename, flags);
      
    for (f = 0; f < FILE_CNT; f++) {
      if (_open_files[f].flags == 0) break;
    }

    if (f == FILE_CNT) {
      /* out of ressources */
      printf("E: out of ressources\n");
      resp[0] = 0;
      resp[1] = 0;
    } else {
      int size = 0;
      
      if (flags == F_DIRECTORY) {
	DIR *dir = opendir((const char *)filename);

	if (dir) {
	  _open_files[f].flags = F_DIRECTORY;
	  _open_files[f].desc.dir = dir;
	}
      } else {
	FILE *file = fopen((const char *)filename, "rb");

	if (file) {
	  _open_files[f].flags = F_RDONLY;
	  _open_files[f].desc.file = file;

	  fseek(_open_files[f].desc.file, 0, SEEK_END);
	  size = ftell(_open_files[f].desc.file);
	  fseek(_open_files[f].desc.file, 0, SEEK_SET);
	}
      }
      
      if (_open_files[f].flags) {
	if (DEBUG) printf("I: open fd is %i\n", f);
	resp[0] = 1;
	resp[1] = f;
	resp[2] = (unsigned char)(size & 0xff);
	resp[3] = (unsigned char)((size >> 8) & 0xff);
	resp[4] = (unsigned char)((size >> 16) & 0xff);
	resp[5] = (unsigned char)((size >> 24) & 0xff);
      } else {
	printf("E: cannot open file\n");
	resp[0] = 0;
	resp[1] = 0;
	resp[2] = 0;
	resp[3] = 0;
	resp[4] = 0;
	resp[5] = 0;
      }
    }

    send_msg(resp, 6);
    break;

  case 'C':
    /* close */
    cmd++; len--;

    cmd[len] = 0;
    if (DEBUG) printf("I: close fd-%u\n", cmd[0]);

    if (_open_files[cmd[0]].flags == 0) {
      resp[0] = 0;
    } else if (_open_files[cmd[0]].flags == F_DIRECTORY) {
      closedir(_open_files[cmd[0]].desc.dir);
      _open_files[cmd[0]].flags = 0;
      _open_files[cmd[0]].desc.dir = NULL;
      resp[0] = 1;
    } else {
      fclose(_open_files[cmd[0]].desc.file);
      _open_files[cmd[0]].flags = 0;
      _open_files[cmd[0]].desc.file = NULL;
      resp[0] = 1;      
    }

    send_msg(resp, 1);
    break;

  case 'R':
    /* read */
    cmd++; len--;

    offset =
      ((off_t)cmd[2]) +
      (((off_t)cmd[3]) << 8) +
      (((off_t)cmd[4]) << 16) +
      (((off_t)cmd[5]) << 24);

    memset(resp, 0, cmd[1]);
    
    if (DEBUG > 1) printf("I: read fd-%i@%Xh len=%u \n", cmd[0], (unsigned int)offset, cmd[1]);

    FILE *fdes = _open_files[cmd[0]].desc.file;
    
    fseek(fdes, offset, SEEK_SET);
    rcnt = fread(resp + 1, 1, cmd[1], fdes);

    if (rcnt >= 0) {
      resp[0] = rcnt;
    } else {
      resp[0] = 0xff;
    }    

    send_msg(resp, cmd[1]+1);
    
    break;
    
  default:
    printf("E: unsupported command ('%c')\n", cmd[0]);
    break;
  }


}

int main (int argc, char *argv[]) {
  char *sdev = 0;
  int tty_fd;

  while (argc > 1) {
    if (strcmp("--help", argv[1]) == 0) {
      usage();
      return 0;
    } else if (strcmp("-v", argv[1]) == 0) {
      argc--; argv++;

      DEBUG = 1;
    } else if (strcmp("-vv", argv[1]) == 0) {
      argc--; argv++;

      DEBUG = 2;
    } else if (strcmp("-vvv", argv[1]) == 0) {
      argc--; argv++;

      DEBUG = 3;
    } else if (strcmp("-s", argv[1]) == 0) {
      if (argc < 3) {
	printf("error: missing argument\n");
	return -1;
      }
      argc--; argv++;

      sdev = argv[1];
      argc--; argv++;
    } else {
      if (!_path) {
	_path = argv[1];
      } else {
	printf("error: multiple paths.\n");
	usage();
	return -1;
      }
      argc--; argv++;
    }
  }
  
  if (!sdev) {
    sdev = "/dev/ttyS0";
  }

  /* input file to program */
  if (!_path) {
    printf("error: no path\n");
    usage();
    return -1;
  }

  /* setup serial link */
  tty_fd = serial_init(sdev);

  if (tty_fd <= 0) {
    printf("error: cannot access device %s\n", sdev);
    return 0;
  } else {
    printf("using device %s\n", sdev);
  }


  /* get command */
  unsigned char buf[256];
  unsigned char cks;
  int len;
  fsm_t framer_fsm = F_SOF;
  int f;

  for (f = 0; f < FILE_CNT; f++) {
    _open_files[f].flags = 0;
    _open_files[f].desc.file = NULL;
  }
  
  /* structure: SOF (0x55) LEN [command] CKS EOF (0xAA) */
  while (1) {

    switch (framer_fsm) {
    case F_SOF:
      len = 0;
      
      if ((read(tty_fd, buf, 1) == 1) &&
	  (buf[0] == 0x55)) {
	framer_fsm = F_LEN;

	if (DEBUG >= 3) printf("55h ");
    
      }
      break;

    case F_LEN:
      if (read(tty_fd, buf+1, 1) == 1) {
	framer_fsm = F_CMD;
	
	if (DEBUG >= 3) printf("%Xh [ ", buf[1]);
      } else {
	framer_fsm = F_SOF;
      }
      break;

    case F_CMD:
      while (len < buf[1]) {
	int nlen = read(tty_fd, buf+2+len, buf[1]-len);

	if (DEBUG >= 3) {
	  int i;

	  for (i = 0; i < nlen; i++) {
	    printf("%Xh ", buf[2+len+i]);
	  }
	}
	  
	if (nlen < 0) {
	  printf("E: communication error\n");
	  framer_fsm = F_SOF;
	  break;
	} else {
	  len += nlen;
	}
      }

      if (DEBUG >= 3) printf("] ");
      
      framer_fsm = F_CKS;
      break;

    case F_CKS:
      if (read(tty_fd, buf+2+len, 1) == 1) {
	int i;
	cks = 0;

	if (DEBUG >= 3) printf("%Xh ", buf[2+len]);

	for (i = 2; i < (len+2); i++) {
	  cks += buf[i];
	}

	if (DEBUG >= 3) printf("(%Xh) ", cks);

	if (cks != buf[2+len]) {
	  /* bad checksum */
	  printf("E: rejected command (bad checksum)\n");
	  framer_fsm = F_SOF;
	  break;
	}

	framer_fsm = F_EOF;
      } else {
	printf("E: communication error\n");
	framer_fsm = F_SOF;
      } 

      break;
	  
    case F_EOF:
      if (read(tty_fd, buf, 1) == 1) {
	if (buf[0] == 0xAA) {
	  /* valid command ! */
	  if (DEBUG >= 3) printf("AAh\n");
	  do_cmd(buf + 2, len);
	} else {
	  printf("E: rejected command (bad EOF)\n");
	}
      } else {
	printf("E: communication error\n");
      }	  

      framer_fsm = F_SOF;
      break;

    default:
      break;
    }
  }

  return 0;
}
