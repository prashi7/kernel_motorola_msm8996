#ifndef _INCLUDE_GUARD_SVG_HELPER_
#define _INCLUDE_GUARD_SVG_HELPER_

#include "types.h"

extern void open_svg(const char *filename, int cpus, int rows);
extern void svg_box(int Yslot, u64 start, u64 end, const char *type);
extern void svg_sample(int Yslot, int cpu, u64 start, u64 end, const char *type);
extern void svg_cpu_box(int cpu, u64 max_frequency, u64 turbo_frequency);


extern void svg_process(int cpu, u64 start, u64 end, const char *type, const char *name);
extern void svg_cstate(int cpu, u64 start, u64 end, int type);
extern void svg_pstate(int cpu, u64 start, u64 end, u64 freq);


extern void svg_time_grid(u64 start, u64 end);
extern void svg_legenda(void);
extern void svg_wakeline(u64 start, int row1, int row2);
extern void svg_partial_wakeline(u64 start, int row1, char *desc1, int row2, char *desc2);
extern void svg_interrupt(u64 start, int row);
extern void svg_text(int Yslot, u64 start, const char *text);
extern void svg_close(void);

#endif
