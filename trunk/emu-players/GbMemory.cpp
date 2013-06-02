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

#include <FBase.h>
#include "GbMemory.h"
#include "GbZ80.h"
#include "GbPapu.h"
#include "GbsPlayer.h"

typedef unsigned char (*read_8_func)(unsigned short);
typedef void (*write_8_func)(unsigned short,unsigned char);

// Not needed for GBS playback; return a dummy value
#define color16(r,g,b) 0

unsigned char	RAM[0x2000],VRAM[0x2000*2],OAM[0xA0],IOREGS[0x50],HIRAM[0x80],EXRAM[128*1024],MBC2RAM[512],WRAM[28*1024];
unsigned char	*ROM0,*ROM1,*RAM1,*WRAM1,*VRAM0;
unsigned char	mbc1Layout;
unsigned short	highRomBits,highRomBitsLatch,hdmasrc,hdmadest,hdmalen;
unsigned char	IME,REG_IE,romSelect,ramSelect,ramSelectLatch,keys,numBanks,divcnt,cgbBgpIdx,cgbBgpInc,cgbObpIdx,cgbObpInc,hdma5;
int				tmcnt,tmrld;
read_8_func read_8_tbl[16];
write_8_func write_8_tbl[16];
GbPapuChip		*papu;
GbsPlayer		*gbsPlayer;

unsigned char cgbBgp[64],cgbObp[64];
unsigned short cgbBgpRgb[32],cgbObpRgb[32];


void mem_write_8_null(unsigned short address,unsigned char data) {}


void mem_write_8_A000_mbc1(unsigned short address,unsigned char data) {
	RAM1[address-0xA000] = data;
}

void mem_write_8_0000_mbc1(unsigned short address,unsigned char data) {
    if (address < 0x2000) {
        if ((data & 0x0F) == 0x0A) {
            RAM1 = &EXRAM[ramSelect*0x2000];
            write_8_tbl[0x0A] = write_8_tbl[0x0B] = mem_write_8_A000_mbc1;
        } else {
        	RAM1 = NULL;
        	write_8_tbl[0x0A] = write_8_tbl[0x0B] = mem_write_8_null;
        }
    } else if (address>=0x2000) {
		romSelect = (data==0)?1:(data&0x1F);
		romSelect &= (numBanks-1);
		AppLog("ROM bank1 selection: %d", romSelect);
		/*if (mbc1Layout == 0)
			ROM1 = &cart[(romSelect+highRomBits)*0x4000];
		else*/
			ROM1 = &cart[(romSelect)*0x4000];
	}
}


void mem_write_8_0000_mbc2(unsigned short address,unsigned char data) {
   	romSelect = (data==0)?1:(data&0xF);
   	ROM1 = &cart[(romSelect)*0x4000];
}


void mem_write_8_0000_mbc3(unsigned short address,unsigned char data) {
    if (address<0x2000) {
        if ((data&0x0F)==0x0A) {
            RAM1 = &EXRAM[ramSelect*0x2000];
            write_8_tbl[0x0A] = write_8_tbl[0x0B] = mem_write_8_A000_mbc1;
        } else {
        	RAM1 = NULL;
        	write_8_tbl[0x0A] = write_8_tbl[0x0B] = mem_write_8_null;
        }
    } else if (address>=0x2000) {
		romSelect = (data==0)?1:(data&0x7F);
		romSelect &= (numBanks-1);
		ROM1 = &cart[(romSelect)*0x4000];
    }
}


void mem_write_8_4000_mbc1(unsigned short address,unsigned char data) {
	if (address<0x6000) {
		if (mbc1Layout==0)
			highRomBitsLatch = highRomBits = (data&3)<<5;
		else {
			ramSelectLatch = ramSelect = data&3;
		}
	} else {
		mbc1Layout = data&1;
		if (mbc1Layout==0)
			{ ramSelect = 0; highRomBits = highRomBitsLatch; }
		else
			{ highRomBits = 0; ramSelect = ramSelectLatch; }
	}
	RAM1 = &EXRAM[ramSelect*0x2000];
	ROM1 = &cart[(romSelect+highRomBits)*0x4000];
}


void mem_write_8_4000_mbc3(unsigned short address,unsigned char data) {
	if (address<0x6000) {
	    ramSelect = data&3;
	} else {
	    // TODO: Handle RTC
	}
	RAM1 = &EXRAM[ramSelect*0x2000];
}


void mem_write_8_4000_mbc5(unsigned short address,unsigned char data)
{
	ramSelect = data & 0x0F;
	RAM1 = &EXRAM[ramSelect*0x2000];
}


