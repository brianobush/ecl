
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <portmidi.h>
#include <porttime.h>

#include "ecl.h"
#include "font.h"

#define NUM_VOICES 16

// #define MIDI_VOICES 16

Uint32 theme[] = {
    0x000000,
    0x72DEC2,
    0xFFFFFF,
    0x444444,
    0xffff00,
    0x666666,
    0xffb545};

typedef struct
{
    int x, y, w, h;
} rect_t;

typedef struct
{
    int chn, val, vel, len;
} note_t;

typedef struct gui_t
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint32 *pixels;
    int hor, ver, pad, width, height, pause, fps, zoom, device, down;
    //PmStream *midi;
    //note_t *voices;
    rect_t cursor;
    char *clip;
    ecl_t *ecl;
} gui_t;

note_t voices[NUM_VOICES];
PmStream *midi;

#define COLOR_BLACK 0x000000
#define color1 0x000000 /* Black */
#define color2 0x72DEC2 /* Turquoise 0x72DEC2, console green 0x399612 */
#define color3 0xFFFFFF /* White */
#define color4 0x444444 /* Gray */
#define color5 0xffff00 /* Orange cursor */
#define color0 0xffb545 /* Orange cursor */

static void error(char *msg, const char *err)
{
    printf("Error %s: %s\n", msg, err);
    exit(1);
}

int clamp(int val, int min, int max)
{
    return (val >= min) ? (val <= max) ? val : max : min;
}

void midi_output(int channel, int note, int octave, int velocity, int length, void *ctx)
{
    note_t *n;
    int i;
    //gui_t *gui = (gui_t *)ctx;
    printf("midi: chan %d, not %d, oct %d, vel %d, len %d\n", channel, note, octave, velocity, length);
    (void)ctx;
    channel = clamp(channel, 0, 16);
    octave = clamp(octave, 0, 6);
    note = 12 * octave + clamp(note, 0, 12);
    velocity = clamp(velocity, 0, 36);
    length = clamp(length, 0, 36);

    /* turn off */
    for (i = 0; i < NUM_VOICES; i++)
    {
        n = &voices[i];

        if (!n->len || n->chn != channel || n->val != note)
        {
            continue;
        }
        printf("turn off %d\n", i);
        Pm_WriteShort(midi,
                      Pt_Time(),
                      Pm_Message(0x90 + n->chn, n->val, 0));
        n->len = 0;
    }
    for (i = 0; i < NUM_VOICES; i++)
    {
        n = &voices[i];
        if (n->len < 1)
        {
            n->chn = channel;
            n->val = note;
            n->vel = velocity;
            n->len = length;
            printf("sending on channel %d %d\n", n->chn, n->val);
            Pm_WriteShort(midi,
                          Pt_Time(),
                          Pm_Message(0x90 + channel, note, velocity * 3));
            break;
        }
    }
}

gui_t *gui_new()
{
    int i, j;
    gui_t *gui;

    gui = calloc(1, sizeof(gui_t));
    gui->hor = 32;
    gui->ver = 48;
    gui->ecl = ecl_new(gui->hor, gui->ver, (unsigned long)42);
    gui->fps = 30;
    gui->zoom = 2;
    gui->pad = 8;
    gui->width = 8 * gui->hor + gui->pad * 2;
    gui->height = 8 * gui->ver + gui->pad * 2;
    gui->down = 0; /* mouse cursor not down */
    //gui->voices = calloc(MIDI_VOICES, sizeof(note_t));

    // for (i = 0; i < MIDI_VOICES; i++)
    // {
    //     note_t *n = &gui->voices[i];
    //     n->chn = i + 1;
    // }

    ecl_set_output(gui->ecl, &midi_output, &gui);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        error("Init", SDL_GetError());
    }
    gui->window = SDL_CreateWindow("ECL",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   gui->width * gui->zoom,
                                   gui->height * gui->zoom,
                                   SDL_WINDOW_SHOWN);
    if (!gui->window)
    {
        error("Window", SDL_GetError());
    }
    gui->renderer = SDL_CreateRenderer(gui->window, -1, 0);
    if (!gui->renderer)
    {
        error("Renderer", SDL_GetError());
    }
    gui->texture = SDL_CreateTexture(gui->renderer,
                                     SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STATIC,
                                     gui->width,
                                     gui->height);
    if (!gui->texture)
    {
        error("Texture", SDL_GetError());
    }
    gui->clip = malloc(gui->hor * gui->ver * sizeof(char));
    gui->pixels = (Uint32 *)malloc(gui->width * gui->height * sizeof(Uint32));
    if (!gui->pixels)
    {
        error("Pixels", "Failed to allocate memory");
    }
    for (i = 0; i < gui->height; i++)
    {
        for (j = 0; j < gui->width; j++)
        {
            gui->pixels[i * gui->width + j] = COLOR_BLACK;
        }
    }

    /* Now init midi */
    gui->device = 1;
    PmError e;
    e = Pm_Initialize();
    printf("midi init error? %d\n", e);
    int num_devices = 0;
    for (i = 0; i < Pm_CountDevices(); i++)
    {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (!info)
            continue;
        num_devices++;
        char const *name = info->name;
        printf("Device #%d -> %s%s\n", i, name, i == gui->device ? "[x]" : "[ ]");
    }
    if (num_devices == 0)
    {
        printf("Error: No PortMIDI output devices detected.\n");
    }

    //PmStream *midi = gui->midi;
    ///Pm_OpenOutput(&midi, gui->device, NULL, 128, 0, NULL, 1);
    e = Pm_OpenOutput(&midi, gui->device, NULL, 0, NULL, NULL, 1);
    printf("midi open output error? %s\n", Pm_GetErrorText(e));

    return gui;
}

