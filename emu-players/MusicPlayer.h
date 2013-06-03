/*
 * MusicPlayer.h
 *
 *  Created on: May 26, 2013
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

#ifndef MUSICPLAYER_H_
#define MUSICPLAYER_H_

#include <string>
#include <stdint.h>
#include "Blip_Buffer.h"


class MetaData
{
public:
	MetaData() :
		mTitle("Unknown"),
		mAuthor("Unknown"),
		mComment("Unknown"),
		mLengthMs(0) {}

	const std::string& GetTitle() const { return mTitle; }
	const std::string& GetAuthor() const { return mAuthor; }
	const std::string& GetComment() const { return mComment; }

	void SetTitle(char *str) { mTitle = str; }
	void SetAuthor(char *str) { mAuthor = str; }
	void SetComment(char *str) { mComment = str; }

	std::string mTitle, mAuthor, mComment;
	int mLengthMs;
};


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

	virtual int Prepare(std::string fileName) = 0;
#ifndef __ANDROID__
	virtual int Prepare(std::wstring fileName) { return Prepare(std::string(fileName.begin(), fileName.end())); }
#endif
	virtual int Run(uint32_t numSamples, int16_t *buffer) = 0;
	virtual int Reset() = 0;

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
	MetaData mMetaData;
	int mState;
};


#endif /* MUSICPLAYER_H_ */