void mem_write_8_8000(unsigned short address,unsigned char data)
{
	VRAM0[address-0x8000] = data;
}



void mem_write_8_A000_mbc2(unsigned short address,unsigned char data)
{
	if (address<0xA200)
		MBC2RAM[address-0xA000] = data&0xF;
}


void mem_write_8_C000(unsigned short address,unsigned char data)
{
	RAM[address&0x1FFF] = data;
}

void mem_write_8_D000(unsigned short address,unsigned char data)
{
	WRAM1[address&0x0FFF] = data;
}

void mem_write_8_F000(unsigned short address,unsigned char data)
{
	int i;

	if (address < 0xFE00) {
		RAM[address&0x1FFF] = data;

	} else if (address < 0xFEA0) {
		OAM[address-0xFE00] = data;

	} else if ((address>=0xFF00)&&(address<=0xFF4F)) {
		IOREGS[address-0xFF00] = data;
		if (address == 0xFF04) {
			IOREGS[0x04] = 0;
		} else if (address==0xFF07) {
			switch (data&3) {
				case 0:
					tmrld = 1024;
					break;
				case 1:
					tmrld = 16;
					break;
				case 2:
					tmrld = 64;
					break;
				case 3:
					tmrld = 256;
					break;
			}
			tmcnt = 0;
		} else if (address == 0xFF40) {
			//fprintf(stderr,"[%04X] = 0x%02x at 0x%04X\n",address,data,cpu.regs.PC);

		} else if (address==0xFF46) {
			for (i=0; i<40*4; i++)
				*(OAM+i) = mem_read_8(data*0x100 + i);
		} else if ((address==0xFF4F)&&(MACH_TYPE&0x80)) {
			VRAM0 = &VRAM[(data & 1) * 0x2000];
			//fprintf(stderr,"[%04X] = 0x%02x\n",address,data);
		}
		if (address >= 0xFF10 && address <= 0xFF3F) {
			papu->Write(address, data);
			if (address == 0xFF24) {
				gbsPlayer->SetMasterVolume((data >> 4) & 7, data & 7);
			}
		}

	} else if ((address>=0xFF51)&&(address<=0xFF55)) {
		//fprintf(stderr,"[%04X] = 0x%02x\n",address,data);
		if (address==0xFF55) {
			if (data) {
				hdma5 = data & 0x7F;
				hdmalen = (hdma5 + 1) << 4;
				hdmasrc &= 0xFFF0;
				hdmadest = (hdmadest & 0x1FFF) | 0x8000;
				while (hdmalen) {
					mem_write_8(hdmadest++, mem_read_8(hdmasrc++));
					hdmalen--;
				}
				hdma5 = 0xFF;
			} else {
				hdma5 |= 0x80;
			}
		} else if (address==0xFF51) {
			hdmasrc = (hdmasrc & 0x00FF) | (data << 8);
  		} else if (address==0xFF52) {
			hdmasrc = (hdmasrc & 0xFF00) | data;
		} else if (address==0xFF53) {
			hdmadest = (hdmadest & 0x00FF) | (data << 8);
  		} else if (address==0xFF54) {
			hdmadest = (hdmadest & 0xFF00) | data;
		}

	} else if ((address==0xFF68)&&(MACH_TYPE & 0x80)) {
		cgbBgpIdx = data & 0x3F;
		cgbBgpInc = (data >> 7) & 1;
		//fprintf(stderr,"[FF68] = 0x%02x\n",data);

	} else if ((address==0xFF69)&&(MACH_TYPE & 0x80)) {
		//fprintf(stderr,"[FF69] = 0x%02x\n",data);
                cgbBgp[cgbBgpIdx] = data;
		i =  cgbBgpIdx & 0x3E;
                cgbBgpRgb[i >> 1] = color16((cgbBgp[i] & 0x1F) << 3, ((cgbBgp[i] & 0xE0) >> 2) | ((cgbBgp[i + 1] & 0x03) << 6), (cgbBgp[i + 1] & 0x7C) << 1);
		cgbBgpIdx = (cgbBgpIdx + cgbBgpInc) & 0x3F;

	} else if ((address==0xFF6A)&&(MACH_TYPE & 0x80)) {
		cgbObpIdx = data & 0x3F;
		cgbObpInc = (data >> 7) & 1;
		//fprintf(stderr,"[FF6A] = 0x%02x\n",data);

	} else if ((address==0xFF6B)&&(MACH_TYPE & 0x80)) {
		//fprintf(stderr,"[FF6B] = 0x%02x\n",data);
                cgbObp[cgbObpIdx] = data;
		i =  cgbObpIdx & 0x3E;
                cgbObpRgb[i >> 1] = color16((cgbObp[i] & 0x1F) << 3, ((cgbObp[i] & 0xE0) >> 2) | ((cgbObp[i + 1] & 0x03) << 6), (cgbObp[i + 1] & 0x7C) << 1);
		cgbObpIdx = (cgbObpIdx + cgbObpInc) & 0x3F;

	// Set WRAM bank 1 for CGB
	} else if ((address==0xFF70)&&(MACH_TYPE & 0x80)) {
		if (data==0) data = 1;
		WRAM1 = &WRAM[((data&7)-1)*0x1000];
		//fprintf(stderr,"[FF70] = 0x%02x\n",data);
		
	} else if ((address>=0xFF80)&&(address<0xFFFF)) {
		//AppLog("HIRAM[%#x] = %#x", address-0xFF80, data);
		HIRAM[address-0xFF80] = data;

	} else if (address==0xFFFF) {
		REG_IE = data;
	}
}


