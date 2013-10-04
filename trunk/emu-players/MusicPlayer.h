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

#include <fstream>
#include <map>
#include <string>
#include <stdint.h>
#include "Blip_Buffer.h"

class MusicPlayer;

class MetaData
{
public:
	MetaData() :
		mTitle("Unknown"),
		mAuthor("Unknown"),
		mComment("Unknown"),
		mLengthMs(MetaData::SONG_LENGTH_INFINITE),
		mSubSongs(1),
		mDefaultSong(1) {}

	enum
	{
		SONG_LENGTH_INFINITE = 0,
	};

	const std::string& GetTitle() const { return mTitle; }
	const std::string& GetAuthor() const { return mAuthor; }
	const std::string& GetComment() const { return mComment; }
	uint32_t GetNumSubSongs() const { return mSubSongs; }
	uint32_t GetDefaultSong() const { return mDefaultSong; }

	int GetLengthMs() const { return mLengthMs; }

	void SetTitle(char *str) { mTitle = str; }
	void SetAuthor(char *str) { mAuthor = str; }
	void SetComment(char *str) { mComment = str; }
	void SetNumSubSongs(uint32_t subSongs) { mSubSongs = subSongs; }
	void SetDefaultSong(uint32_t defaultSong) { mDefaultSong = defaultSong; }
	void SetLengthMs(int lengthMs) { mLengthMs = lengthMs; }

private:
	std::string mTitle, mAuthor, mComment;
	int mLengthMs;
	uint32_t mSubSongs;
	uint32_t mDefaultSong;
};


typedef MusicPlayer* (*PlayerFactory)(void);


class MusicPlayer
{
public:
	MusicPlayer() :
		mBlipBuf(NULL), mSynth(NULL), mState(STATE_CREATED) {}

	virtual ~MusicPlayer()
	{
		delete mBlipBuf; mBlipBuf = NULL;
		delete [] mSynth; mSynth = NULL;
	}

	static bool IsSupportedFileType(std::string fileName);
	static MusicPlayer *MusicPlayerFactory(std::string fileName);

	virtual int OpenFile(std::ifstream& musicFile, std::string fileName, size_t& fileSize);

	/*
	 * Prepare playback of the file specified by fileName
	 */
	virtual int Prepare(std::string fileName);
#ifdef __TIZEN__
	virtual int Prepare(std::wstring fileName) { return Prepare(std::string(fileName.begin(), fileName.end())); }
#endif

	/*
	 * Run the player for numSamples samples and store the output in buffer
	 */
	virtual int Run(uint32_t numSamples, int16_t *buffer) = 0;
	virtual int Reset() = 0;

	virtual int GetState() const { return mState; }

	const std::string& GetTitle() const { return mMetaData.GetTitle(); }
	const std::string& GetAuthor() const { return mMetaData.GetAuthor(); }
	const std::string& GetComment() const { return mMetaData.GetComment(); }

	/*
	 * Get the number of subsongs in the prepared file
	 */
	uint32_t GetNumSubSongs() const { return mMetaData.GetNumSubSongs(); }

	uint32_t GetDefaultSong() const { return mMetaData.GetDefaultSong(); }

	/*
	 * Get the length of the current (sub)song in milliseconds
	 */
	int GetLengthMs() const { return mMetaData.GetLengthMs(); }

	virtual void SetSubSong(uint32_t subSong) {}

	enum
	{
		STATE_CREATED,
		STATE_PREPARING,
		STATE_PREPARED,
		STATE_PLAYING,
	};

	enum
	{
		OK = 0,
		ERROR_UNKNOWN = -1,
		ERROR_FILE_IO = -2,
		ERROR_OUT_OF_MEMORY = -3,
		ERROR_UNRECOGNIZED_FORMAT = -4,
		ERROR_DECOMPRESSION = -5,
		ERROR_BAD_STATE = -6,
	};

protected:
	Blip_Buffer *mBlipBuf;
	Blip_Synth<blip_low_quality,4096> *mSynth;
	MetaData mMetaData;
	int mState;
	static std::map<std::string, PlayerFactory> mSupportedFormats;
};


#endif /* MUSICPLAYER_H_ */
