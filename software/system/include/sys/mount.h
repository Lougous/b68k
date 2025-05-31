//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//

#ifndef _mount_h_
#define _mount_h_

int mount(const char *source, const char *target,
	  const char *filesystemtype, unsigned long mountflags,
	  const void *data);

#endif // _mount_h_
