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

#define NLOG_LEVEL_VERBOSE 0

#include <string.h>
#include <stddef.h>
#include "NativeLogger.h"
#include "SpcMapper.h"
#include "SDsp.h"
#include "SSmp.h"


static const uint8_t IPL_ROM[] =
{
	0xCD, 0xEF,			// mov  x,$EF
	0xBD,           	// mov  sp,x
	0xE8, 0x00,     	// mov  a,00
	0xC6,           	// -: mov  (x),a
	0x1D,           	// dec  x
	0xD0, 0xFC,     	// bne  -
	0x8F, 0xAA, 0xF4,   // mov  [$F4],$AA
	0x8F, 0xBB, 0xF5,   // mov  [$F5],$BB
	0x78, 0xCC, 0xF4,	// -: cmp  [F4],CC        ; wait for initial "kick" value
	0xD0, 0xFB,        	// bne -
	0x2F, 0x19,        	// bra main
		                //   @@transfer_data:
	0xEB, 0xF4,        	// -: mov  y,[$F4]
	0xD0, 0xFC,        	// bne  -
		                //   @@transfer_lop:
	0x7E, 0xF4,        	// cmp  y,[F4]
	0xD0, 0x0B,        	// bne  next
	0xE4, 0xF5,        	// mov  a,[F5]     ;get data
	0xCB, 0xF4,        	// mov  [F4],y     ;ack data
	0xD7, 0x00,        	// mov  [[00]+y],a ;store data
	0xFC,           	// inc  y          ;addr lsb
	0xD0, 0xF3,        	// jnz  @@transfer_lop
	0xAB, 0x01,        	// inc  [$01]       ;addr msb
		                //   @@
	0x10, 0xEF,        	// jns  @@transfer_lop     ;strange...
	0x7E, 0xF4,        	// cmp  y,[F4]
	0x10, 0xEB,        	// jns  @@transfer_lop
		                //   main:
	0xBA, 0xF6,        	// movw ya,[F6]                ;\copy transfer (or entrypoint)
	0xDA, 0x00,        	// movw [00],ya    ;addr       ;/address to RAM at [0000h]
	0xBA, 0xF4,        	// movw ya,[F4]    ;cmd:kick
	0xC4, 0xF4,        	// mov  [F4],a     ;ack kick
	0xDD,           	// mov  a,y        ;cmd
	0x5D,           	// mov  x,a        ;cmd
	0xD0, 0xDB,        	// next: bne  @@transfer_data
	0x1F, 0x00, 0x00,   // jmp  [$0000+x]
	0xC0, 0xFF        	// dw $FFC0  ;reset vector
};


SpcMapper::SpcMapper()
	: mTest(0x0A)
	, mCtrl(0xB0)
	, mSDsp(NULL)
{
	// TODO: implement
	mRam = new uint8_t[64 * 1024];
}

SpcMapper::~SpcMapper()
{
	delete [] mRam;
	mRam = NULL;
}


void SpcMapper::Reset()
{
	// TODO: implement
}


uint8_t SpcMapper::ReadByte(uint16_t addr)
{
	if (addr >= 0xFFC0 && (mCtrl & 0x80)) {
		return IPL_ROM[addr & 0x3F];
	}
	return mRam[addr];
}


void SpcMapper::WriteByte(uint16_t addr, uint8_t data)
{
	if (addr < 0xFFC0 || (mCtrl & 0x80) == 0) {
		mRam[addr] = data;
	}

	switch (addr) {
	case R_TEST:
		if (data & 4) {
			// "Crash" the CPU
			mSSmp->mRegs.PC ^= 0xAA55;
			mSSmp->mRegs.SP ^= 0xAA;
			mSSmp->mHalted = true;
		}
		break;

	case R_CONTROL:
		mCtrl = data;
		break;

	case R_DSPDATA:
		mSDsp->Write(mRam[R_DSPADDR], data);
		break;

	default:
		break;
	}
}

