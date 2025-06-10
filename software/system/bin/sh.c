//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/binaries - sh
//

#include <stdio.h>
#include <stddef.h>
#include <types.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

#define BUFLEN    128
#define MAX_ARGS  8
#define MAX_PATH  8

char line[BUFLEN];
char *c_args[MAX_ARGS];

//char cpath[PATH_MAX+1];  // current path
//char opath[PATH_MAX+1];  // old path

// extract words form a string and put them in a table
// input string is alterated, and words point within the string
static short _cut_words (char *cline, char *words[], short max)
{
  short argc = 0;
  int in = 0;
  in = 0;

  words[0] = 0;

  while (*cline) {
    if (in == 0 && *cline != ' ') {
      // start of word
      in = 1;
      words[argc++] = cline;

    } else if (in == 1 && *cline == ' ') {
      // end of word
      *cline = 0;
      in = 0;

      // wont'be able to cut more
      if (argc == (max - 1)) {
	break;
      }
    }

    cline++;
  }

  words[argc] = 0;

  return argc;
}

static char *_trim_slash(char *str)
{
  while (*str && (*str != '/')) str++;
  while (*str && (*str == '/')) str++;

  return str;
}

// len of first path element in name
static short _subpathlen (char *path)
{
  char *at = path;

  while (*at && *at != '/') at++;

  return (short)(at - path);
}

// build the full path, starting from current directory (relative) or / (absolute)
//  - resolves . and .. special directories
//  - resulting path always starts with /
static short _build_path(char *dst, char *path)
{
  if (!path) path = "/";  // TODO: use $HOME
  char *cpath = getenv("PWD");
  char *wptr;
  short avail;

  if (*path == '/') {
    // absolute path
    wptr = dst;
    // default heading /, will be overwritten by any path element
    wptr[0] = '/';
    wptr[1] = 0;
    avail = PATH_MAX;
    path = _trim_slash(path);
  } else {
    // relative, starts from current directory
    strcpy(dst, cpath);

    if (strlen(cpath) == 1) {
      // must be /, will be overwritten by any path element
      wptr = dst;
      avail = PATH_MAX;;
    } else {
      wptr = dst+strlen(cpath);
      avail = PATH_MAX-strlen(cpath);
    }
  }
  
  while (*path) {
    // apply path elements one by one
    // cut path element
    short elen = _subpathlen(path);
    char cbak = path[elen];
    path[elen] = 0;

    // special paths . and ..
    if (path[0] == '.') {
      if (elen == 1) {
	// .
	path[1] = cbak;
	path++;
	path = _trim_slash(path);
	continue;
      }
      
      if (elen == 2 && path[1] == '.') {
	// ..
	if (avail < PATH_MAX) {
	  // not at root yet
	  while (*--wptr != '/') avail++;
	  avail++;
	  
	  if (avail == PATH_MAX) {
	    // rewind up to root
	    wptr[0] = '/';
	    wptr[1] = 0;
	  } else {
	    *wptr = 0;
	  }
	}
	
	path[2] = cbak;
	path += 2;
	path = _trim_slash(path);
	continue;
      }
    }

    // regular path element
    if (avail < elen+1) {
      // too long
      return -ENAMETOOLONG;
    }
    
    *wptr++ = '/';
    strcpy(wptr, path);
    wptr += elen;
    avail -= (elen+1);

    // next path element
    path[elen] = cbak;
    path++;
    path = _trim_slash(path);
  }

  return 0;
}

static short _command_valid (char *path)
{
  int fd = open(path, O_RDONLY);

  if (fd < 0) {
    return 0;
  }

  // found
  // TODO : check executable
  close(fd);

  return 1;
}

short _search_command (char **pcmd)
{
  // first check if command has path elements
  char *pathelem = *pcmd;

  while (*pathelem && (*pathelem != '/')) {
    pathelem++;
  }

  if (*pathelem == '/') {
    // it has path elements
    return _command_valid(*pcmd);
  }
  
  // explore PATH possibilities
  pathelem = getenv("PATH");
  static char fullpath[PATH_MAX];

  if (pathelem) {
    while (*pathelem) {
      // search for end of PATH element
      char *end = pathelem;

      while (*end && (*end != ':')) {
	end++;
      }

      char eol_save = *end;
      *end = 0;

      // TODO: possible buffer overfow ?
      sprintf(fullpath, "%s/%s", pathelem, *pcmd);
      
      *end = eol_save;

      if (_command_valid(fullpath)) {
	*pcmd = fullpath;
	return 1;
      }

      // next path element
      pathelem = end;
    }
  }

  // not found in path
  return 0;
}
  
short _do_words (void) {
  // built-in commands
  if (strcmp(c_args[0], "exit") == 0) return -1;

  if (strcmp(c_args[0], "cd") == 0) {
    char path[PATH_MAX+1];

    if (c_args[1] && (c_args[1][0] == '-') && (c_args[1][1] == 0)) {
      // cd -
      char *opath = getenv("OLDPWD");

      if (! opath) {
	printf("sh: cd: OLDPWD not set\n");
	return 0;
      }
	  
      if (chdir(opath) < 0) {
	printf("sh: cd: %s: No such directory\n", opath);
	return 0;
      }

      strcpy(path, getenv("PWD"));
      setenv("PWD", opath, 1);
      setenv("OLDPWD", path, 1);
    } else {
      if (_build_path(path, c_args[1]) < 0) {
	printf("sh: cd: %s: Path too long\n", c_args[1]);
	return 0;
      }
	
      if (chdir(path) < 0) {
	printf("sh: cd: %s: No such directory\n", path);
	return 0;
      }

      setenv("OLDPWD", getenv("PWD"), 1);
      setenv("PWD", path, 1);
    }

    return 0;
  }
  else if (strcmp(c_args[0], "pwd") == 0) {
    printf("%s\n", getenv("PWD"));  // TODO use $PWD
    return 0;
  }
	
  // external commands
  if (_search_command(c_args) == 0) {
    printf("Command '%s' not found\n", c_args[0]);
    return 0;
  }
    
  // file exists, create new process
  int pid = fork();
    
  if (pid < 0) {
    printf("cannot fork\n");
  } else if (pid == 0) {
    // child
    execve(c_args[0], c_args, environ);

    // only when exec fails
    printf("cannot exec\n");
    exit(0);
  } else {
    // parent
    wait(NULL);

    // TODO: process exit condition to look at
    kill(pid, 1);
  }

  return 0;
}  

int main (int argc, char *argv[])
{
  setenv("PWD", "/", 1);

  while (1) {
    // TODO: use PS1 environment variable
    printf("# ");

    // get a line
    {
      char *pc = line;
      short len = 0;

      while (1) {
	int n = read(0 /* stdin */, pc, 1);

	if (n < 0) break;  // IO error

	if (n) {
	  if (*pc == '\n') {
	    // line return
	    *pc = 0;
	    break;
	  } else if (*pc == 8) {
	    // backspace
	    if (len) {
	      len--;
	      pc--;
	    }
	    continue;
	  } else if (*pc == 0) {
	    break;
	  }

	  pc++;
	  len++;  // TODO: buffer overflow !
	}
      }
    }

    // break down command & arguments
    (void)_cut_words(line, c_args, MAX_ARGS);

    if (c_args[0]) {
      if (_do_words() < 0) {
	break;
      };
    }
  }

  return 0;
}
