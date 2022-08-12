 //
//  CHIP8_CPU.cpp
//  CHIP-8 Emulator
//
//  Created by Cristiano Douglas on 06/08/18.
//  Copyright © 2018 tydresdevelopment. All rights reserved.
//

#include "CHIP8_CPU.hpp"

/******************************************************************************/
/**
 Release memory
 */
CHIP8_CPU::~CHIP8_CPU() {
    
    free(this->MEMORY);
}

/******************************************************************************/
/**
 Initialize and Reset
 */
void CHIP8_CPU::cpu_INIT() {
    
    // Initialize Opcode = 0x0000
    this->OPCODE = 0x0000;
    
    // Program Counter to 0x200
    this->PC = 0x200;
    
    // Initialize Index = 0x0000
    this->IR = 0x0000;
    
    // Initialize Stack Pointer = 0x0000
    this->SP = 0x0000;
    
    // Initialize Time
    this->CURRENT_TIMER = chrono::high_resolution_clock::now();
    
    // Load fontset to memory
    for (uint8_t mem_idx=0; mem_idx<0x50; mem_idx++) {
        this->MEMORY[mem_idx] = this->FONTSET[mem_idx];
    }
    
    this->SHUTDOWN_FLAG = false;
    
}

/******************************************************************************/
/**
 Load the ROM Data to Memory

 @param _rom CHIP8_ROM
 */
void CHIP8_CPU::cpu_LOAD_ROM(CHIP8_ROM *_rom) {
    
    char* buffer_data = (char*) malloc(sizeof(char)*_rom->rom_GET_DATA_SIZE());
    memcpy(buffer_data,_rom->rom_GET_DATA(),sizeof(char)*_rom->rom_GET_DATA_SIZE());
    
    cout << endl << "·······················································" << endl;
    cout << "Copying ROM Data to CHIP-8 Memory" << endl;
    cout << "ROM Data Size: " << _rom->rom_GET_DATA_SIZE() << endl;
    cout << "·······················································" << endl;
    
    // Copy ROM data to CHIP-8 Memory
    for (int mem_index = 0; mem_index<_rom->rom_GET_DATA_SIZE(); mem_index++) {

        uint8_t data_byte = (uint8_t)*(buffer_data+mem_index);
        //cout << setfill('0') << setw(4) << hex << (unsigned short)data_byte << dec << " ";
        printf("%.2X", data_byte);
        //memcpy(this->MEMORY+(_CHIP8_MEMORY_I_START_+mem_index),buffer_data+mem_index,sizeof(char));
        *(this->MEMORY+(_CHIP8_MEMORY_I_START_+mem_index)) = *(buffer_data+mem_index);
        
    }
    
    cout << endl << "·······················································" << endl;
    
    free(buffer_data);
}

/******************************************************************************/
/**
 Process the CPU Emulating Cycle
 */
