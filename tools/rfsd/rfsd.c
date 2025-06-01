//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// RFS remote server
//

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

typedef uint32_t u32_t;
typedef uint8_t u8_t;
#include "../../software/system/include/sys/rfs.h"

#define PAGE_SIZE 64

struct termios tio;
int _tty_fd;

void serial_init(char *dev) {

  _tty_fd=open(dev, O_RDWR | O_NOCTTY);

  if (_tty_fd == -1) {
    printf("failed to open device: %s\n", dev);
    return;
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
  
  if (tcsetattr(_tty_fd,TCSANOW,&tio) == -1) {
    printf("failed to configure serial port\n");
  }

  return;
}

static void _send_char (u8_t c)
{
  int nb = write(_tty_fd, &c, 1);

  if (nb != 1) {
    printf("\nerror: nb sent is not 1 (%d)\n", nb);
  }  
}

static void _send_msg(u8_t *msg, u8_t len)
{
  _send_char(0x55);
  _send_char(len);

  u8_t csum = 0;
  
  while (len--) {
    csum += *msg;
    _send_char(*msg++);
  }

  _send_char(csum);
  _send_char(0xAA);
}

int serial_recv(char *c) {
  int nb = read(_tty_fd, c, 1);
  
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

struct desc {
  struct desc *next;
  
  union {
    FILE *file;
    DIR *dir;
  } open;

  struct stat stat;

  char *pathname;
};

struct desc *_descriptors = NULL;
char *_path = NULL;

int DEBUG = 0;

struct desc *_find_by_node(rnode_t rnode)
{
  struct desc *pd = _descriptors;

  while (pd) {
    if (pd->stat.st_ino == rnode) {
      return pd;
    }

    pd = pd->next;
  }

  return NULL;
}

void do_cmd (unsigned char *cmd)
{
  if (_descriptors == NULL && cmd[0] != 'I') {
    printf("E: ignoring command %c\n", cmd[0]);
    return ;
  }
  
  switch (cmd[0]) {
  case 'I':
    /* init */
    if (DEBUG) printf("C: (re)initialize\n");

    struct desc *pd = _descriptors;

    while (pd) {
      if ((pd->stat.st_mode & S_IFMT) == S_IFDIR && pd->open.dir) {
	closedir(pd->open.dir);
      } else if (pd->open.file) {
	fclose(pd->open.file);
      }

      if (pd->pathname) {
	free(pd->pathname);
      }

      struct desc *pdfree = pd;

      pd = pd->next;
      free(pdfree);
    }

    _descriptors = NULL;

    // 
    _descriptors = (struct desc *)malloc(sizeof(struct desc));
    _descriptors->next = NULL;

    u8_t areset[RFS_PAYLOAD_LEN(sizeof(rfs_areset_t))] = { 0 };
    areset[0] = 'I';
    
    if (stat(_path, &_descriptors->stat) != 0) {
      printf("E: cannot stat root directory (%s)\n", _path);
      free(_descriptors);
      _descriptors = NULL;
      
      _send_msg(areset, sizeof(areset));
      break;
    }

    if ((_descriptors->stat.st_mode & S_IFMT) != S_IFDIR) {
      printf("E: root directory (%s) is no actual directory (\n", _path);
      free(_descriptors);
      _descriptors = NULL;
      
      _send_msg(areset, sizeof(areset));
      break;
    }
      
    _descriptors->open.dir = NULL;
    _descriptors->pathname = strdup(_path);
      
    rfs_u32le_at(areset+1, _descriptors[0].stat.st_ino);
    _send_msg(areset, sizeof(areset));

    if (DEBUG) printf("I: root directory inode is %Xh\n", (uint32_t)_descriptors[0].stat.st_ino);
    
    break;

  case 'L':
    // lookup
    {
      u8_t alookup[RFS_PAYLOAD_LEN(sizeof(rfs_alookup_t))] = { 0 };
      alookup[0] = 'l';
      
      rnode_t rnode = rfs_read_u32le(cmd+1);
      //u8_t pid = cmd[5];
      char *name = (char *)(cmd+6);

      if (DEBUG) printf("C: looking up for %s\n", name);

      // find descriptor for this node
      struct desc *pdesc = _find_by_node(rnode);

      if (! pdesc) {
	printf("E: invalid node %Xh (looking up for %s)\n", rnode, name);
	_send_msg(alookup, sizeof(alookup));
	break;
      }

      if ((pdesc->stat.st_mode & S_IFMT) != S_IFDIR) {
	printf("E: looking up in node %Xh that is no directory (looking up for %s)\n", rnode, name);
	_send_msg(alookup, sizeof(alookup));
	break;
      }

      // build path
      char buf[256];

      snprintf(buf, sizeof(buf), "%s/%s", pdesc->pathname, name);

      struct desc *pchild = (struct desc *)malloc(sizeof(struct desc));
    
      if (stat(buf, &pchild->stat) == 0) {
	pchild->open.dir = NULL;
	pchild->pathname = strdup(buf);
	pchild->next = _descriptors;

	_descriptors = pchild;

	u8_t *pc = alookup + 1;
	pc = rfs_u32le_at(pc, pchild->stat.st_ino);
	*pc++ = (pchild->stat.st_mode & S_IFMT) == S_IFDIR ? RFS_TYPE_DIR : RFS_TYPE_REG;
	
	_send_msg(alookup, sizeof(alookup));
	if (DEBUG) printf("I: file exists with node %Xh\n", (uint32_t)pchild->stat.st_ino);
      } else {
	if (DEBUG) printf("I: file does not exist\n");
	_send_msg(alookup, sizeof(alookup));
      }
    }
    break;
    
  case 'O':
    // open
    {
      u8_t aopen[RFS_PAYLOAD_LEN(sizeof(rfs_aopen_t))] = { 0 };
      aopen[0] = 'o';
      
      rnode_t rnode = rfs_read_u32le(cmd+1);
      u8_t flags = cmd[5];
      //u8_t pid = buf[6];
      
      if (DEBUG > 1) printf("C: open node %Xh\n", rnode);

      // find descriptor for this node
      struct desc *pdesc = _find_by_node(rnode);
      
      if (! pdesc) {
	printf("E: invalid node %Xh (open)\n", rnode);
	_send_msg(aopen, sizeof(aopen));
	break;
      }

      u32_t size = 0;

      if (flags == F_DIRECTORY) {
	pdesc->open.dir = opendir((const char *)pdesc->pathname);
      } else {
	pdesc->open.file = fopen((const char *)pdesc->pathname, "rb");

	if (pdesc->open.file) {
	  fseek(pdesc->open.file, 0, SEEK_END);
	  size = ftell(pdesc->open.file);
	  fseek(pdesc->open.file, 0, SEEK_SET);
	}
      }

      if (DEBUG) printf("I: open %s, size %Xh\n", pdesc->pathname, size);

      aopen[1] = pdesc->open.dir ? 1 : 0;
      (void)rfs_u32le_at(aopen+2, size);
      _send_msg(aopen, sizeof(aopen));

    }
    break;

  case 'C':
    // close
    {
      u8_t aclose[RFS_PAYLOAD_LEN(sizeof(rfs_aclose_t))] = { 0 };
      aclose[0] = 'c';
      
      rnode_t rnode = rfs_read_u32le(cmd+1);

      if (DEBUG > 1) printf("C: close node %Xh\n", rnode);

      // find descriptor for this node
      struct desc *pdesc = _find_by_node(rnode);
      
      if (! pdesc) {
	printf("E: invalid node %Xh (close)\n", rnode);
	
	aclose[1] = 0;
	_send_msg(aclose, sizeof(aclose));

	break;
      }

      if ((pdesc->stat.st_mode & S_IFMT) == S_IFDIR && pdesc->open.dir) {
        closedir(pdesc->open.dir);
	pdesc->open.dir = NULL;
      } else if ((pdesc->stat.st_mode & S_IFMT) != S_IFDIR && pdesc->open.file) {
	fclose(pdesc->open.file);
	pdesc->open.file = NULL;
      }

      aclose[1] = 1;
      _send_msg(aclose, sizeof(aclose));
    }
    break;

  case 'D':
    /* getdents */
    {
      u8_t agetdents[RFS_PAYLOAD_LEN(sizeof(rfs_agetdents_t))] = { 0 };
      agetdents[0] = 'd';
      agetdents[1] = RFS_TYPE_UNK;

      rnode_t rnode = rfs_read_u32le(cmd+1);
      //u8_t dpos = cmd[5];

      if (DEBUG > 1) printf("C: getdents node %Xh\n", rnode);

      // find descriptor for this node
      struct desc *pdesc = _find_by_node(rnode);
      
      if (! pdesc) {
	printf("E: invalid node %Xh (getdents)\n", rnode);
	
	_send_msg(agetdents, sizeof(agetdents));

	break;
      }

      if (! pdesc->open.dir) {
	printf("E: getdents: not open\n");
	
	_send_msg(agetdents, sizeof(agetdents));

	break;
      }
	
      struct dirent *de = readdir(pdesc->open.dir);

      if (de) {
	if (strlen(de->d_name) < RFS_MAX_NAME_LEN) {
	  if (de->d_type == DT_REG) agetdents[1] = RFS_TYPE_REG;
	  if (de->d_type == DT_DIR) agetdents[1] = RFS_TYPE_DIR;
	  if (DEBUG > 1) printf("   %s (d_type=%u  type=%u)\n", de->d_name, de->d_type, agetdents[1]);

	  strcpy((char *)agetdents + 2, de->d_name);
	} else {
	  printf("E: getdents: name too long\n");
	}	
      } else {
	// last entry
	agetdents[1] = RFS_TYPE_UNK;
      }

      _send_msg(agetdents, sizeof(agetdents));
    }
      
    break;

  case 'R':
    // read
    {
      u8_t aread[RFS_MAX_READ+1] = { 0 };  // +1 byte CID
      aread[0] = 'r';
      
      u8_t len = cmd[1];
      rnode_t rnode = rfs_read_u32le(cmd+2);
      off_t offset = rfs_read_u32le(cmd+6);

      if (DEBUG > 1) printf("C: read node %Xh@%Xh len=%u \n", rnode, (unsigned int)offset, len);

      // check length
      if (len > RFS_MAX_READ) {
	printf("E: length exceeds RFS_MAX_READ\n");
	
	aread[1] = RFS_AREAD_KO;
	_send_msg((unsigned char *)&aread, RFS_PAYLOAD_LEN(sizeof(rfs_aread_ko_t)));
	break;
      }
      
      // find descriptor for this node
      struct desc *pdesc = _find_by_node(rnode);
      
      if (! pdesc) {
	printf("E: invalid node %Xh (close)\n", rnode);
	
	aread[1] = RFS_AREAD_KO;
	_send_msg((unsigned char *)&aread, RFS_PAYLOAD_LEN(sizeof(rfs_aread_ko_t)));
	break;
      }

      FILE *fdes = pdesc->open.file;
    
      fseek(fdes, offset, SEEK_SET);

      ssize_t rcnt = fread(aread + 2, 1, len, fdes);

      if (rcnt >= 0) {
	aread[1] = RFS_AREAD_OK;
      } else {
	aread[1] = RFS_AREAD_KO;
      }    

      _send_msg(aread, RFS_PAYLOAD_LEN(sizeof(rfs_aread_ko_t))+len);
    }
    break;

  case 'M':
    // mkdir
    {
      u8_t amkdir[RFS_PAYLOAD_LEN(sizeof(rfs_amkdir_t))];
      amkdir[0] = 'm';
      amkdir[1] = -1;

       rnode_t rnode = rfs_read_u32le(cmd+1);
      u8_t flags = cmd[5];
      u8_t pid = cmd[6];
      char *name = (char *)cmd+7;

      if (DEBUG > 1) printf("C: mkdir node %Xh: %s (flags=%u, PID=%u)\n", rnode, name, flags, pid);

      // find descriptor for this node
      struct desc *pdesc = _find_by_node(rnode);
      
      if (! pdesc) {
	printf("E: invalid node %Xh (mkdir)\n", rnode);
	_send_msg(amkdir, sizeof(amkdir));
	break;
      }

      char fullname[128];
      snprintf(fullname, sizeof(fullname), "%s/%s", pdesc->pathname, name);
      if (DEBUG > 1) printf("C: mkdir full path %s\n", fullname);
      
      int ret = mkdir(fullname, 0 /* mode_t mode */);

      if (ret < 0) {
	printf("W: mkdir failed (errno=%i)\n", errno);
	ret = errno;
      }

      amkdir[1] = ret;
     _send_msg(amkdir, sizeof(amkdir));
      
    }
    break;
    
  default:
    printf("E: unsupported command ('%c')\n", cmd[0]);
    break;
  }


}

int main (int argc, char *argv[]) {
  char *sdev = 0;

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
  serial_init(sdev);

  if (_tty_fd <= 0) {
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

  _descriptors = NULL;
  
  /* structure: SOF (0x55) LEN [command] CKS EOF (0xAA) */
  while (1) {

    switch (framer_fsm) {
    case F_SOF:
      len = 0;
      
      if (read(_tty_fd, buf, 1) == 1) {
	if (buf[0] == 0x55) {
	  framer_fsm = F_LEN;

	  if (DEBUG >= 3) printf("> 55h ");
    
	} else {
	  if (DEBUG >= 3) printf("ignoring byte %02Xh\n", buf[0]);
	}
      }
      break;

    case F_LEN:
      if (read(_tty_fd, buf+1, 1) == 1) {
	framer_fsm = F_CMD;
	
	if (DEBUG >= 3) printf("%Xh [ ", buf[1]);
      } else {
	framer_fsm = F_SOF;
      }
      break;

    case F_CMD:
      while (len < buf[1]) {
	int nlen = read(_tty_fd, buf+2+len, buf[1]-len);

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
      if (read(_tty_fd, buf+2+len, 1) == 1) {
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
      if (read(_tty_fd, buf+2+len+1, 1) == 1) {
	if (buf[2+len+1] == 0xAA) {
	  /* valid command ! */
	  if (DEBUG >= 3) printf("AAh\n");
	  do_cmd(buf+2);  // skip header
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
