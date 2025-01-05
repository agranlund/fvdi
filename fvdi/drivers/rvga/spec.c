/*
 * fVDI device driver specific setup
 *
 * Copyright 1998-2002, Johan Klockars
 * This software is licensed under the GNU General Public License.
 * Please, see LICENSE.TXT for further information.
 */

#include "fvdi.h"
#include "relocate.h"

#include "os.h"
#include "driver.h"
//#include "bitplane.h"
#include "string/memset.h"


extern short fix_shape;
extern short no_restore;

long CDECL x_get_colour(Workstation *wk, long colour);
void CDECL x_get_colours(Workstation *wk, long colour, short *foreground, short *background);

long CDECL c_get_colour(Virtual *vwk, long colour);
void CDECL c_get_colours(Virtual *vwk, long colour, unsigned long *foreground, unsigned long *background);
void CDECL c_set_colours(Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[]);

long CDECL c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour);
long CDECL c_read_pixel(Virtual *vwk, MFDB *mfdb, long x, long y);
long CDECL c_line_draw(Virtual *vwk, long x1, long y1, long x2, long y2, long pattern, long colour, long mode);
long CDECL c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour);
long CDECL c_fill_area(Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style);
long CDECL c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation);
long CDECL c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse);


#if 0
#define FAST		/* Write in FastRAM buffer */
#define BOTH		/* Write in both FastRAM and on screen */
#else
#undef FAST
#undef BOTH
#endif

//#define temp_phys_addr 0x44000000UL
#define temp_phys_addr 0x44000000UL

static char const red[] = { 6 };
static char const green[] = { 6 };
static char const blue[] = { 6 };
static char const none[] = { 0 };

static Mode const mode[4] = {
    { 8, CHUNKY | CHECK_PREVIOUS, { red, green, blue, none, none, none }, 0, 2, 1, 1 }
};

struct {
	short width;
	short height;
} pixel;

char driver_name[] = "RVGA 8bit (shadow)";

long CDECL(*write_pixel_r) (Virtual *vwk, MFDB * mfdb, long x, long y, long colour) = c_write_pixel;
long CDECL(*read_pixel_r) (Virtual *vwk, MFDB * mfdb, long x, long y) = c_read_pixel;
long CDECL(*line_draw_r) (Virtual *vwk, long x1, long y1, long x2, long y2, long pattern, long colour, long mode) = c_line_draw;
long CDECL(*expand_area_r) (Virtual *vwk, MFDB * src, long src_x, long src_y, MFDB * dst, long dst_x, long dst_y, long w, long h, long operation, long colour) = c_expand_area;
long CDECL(*fill_area_r) (Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style) = c_fill_area;
long CDECL(*fill_poly_r) (Virtual *vwk, short points[], long n, short index[], long moves, short *pattern, long colour, long mode, long interior_style) = 0;
long CDECL(*blit_area_r) (Virtual *vwk, MFDB * src, long src_x, long src_y, MFDB * dst, long dst_x, long dst_y, long w, long h, long operation) = c_blit_area;
long CDECL(*text_area_r) (Virtual *vwk, short *text, long length, long dst_x, long dst_y, short *offsets) = 0;
long CDECL(*mouse_draw_r) (Workstation *wk, long x, long y, Mouse * mouse) = c_mouse_draw;

long CDECL(*get_colour_r) (Virtual *vwk, long colour) = c_get_colour;
void CDECL(*get_colours_r) (Virtual *vwk, long colour, unsigned long *foreground, unsigned long *background) = 0;
void CDECL(*set_colours_r) (Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[]) = c_set_colours;

long wk_extend = 0;
short accel_s = 0;
short accel_c = A_SET_PAL | A_GET_COL | A_SET_PIX | A_GET_PIX | A_BLIT | A_FILL | A_EXPAND | A_LINE | A_MOUSE;




const Mode *graphics_mode = &mode[0];

short shadow = 0;
short fix_shape = 0;
short no_restore = 0;
short depth = 0;