void gui_free(gui_t *gui)
{

    if (gui)
    {
        SDL_DestroyTexture(gui->texture);
        SDL_DestroyRenderer(gui->renderer);
        SDL_DestroyWindow(gui->window);
        SDL_Quit();
        ecl_free(gui->ecl);
        //free(gui->voices);
        free(gui->clip);
        free(gui->pixels);
        free(gui);
    }
}

void put_pixel(gui_t *gui, int x, int y, int color)
{
    if (x >= 0 && x < gui->width - 8 && y >= 0 && y < gui->height - 8)
        gui->pixels[(y + gui->pad) * gui->width + (x + gui->pad)] = theme[color];
}

void do_insert(gui_t *gui, char c)
{
    printf("x %d, y %d, c %c\n", gui->cursor.x, gui->cursor.y, c);
    int pos = (gui->cursor.x * gui->ver) + gui->cursor.y;
    printf("insert %d\n", pos);
    ecl_set(gui->ecl, pos, c);
}

void do_zoom(gui_t *gui, int mod)
{
    if ((mod > 0 && gui->zoom < 4) || (mod < 0 && gui->zoom > 1))
    {
        gui->zoom += mod;
        printf("Seeting zoom %d\n", gui->zoom);
        SDL_SetWindowSize(gui->window, gui->width * gui->zoom, gui->height * gui->zoom);
    }
}

int selected(gui_t *gui, int x, int y)
{
    return x < gui->cursor.x + gui->cursor.w &&
           x >= gui->cursor.x &&
           y < gui->cursor.y + gui->cursor.h &&
           y >= gui->cursor.y;
}

/* TODO: rewrite */
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
    printf("Unhandled %c\n", c);
    return c;
}

/* TODO: rewrite */
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

void draw_tile(gui_t *gui, int x, int y, char c, int type)
{
    int v, h;

    int sel = selected(gui, x, y);
    Uint8 *bitmap = font[get_font(x, y, c, type, sel)];

    for (v = 0; v < 8; v++)
    {
        for (h = 0; h < 8; h++)
        {
            int style = get_style((bitmap[v] & 1 << h), type, sel);
            put_pixel(gui, x * 8 + h, y * 8 + v, style);
        }
    }
}

void gui_draw(gui_t *gui)
{
    int i, x, y, state, style;
    // 1 is faint = TYPE_EMPTY
    // 2 is regular, but turquoise  = TYPE_ARGS
    // 3 is bold = TYPE_COMMAND
    // 4 is regular = TYPE_NUMBER
    // 5 is inverted = TYPE_OUPUT

    for (y = 0; y < gui->hor; y++)
    {
        for (x = 0; x < gui->ver; x++)
        {
            i = y * gui->ver + x;
            state = ecl_get_state(gui->ecl, i);
            switch (state)
            {
            case STATE_CMD:
                style = 3;
                break;
            case STATE_NUM:
            case STATE_ARG:
            case STATE_NEW:
                style = 2;
                break;
                /// 5?
            case STATE_EMPTY:
            default:
                style = 1;
                break;
            }
            draw_tile(gui, y, x, ecl_get(gui->ecl, i), style);
        }
    }
    SDL_UpdateTexture(gui->texture, NULL, gui->pixels, gui->width * sizeof(Uint32));
    SDL_RenderClear(gui->renderer);
    SDL_RenderCopy(gui->renderer, gui->texture, NULL, NULL);
    SDL_RenderPresent(gui->renderer);
}

void do_select(gui_t *gui, int x, int y, int w, int h)
{
    gui->cursor.x = clamp(x, 0, gui->hor - 1);
    gui->cursor.y = clamp(y, 0, gui->ver - 1);
    gui->cursor.w = clamp(w, 1, 36);
    gui->cursor.h = clamp(h, 1, 36);
    gui_draw(gui);
}

