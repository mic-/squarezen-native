/*
 * MusicPlayer.h
 *
 *  Created on: May 26, 2013
 *      Author: Mic
 */

#ifndef MUSICPLAYER_H_
#define MUSICPLAYER_H_

#include <FBase.h>
#include <stdint.h>
#include "Blip_Buffer.h"


class MusicPlayer
{
public:
	MusicPlayer() :
		mBlipBuf(NULL), mSynth(NULL), mState(STATE_CREATED) {}

	virtual ~MusicPlayer()
	{
		if (mBlipBuf) { delete mBlipBuf; mBlipBuf = NULL; }
		if (mSynth) { delete [] mSynth; mSynth = NULL; }
	}

	virtual result Prepare(Tizen::Base::String fileName) = 0;
	virtual result Run(uint32_t numSample, int16_t *buffer) = 0;
	virtual result Reset() = 0;

	virtual int GetState() const { return mState; }

	enum
	{
		STATE_CREATED,
		STATE_PREPARED,
		STATE_PLAYING,
	};

protected:
	Blip_Buffer *mBlipBuf;
	Blip_Synth<blip_low_quality,82> *mSynth;
	int mState;
};


#endif /* MUSICPLAYER_H_ */