void CHIP8_CPU::cpu_CYCLE() {
    
    //system("clear");
    
    while (!this->SHUTDOWN_FLAG) {
        
        auto time = chrono::high_resolution_clock::now();//(double)(clock())
        const auto duration_msecs = chrono::duration_cast<chrono::milliseconds>(time-this->CURRENT_TIMER).count();
       
        //cout << " Duration: " << duration_msecs << endl;
        int expected_time = (1000/60);

        if (duration_msecs<expected_time) {
            
            //return false;
            //this->CURRENT_TIMER = chrono::high_resolution_clock::now();
            //clock_t remaining_time = expected_time-duration_msecs;
            //cout << " Remaining Time: " << remaining_time << endl;
            this_thread::sleep_for(chrono::milliseconds(4));
            
            
        }
        this->CURRENT_TIMER = chrono::high_resolution_clock::now();
        
        if (this->DELAY_TIMER>0x00) this->DELAY_TIMER -= 0x01;
        if (this->SOUND_TIMER>0x00) {
            this->SOUND_TIMER -= 0x01;
            if (this->SOUND_TIMER==0x00) {
                
            #ifdef __APPLE__
                //system("afplay /System/Library/Sounds/Tink.aiff");
            #endif
            }
        }
        
        this->OPCODE = this->MEMORY[this->PC] << 8 | this->MEMORY[this->PC+0x01];
        
        printf("\n0x%.4X - ",this->OPCODE);
        switch (this->OPCODE & 0xF000 ) {
            
            case 0x0000: // 0 Family
            {
                
                switch (this->OPCODE & 0x00FF) {
                        
                    case 0x00E0: // 00E0 - CLEAR THE DISPLAY
                    {
                        this->CLEAR_FLAG = true;
                        for (uint8_t y=0x00;y<_CHIP8_GFX_HEIGHT_;y++) {
                            for (uint8_t x=0x00; x<_CHIP8_GFX_WIDTH_; x++) {
                                this->GFX[(x)+((y)*64)] = 0;
                            }
                        }
                        this->DRAW_FLAG = true;
                        this->PC+=0x02;
                        
                        break;
                    }
                    case 0x00EE: // 00EE - RETURN FROM SUBROUTINE
                    {
                        
                        this->PC = this->STACK[this->SP-0x01];
                        this->SP -= 0x01;
                        //printf("LAST-eRETURN *[%d]\n",this->PC);
                        this->PC+=0x02;
                        printf("RTN *[%d]\n",this->PC);
                        
                        break;
                    }
                    default:
                        this->PC+=0x02;
                        break;
                }
                break;
            }
                
                
            case 0x1000: // 1NNN - JMP NNN
            {

                uint16_t addr = (uint16_t)(this->OPCODE & 0x0FFF);
                this->PC = addr;
                
                printf("JMP %.3X \n",addr);
                break;
            }
                
            case 0x2000: // 2NNN - JSR NNN
            {
                
                uint16_t addr = (uint16_t)(this->OPCODE & 0x0FFF);
                this->STACK[this->SP] = this->PC;
                this->SP += 0x01;
                printf("JSR %d - STACK[%d]\n",addr,this->PC);
                this->PC = addr;
                
                break;
            }
                
            case 0x3000: // 3XNN - SE VX,NN
            {
                uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                uint8_t byte = (uint8_t)(this->OPCODE & 0x00FF);
                
                if (this->V_REGISTER[reg_x]==byte)
                    this->PC+=0x04;
                else
                    this->PC+=0x02;
                
                printf("SE [%.1X],%.2X\n", (uint16_t)reg_x,(uint16_t)byte);
                break;
            }
                
            case 0x4000: // 4XNN - SNE VX,NN
            {
                uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                uint8_t byte = (uint8_t)(this->OPCODE & 0x00FF);
                
                if (this->V_REGISTER[reg_x]!=byte)
                    this->PC+=0x04;
                else
                    this->PC+=0x02;
                
                printf("SNE [%.1X],%.2X\n", (uint16_t)reg_x,(uint16_t)byte);
                break;
            }
                
            case 0x5000: // 5XY0 - SE VX,VY
            {
                uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
      
                if (this->V_REGISTER[reg_x]==this->V_REGISTER[reg_y])
                    this->PC+=0x04;
                else
                    this->PC+=0x02;
                
                printf("SE [%.1X],V[%.1X]\n", (uint16_t)reg_x,(uint16_t)reg_y);
                break;
            }
                
            case 0x6000: // 6XNN - MOV VX,RR
            {
                uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                uint8_t reg_d = (uint8_t)(this->OPCODE & 0x00FF);
                this->V_REGISTER[reg_x] = reg_d;
                this->PC+=0x02;
                printf("MOV V[%.1X], %d \n", (uint16_t)reg_x,(uint16_t)reg_d);
            break;
            }
                
            case 0x7000: // 7XNN - ADD VX,NN
            {
                uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                uint8_t reg_d = (uint8_t)(this->OPCODE & 0x00FF);
                
                this->V_REGISTER[reg_x] += reg_d;
                this->PC+=0x02;
                printf("ADD V[%.1X], %d \n", (uint16_t)reg_x,(uint16_t)reg_d);
            break;
            }
                
            case 0x8000: // 8 FAMILY
            {
                switch (this->OPCODE & 0x000F) {
                        
                    case 0x0000: // // 8XY0 - MOV VX,VY
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                        
                        this->V_REGISTER[reg_x] = this->V_REGISTER[reg_y];
                        this->PC+=0x02;
                        printf("MOV V[%.1X],V[%.1X] \n", (uint16_t)reg_x,(uint16_t)reg_y);
                        
                        break;

                    }
                        
                    case 0x0001 : // // 8XY1 - XOR VX,VY
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                        
                        this->V_REGISTER[reg_x] = this->V_REGISTER[reg_x] ^ this->V_REGISTER[reg_y];
                        
                        this->PC+=0x02;
                        printf("XOR V[%.1X],V[%.1X] \n", (uint16_t)reg_x,(uint16_t)reg_y);
                        
                        break;
                        
                    }
                        
                    case 0x0002 : // // 8XY2 - AND VX,VY
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                        
                        this->V_REGISTER[reg_x] = this->V_REGISTER[reg_x] & this->V_REGISTER[reg_y];
                        
                        this->PC+=0x02;
                        printf("AND V[%.1X],V[%.1X] \n", (uint16_t)reg_x,(uint16_t)reg_y);
                        
                        break;
                        
                    }
                        
                    case 0x0003 : // // 8XY3 - XOR VX,VY
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                        
                        this->V_REGISTER[reg_x] = this->V_REGISTER[reg_x] ^ this->V_REGISTER[reg_y];
                        
                        this->PC+=0x02;
                        printf("XOR V[%.1X],V[%.1X] \n", (uint16_t)reg_x,(uint16_t)reg_y);
                        
                        break;
                        
                    }
                        
                    case 0x0004 : // // 8XY4 - ADD VX,VY
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                        
                        uint16_t result = this->V_REGISTER[reg_x] + this->V_REGISTER[reg_y];
                        
                        if (result>0xFF) this->V_REGISTER[0xF] = 1;
                        else this->V_REGISTER[0xF] = 0;
                        
                        this->V_REGISTER[reg_x] = (uint8_t)result;
                        
                        this->PC+=0x02;
                        printf("ADD V[%.1X],V[%.1X] \n", (uint16_t)reg_x,(uint16_t)reg_y);
                        
                        break;
                        
                    }
                        
                    case 0x0005 : // // 8XY5 - SUB VX,VY
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                        
                        uint16_t result = (uint16_t)this->V_REGISTER[reg_x] - this->V_REGISTER[reg_y];
                        
                        if (this->V_REGISTER[reg_x] > this->V_REGISTER[reg_y]) this->V_REGISTER[0xF] = 1;
                        else this->V_REGISTER[0xF] = 0;
                        
                        this->V_REGISTER[reg_x] = (uint8_t)result;
                        
                        this->PC+=0x02;
                        printf("SUB V[%.1X],V[%.1X] \n", (uint16_t)reg_x,(uint16_t)reg_y);
                        
                        break;
                        
                    }
                        
                    case 0x0006 : // // 8XY6 - SHR VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        this->V_REGISTER[0xF] = this->V_REGISTER[reg_x] & 0xFE;
                        
                        this->V_REGISTER[reg_x] = (uint8_t)(this->V_REGISTER[reg_x] >> 1);
                        
                        this->PC+=0x02;
                        printf("SHR V[%.1X]\n", (uint16_t)reg_x);
                        
                        break;
                        
                    }
                        
                    case 0x0007 : // // 8XY7 - SUBN VX,VY
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                        
                        uint16_t result = (uint16_t)this->V_REGISTER[reg_y] - this->V_REGISTER[reg_x];
                        
                        if (this->V_REGISTER[reg_y] > this->V_REGISTER[reg_x]) this->V_REGISTER[0xF] = 1;
                        else this->V_REGISTER[0xF] = 0;
                        
                        this->V_REGISTER[reg_x] = (uint8_t)result;
                        
                        this->PC+=0x02;
                        printf("SUBN V[%.1X],V[%.1X] \n", (uint16_t)reg_x,(uint16_t)reg_y);
                        
                        break;
                        
                    }
                        
                    case 0x000E : // // 8XY6 - SHL VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        this->V_REGISTER[0xF] = this->V_REGISTER[reg_x] >> 7;
                        
                        this->V_REGISTER[reg_x] = (uint8_t)(this->V_REGISTER[reg_x] << 1);
                        
                        this->PC+=0x02;
                        printf("SHL V[%.1X]\n", (uint16_t)reg_x);
                        
                        break;
                        
                    }
                    default:
                        break;
                }
                
                break;
            }
                
            case 0x9000: // 9XY0 - SNE VX,VY
            {
                uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                
                if (this->V_REGISTER[reg_x]!=this->V_REGISTER[reg_y])
                    this->PC+=0x04;
                else
                    this->PC+=0x02;
                
                printf("SNE [%.1X],V[%.1X]\n", (uint16_t)reg_x,(uint16_t)reg_y);
                break;
            }
            
            case 0xA000: // ANNN - MOV I,NNN
            {
                uint16_t addr = (uint16_t)(this->OPCODE & 0x0FFF);
                
                this->IR = addr;
                this->PC+=0x02;
                printf("MOV I, %.3X \n", addr);
                break;
            }
                
            case 0xB000: // BNNN - JMP V0+NNN
            {
                uint16_t pcloc = (uint16_t)(this->OPCODE & 0x0FFF);
                
                this->PC+=pcloc+this->V_REGISTER[0x0];
                printf("JMP V0+0x%.3X \n", pcloc);
                break;
            }
                
            case 0xC000: // CXNN - RND VX,NN
            {
                uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                uint8_t nn = (uint8_t)(this->OPCODE & 0x00FF);
                
                this->V_REGISTER[reg_x] = (rand() % 255) & nn;
                this->PC+=0x02;
                printf("RND V[%.1X]&%.X2\n", (uint16_t)reg_x,(uint16_t)nn);
                break;
            }
                
            case 0xD000: // DXYN - DRAW VX,VY,N
            {
                uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                uint8_t reg_y = (uint8_t)((this->OPCODE & 0x00F0) >> 4);
                uint8_t height = (uint8_t)(this->OPCODE & 0x000F);
                uint8_t dx = this->V_REGISTER[reg_x];
                uint8_t dy = this->V_REGISTER[reg_y];
                printf("DRAW X: %d, Y: %d\n", (uint16_t)dx,(uint16_t)dy);
                
                uint16_t alloc_size = sizeof(uint8_t)*0x08*height;
                uint8_t* sprite_buffer = (uint8_t*)malloc(alloc_size);
                
                // Read sprite
                memcpy(sprite_buffer, this->MEMORY+this->IR, alloc_size);
                this->V_REGISTER[0xF] = 0;
                for (uint8_t y=0x00;y<height;y+=0x01) {
                    //cout << endl;
                    uint8_t mono_pixel = (uint8_t)*(sprite_buffer+y);
                    for (uint8_t x=0x00; x<0x08; x+=0x01) {
                        uint8_t pixel_byte = (uint8_t)(mono_pixel & (0x80 >> x));
                        bitset<1> bit_set(pixel_byte);
                        //cout << bit_set;
                        uint16_t c_pos=(dx+x)+((dy+y)*0x40);
                        if (pixel_byte != 0) {
                            if (this->GFX[c_pos]==1)
                                this->V_REGISTER[0xF] = 1;
                            this->GFX[c_pos] ^= 1;
                        } else {
                            //void* none = NULL;
                        }
                        
                    }
                }
                this->DRAW_FLAG = true;
                //cout << endl;
                this->PC+=0x02;
                
                break;
            }
                
            case 0xE000: // E FAMILY
            {
                switch (this->OPCODE & 0x00FF) {
                        
                    case 0x009E: // // EX9E - SKP VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        if (this->KEY[this->V_REGISTER[reg_x]]==1){
                            this->PC+=0x04;
                        } else {
                            this->PC+=0x02;
                        }
                        printf("SKP V[%.1X] \n", (uint16_t)reg_x);
                        
                        break;
                        
                    }
                    case 0x00A1: // // EXA1 - SKNP VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        if (this->KEY[this->V_REGISTER[reg_x]]!=1){
                            this->PC+=0x04;
                        } else {
                            this->PC+=0x02;
                        }
                        printf("SKNP V[%.1X] \n", (uint16_t)reg_x);
                        
                        break;
                        
                    }
                    default:
                        break;
                }
                
                break;
            }
                
                
            case 0xF000: // F FAMILY
            {
                switch (this->OPCODE & 0x00FF) {
                    
                    case 0x0007: // FX07 - LD VX,07
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        this->V_REGISTER[reg_x] = this->DELAY_TIMER;
                        
                        printf("LD V[%.1X],DT\n", (uint16_t)reg_x);
                        
                        this->PC+=0x02;
                        break;
                        
                    }
                    
                    case 0x000A: // FX0A - LD VX,K
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        for (uint8_t _key=0x0; _key<=0xF; _key+=0x1) {
                            printf("KEY[%d] = %d\n",_key,this->KEY[_key]);
                            if (this->KEY[_key]==1){
                                this->V_REGISTER[reg_x] =_key;
                                this->PC+=0x02;
                                break;
                            }
                        }
                        
                        printf("LD V[%.1X],K\n", (uint16_t)reg_x);
                        
                        break;
                        
                    }
                        
                    case 0x0015: // FX15 - LD DT,VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        this->DELAY_TIMER = this->V_REGISTER[reg_x];
                        
                        printf("LD DT,V[%.1X]\n", (uint16_t)reg_x);
                        
                        this->PC+=0x02;
                        break;
                        
                    }
                        
                    case 0x0018: // FX18 - LD ST,VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        this->SOUND_TIMER = this->V_REGISTER[reg_x];
                        
                        printf("LD ST,V[%.1X]\n", (uint16_t)reg_x);
                        
                        this->PC+=0x02;
                        break;
                        
                    }
                        
                    case 0x001E: // FX1E - ADD IR,VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        this->IR += this->V_REGISTER[reg_x];
                        
                        printf("ADD IR,V[%.1X]\n", (uint16_t)reg_x);
                        
                        this->PC+=0x02;
                        break;
                        
                    }
                        
                    case 0x0029: // FX29 - I=memory[fontset[VX]]
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        this->IR = 0x00+(this->V_REGISTER[reg_x]*0x05);
                        printf("IR=Memory[0x00+V[%.1X]]\n", (uint16_t)reg_x);
                        
                        this->PC+=0x02;
                        break;
                    }
                        
                    case 0x0033: // FX33 - LD B,VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        
                        this->MEMORY[this->IR] = this->V_REGISTER[reg_x]/100;
                        this->MEMORY[this->IR+0x01] = (this->V_REGISTER[reg_x] / 10) % 10;
                        this->MEMORY[this->IR+0x02] = (this->V_REGISTER[reg_x] % 100) % 10;
                        
                        printf("LD B,V[%.1X]\n", (uint16_t)reg_x);
                        
                        this->PC+=0x02;
                        break;
                        
                    }
                        
                        
                    case 0x0055: // FX55 - LD I,VX- MEMORY[I++] = V0..VX
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        for (uint8_t reg_idx=0x00; reg_idx<=reg_x; reg_idx++) {
                            this->MEMORY[this->IR+reg_idx] = this->V_REGISTER[reg_idx];
                        }
                        printf("LD MEMORY[IR],V0..V[%.1X] \n", (uint16_t)reg_x);
                        this->PC+=0x02;
                        break;
                    }
                        
                    case 0x0065: // FX65 - LD VX,I - V0..VX = MEMORY[I++]
                    {
                        uint8_t reg_x = (uint8_t)((this->OPCODE & 0x0F00) >> 8);
                        for (uint8_t reg_idx=0x00; reg_idx<=reg_x; reg_idx++) {
                            this->V_REGISTER[reg_idx] = this->MEMORY[this->IR+reg_idx];
                        }
                        printf("LD V0..V[%.1X],MEMORY[IR] \n", (uint16_t)reg_x);
                        this->PC+=0x02;
                        break;
                    }
                        
                    default:
                        break;
                        
                }
                
                
                
            }
                
                
        }
    
    }
    //return (this->PC>=_CHIP8_MEMORY_SIZE_);
    
}

/******************************************************************************/
