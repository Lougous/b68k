
#ifndef _mem_h_
#define _mem_h_


typedef u32_t mem_pa_t;  // physical address
typedef u32_t mem_va_t;  // virtual address

#define MEM_OUT_OF_MEMORY  -1

#define MEM_ALLOC_NO_INIT  1

extern void mem_init (void);
extern mem_pa_t mem_realloc (pid_t pid, mem_pa_t addr, u32_t old_sz, u32_t new_sz, u16_t options);
extern void mem_free(mem_pa_t addr, u32_t sz);
extern void mem_setup_mmu (mem_pa_t addr, u32_t sz);

#endif /* _mem_h_ */