unsigned char mem_read_8_null(unsigned short address) {
	return 0;
}


unsigned char mem_read_8_0000(unsigned short address) {
	return ROM0[address];
}


unsigned char mem_read_8_4000(unsigned short address) {
	return ROM1[address-0x4000];
}


unsigned char mem_read_8_8000(unsigned short address) {
	return VRAM0[address-0x8000];
}


unsigned char mem_read_8_A000_mbc1(unsigned short address) {
	return RAM1[address-0xA000];
}


unsigned char mem_read_8_A000_mbc2(unsigned short address) {
	if (address<0xA200)
		return MBC2RAM[address-0xA000]&0xF;
	return 0;
}


unsigned char mem_read_8_C000(unsigned short address) {
	return RAM[address&0x1FFF];
}


unsigned char mem_read_8_D000(unsigned short address) {
	return WRAM1[address&0x0FFF];
}


unsigned char mem_read_8_F000(unsigned short address) {
	if (address < 0xFE00) {
		return RAM[address&0x1FFF];

	} else if (address < 0xFEA0) {
		return OAM[address-0xFE00];

	} else if ((address>=0xFF00)&&(address<0xFF4C)) {
		if (address==0xFF00) {
			if ((IOREGS[0]&0x30)==0x10)
				return (IOREGS[0]&0xF0)|((keys>>4)&0xF);
			else if ((IOREGS[0]&0x30)==0x20)
				return (IOREGS[0]&0xF0)|(keys&0xF);
			else
				return IOREGS[0];
		} else
		{
 			return IOREGS[address-0xFF00];
        }

	} else if ((address==0xFF55)&&(MACH_TYPE&0x80)) {
		return hdma5;

	} else if ((address>=0xFF80)&&(address<0xFFFF)) {
		return HIRAM[address-0xFF80];

	} else if (address==0xFFFF) {
		return REG_IE;
	}

	return 0xFF;
}


