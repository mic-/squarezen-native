/*
 * YmPlayer.h
 *
 *  Created on: May 24, 2013
 *      Author: Mic
 */

#ifndef YMPLAYER_H_
#define YMPLAYER_H_

#include <stdint.h>
#include <stddef.h>
#include <FBase.h>
#include <FIo.h>
#include <FApp.h>
#include <FMedia.h>
#include "YM2149.h"
#include "Blip_Buffer.h"
#include "MusicPlayer.h"


class YmPlayer : public MusicPlayer {
public:
	YmPlayer();

	virtual result Prepare(Tizen::Base::String fileName);
	virtual result Run(uint32_t numSample, int16_t *buffer);
	virtual result Reset();

	/*result Init(Tizen::Base::String fileName);
	void Close();
	void Run(uint32_t numSamples, int16_t *buffer);*/

	const char *GetSongName() const { return (const char*)mSongName; }
	const char *GetAuthorName() const { return (const char*)mAuthorName; }
	const char *GetSongComment() const { return (const char*)mSongComment; }

	YmChip mChip;

private:
	void PresentBuffer(int16_t *out, Blip_Buffer *in);

	uint8_t *mYmData;
	uint32_t mCycleCount, mFrameCycles;
	uint32_t mFrame, mNumFrames;
	uint32_t mDataOffs;
	uint8_t *mYmRegStream;
	uint8_t mYmRegs[16];

	int16_t *mTempBuffer;

	char *mSongName, *mAuthorName, *mSongComment;

	//Blip_Buffer mBlipBuf;
	//Blip_Synth<blip_low_quality,82> mSynth[3];
};


#endif /* YMPLAYER_H_ */
