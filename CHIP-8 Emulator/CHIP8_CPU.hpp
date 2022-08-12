//
//  CHIP8_CPU.hpp
//  CHIP-8 Emulator
//
//  Created by Cristiano Douglas on 06/08/18.
//  Copyright © 2018 tydresdevelopment. All rights reserved.
//

#ifndef CHIP8_CPU_hpp
#define CHIP8_CPU_hpp

#include <stdio.h>
#include <ctime>
#include <chrono>
#include <thread>
#include <bitset>
#include <vector>

#include "CHIP8_ROM.hpp"

#define _RELEASE_             (0x0)
#define _DEBUG_               (0x1)
#define _MODE_                _DEBUG_

#define CH_printf           \/\/printf

#define _CHIP8_MEMORY_SIZE_         (0x1000)
#define _CHIP8_GFX_WIDTH_           (0x40)
#define _CHIP8_GFX_HEIGHT_          (0x20)
#define _CHIP8_REGISTERS_SIZE_      (0x10)
#define _CHIP8_STACK_SIZE_          (0x10)
#define _CHIP8_KEY_SIZE_            (0x10)
#define _CHIP8_MEMORY_FONTSET_SIZE_ (0x200)
#define _CHIP8_MEMORY_I_START_      (0x200)
#define _CHIP8_FONTSET_SIZE_        (0x50)

#define _CHIP8_FONTSET_0_           0xF0, 0x90, 0x90, 0x90, 0xF0
#define _CHIP8_FONTSET_1_           0x20, 0x60, 0x20, 0x20, 0x70
#define _CHIP8_FONTSET_2_           0xF0, 0x10, 0xF0, 0x80, 0xF0
#define _CHIP8_FONTSET_3_           0xF0, 0x10, 0xF0, 0x10, 0xF0
#define _CHIP8_FONTSET_4_           0x90, 0x90, 0xF0, 0x10, 0x10
#define _CHIP8_FONTSET_5_           0xF0, 0x80, 0xF0, 0x10, 0xF0
#define _CHIP8_FONTSET_6_           0xF0, 0x80, 0xF0, 0x90, 0xF0
#define _CHIP8_FONTSET_7_           0xF0, 0x10, 0x20, 0x40, 0x40
#define _CHIP8_FONTSET_8_           0xF0, 0x90, 0xF0, 0x90, 0xF0
#define _CHIP8_FONTSET_9_           0xF0, 0x90, 0xF0, 0x10, 0xF0
#define _CHIP8_FONTSET_A_           0xF0, 0x90, 0xF0, 0x90, 0x90
#define _CHIP8_FONTSET_B_           0xE0, 0x90, 0xE0, 0x90, 0xE0
#define _CHIP8_FONTSET_C_           0xF0, 0x80, 0x80, 0x80, 0xF0
#define _CHIP8_FONTSET_D_           0xE0, 0x90, 0x90, 0x90, 0xE0
#define _CHIP8_FONTSET_E_           0xF0, 0x80, 0xF0, 0x80, 0xF0
#define _CHIP8_FONTSET_F_           0xF0, 0x80, 0xF0, 0x80, 0x80

//#define _CHIP8_KEYMAP_              

/******************************************************************************/
// CHIP-8 CPU
/******************************************************************************/
class CHIP8_CPU {
    
    /*........................................................................*/
    // Memory (4096 bytes)
    uint8_t     MEMORY[_CHIP8_MEMORY_SIZE_];
    
    // Operation code (35 CPU Instructions)
    uint16_t    OPCODE;
    
    // Registers (16 CPU Registers)
    uint8_t     V_REGISTER[_CHIP8_REGISTERS_SIZE_];
    
    // Index Register and Program Counter
    uint16_t    IR;
    uint16_t    PC;
    
    // Stack
    uint16_t    STACK[_CHIP8_STACK_SIZE_];
    uint16_t    SP;
    
    // Timers
    uint8_t     DELAY_TIMER;
    uint8_t     SOUND_TIMER;
    
    // Fontset
    uint8_t     FONTSET[_CHIP8_FONTSET_SIZE_] = {
        _CHIP8_FONTSET_0_, _CHIP8_FONTSET_1_, _CHIP8_FONTSET_2_,
        _CHIP8_FONTSET_3_, _CHIP8_FONTSET_4_, _CHIP8_FONTSET_5_,
        _CHIP8_FONTSET_6_, _CHIP8_FONTSET_7_, _CHIP8_FONTSET_8_,
        _CHIP8_FONTSET_9_, _CHIP8_FONTSET_A_, _CHIP8_FONTSET_B_,
        _CHIP8_FONTSET_C_, _CHIP8_FONTSET_D_, _CHIP8_FONTSET_E_,
        _CHIP8_FONTSET_F_
    };
    
    chrono::time_point<chrono::steady_clock, chrono::nanoseconds>     CURRENT_TIMER;
    
   
public:
    // Keys (Hex keyboard)
    uint8_t     KEY[_CHIP8_KEY_SIZE_];
    
     // Graphics (64x32)
    uint8_t     GFX[_CHIP8_GFX_WIDTH_*_CHIP8_GFX_HEIGHT_];
    bool        CLEAR_FLAG;
    bool        DRAW_FLAG;
    bool        SHUTDOWN_FLAG;
    
    /*........................................................................*/
    
public:
    
    /*........................................................................*/
    ~CHIP8_CPU();
    
    void    cpu_INIT();
    void    cpu_LOAD_ROM(CHIP8_ROM*);
    void    cpu_CYCLE();
    
    /*........................................................................*/
};
/******************************************************************************/

#endif /* CHIP8_CPU_hpp */
