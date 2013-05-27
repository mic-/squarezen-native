/*
 * SN76489.h
 *
 *  Created on: May 23, 2013
 *      Author: Mic
 */

#ifndef SN76489_H_
#define SN76489_H_

#include "Oscillator.h"
#include "VgmPlayer.h"


class SnChip;
class VgmPlayer;


class SnChannel : public Oscillator
{
public:
	virtual ~SnChannel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint8_t addr, uint8_t val);

	void SetChip(SnChip *chip) { mChip = chip; }
	void SetIndex(uint8_t index) { mIndex = index; }

	enum
	{
		NOISE_PERIOD_MASK = 3,
		NOISE_PERIOD_TONE2 = 3,
	};

	enum
	{
		NOISE_TYPE_MASK = 4,
		NOISE_TYPE_WHITE = 4,
		NOISE_TYPE_PER = 0,
	};

	SnChip *mChip;
	uint32_t mLfsr;
	uint16_t mVol;
	uint16_t mOut;
	uint8_t mIndex;
	uint8_t mPhase;
};


class SnChip
{
public:
	void Reset();
	void Step();
	void Write(uint8_t addr, uint8_t val);

	static const uint16_t SN76489_VOL_TB[16];

	enum
	{
		CMD_TYPE_MASK = 0x10,
		CMD_TYPE_FREQ = 0x00,
		CMD_TYPE_VOL = 0x10,
		CMD_LATCH_MASK = 0x80,
	};

	uint8_t mLatchedByte;
	SnChannel mChannels[4];
};


#endif /* SN76489_BLIP_H_ */
