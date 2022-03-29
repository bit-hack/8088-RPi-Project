#include <stdio.h>
#include <fstream>
#include <thread>

#include <unistd.h>

#include <SDL2/SDL.h>

#include "x86.h"
#include "font.h"
#include "vga.h"
#include "timer.h"
#include "drives.h"
#include "keycodes.h"


bool Stop_Flag;


void events_poll();
void Update_Screen(SDL_Window *Window, SDL_Renderer *Renderer);

int main(int argc, char* argv[]) {

    SDL_Window   *window   = NULL;         // Declare a pointer
    SDL_Renderer *renderer = NULL;
    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "pi86",                            // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        720,                               // width, in pixels
        400,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
    );
    if (!window) {
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        return 1;
    }

    //this makes it full screen
    //SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

    //The bios file to load
    CPU_Load_Bios("bios.bin");

    CPU_Start();

    // Drive images a: and C:
    Start_Drives("floppy.img", "hdd.img");

    // Starts the system timer, IRQ0 / INT 0x08
    Start_System_Timer();

    // create a thread to render the screen
    thread screen_loop(Update_Screen, window, renderer);

    while (Stop_Flag != true) {

        CPU_Clock(CPU_TYPE_V20, 100000);

        // check for a stop command
        if(Read_Memory_Byte(0xF80FF) == 0X00) {
            Stop_Flag = true;
            break;
        }

        events_poll();

        // dont burn all of the CPU cycles
        usleep(10);
    }

    screen_loop.join();

    // return from full screen
    //SDL_SetWindowFullscreen(window, 0);

    // Close and destroy the window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    // Clean up
    SDL_Quit();
    return 0;
}

void Update_Screen(SDL_Window *Window, SDL_Renderer *Renderer)
{
    static char Video_Memory_40x25[2000];
    static char Video_Memory_80x25[4000];
    static char Video_Memory_320x200[0x4000];
    static char Cursor_Position[2];

    while (!Stop_Flag)
    {
        while (Read_Memory_Byte(0x00449) == 0x00 ||
               Read_Memory_Byte(0x00449) == 0x01)
        {
            Read_Memory_Array(0xB8000, Video_Memory_40x25, 2000);
            Read_Memory_Array(0x00450, Cursor_Position, 2);
            Mode_0_40x25(Renderer, Video_Memory_40x25, Cursor_Position);
            if (Stop_Flag)
                return;
        }

        while (Read_Memory_Byte(0x00449) == 0x02 ||
               Read_Memory_Byte(0x00449) == 0x03)
        {
            Read_Memory_Array(0xB8000, Video_Memory_80x25, 4000);
            Read_Memory_Array(0x00450, Cursor_Position, 2);
            Mode_2_80x25(Renderer, Video_Memory_80x25, Cursor_Position);
            if (Stop_Flag)
                return;
        }

        while (Read_Memory_Byte(0x00449) == 0x04 &&
               Read_Memory_Byte(0x00466) == 0x00)
        {
            Read_Memory_Array(0xB8000, Video_Memory_320x200, 0x4000);
            Graphics_Mode_320_200_Palette_0(Renderer, Video_Memory_320x200);
            if (Stop_Flag)
                return;
        }

        while (Read_Memory_Byte(0x00449) == 0x04 &&
               Read_Memory_Byte(0x00466) == 0x01)
        {
            Read_Memory_Array(0xB8000, Video_Memory_320x200, 0x4000);
            Graphics_Mode_320_200_Palette_1(Renderer, Video_Memory_320x200);
            if (Stop_Flag)
                return;
        }

        usleep(10);
    }
}

void Insert_Key(char character_code, char scan_code) //Interrupt_9
{
    char Key_Buffer_Tail =  Read_Memory_Byte(0x041C);            // Read the position of the keyboard buffer tail pointer
    Write_Memory_Byte(0x400 + Key_Buffer_Tail, character_code);  // Write Character code at the keyboard buffer tail pointer
    Write_Memory_Byte(0x401 + Key_Buffer_Tail, scan_code);       // Write scan code at the keyboard buffer tail pointer
    Key_Buffer_Tail = Key_Buffer_Tail + 2;                       // Add 2 to the keyboard buffer tail pointer
    if (Key_Buffer_Tail >=  Read_Memory_Byte(0x0482))            // Check to see if the keyboard buffer tail pointer is at the end of the buffer
    {
        Key_Buffer_Tail = Read_Memory_Byte(0x0480);
    }
    Write_Memory_Byte(0x041C, Key_Buffer_Tail);                  // Write the new keyboard buffer tail pointer
}

static void event_keyboard(const SDL_Event &e) {

    const uint32_t ccl = character_codes_lowercase[e.key.keysym.scancode];
    const uint32_t ccu = character_codes_uppercase[e.key.keysym.scancode];
    const uint32_t ccc = character_codes_ctrl[e.key.keysym.scancode];
    const uint32_t sc  = scan_codes[e.key.keysym.scancode];

    switch (e.key.keysym.mod) {
    case KMOD_NONE:
    case KMOD_NUM:
        Insert_Key(ccl, sc);
        break;
    case KMOD_LSHIFT:
    case KMOD_RSHIFT:
        Insert_Key(ccu, sc);
        break;
    case KMOD_LSHIFT | KMOD_NUM:
    case KMOD_RSHIFT | KMOD_NUM:
    case KMOD_LSHIFT | KMOD_RSHIFT:
    case KMOD_LSHIFT | KMOD_RSHIFT | KMOD_NUM:
    case KMOD_CAPS:
        Insert_Key(ccu, sc);
        break;
    case KMOD_CAPS | KMOD_LSHIFT:
    case KMOD_CAPS | KMOD_RSHIFT:
    case KMOD_CAPS | KMOD_LSHIFT | KMOD_RSHIFT:
        Insert_Key(ccl, sc);
        break;
    case KMOD_CAPS | KMOD_NUM:
        Insert_Key(ccu, sc);
        break;
    case KMOD_CAPS | KMOD_LSHIFT | KMOD_NUM:
    case KMOD_CAPS | KMOD_RSHIFT | KMOD_NUM:
    case KMOD_CAPS | KMOD_LSHIFT | KMOD_RSHIFT | KMOD_NUM:
        Insert_Key(ccl, sc);
        break;
    case KMOD_LCTRL:
        if(e.key.keysym.scancode == 0x2A) {
            CPU_Reset();
        }
        Insert_Key(ccc, sc);
        break;
    case KMOD_RCTRL:
        Insert_Key(ccc, sc);
        break;
    case KMOD_LALT:
        if(e.key.keysym.scancode == 0x2A) {
            Write_Memory_Byte(0x00449, 0x02);
        }
        Insert_Key(ccc, sc);
        break;
    case KMOD_RALT:
        break;
    default:
        break;
    }

    Write_IO_Byte(0x0060, scan_codes[e.key.keysym.scancode]);
    IRQ1();
}

void events_poll() {
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT) {
            Stop_Flag = true;
            break;
        }
        if (e.type == SDL_KEYDOWN) {
            event_keyboard(e);
        }
    }
}
