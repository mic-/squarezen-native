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

#define NLOG_LEVEL_DEBUG 0

#include <string.h>
#include <stddef.h>
#include "NativeLogger.h"
#include "SidMapper.h"
#include "SidPlayer.h"
#include "Emu6502.h"

static const uint8_t KERNEL_EA31_IRQ_HANDLER[] =
{
	0x4C, 0x81, 0xEA	// JMP $EA81
};

static const uint8_t KERNEL_EA81_IRQ_TAIL[] =
{
	0x68, 0xA8,		// PLA, TAY
	0x68, 0xAA,		// PLA, TAX
	0x68,			// PLA
	0x40			// RTI
};

static const uint8_t KERNEL_FF48_IRQ_HANDLER[] =
{
	0x48,				// PHA
	0x8A, 0x48,			// TXA, PHA
	0x98, 0x48,			// TYA, PHA
	0x6C, 0x14, 0x03	// JMP ($0314)
};


SidMapper::SidMapper()
	: mSidPlayer(NULL)
	, mSid(NULL)
{
	mRam = new uint8_t[64 * 1024];
	memset(mRam, 0, 64*1024);
}


SidMapper::~SidMapper()
{
	delete [] mRam;
	mRam = NULL;
}


uint8_t SidMapper::ReadByte(uint32_t addr)
{
	uint8_t bankSelect = mRam[1] & 7;

	if ((addr >> 12) == 0xD && bankSelect >= 5) {
		// I/O at D000-DFFF
		if (addr < 0xD400) {
			// VIC-II
			return mVicII.Read(addr);
		} else if (addr >= 0xDC00 && addr <= 0xDCFF) {
			// CIA1
			return mCia[0].Read(addr & 0xDC0F);

		} else if (addr >= 0xDD00 && addr <= 0xDDFF) {
			// CIA2
			return mCia[1].Read(addr & 0xDD0F);

		} else if (addr >= Mos6581::REGISTER_BASE && addr <= Mos6581::REGISTER_BASE + Mos6581::R_FILTER_MODEVOL) {
			NLOGD("SidMapper", "Reading from SID area (%#x)", addr);
		}

	} else if ((addr >> 12) >= 0xE && (bankSelect & 3) >= 2) {
		NLOGD("SidMapper", "Reading from kernel ROM (%#x) at PC=%#x", addr, m6502->mRegs.PC);
		if (addr >= 0xEA31 && addr <= 0xEA33) {
			return KERNEL_EA31_IRQ_HANDLER[addr - 0xEA31];
		} else if (addr >= 0xEA81 && addr <= 0xEA86) {
			return KERNEL_EA81_IRQ_TAIL[addr - 0xEA81];
		} else if (addr >= 0xFF48 && addr <= 0xFF4F) {
			return KERNEL_FF48_IRQ_HANDLER[addr - 0xFF48];
		} else if (addr == 0xFFFE) {
			return 0x48;
		} else if (addr == 0xFFFF) {
			return 0xFF;
		} else {
			return 0;
		}
	}
	return mRam[addr];
}


void SidMapper::WriteByte(uint32_t addr, uint8_t data)
{
	NLOGV("SidMapper", "Write(%#x, %#x)", addr, data);

	uint8_t bankSelect = mRam[1] & 7;

	switch (addr >> 12) {
	case 0xA: case 0xB:
		if ((bankSelect & 3) != 3) {
			mRam[addr] = data;
		}
		break;
	case 0xD:
		//NLOGD("SidMapper", "addr=%#x, data=%#x, bankSelect=%d", addr, data, bankSelect);
		if (bankSelect >= 5) {
			// I/O at D000-DFFF
			if (addr < 0xD400) {
				// VIC-II
				mVicII.Write(addr, data);
			} else if (addr >= 0xDC00 && addr <= 0xDCFF) {
				// CIA1
				mCia[0].Write(addr & 0xDC0F, data);

			} else if (addr >= 0xDD00 && addr <= 0xDDFF) {
				// CIA2
				mCia[1].Write(addr & 0xDD0F, data);

			} else if (addr >= Mos6581::REGISTER_BASE && addr <= Mos6581::REGISTER_BASE + 0x3FF) {
				addr &= (Mos6581::REGISTER_BASE + 0x1F);
				mSid->Write(addr, data);
				if (addr == Mos6581::REGISTER_BASE + Mos6581::R_FILTER_MODEVOL) {
					mSidPlayer->SetMasterVolume(data & 0x0F);
				}
			}
		} else if (bankSelect == 4 || bankSelect == 0) {
			// RAM at D000-DFFF
			mRam[addr] = data;
			NLOGD("SidMapper", "D000-DFFF mapped to RAM");
		} else {
			NLOGD("SidMapper", "D000-DFFF mapped to ROM");
		}
		break;
	case 0xE: case 0xF:
		if ((bankSelect & 3) < 2) {
			mRam[addr] = data;
		}
		break;
	default:
		if (addr == 1) {
			NLOGD("SidMapper", "New bank select value: %#x at PC=%#x", data, m6502->mRegs.PC);
		}
		mRam[addr] = data;
		break;
	}

}


