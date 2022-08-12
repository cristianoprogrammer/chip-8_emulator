//
//  CHIP8_ROM.cpp
//  CHIP-8 Emulator
//
//  Created by Cristiano Douglas on 06/08/18.
//  Copyright © 2018 tydresdevelopment. All rights reserved.
//

#include "CHIP8_ROM.hpp"

/******************************************************************************/
/** Class Destructor
 */
CHIP8_ROM::~CHIP8_ROM() {
    
    free(this->ROM_DATA);

}

/******************************************************************************/
/**
 Load CHIP-8 ROM from a file

 @param _path Path to ROM File
 */
void    CHIP8_ROM::rom_LOAD(const char _path[256]) {
    
    cout << endl << "·······················································" << endl;
    cout << "[ Loading ROM File ]" << endl << endl;
    cout << "Path: " << _path << endl;
    
    ifstream rom_file (_path,ios::in|ios::binary|ios::ate);
    if (rom_file.is_open())
    {
        cout << "Successfully load!" << endl;
        this->DATA_SIZE = rom_file.tellg();
        this->ROM_DATA  = (char*) malloc(sizeof(char)*this->DATA_SIZE);
        rom_file.seekg(0,ios::beg);
        rom_file.read(this->ROM_DATA,this->DATA_SIZE);
        rom_file.close();
        
    } else {
        cout << "Load fail" << endl;
    }
    cout << "·······················································" << endl;
    
}

/******************************************************************************/
/**
 Get the ROM Data

 @return Pointer to ROM data
 */
char*   CHIP8_ROM::rom_GET_DATA() {
    
    return this->ROM_DATA;
}


/******************************************************************************/
/**
 Get the ROM Data size

 @return Size of ROM Data in Bytes
 */
streampos CHIP8_ROM::rom_GET_DATA_SIZE() {
    
    return this->DATA_SIZE;
}

/******************************************************************************/