void do_move(gui_t *gui, int x, int y)
{
    do_select(gui, gui->cursor.x + x, gui->cursor.y + y, gui->cursor.w, gui->cursor.h);
}

int get_cell(gui_t *gui, int x, int y)
{
    return y + (x * gui->ver);
}

// int set_cell(gui_t *gui, int x, int y, char v)
// {
//     return y + (x * gui->ver);
// }

void copy_clip(gui_t *gui)
{
    int x, y, i = 0;

    printf("cursor x %d, y %d\n", gui->cursor.x, gui->cursor.y);
    for (x = 0; x < gui->cursor.w; x++)
    {
        for (y = 0; y < gui->cursor.h; y++)
        {
            gui->clip[i++] = ecl_get(gui->ecl, get_cell(gui, gui->cursor.x + x, gui->cursor.y + y));
        }
        gui->clip[i++] = '\n';
    }
    gui->clip[i] = 0;
    gui_draw(gui);
}

void paste_clip(gui_t *gui)
{
    int i = 0, x, y;
    char ch;

    x = gui->cursor.x;
    y = gui->cursor.y;
    while ((ch = gui->clip[i++]))
    {
        if (ch == '\n')
        {
            // x = gui->cursor.x;
            // y++;
            y = gui->cursor.y;
            x++;
        }
        else
        {
            ecl_set(gui->ecl, get_cell(gui, x, y), ch);
            //x++;
            y++;
        }
    }
    gui_draw(gui);
}

void cut_clip(gui_t *gui)
{
    int x, y;

    copy_clip(gui);

    for (x = 0; x < gui->cursor.w; x++)
        for (y = 0; y < gui->cursor.h; y++)
            ecl_set(gui->ecl, get_cell(gui, gui->cursor.x + x, gui->cursor.y + y), '.');
    gui_draw(gui);
}

static void gui_scankey(gui_t *gui, SDL_Event *event)
{
    int shift = SDL_GetModState() & KMOD_LSHIFT || SDL_GetModState() & KMOD_RSHIFT;
    int ctrl = SDL_GetModState() & KMOD_LCTRL || SDL_GetModState() & KMOD_RCTRL ||
               SDL_GetModState() & KMOD_LGUI || SDL_GetModState() & KMOD_RGUI;

    if (ctrl)
    {
        switch (event->key.keysym.sym)
        {
        case SDLK_x:
            cut_clip(gui);
            break;
        case SDLK_c:
            copy_clip(gui);
            break;
        case SDLK_v:
            paste_clip(gui);
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
            do_zoom(gui, 1);
            break;
        case SDLK_UNDERSCORE:
        case SDLK_MINUS:
            do_zoom(gui, -1);
            break;
        case SDLK_SPACE:
            gui->pause = !gui->pause;
            break;
        case SDLK_ESCAPE:
            do_select(gui, 0, 0, 1, 1);
            break;
        case SDLK_UP:
            do_move(gui, 0, -1);
            break;
        case SDLK_DOWN:
            do_move(gui, 0, 1);
            break;
        case SDLK_LEFT:
            do_move(gui, -1, 0);
            break;
        case SDLK_RIGHT:
            do_move(gui, 1, 0);
            break;
        case SDLK_SLASH:
            do_insert(gui, '?');
            break;
        case SDLK_BACKSPACE:
            do_insert(gui, 0);
            break;
        case SDLK_0:
            do_insert(gui, '0');
            break;
        case SDLK_1:
            do_insert(gui, '1');
            break;
        case SDLK_2:
            do_insert(gui, '2');
            break;
        case SDLK_3:
            do_insert(gui, '3');
            break;
        case SDLK_4:
            do_insert(gui, (shift) ? '$' : '4');
            break;
        case SDLK_5:
            do_insert(gui, '5');
            break;
        case SDLK_6:
            do_insert(gui, '6');
            break;
        case SDLK_7:
            do_insert(gui, '7');
            break;
        case SDLK_8:
            do_insert(gui, '8');
            break;
        case SDLK_9:
            do_insert(gui, '9');
            break;
        case SDLK_a:
            do_insert(gui, shift ? 'A' : 'a');
            break;
        case SDLK_b:
            do_insert(gui, shift ? 'B' : 'b');
            break;
        case SDLK_c:
            do_insert(gui, shift ? 'C' : 'c');
            break;
        case SDLK_d:
            do_insert(gui, shift ? 'D' : 'd');
            break;
        case SDLK_e:
            do_insert(gui, shift ? 'E' : 'e');
            break;
        case SDLK_f:
            do_insert(gui, shift ? 'F' : 'f');
            break;
        case SDLK_g:
            do_insert(gui, shift ? 'G' : 'g');
            break;
        case SDLK_h:
            do_insert(gui, shift ? 'H' : 'h');
            break;
        case SDLK_i:
            do_insert(gui, shift ? 'I' : 'i');
            break;
        case SDLK_j:
            do_insert(gui, shift ? 'J' : 'j');
            break;
        case SDLK_k:
            do_insert(gui, shift ? 'K' : 'k');
            break;
        case SDLK_l:
            do_insert(gui, shift ? 'L' : 'l');
            break;
        case SDLK_m:
            do_insert(gui, shift ? 'M' : 'm');
            break;
        case SDLK_n:
            do_insert(gui, shift ? 'N' : 'n');
            break;
        case SDLK_o:
            do_insert(gui, shift ? 'O' : 'o');
            break;
        case SDLK_p:
            do_insert(gui, shift ? 'P' : 'p');
            break;
        case SDLK_q:
            do_insert(gui, shift ? 'Q' : 'q');
            break;
        case SDLK_r:
            do_insert(gui, shift ? 'R' : 'r');
            break;
        case SDLK_s:
            do_insert(gui, shift ? 'S' : 's');
            break;
        case SDLK_t:
            do_insert(gui, shift ? 'T' : 't');
            break;
        case SDLK_u:
            do_insert(gui, shift ? 'U' : 'u');
            break;
        case SDLK_v:
            do_insert(gui, shift ? 'V' : 'v');
            break;
        case SDLK_w:
            do_insert(gui, shift ? 'W' : 'w');
            break;
        case SDLK_x:
            do_insert(gui, shift ? 'X' : 'x');
            break;
        case SDLK_y:
            do_insert(gui, shift ? 'Y' : 'y');
            break;
        case SDLK_z:
            do_insert(gui, shift ? 'Z' : 'z');
            break;
        case SDLK_COMMA:
            if (shift)
            {
                do_insert(gui, '<');
            }
            break;
        case SDLK_PERIOD:
            if (shift)
            {
                do_insert(gui, '>');
            }
            break;
        default:
            break;
        }
    }
}

