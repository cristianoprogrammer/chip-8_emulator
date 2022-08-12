//
//  main.cpp
//  CHIP-8 Emulator
//
//  Created by Cristiano Douglas on 06/08/18.
//  Copyright © 2018 tydresdevelopment. All rights reserved.
//

#include <iostream>
#include "CHIP8_CPU.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

//#include "nngui.h"

#define _WINDOW_TITLE_ "CHIP-8 Emulator - by Cristiano Douglas"
#define _WINDOW_WIDTH_  640
#define _WINDOW_HEIGHT_ 320

using namespace std;

SDL_Window*     window  = NULL;
SDL_Surface*    screen = NULL;
SDL_Renderer*   renderer = NULL;
SDL_Event       event;
bool            quit_event = false;

CHIP8_CPU* cpu;
CHIP8_ROM* rom;

bool initSDL() {
    
    cout << endl << "·······················································" << endl;
    SDL_version version;
    SDL_GetVersion(&version);
    
    cout << "SDL-Version Major: " << (uint16_t)version.major << " Minor: " << (uint16_t)version.minor << endl;
    
      if ( SDL_Init(SDL_INIT_VIDEO) == 0) {
        
        cout << "SDL-Init Success!" << endl;
        
        window = SDL_CreateWindow(_WINDOW_TITLE_, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _WINDOW_WIDTH_, _WINDOW_HEIGHT_, SDL_WINDOW_SHOWN);
        
        if (window==NULL) {
            
            cout << "SDL-CreateWindow Error: " << SDL_GetError() <<endl;
            return false;
            
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        
        
        return true;
        
    } else {
        
        cout << "SDL-Init Error: " << SDL_GetError() <<endl;
        return false;
    }
    
}

int main(int argc, const char * argv[])
{
    
    for (short i=1; i<argc; i++) {
        cout << argv[i] << endl;
        
    }
    
    cout << "•••••••••••••••••••••••••••••••••••••••••••••••••••••••" << endl;
    cout << "                   CHIP-8 Emulator\n" << endl;
    cout << R"~(CHIP-8 is an interpreted programming language, developed by Joseph Weisbecker. It was initially used on the COSMAC VIP and Telmac 1800 8-bit microcomputers in the mid-1970s. CHIP-8 programs are run on a CHIP-8 virtual machine. It was made to allow video games to be more easily programmed for these computers.)~" << endl << endl;
    cout << "•••••••••••••••••••••••••••••••••••••••••••••••••••••••" << endl << endl;
    cout << "Programming by: Cristiano Douglas" << endl;
    cout << "email: cristianoprogrammer@gmail.com" << endl << endl;
    cout << "•••••••••••••••••••••••••••••••••••••••••••••••••••••••" << endl;
    
    cpu = new CHIP8_CPU();
    rom = new CHIP8_ROM();
    
    if (argc>1) {rom->rom_LOAD(argv[1]);}
    else rom->rom_LOAD("games/Airplane.ch8");
    
    cpu->cpu_INIT();
    cpu->cpu_LOAD_ROM(rom);
    
    initSDL();
    
    std::thread _cpuThread(&CHIP8_CPU::cpu_CYCLE,cpu);
    
    while (!quit_event) {
        
        while (SDL_PollEvent(&event)) {
            
            if (event.type==SDL_QUIT) { cpu->SHUTDOWN_FLAG=true; quit_event=true; }
            else if (event.type==SDL_KEYDOWN) {
                if (event.key.keysym.sym==SDLK_ESCAPE) { cpu->SHUTDOWN_FLAG=true; quit_event=true; }
                else if (event.key.keysym.sym==SDLK_1) { cpu->KEY[0x1] = 0x1; }
                else if (event.key.keysym.sym==SDLK_2) { cpu->KEY[0x2] = 0x1; }
                else if (event.key.keysym.sym==SDLK_3) { cpu->KEY[0x3] = 0x1; }
                else if (event.key.keysym.sym==SDLK_4) { cpu->KEY[0xC] = 0x1; }
                else if (event.key.keysym.sym==SDLK_q) { cpu->KEY[0x4] = 0x1; }
                else if (event.key.keysym.sym==SDLK_w) { cpu->KEY[0x5] = 0x1; }
                else if (event.key.keysym.sym==SDLK_e) { cpu->KEY[0x6] = 0x1; }
                else if (event.key.keysym.sym==SDLK_r) { cpu->KEY[0xD] = 0x1; }
                else if (event.key.keysym.sym==SDLK_a) { cpu->KEY[0x7] = 0x1; }
                else if (event.key.keysym.sym==SDLK_s) { cpu->KEY[0x8] = 0x1; }
                else if (event.key.keysym.sym==SDLK_d) { cpu->KEY[0x9] = 0x1; }
                else if (event.key.keysym.sym==SDLK_f) { cpu->KEY[0xE] = 0x1; }
                else if (event.key.keysym.sym==SDLK_z) { cpu->KEY[0xA] = 0x1; }
                else if (event.key.keysym.sym==SDLK_x) { cpu->KEY[0x0] = 0x1; }
                else if (event.key.keysym.sym==SDLK_c) { cpu->KEY[0xB] = 0x1; }
                else if (event.key.keysym.sym==SDLK_v) { cpu->KEY[0xF] = 0x1; }
                
            }
            else if (event.type==SDL_KEYUP) {
                if (event.key.keysym.sym==SDLK_1) { cpu->KEY[0x1] = 0x0; }
                else if (event.key.keysym.sym==SDLK_2) { cpu->KEY[0x2] = 0x0; }
                else if (event.key.keysym.sym==SDLK_3) { cpu->KEY[0x3] = 0x0; }
                else if (event.key.keysym.sym==SDLK_4) { cpu->KEY[0xC] = 0x0; }
                else if (event.key.keysym.sym==SDLK_q) { cpu->KEY[0x4] = 0x0; }
                else if (event.key.keysym.sym==SDLK_w) { cpu->KEY[0x5] = 0x0; }
                else if (event.key.keysym.sym==SDLK_e) { cpu->KEY[0x6] = 0x0; }
                else if (event.key.keysym.sym==SDLK_r) { cpu->KEY[0xD] = 0x0; }
                else if (event.key.keysym.sym==SDLK_a) { cpu->KEY[0x7] = 0x0; }
                else if (event.key.keysym.sym==SDLK_s) { cpu->KEY[0x8] = 0x0; }
                else if (event.key.keysym.sym==SDLK_d) { cpu->KEY[0x9] = 0x0; }
                else if (event.key.keysym.sym==SDLK_f) { cpu->KEY[0xE] = 0x0; }
                else if (event.key.keysym.sym==SDLK_z) { cpu->KEY[0xA] = 0x0; }
                else if (event.key.keysym.sym==SDLK_x) { cpu->KEY[0x0] = 0x0; }
                else if (event.key.keysym.sym==SDLK_c) { cpu->KEY[0xB] = 0x0; }
                else if (event.key.keysym.sym==SDLK_v) { cpu->KEY[0xF] = 0x0; }
                
            }
            
        }
        
        //if (cpu->DRAW_FLAG) {
            cpu->DRAW_FLAG = false;
            SDL_SetRenderDrawColor(renderer, 156, 186, 41, 255);
            SDL_RenderClear(renderer);
            //cout << endl << endl << endl;
        
            for (int y=0; y<_CHIP8_GFX_HEIGHT_; y++) {
                //cout << endl;
                for (int x=0; x<_CHIP8_GFX_WIDTH_; x++) {
                    if (cpu->GFX[(y*0x40)+x] == 1) {
                        //cout << "#";
                        SDL_Rect rect = {x*10,y*10,10,10};
                        //SDL_SetRenderDrawColor(renderer, 50, 97, 50, 255);
                        SDL_SetRenderDrawColor(renderer, 17, 55, 17, 255);
                        
                        //SDL_SetRenderDrawColor(renderer, 50, 97, 50, 255);
                        SDL_RenderFillRect(renderer, &rect);
                        
                        SDL_SetRenderDrawColor(renderer, 140, 171, 38, 255);
                        //SDL_SetRenderDrawColor(renderer, 17, 55, 17, 255);
                        
                        
                        SDL_RenderDrawRect(renderer, &rect);
                        
                        //SDL_RenderDrawPoint(renderer, x, y);
                    } else {
                        
                        SDL_Rect rect = {x*10,y*10,10,10};
                        //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        //SDL_RenderFillRect(renderer, &rect);
                        SDL_SetRenderDrawColor(renderer, 140, 171, 38, 255);
                        SDL_RenderDrawRect(renderer, &rect);
                        //cout << ".";
                        
                    }

                }
            }
        //}
        SDL_RenderPresent(renderer);
    }
    
    _cpuThread.join();
    
    //delete cpu;
    //delete rom;
    
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
