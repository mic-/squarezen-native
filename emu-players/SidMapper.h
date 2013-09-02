/*
 * SidMapper.h
 *
 *  Created on: Jul 4, 2013
 *
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

#ifndef SIDMAPPER_H_
#define SIDMAPPER_H_

#include <stdint.h>
#include "MemoryMapper.h"
#include "Mos6581.h"

class Emu6502;
class Cia6526;
class SidPlayer;
class SidMapper;


class Cia6526Timer : public Oscillator
{
public:
	Cia6526Timer(Cia6526 *cia, uint8_t index) : mCia(cia), mIndex(index) {}
	virtual ~Cia6526Timer() {}

	virtual void Reset();
	virtual void Step();

	enum
	{
		CTRL_START = 0x01,
		CTRL_RESTART = 0x08,
		CTRL_LOAD = 0x10,

	};

	Cia6526 *mCia;
	uint8_t mCtrl;
	uint8_t mIndex;
};


class Cia6526
{
public:
	Cia6526();
	virtual ~Cia6526();

	void Reset();
	void Write(uint16_t addr, uint8_t data);
	uint8_t Read(uint16_t addr);

	void SetMapper(SidMapper *memory) { mMemory = memory; }

	enum
	{
		TIMER_A = 0,
		TIMER_B = 1,
	};

	enum
	{
		CTRL_TIMER_A_IRQ = 0x01,
		CTRL_TIMER_B_IRQ = 0x02,
	};

	enum
	{
		STATUS_TIMER_A_UNDERFLOW = 0x01,
		STATUS_TIMER_B_UNDERFLOW = 0x02,
		STATUS_NMI = 0x80,
	};

	SidMapper *mMemory;
	Cia6526Timer *mTimer[2];
	uint8_t mCtrl;
	uint8_t mStatus;
};


class VicII : public Oscillator
{
public:
	VicII() {}
	virtual ~VicII() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t data);
	uint8_t Read(uint16_t addr);

	void SetMapper(SidMapper *memory) { mMemory = memory; }

	SidMapper *mMemory;
	uint32_t mCycles;
	uint32_t mScanline, mRasterIrqLine;
	uint8_t mCtrl;
	uint8_t mStatus;
};


class SidMapper : public MemoryMapper
{
public:
	SidMapper();
	virtual ~SidMapper();

	uint8_t *GetRamPointer() const { return mRam; }
	void SetCpu(Emu6502 *cpu) { m6502 = cpu; }
	void SetSid(Mos6581 *sid) { mSid = sid; }
	void SetSidPlayer(SidPlayer *sidPlayer) { mSidPlayer = sidPlayer; }

	virtual void Reset();
	virtual uint8_t ReadByte(uint16_t addr);
	virtual void WriteByte(uint16_t addr, uint8_t data);

	enum
	{
		CIA1 = 0,
		CIA2 = 1,
	};

	Emu6502 *m6502;
	Cia6526 mCia[2];
	VicII mVicII;
	SidPlayer *mSidPlayer;

private:
	uint8_t *mRam;
	Mos6581 *mSid;
};


#endif	/* SIDMAPPER_H_ */
