#ifndef ___ASM_SPARC_MODULE_H
#define ___ASM_SPARC_MODULE_H
#if defined(__sparc__) && defined(__arch64__)
#include <asm-sparc/module_64.h>
#else
#include <asm-sparc/module_32.h>
#endif
#endif
