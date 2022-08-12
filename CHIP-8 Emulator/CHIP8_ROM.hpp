//
//  CHIP8_ROM.hpp
//  CHIP-8 Emulator
//
//  Created by Cristiano Douglas on 06/08/18.
//  Copyright © 2018 tydresdevelopment. All rights reserved.
//

#ifndef CHIP8_ROM_hpp
#define CHIP8_ROM_hpp

#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

/******************************************************************************/
// CHIP-8 ROM
/******************************************************************************/
class CHIP8_ROM {
    
    /*........................................................................*/
    // Pointer to ROM data
    char* ROM_DATA;
    // Size of ROM data
    streampos DATA_SIZE;
    /*........................................................................*/
    
public:
    
    /*........................................................................*/
    ~CHIP8_ROM();
    
    void    rom_LOAD(const char path[256]);
    char*   rom_GET_DATA();
    streampos rom_GET_DATA_SIZE();
    /*........................................................................*/
    
};
/******************************************************************************/


#endif /* CHIP8_ROM_hpp */