void SidMapper::Reset()
{
	mRam[0x01] = 0x37;
	mRam[0x314] = 0x31;
	mRam[0x315] = 0xEA;
	mRam[0xEA31] = 0x40;  // RTI

	mCia[0].SetMapper(this);
	mCia[1].SetMapper(this);

	mCia[0].Reset();
	mCia[1].Reset();

	mVicII.SetMapper(this);
	mVicII.Reset();
}


void Cia6526Timer::Reset()
{
	mPeriod = 0;
	mCtrl = 0;
}

void Cia6526Timer::Step()
{
	if (mCtrl & CTRL_START) {
		if (mPos) {
			mPos--;
			if (!mPos) {
				NLOGV("Cia6526Timer", "Underflow on timer %d, ctrl=%#x, P=%#x",
						mIndex, mCia->mCtrl, mCia->mMemory->m6502->mRegs.F);

				mCia->mStatus |= (Cia6526::STATUS_TIMER_A_UNDERFLOW << mIndex);

				if (mCtrl & CTRL_RESTART) {
					mPos = mPeriod;
				}
				if (mCia->mCtrl & (Cia6526::CTRL_TIMER_A_IRQ << mIndex)) {
					mCia->mStatus |= Cia6526::STATUS_NMI;
					if (!(mCia->mMemory->m6502->mRegs.F & Emu6502::FLAG_I)) {
						if ((mCia->mMemory->ReadByte(0x0001) & 3) < 2) {
							// RAM at $E000-FFFF
							mCia->mMemory->m6502->Irq(0xFFFE);
						} else {
							// ROM at $E000-FFFF
							mCia->mMemory->m6502->Irq(0xFFFE); //0x0314);
						}
						mCia->mMemory->mSidPlayer->TimerIrq(Cia6526::TIMER_A);
					}
				}
			}
		}
	}
}


Cia6526::Cia6526()
{
	mTimer[0] = new Cia6526Timer(this, 0);
	mTimer[1] = new Cia6526Timer(this, 1);
}


Cia6526::~Cia6526()
{
	delete mTimer[0];
	delete mTimer[1];
}


void Cia6526::Reset()
{
	mCtrl = mStatus = 0;
	mTimer[TIMER_A]->Reset();
	mTimer[TIMER_B]->Reset();
}


uint8_t Cia6526::Read(uint16_t addr)
{
	uint8_t reg = addr & 0x1F;
	uint8_t result;

	NLOGD("Cia6526", "Read(%#x)", addr);

	switch (reg) {
	case 0x04:
		return mTimer[TIMER_A]->mPos & 0xFF;
		break;
	case 0x05:
		return (mTimer[TIMER_A]->mPos >> 8) & 0xFF;
		break;

	case 0x06:
		return mTimer[TIMER_B]->mPos & 0xFF;
		break;
	case 0x07:
		return (mTimer[TIMER_B]->mPos >> 8) & 0xFF;
		break;

	case 0x0D:
		result = mStatus;
		mStatus = 0;
		return result;
		break;

	case 0x0E:
		return mTimer[TIMER_A]->mCtrl;
		break;
	case 0x0F:
		return mTimer[TIMER_B]->mCtrl;
		break;

	default:
		break;
	}

	return 0;
}