static Option const options[] = {
    { "debug",      { &debug },             2 },  /* debug, turn on debugging aids */
    { "shadow",     { &shadow },            0 },  /* Use a separate buffer of the screen in RAM */
    { "fixshape",   { &fix_shape },         0 },  /* fixed shape; do not allow mouse shape changes */
    { "norestore",  { &no_restore },        0 },
};

/*
 * Handle any driver specific parameters
 */
long check_token(char *token, const char **ptr)
{
    int i;
    int normal;
    char *xtoken;

    xtoken = token;
    switch (token[0])
    {
    case '+':
        xtoken++;
        normal = 1;
        break;
    case '-':
        xtoken++;
        normal = 0;
        break;
    default:
        normal = 1;
        break;
    }
    for (i = 0; i < (int)(sizeof(options) / sizeof(Option)); i++)
    {
        if (access->funcs.equal(xtoken, options[i].name))
        {
            switch (options[i].type)
            {
            case -1:      /* Function call */
                return (options[i].var.func)(ptr);
            case 0:        /* Default 1, set to 0 */
                *options[i].var.s = 1 - normal;
                return 1;
            case 1:      /* Default 0, set to 1 */
                *options[i].var.s = normal;
                return 1;
            case 2:      /* Increase */
                *options[i].var.s += -1 + 2 * normal;
                return 1;
            case 3:
                if ((*ptr = access->funcs.skip_space(*ptr)) == NULL)
                {
                    ;  /* *********** Error, somehow */
                }
                *ptr = access->funcs.get_token(*ptr, token, 80);
                *options[i].var.s = token[0];
                return 1;
            }
        }
    }

    return 0;
}


static unsigned char *screen_address = (unsigned char*)0x44000000UL;


/*
 * Do whatever setup work might be necessary on boot up
 * and which couldn't be done directly while loading.
 * Supplied is the default fVDI virtual workstation.
 */
long initialize(Virtual *vwk)
{
	Workstation *wk;
	int old_palette_size;
	Colour *old_palette_colours;

#ifdef FAST
    char *buf;
    int fast_w_bytes;
#endif

    shadow = 0;

    debug = access->funcs.misc(0, 1, 0);

	vwk = me->default_vwk;	/* This is what we're interested in */	
	wk = vwk->real_address;

	wk->screen.look_up_table = 0;			/* Was 1 (???)  Shouldn't be needed (graphics_mode) */
	wk->screen.mfdb.standard = 0;
	if (wk->screen.pixel.width > 0)        /* Starts out as screen width */
		wk->screen.pixel.width = (wk->screen.pixel.width * 1000L) / wk->screen.mfdb.width;
	else                                   /*   or fixed DPI (negative) */
		wk->screen.pixel.width = 25400 / -wk->screen.pixel.width;
	if (wk->screen.pixel.height > 0)        /* Starts out as screen height */
		wk->screen.pixel.height = (wk->screen.pixel.height * 1000L) / wk->screen.mfdb.height;
	else                                    /*   or fixed DPI (negative) */
		wk->screen.pixel.height = 25400 / -wk->screen.pixel.height;

	if (loaded_palette)
		access->funcs.copymem(loaded_palette, default_vdi_colors, 256 * 3 * sizeof(short));
	if ((old_palette_size = wk->screen.palette.size) != 256) {	/* Started from different graphics mode? */
		old_palette_colours = wk->screen.palette.colours;
		wk->screen.palette.colours = (Colour *)access->funcs.malloc(256L * sizeof(Colour), 3);	/* Assume malloc won't fail. */
		if (wk->screen.palette.colours) {
			wk->screen.palette.size = 256;
			if (old_palette_colours)
				access->funcs.free(old_palette_colours);	/* Release old (small) palette (a workaround) */
		} else
			wk->screen.palette.colours = old_palette_colours;
	}

	c_initialize_palette(vwk, 0, wk->screen.palette.size, default_vdi_colors, wk->screen.palette.colours);

	device.byte_width = wk->screen.wrap;
	device.address = wk->screen.mfdb.address;

	pixel.width = wk->screen.pixel.width;
	pixel.height = wk->screen.pixel.height;


#ifdef FAST
    if (shadow)
    {
        fast_w_bytes = wk->screen.wrap;
        buf = (char *) access->funcs.malloc(fast_w_bytes * wk->screen.mfdb.height + 255, 1);
        if (buf)
        {
            wk->screen.shadow.buffer = buf;
            wk->screen.shadow.address = (void *) (((long) buf + 255) & 0xffffff00);
            wk->screen.shadow.wrap = fast_w_bytes;
        } else
        {
            access->funcs.error("Can't allocate FastRAM!", 0);
            wk->screen.shadow.buffer = 0;
            wk->screen.shadow.address = 0;
        }
#ifndef BOTH
        wk->screen.mfdb.address = wk->screen.shadow.address;
#endif
    }
#endif
    if (!wk->screen.shadow.address)
        driver_name[10] = 0;


    graphics_mode = &mode[0];
    //setup_scrninfo(me->device, graphics_mode);

    PRINTF(("%dx%dx%d screen at %08lx\n", wk->screen.mfdb.width, wk->screen.mfdb.height, wk->screen.mfdb.bitplanes,
            (long) wk->screen.mfdb.address));

    return 1;
}

