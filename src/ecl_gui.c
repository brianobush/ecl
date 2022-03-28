#include <stdio.h>

#include <SDL2/SDL.h>
#include <portmidi.h>
#include <porttime.h>

#include "ecl.h"

#define ROTATE 1
#define HOR 64
#define VER 32
#define PAD 8
#define color1 0x000000 /* Black */
#define color2 0x72DEC2 /* Turquoise 0x72DEC2, console green 0x399612 */
#define color3 0xFFFFFF /* White */
#define color4 0x444444 /* Gray */
#define color5 0xffff00 /* Orange cursor */
#define color0 0xffb545 /* Orange cursor */

#define SZ (HOR * VER * 16)

#define DEVICE 0

typedef struct
{
	int channel, note, velocity, len;
} note_t;

Uint32 theme[] = {
	0x000000,
	0x72DEC2,
	0xFFFFFF,
	0x444444,
	0xffff00,
	0x666666,
	0xffb545};

// Every character in the font is encoded row-wise in 8 bytes. (Row major, little-endian)

// The least significant bit of each byte corresponds to the first pixel in a
//  row.

// The character 'A' (0x41 / 65) is encoded as
// { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}

//     0x0C => 0000 1100 => ..XX....
//     0X1E => 0001 1110 => .XXXX...
//     0x33 => 0011 0011 => XX..XX..
//     0x33 => 0011 0011 => XX..XX..
//     0x3F => 0011 1111 => xxxxxx..
//     0x33 => 0011 0011 => XX..XX..
//     0x33 => 0011 0011 => XX..XX..
//     0x00 => 0000 0000 => ........

// To access the nth pixel in a row, right-shift by n.

//                          . . X X . . . .
//                          | | | | | | | |
//     (0x0C >> 0) & 1 == 0-+ | | | | | | |
//     (0x0C >> 1) & 1 == 0---+ | | | | | |
//     (0x0C >> 2) & 1 == 1-----+ | | | | |
//     (0x0C >> 3) & 1 == 1-------+ | | | |
//     (0x0C >> 4) & 1 == 0---------+ | | |
//     (0x0C >> 5) & 1 == 0-----------+ | |
//     (0x0C >> 6) & 1 == 0-------------+ |
//     (0x0C >> 7) & 1 == 0---------------+

