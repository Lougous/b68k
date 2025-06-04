//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//

#ifndef _vgm_h_
#define _vgm_h_

int16_t vgm_init(void);
int16_t vgm_load_file(char *fname);
int16_t vgm_update(void);

#endif // _vgm_h_
