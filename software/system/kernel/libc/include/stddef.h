
#ifndef _STDDEF_H_
#define _STDDEF_H_

typedef unsigned int size_t;

/* Offset of member MEMBER in a struct of type TYPE. */
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

#define NULL  0

#endif /* _STDDEF_H_ */