void do_mouse(gui_t *gui, SDL_Event *event)
{
    int x = (event->motion.x - (gui->pad * gui->zoom)) / gui->zoom;
    int y = (event->motion.y - (gui->pad * gui->zoom)) / gui->zoom;
    //printf("mouse x %d, y %d\n", x, y);
    switch (event->type)
    {
    case SDL_MOUSEBUTTONUP:
        gui->down = 0;
        break;
    case SDL_MOUSEBUTTONDOWN:
        do_select(gui, x / 8, y / 8, 1, 1);
        gui->down = 1;
        break;
    case SDL_MOUSEMOTION:
        if (gui->down)
            do_select(gui,
                      gui->cursor.x,
                      gui->cursor.y,
                      x / 8 - gui->cursor.x + 1,
                      y / 8 - gui->cursor.y + 1);
        break;
    }
}

void run_midi(gui_t *gui)
{
    int i;
    (void)gui;
    for (i = 0; i < NUM_VOICES; ++i)
    {
        note_t *n = &voices[i];
        if (n->len > 0)
        {
            n->len--;
            if (n->len == 0)
                Pm_WriteShort(midi,
                              Pt_Time(),
                              Pm_Message(0x90 + n->chn, n->val, 0));
        }
    }
}

void gui_loop(gui_t *gui)
{
    int tick, ticknext = 0, tickrun = 0, quit = 0;
    SDL_Event event;

    while (!quit)
    {
        //printf("looping\n");
        tick = SDL_GetTicks();
        if (tick < ticknext)
            SDL_Delay(ticknext - tick);
        ticknext = tick + (1000 / gui->fps);

        if (!gui->pause && tickrun >= 8)
        {
            ecl_eval(gui->ecl);
            run_midi(gui);
            gui_draw(gui);
            tickrun = 0;
        }
        tickrun++;

        while (SDL_PollEvent(&event) != 0 && !quit)
        {
            if (event.type == SDL_QUIT)
            {
                printf("see ya!\n");
                quit = 1;
            }
            else if (event.type == SDL_MOUSEBUTTONUP ||
                     event.type == SDL_MOUSEBUTTONDOWN ||
                     event.type == SDL_MOUSEMOTION)
            {
                do_mouse(gui, &event);
            }
            else if (event.type == SDL_KEYDOWN)
            {
                gui_scankey(gui, &event);
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
                {
                    gui_draw(gui);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    int i;
    const char *fn = 0;
    gui_t *gui;

    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-f"))
        {
            if (i < argc - 1)
            {
                fn = argv[++i];
            }
        }
    }
    gui = gui_new();
    if (fn)
    {
        FILE *file = fopen(fn, "r");
        if (!ecl_load(gui->ecl, file))
        {
            printf("Failed to load %s", fn);
        }
        fclose(file);
    }
    gui_loop(gui);
    gui_free(gui);
    return 0;
}
