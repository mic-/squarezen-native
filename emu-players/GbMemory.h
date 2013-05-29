/*
 * Copyright 2013 Mic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GBMEMORY_H__
#define GBMEMORY_H__

#include "GbCommon.h"

extern unsigned char IME,REG_IE;
extern unsigned char RAM[0x2000],VRAM[0x2000*2],OAM[0xA0],IOREGS[0x50],HIRAM[0x80];

#define MACH_TYPE cart[0x143]
#define CART_TYPE cart[0x147]

#define TYPE_MBC1 1
#define TYPE_MBC1_RAM 2
#define TYPE_MBC1_RAM_BATT 3
#define TYPE_MBC2 5
#define TYPE_MBC2_BATT 6
#define TYPE_MBC3 17
#define TYPE_MBC3_RAM 18
#define TYPE_MBC3_RAM_BATT 19
#define TYPE_MBC5 25
#define TYPE_MBC5_RAM 26
#define TYPE_MBC5_RAM_BATT 27


#define REG_DIV  IOREGS[0x04]
#define REG_TIMA IOREGS[0x05]
#define REG_TMA  IOREGS[0x06]
#define REG_TAC  IOREGS[0x07]
#define REG_IF   IOREGS[0x0F]
#define REG_LCDC IOREGS[0x40]
#define REG_STAT IOREGS[0x41]
#define REG_SCY  IOREGS[0x42]
#define REG_SCX  IOREGS[0x43]
#define REG_LY   IOREGS[0x44]
#define REG_LYC  IOREGS[0x45]
#define REG_BGP  IOREGS[0x47]
#define REG_OBP0 IOREGS[0x48]
#define REG_OBP1 IOREGS[0x49]
#define REG_WY   IOREGS[0x4A]
#define REG_WX   IOREGS[0x4B]
#define REG_SVBK IOREGS[0x70]


extern int mem_reset();
extern unsigned char mem_read_8(unsigned short);
extern unsigned short mem_read_16(unsigned short);
extern void mem_write_8(unsigned short,unsigned char);
extern void mem_write_16(unsigned short,unsigned short);
extern void mem_key_down();

#endif