void Cia6526::Write(uint16_t addr, uint8_t data)
{
	uint8_t reg = addr & 0x1F;

	NLOGD("Cia6526", "Write(%#x, %#x)", addr, data);

	switch (reg) {
	case 0x04:
		mTimer[TIMER_A]->mPeriod = (mTimer[TIMER_A]->mPeriod & 0xFF00) | data;
		break;
	case 0x05:
		mTimer[TIMER_A]->mPeriod = (mTimer[TIMER_A]->mPeriod & 0x00FF) | ((uint16_t)data << 8);
		break;
	case 0x06:
		mTimer[TIMER_B]->mPeriod = (mTimer[TIMER_B]->mPeriod & 0xFF00) | data;
		break;
	case 0x07:
		mTimer[TIMER_B]->mPeriod = (mTimer[TIMER_B]->mPeriod & 0x00FF) | ((uint16_t)data << 8);
		break;

	case 0x0D:
		for (int i = 0; i <= 4; i++) {
			if (data & 0x80) {
				if (data & (1 << i)) {
					mCtrl |= (1 << i);
				}
			} else {
				if (data & (1 << i)) {
					mCtrl &= ~(1 << i);
				}
			}
		}
		break;

	case 0x0E:
		mTimer[TIMER_A]->mCtrl = data;
		if (data & Cia6526Timer::CTRL_LOAD) {
			mTimer[TIMER_A]->mPos = mTimer[TIMER_A]->mPeriod;
		}
		break;
	case 0x0F:
		mTimer[TIMER_B]->mCtrl = data;
		if (data & Cia6526Timer::CTRL_LOAD) {
			mTimer[TIMER_B]->mPos = mTimer[TIMER_B]->mPeriod;
		}
		break;
	default:
		break;
	}
}


void VicII::Reset()
{
	mCycles = 0;
	mScanline = mRasterIrqLine = 0;
	mCtrl = mStatus = 0;
}

void VicII::Step()
{
	mCycles++;
	if (mCycles >= 63) {
		mCycles = 0;
		mScanline++;
		if (mScanline >= 312) {
			mScanline = 0;
		}

		if (mScanline == mRasterIrqLine) {
			NLOGD("VicII", "scanline=%d, ctrl=%#x, status=%#x, mRegs.F=%#x", mScanline, mCtrl, mStatus, mMemory->m6502->mRegs.F);
		}

		if ((mCtrl & 1) && ((mStatus & 1) == 0) && (mScanline == mRasterIrqLine)) {
			mStatus |= 1;
			if (!(mMemory->m6502->mRegs.F & Emu6502::FLAG_I)) {
				if ((mMemory->ReadByte(0x0001) & 3) < 2) {
					// RAM at $E000-FFFF
					mMemory->m6502->Irq(0xFFFE);
				} else {
					// ROM at $E000-FFFF
					mMemory->m6502->Irq(0xFFFE); //0x0314);
				}
				// ToDo: add a VicIrq method or make a general Irq method?
				mMemory->mSidPlayer->TimerIrq(Cia6526::TIMER_A);
			}
		}
	}
}

uint8_t VicII::Read(uint16_t addr)
{
	NLOGD("SidMapper", "Read(%#x)", addr);

	switch (addr & 0x3FF) {
	case 0x19:
		return mStatus;
	case 0x1A:
		return mCtrl;
	}
	return 0;
}

void VicII::Write(uint32_t addr, uint8_t data)
{
	NLOGD("SidMapper", "Write(%#x, %#x)", addr, data);

	switch (addr & 0x3FF) {
	case 0x11:
		mRasterIrqLine = (mRasterIrqLine & 0x100) | data;
		break;
	case 0x12:
		mRasterIrqLine = (mRasterIrqLine & 0xFF) | ((uint16_t)(data & 0x80) << 1);
		break;
	case 0x19:
		mStatus = data;
		break;
	case 0x1A:
		mCtrl = data;
		break;
	}

}

