/*
 * VgmPlayer.h
 *
 *  Created on: May 23, 2013
 *      Author: Mic
 */

#ifndef VGMPLAYER_H_
#define VGMPLAYER_H_

#include <FBase.h>
#include "SN76489.h"
#include "Blip_Buffer.h"
#include "MusicPlayer.h"

class SnChip;


class VgmPlayer : public MusicPlayer
{
public:
	VgmPlayer();

	virtual result Prepare(Tizen::Base::String fileName);
	virtual result Run(uint32_t numSample, int16_t *buffer);
	virtual result Reset();

private:
	void PresentBuffer(int16_t *out, Blip_Buffer *in);
	uint8_t GetData();
	void Step();

	uint32_t mWait;
	uint32_t mCycleCount, mSampleCycles;
	uint32_t mDataPos, mDataLen;
	Tizen::Base::ByteBuffer mVgmData;
	int16_t *mTempBuffer;
	SnChip *mSN76489;
};

#endif /* VGMPLAYER_H_ */