Uint8 font[128][8] = {
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0000 (nul)
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0001
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0002
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0003
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0004
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0005
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0006
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0007
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0008
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0009
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000A
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000B
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000C
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000D
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000E
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000F
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0010
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0011
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0012
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0013
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0014
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0015
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0016
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0017
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0018
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0019
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001A
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001B
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001C
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001D
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001E
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001F
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0020 (space)
	{0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // U+0021 (!)
	{0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0022 (")
	{0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // U+0023 (#)
	{0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // U+0024 ($)
	{0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // U+0025 (%)
	{0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // U+0026 (&)
	{0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0027 (')
	{0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // U+0028 (()
	{0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // U+0029 ())
	{0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // U+002A (*)
	{0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // U+002B (+)
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+002C (,)
	{0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // U+002D (-)
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+002E (.)
	{0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // U+002F (/)
	{0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // U+0030 (0)
	{0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // U+0031 (1)
	{0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // U+0032 (2)
	{0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // U+0033 (3)
	{0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // U+0034 (4)
	{0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // U+0035 (5)
	{0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // U+0036 (6)
	{0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // U+0037 (7)
	{0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // U+0038 (8)
	{0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // U+0039 (9)
	{0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+003A (:)
	{0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+003B (;)
	{0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // U+003C (<)
	{0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // U+003D (=)
	{0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // U+003E (>)
	{0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // U+003F (?)
	{0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // U+0040 (@)
	{0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // U+0041 (A)
	{0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // U+0042 (B)
	{0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // U+0043 (C)
	{0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // U+0044 (D)
	{0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // U+0045 (E)
	{0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // U+0046 (F)
	{0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // U+0047 (G)
	{0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // U+0048 (H)
	{0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0049 (I)
	{0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // U+004A (J)
	{0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // U+004B (K)
	{0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // U+004C (L)
	{0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // U+004D (M)
	{0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // U+004E (N)
	{0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // U+004F (O)
	{0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // U+0050 (P)
	{0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // U+0051 (Q)
	{0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // U+0052 (R)

	{0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // U+0053 (S)

	{0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0054 (T)
	{0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // U+0055 (U)
	{0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0056 (V)
	{0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // U+0057 (W)
	{0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // U+0058 (X)
	{0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // U+0059 (Y)
	{0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // U+005A (Z)
	{0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00}, // U+005B ([)
	{0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00}, // U+005C (\)
	{0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00}, // U+005D (])
	{0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // U+005E (^)
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // U+005F (_)
	{0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0060 (`)
	{0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00}, // U+0061 (a)
	{0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00}, // U+0062 (b)
	{0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00}, // U+0063 (c)
	{0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00}, // U+0064 (d)
	{0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00}, // U+0065 (e)
	{0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00}, // U+0066 (f)
	{0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0067 (g)
	{0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00}, // U+0068 (h)
	{0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0069 (i)
	{0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E}, // U+006A (j)
	{0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00}, // U+006B (k)
	{0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+006C (l)
	{0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00}, // U+006D (m)
	{0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00}, // U+006E (n)
	{0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00}, // U+006F (o)
	{0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F}, // U+0070 (p)
	{0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78}, // U+0071 (q)
	{0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00}, // U+0072 (r)
	{0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00}, // U+0073 (s)
	{0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00}, // U+0074 (t)
	{0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00}, // U+0075 (u)
	{0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0076 (v)
	{0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00}, // U+0077 (w)
	{0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00}, // U+0078 (x)
	{0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0079 (y)
	{0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00}, // U+007A (z)
	{0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00}, // U+007B ({)
	{0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, // U+007C (|)
	{0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00}, // U+007D (})
	{0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+007E (~)
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // U+007F
};

typedef struct
{
	int x, y, w, h;
} Rect2d;

int colors[] = {color1, color2, color3, color4, color0};
int WIDTH = 8 * HOR + PAD * 2;
int HEIGHT = 8 * VER + PAD * 2;
int FPS = 30, DOWN = 0, ZOOM = 2, PAUSE = 0;
SDL_Window *gWindow = NULL;
SDL_Renderer *gRenderer = NULL;
SDL_Texture *gTexture = NULL;
Uint32 *pixels;
PmStream *midi;

Rect2d cursor;
ecl_t *ecl;

int error(char *msg, const char *err)
{
	printf("Error %s: %s\n", msg, err);
	return 0;
}

void quit(void)
{
	free(pixels);
	SDL_DestroyTexture(gTexture);
	gTexture = NULL;
	SDL_DestroyRenderer(gRenderer);
	gRenderer = NULL;
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	SDL_Quit();
	exit(0);
}

void init_midi(void)
{
	int i;
	Pm_Initialize();
	for (i = 0; i < Pm_CountDevices(); ++i)
	{
		char const *name = Pm_GetDeviceInfo(i)->name;
		printf("Device #%d -> %s%s\n", i, name, i == DEVICE ? "[x]" : "[ ]");
	}
	Pm_OpenOutput(&midi, DEVICE, NULL, 128, 0, NULL, 1);
}

int init(void)
{
	int i, j;
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		return error("Init", SDL_GetError());
	}
	gWindow = SDL_CreateWindow("ECL",
							   SDL_WINDOWPOS_UNDEFINED,
							   SDL_WINDOWPOS_UNDEFINED,
							   WIDTH * ZOOM,
							   HEIGHT * ZOOM,
							   SDL_WINDOW_SHOWN);
	if (gWindow == NULL)
	{
		return error("Window", SDL_GetError());
	}
	gRenderer = SDL_CreateRenderer(gWindow, -1, 0);
	if (gRenderer == NULL)
	{
		return error("Renderer", SDL_GetError());
	}
	gTexture = SDL_CreateTexture(gRenderer,
								 SDL_PIXELFORMAT_ARGB8888,
								 SDL_TEXTUREACCESS_STATIC,
								 WIDTH,
								 HEIGHT);
	if (gTexture == NULL)
	{
		return error("Texture", SDL_GetError());
	}
	pixels = (Uint32 *)malloc(WIDTH * HEIGHT * sizeof(Uint32));
	if (pixels == NULL)
	{
		return error("Pixels", "Failed to allocate memory");
	}
	for (i = 0; i < HEIGHT; i++)
	{
		for (j = 0; j < WIDTH; j++)
		{
			pixels[i * WIDTH + j] = color1;
		}
	}
	init_midi();
	return 1;
}

// int load_font(void)
// {
// 	FILE *f = fopen("nasu-export.chr", "rb");
// 	if (!f)
// 	{
// 		return error("Font", "Invalid font file");
// 	}
// 	if (!fread(font, sizeof(font), 1, f))
// 	{
// 		return error("Font", "Invalid font size");
// 	}
// 	fclose(f);
// 	return 1;
// }

void set_play(int val)
{
	PAUSE = val;
}

int get_font(int x, int y, char c, int type, int sel)
{

	if (valid_char(c))
	{
		return c;
	}
	else if (x % 8 == 0 && y % 8 == 0)
	{
		return '+';
	}
	else if (sel || type || (x % 2 == 0 && y % 2 == 0))
	{
		return '.';
	}
	// else if (cursor.x == x && cursor.y == y)
	// {
	// 	return '@';
	// }
	printf("Unhandled %c\n", c);
	return c;
}

int get_style(int clr, int type, int sel)
{
	if (sel)
		return clr == 0 ? 4 : 0;
	if (type == 2)
		return clr == 0 ? 0 : 1;
	if (type == 3)
		return clr == 0 ? 1 : 0;
	if (type == 4)
		return clr == 0 ? 0 : 2;
	if (type == 5)
		return clr == 0 ? 2 : 0;
	return clr == 0 ? 0 : 3;
}

int selected(int x, int y)
{
	return x < cursor.x + cursor.w && x >= cursor.x && y < cursor.y + cursor.h && y >= cursor.y;
}

void put_pixel(Uint32 *dst, int x, int y, int color)
{
	if (x >= 0 && x < WIDTH - 8 && y >= 0 && y < HEIGHT - 8)
		dst[(y + PAD) * WIDTH + (x + PAD)] = theme[color];
}

void draw_tile(Uint32 *dst, int x, int y, char c, int type)
{
	int v, h;

	int sel = selected(x, y);
	Uint8 *bitmap = font[get_font(x, y, c, type, sel)];

	if (1)
	{
		for (v = 0; v < 8; v++)
		{
			for (h = 0; h < 8; h++)
			{
				int style = get_style((bitmap[v] & 1 << h), type, sel);
				put_pixel(dst, x * 8 + h, y * 8 + v, style);
			}
		}
	}
	if (0)
		for (h = 0; h < 8; h++)
		{
			for (v = 0; v < 8; v++)
			{

				if (bitmap[h] & (1 << v))
				{
					int style = get_style((bitmap[h] & 1 << v), type, sel);
					put_pixel(dst, x * 8 + v, y * 8 + h, style);
				}
			}
		}
}

void draw(Uint32 *dst)
{
	int i, x, y, state, style;
	// 1 is faint = TYPE_EMPTY
	// 2 is regular, but turquoise  = TYPE_ARGS
	// 3 is bold = TYPE_COMMAND
	// 4 is regular = TYPE_NUMBER
	// 5 is inverted = TYPE_OUPUT
	int ylimit, xlimit;
	if (ROTATE)
	{
		ylimit = HOR;
		xlimit = VER;
	}
	else
	{
		ylimit = VER;
		xlimit = HOR;
	}
	for (y = 0; y < ylimit; y++)
	{
		for (x = 0; x < xlimit; x++)
		{
			i = y * xlimit + x;
			state = ecl_get_state(ecl, i);
			switch (state)
			{
			case STATE_ARG:
				style = 2;
				break;
			case STATE_CMD:
				style = 3;
				break;
			case STATE_NUM:
			case STATE_NEW:
				style = 2;
				break;
				/// 5?
			case STATE_EMPTY:
			default:
				style = 1;
				break;
			}
			if (ROTATE)
			{
				draw_tile(dst, y, x, ecl_get(ecl, i), style);
			}
			else
			{
				draw_tile(dst, x, y, ecl_get(ecl, i), style);
			}
		}
	}

	SDL_UpdateTexture(gTexture, NULL, dst, WIDTH * sizeof(Uint32));
	SDL_RenderClear(gRenderer);
	SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
	SDL_RenderPresent(gRenderer);
}

int clamp(int val, int min, int max)
{
	return (val >= min) ? (val <= max) ? val : max : min;
}

void do_insert(char c)
{
	printf("x %d, y %d, c %c\n", cursor.x, cursor.y, c);
	int pos;
	if (ROTATE)
	{
		pos = (cursor.x * VER) + cursor.y;
	}
	else
	{
		pos = (cursor.y * HOR) + cursor.x;
	}
	printf("insert %d\n", pos);
	ecl_set(ecl, pos, c);
}

void select(int x, int y, int w, int h)
{
	cursor.x = clamp(x, 0, HOR - 1);
	cursor.y = clamp(y, 0, VER - 1);
	cursor.w = clamp(w, 1, 36);
	cursor.h = clamp(h, 1, 36);
	draw(pixels);
}

void do_zoom(int mod)
{
	if ((mod > 0 && ZOOM < 4) || (mod < 0 && ZOOM > 1))
	{
		ZOOM += mod;
		printf("Seeting zoom %d\n", ZOOM);
		SDL_SetWindowSize(gWindow, WIDTH * ZOOM, HEIGHT * ZOOM);
	}
}

void do_move(int x, int y)
{
	select(cursor.x + x, cursor.y + y, cursor.w, cursor.h);
}

void play_midi(int channel, int octave, int note)
{
	Pm_WriteShort(midi,
				  Pt_Time(),
				  Pm_Message(0x90 + channel, (octave * 12) + note, 100));
	Pm_WriteShort(midi,
				  Pt_Time(),
				  Pm_Message(0x90 + channel, (octave * 12) + note, 0));
	printf("%d -> %d\n", channel, (octave * 12) + note);
}

void do_mouse(SDL_Event *event)
{
	int x = (event->motion.x - (PAD * ZOOM)) / ZOOM;
	int y = (event->motion.y - (PAD * ZOOM)) / ZOOM;
	//printf("mouse x %d, y %d\n", x, y);
	switch (event->type)
	{
	case SDL_MOUSEBUTTONUP:
		DOWN = 0;
		break;
	case SDL_MOUSEBUTTONDOWN:
		select(x / 8, y / 8, 1, 1);
		DOWN = 1;
		break;
	case SDL_MOUSEMOTION:
		if (DOWN)
			select(
				cursor.x,
				cursor.y,
				x / 8 - cursor.x + 1,
				y / 8 - cursor.y + 1);
		break;
	}
}

void do_key(SDL_Event *event)
{
	int shift = SDL_GetModState() & KMOD_LSHIFT || SDL_GetModState() & KMOD_RSHIFT;
	int ctrl = SDL_GetModState() & KMOD_LCTRL || SDL_GetModState() & KMOD_RCTRL;

	if (ctrl)
	{
		switch (event->key.keysym.sym)
		{
		case SDLK_x:
			//cutclip(&cursor, clip);
			break;
		case SDLK_c:
			//copyclip(&cursor, clip);
			break;
		case SDLK_v:
			printf("paste\n"); //pasteclip(&cursor, clip, shift);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (event->key.keysym.sym)
		{
		case SDLK_EQUALS:
		case SDLK_PLUS:
			do_zoom(1);
			break;
		case SDLK_UNDERSCORE:
		case SDLK_MINUS:
			do_zoom(-1);
			break;
		case SDLK_UP:
			do_move(0, -1);
			break;
		case SDLK_DOWN:
			do_move(0, 1);
			break;
		case SDLK_LEFT:
			do_move(-1, 0);
			break;
		case SDLK_RIGHT:
			do_move(1, 0);
			break;
		case SDLK_BACKSPACE:
			do_insert(0);
			break;
		// case SDLK_ASTERISK: insert('*'); break;
		// case SDLK_HASH: insert('#'); break;
		// case SDLK_PERIOD: insert('.'); break;
		// case SDLK_COLON: insert(':'); break;
		// case SDLK_SEMICOLON: insert(':'); break;
		case SDLK_ESCAPE:
			select(0, 0, 1, 1);
			break;
		case SDLK_SPACE:
			set_play(!PAUSE);
			break;
		case SDLK_SLASH:
			if (shift)
			{
				do_insert('?');
			}
			break;
		case SDLK_0:
			do_insert('0');
			break;
		case SDLK_1:
			do_insert('1');
			break;
		case SDLK_2:
			do_insert('2');
			break;
		case SDLK_3:
			do_insert('3');
			break;
		case SDLK_4:
			do_insert((shift) ? '$' : '4');
			break;
		case SDLK_5:
			do_insert('5');
			break;
		case SDLK_6:
			do_insert('6');
			break;
		case SDLK_7:
			do_insert('7');
			break;
		case SDLK_8:
			do_insert('8');
			break;
		case SDLK_9:
			do_insert('9');
			break;
		case SDLK_a:
			do_insert(shift ? 'A' : 'a');
			break;
		case SDLK_b:
			do_insert(shift ? 'B' : 'b');
			break;
		case SDLK_c:
			do_insert(shift ? 'C' : 'c');
			break;
		case SDLK_d:
			do_insert(shift ? 'D' : 'd');
			break;
		case SDLK_e:
			do_insert(shift ? 'E' : 'e');
			break;
		case SDLK_f:
			do_insert(shift ? 'F' : 'f');
			break;
		case SDLK_g:
			do_insert(shift ? 'G' : 'g');
			break;
		case SDLK_h:
			do_insert(shift ? 'H' : 'h');
			break;
		case SDLK_i:
			do_insert(shift ? 'I' : 'i');
			break;
		case SDLK_j:
			do_insert(shift ? 'J' : 'j');
			break;
		case SDLK_k:
			do_insert(shift ? 'K' : 'k');
			break;
		case SDLK_l:
			do_insert(shift ? 'L' : 'l');
			break;
		case SDLK_m:
			do_insert(shift ? 'M' : 'm');
			break;
		case SDLK_n:
			do_insert(shift ? 'N' : 'n');
			break;
		case SDLK_o:
			do_insert(shift ? 'O' : 'o');
			break;
		case SDLK_p:
			do_insert(shift ? 'P' : 'p');
			break;
		case SDLK_q:
			do_insert(shift ? 'Q' : 'q');
			break;
		case SDLK_r:
			do_insert(shift ? 'R' : 'r');
			break;
		case SDLK_s:
			do_insert(shift ? 'S' : 's');
			break;
		case SDLK_t:
			do_insert(shift ? 'T' : 't');
			break;
		case SDLK_u:
			do_insert(shift ? 'U' : 'u');
			break;
		case SDLK_v:
			do_insert(shift ? 'V' : 'v');
			break;
		case SDLK_w:
			do_insert(shift ? 'W' : 'w');
			break;
		case SDLK_x:
			do_insert(shift ? 'X' : 'x');
			break;
		case SDLK_y:
			do_insert(shift ? 'Y' : 'y');
			break;
		case SDLK_z:
			do_insert(shift ? 'Z' : 'z');
			break;
		case SDLK_COMMA:
			if (shift)
			{
				do_insert('<');
			}
			break;
		case SDLK_PERIOD:
			if (shift)
			{
				do_insert('>');
			}
			break;
		default:
			break;
		}
	}
}

void midi_output(int channel, int note, int octave, int velocity, int length, void *ctx)
{
	printf("midi: chan %d, not %d, oct %d, vel %d, len %d\n", channel, note, octave, velocity, length);
	(void)ctx;
}

int main(int argc, char *argv[])
{
	int ticknext = 0, tickrun = 0;

	(void)argc;
	(void)argv;

	if (!init())
	{
		return error("Init", "Failure");
	}
	// if (!load_font())
	// {
	// 	return error("Font", "Failure");
	// }

	ecl = ecl_new(HOR, VER, (unsigned long)42);
	ecl_set_output(ecl, &midi_output, 0);

	if (0)
	{
		char *test = "1    >1   ";
		ecl_load_buffer(ecl, test, strlen(test), 0);

		//char *test2 = "G    J     J1      J2      Z    Z0     Z1    Z2   Q ";
		///ecl_load_buffer(ecl, test2, strlen(test2), 128);
	}
	else
	{
		const char *filename = "test.ecl";
		printf("loading from %s\n", filename);
		FILE *file = fopen(filename, "r");
		if (!ecl_load(ecl, file))
		{
			printf("Failed to load %s", filename);
		}
		fclose(file);
		ecl_dump(ecl);

		// FILE *file2 = fopen("test4.ecl", "w");
		// ecl_save(ecl, file2);
		// fclose(file2);
	}
	/* selection is not be part of ecl, 
		only the view manages selection */

	// if(argc > 0)
	// 	if(!loadorca(fopen(argv[1], "r")))
	// 		return error("Load", "Failure");

	select(0, 0, 1, 1);

	while (1)
	{
		int tick = SDL_GetTicks();
		SDL_Event event;
		if (tick < ticknext)
			SDL_Delay(ticknext - tick);
		ticknext = tick + (1000 / FPS);

		if (!PAUSE && tickrun >= 8)
		{
			ecl_eval(ecl);
			//play_midi();
			draw(pixels);
			tickrun = 0;
		}
		tickrun++;

		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_QUIT)
			{
				quit();
			}
			else if (event.type == SDL_MOUSEBUTTONUP ||
					 event.type == SDL_MOUSEBUTTONDOWN ||
					 event.type == SDL_MOUSEMOTION)
			{
				do_mouse(&event);
			}
			else if (event.type == SDL_KEYDOWN)
			{
				do_key(&event);
			}
			else if (event.type == SDL_WINDOWEVENT)
			{
				if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
				{
					draw(pixels);
				}
			}
		}
	}
	quit();
	return 0;
}
