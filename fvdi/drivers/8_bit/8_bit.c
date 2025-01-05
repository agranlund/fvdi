#if 1
#define FAST		/* Write in FastRAM buffer */
#define BOTH		/* Write in both FastRAM and on screen */
#else
#undef FAST
#undef BOTH
#endif

#include "../8_bit/8b_exp.c"
#include "../8_bit/8b_blit.c"
#include "../8_bit/8b_line.c"
#include "../8_bit/8b_fill.c"
#include "../8_bit/8b_scr.c"
#include "../8_bit/8b_pal.c"
#include "../8_bit/8b_spec.c"
#include "../8_bit/8b_mouse.c"