int mem_reset()
{
	keys = 0xff;
	IME = 0;
	romSelect = 1;
	ramSelect = 0;
	mbc1Layout = 0;
	highRomBits = 0;
	divcnt = 0;
	cgbBgpIdx = cgbBgpInc = 0;
	cgbObpIdx = cgbObpInc = 0;
	hdma5 = 0xFF;
	REG_DIV = 0;
	REG_TAC = 0;
	ROM0 = cart;
	ROM1 = ROM0 + 0x4000;
	RAM1 = NULL;
	WRAM1 = WRAM;
	VRAM0 = VRAM;

	/*if ((CART_TYPE==TYPE_MBC1)||(CART_TYPE==TYPE_MBC1_RAM)||(CART_TYPE==TYPE_MBC1_RAM_BATT)) {
		read_8_tbl[0x0] = mem_read_8_0000;
		read_8_tbl[0x1] = mem_read_8_0000;
		read_8_tbl[0x2] = mem_read_8_0000;
		read_8_tbl[0x3] = mem_read_8_0000;
		read_8_tbl[0x4] = mem_read_8_4000;
		read_8_tbl[0x5] = mem_read_8_4000;
		read_8_tbl[0x6] = mem_read_8_4000;
		read_8_tbl[0x7] = mem_read_8_4000;
		read_8_tbl[0x8] = mem_read_8_8000;
		read_8_tbl[0x9] = mem_read_8_8000;
		read_8_tbl[0xA] = mem_read_8_A000_mbc1;
		read_8_tbl[0xB] = mem_read_8_A000_mbc1;
		read_8_tbl[0xC] = mem_read_8_C000;
		read_8_tbl[0xD] = mem_read_8_C000;
		read_8_tbl[0xE] = mem_read_8_C000;
		read_8_tbl[0xF] = mem_read_8_F000;

		write_8_tbl[0x0] = mem_write_8_0000_mbc1;
		write_8_tbl[0x1] = mem_write_8_0000_mbc1;
		write_8_tbl[0x2] = mem_write_8_0000_mbc1;
		write_8_tbl[0x3] = mem_write_8_0000_mbc1;
		write_8_tbl[0x4] = mem_write_8_4000_mbc1;
		write_8_tbl[0x5] = mem_write_8_4000_mbc1;
		write_8_tbl[0x6] = mem_write_8_4000_mbc1;
		write_8_tbl[0x7] = mem_write_8_4000_mbc1;
		write_8_tbl[0x8] = mem_write_8_8000;
		write_8_tbl[0x9] = mem_write_8_8000;
		write_8_tbl[0xA] = mem_write_8_null;
		write_8_tbl[0xB] = mem_write_8_null;
		write_8_tbl[0xC] = mem_write_8_C000;
		write_8_tbl[0xD] = mem_write_8_C000;
		write_8_tbl[0xE] = mem_write_8_C000;
		write_8_tbl[0xF] = mem_write_8_F000;

	} else if ((CART_TYPE==TYPE_MBC2)||(CART_TYPE==TYPE_MBC2_BATT)) {

	} else if ((CART_TYPE==TYPE_MBC3)||(CART_TYPE==TYPE_MBC3_RAM)||(CART_TYPE==TYPE_MBC3_RAM_BATT)) {*/
		read_8_tbl[0x0] = mem_read_8_0000;
		read_8_tbl[0x1] = mem_read_8_0000;
		read_8_tbl[0x2] = mem_read_8_0000;
		read_8_tbl[0x3] = mem_read_8_0000;
		read_8_tbl[0x4] = mem_read_8_4000;
		read_8_tbl[0x5] = mem_read_8_4000;
		read_8_tbl[0x6] = mem_read_8_4000;
		read_8_tbl[0x7] = mem_read_8_4000;
		read_8_tbl[0x8] = mem_read_8_8000;
		read_8_tbl[0x9] = mem_read_8_8000;
		read_8_tbl[0xA] = mem_read_8_A000_mbc1;
		read_8_tbl[0xB] = mem_read_8_A000_mbc1;
		read_8_tbl[0xC] = mem_read_8_C000;
		read_8_tbl[0xD] = mem_read_8_C000;
		read_8_tbl[0xE] = mem_read_8_C000;
		read_8_tbl[0xF] = mem_read_8_F000;

		write_8_tbl[0x0] = mem_write_8_0000_mbc3;
		write_8_tbl[0x1] = mem_write_8_0000_mbc3;
		write_8_tbl[0x2] = mem_write_8_0000_mbc3;
		write_8_tbl[0x3] = mem_write_8_0000_mbc3;
		write_8_tbl[0x4] = mem_write_8_4000_mbc3;
		write_8_tbl[0x5] = mem_write_8_4000_mbc3;
		write_8_tbl[0x6] = mem_write_8_4000_mbc3;
		write_8_tbl[0x7] = mem_write_8_4000_mbc3;
		write_8_tbl[0x8] = mem_write_8_8000;
		write_8_tbl[0x9] = mem_write_8_8000;
		write_8_tbl[0xA] = mem_write_8_null;
		write_8_tbl[0xB] = mem_write_8_null;
		write_8_tbl[0xC] = mem_write_8_C000;
		write_8_tbl[0xD] = mem_write_8_C000;
		write_8_tbl[0xE] = mem_write_8_C000;
		write_8_tbl[0xF] = mem_write_8_F000;

	/*} else if ((CART_TYPE==TYPE_MBC5)||(CART_TYPE==TYPE_MBC5_RAM)) {
		read_8_tbl[0x0] = mem_read_8_0000;
		read_8_tbl[0x1] = mem_read_8_0000;
		read_8_tbl[0x2] = mem_read_8_0000;
		read_8_tbl[0x3] = mem_read_8_0000;
		read_8_tbl[0x4] = mem_read_8_4000;
		read_8_tbl[0x5] = mem_read_8_4000;
		read_8_tbl[0x6] = mem_read_8_4000;
		read_8_tbl[0x7] = mem_read_8_4000;
		read_8_tbl[0x8] = mem_read_8_8000;
		read_8_tbl[0x9] = mem_read_8_8000;
		read_8_tbl[0xA] = mem_read_8_A000_mbc1;
		read_8_tbl[0xB] = mem_read_8_A000_mbc1;
		read_8_tbl[0xC] = mem_read_8_C000;
		read_8_tbl[0xD] = mem_read_8_C000;
		read_8_tbl[0xE] = mem_read_8_C000;
		read_8_tbl[0xF] = mem_read_8_F000;

		write_8_tbl[0x0] = mem_write_8_0000_mbc5;
		write_8_tbl[0x1] = mem_write_8_0000_mbc5;
		write_8_tbl[0x2] = mem_write_8_2000_mbc5;
		write_8_tbl[0x3] = mem_write_8_3000_mbc5;
		write_8_tbl[0x4] = mem_write_8_4000_mbc5;
		write_8_tbl[0x5] = mem_write_8_null;
		write_8_tbl[0x6] = mem_write_8_null;
		write_8_tbl[0x7] = mem_write_8_null;
		write_8_tbl[0x8] = mem_write_8_8000;
		write_8_tbl[0x9] = mem_write_8_8000;
		write_8_tbl[0xA] = mem_write_8_null;
		write_8_tbl[0xB] = mem_write_8_null;
		write_8_tbl[0xC] = mem_write_8_C000;
		write_8_tbl[0xD] = mem_write_8_C000;
		write_8_tbl[0xE] = mem_write_8_C000;
		write_8_tbl[0xF] = mem_write_8_F000;

	} else {
		read_8_tbl[0x0] = mem_read_8_0000;
		read_8_tbl[0x1] = mem_read_8_0000;
		read_8_tbl[0x2] = mem_read_8_0000;
		read_8_tbl[0x3] = mem_read_8_0000;
		read_8_tbl[0x4] = mem_read_8_4000;
		read_8_tbl[0x5] = mem_read_8_4000;
		read_8_tbl[0x6] = mem_read_8_4000;
		read_8_tbl[0x7] = mem_read_8_4000;
		read_8_tbl[0x8] = mem_read_8_8000;
		read_8_tbl[0x9] = mem_read_8_8000;
		read_8_tbl[0xA] = mem_read_8_null;
		read_8_tbl[0xB] = mem_read_8_null;
		read_8_tbl[0xC] = mem_read_8_C000;
		read_8_tbl[0xD] = mem_read_8_C000;
		read_8_tbl[0xE] = mem_read_8_C000;
		read_8_tbl[0xF] = mem_read_8_F000;

		write_8_tbl[0x0] = mem_write_8_null;
		write_8_tbl[0x1] = mem_write_8_null;
		write_8_tbl[0x2] = mem_write_8_null;
		write_8_tbl[0x3] = mem_write_8_null;
		write_8_tbl[0x4] = mem_write_8_null;
		write_8_tbl[0x5] = mem_write_8_null;
		write_8_tbl[0x6] = mem_write_8_null;
		write_8_tbl[0x7] = mem_write_8_null;
		write_8_tbl[0x8] = mem_write_8_8000;
		write_8_tbl[0x9] = mem_write_8_8000;
		write_8_tbl[0xA] = mem_write_8_null;
		write_8_tbl[0xB] = mem_write_8_null;
		write_8_tbl[0xC] = mem_write_8_C000;
		write_8_tbl[0xD] = mem_write_8_C000;
		write_8_tbl[0xE] = mem_write_8_C000;
		write_8_tbl[0xF] = mem_write_8_F000;
	}

	// Set access handlers for WRAM bank 1 (CGB mode)
	if (MACH_TYPE & 0x80) {
		read_8_tbl[0xD] = mem_read_8_D000;
		write_8_tbl[0xD] = mem_write_8_D000;
	}*/

	IOREGS[0x4D] = 0;

	return 1;
}


void mem_set_papu(GbPapuChip *p)
{
	papu = p;
}

void mem_set_gbsplayer(GbsPlayer *p)
{
	gbsPlayer = p;
}

unsigned char mem_read_8(unsigned short address)
{
	return read_8_tbl[address>>12](address);
}


unsigned short mem_read_16(unsigned short address)
{
	return mem_read_8(address) + ((unsigned short)mem_read_8(address+1)<<8);
}



void mem_write_8(unsigned short address,unsigned char data)
{
	write_8_tbl[address>>12](address,data);
}


void mem_write_16(unsigned short address,unsigned short data)
{
	mem_write_8(address,data&0xFF);
	mem_write_8(address+1,data>>8);
}