/*
 *
 */
long CDECL setup(long type, long value)
{
	long ret;

	ret = -1;
	switch(type) {
	case Q_NAME:
		ret = (long)driver_name;
		break;
	case S_DRVOPTION:
		ret = tokenize((char *)value);
		break;
	}

	return ret;
}

/*
 * Initialize according to parameters (boot and sent).
 * Create new (or use old) Workstation and default Virtual.
 * Supplied is the default fVDI virtual workstation.
 */
Virtual *CDECL opnwk(Virtual *vwk)
{
	Workstation *wk;
	vwk = me->default_vwk;  /* This is what we're interested in */
	wk = vwk->real_address;

	/* Switch to SAGA screen */
#if 0
	saga_set_clock(mi);
	saga_set_modeline(mi, SAGA_VIDEO_FORMAT_RGB16);
	saga_set_panning(screen_address);
#endif

	/* update the settings */
	wk->screen.mfdb.width = 800;
	wk->screen.mfdb.height = 600;
	wk->screen.mfdb.bitplanes = 8;

	/*
	 * Some things need to be changed from the
	 * default workstation settings.
	 */
	wk->screen.mfdb.address = (short *)screen_address;
	wk->screen.mfdb.wdwidth = (wk->screen.mfdb.width + 15) / 16;
	wk->screen.wrap = wk->screen.mfdb.width * (wk->screen.mfdb.bitplanes / 8);

	wk->screen.coordinates.max_x = wk->screen.mfdb.width - 1;
	wk->screen.coordinates.max_y = wk->screen.mfdb.height - 1;

	wk->screen.look_up_table = 0;			/* Was 1 (???)	Shouldn't be needed (graphics_mode) */
	wk->screen.mfdb.standard = 0;

	if (pixel.width > 0)			/* Starts out as screen width */
		wk->screen.pixel.width = (pixel.width * 1000L) / wk->screen.mfdb.width;
	else								   /*	or fixed DPI (negative) */
		wk->screen.pixel.width = 25400 / -pixel.width;

	if (pixel.height > 0)		/* Starts out as screen height */
		wk->screen.pixel.height = (pixel.height * 1000L) / wk->screen.mfdb.height;
	else									/*	 or fixed DPI (negative) */
		wk->screen.pixel.height = 25400 / -pixel.height;

	wk->mouse.position.x = ((wk->screen.coordinates.max_x - wk->screen.coordinates.min_x + 1) >> 1) + wk->screen.coordinates.min_x;
	wk->mouse.position.y = ((wk->screen.coordinates.max_y - wk->screen.coordinates.min_y + 1) >> 1) + wk->screen.coordinates.min_y;

    return 0;
}

/*
 * 'Deinitialize'
 */
void CDECL clswk(Virtual *vwk)
{
    (void) vwk;
}
