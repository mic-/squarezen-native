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
		mSubTitle(""),
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
	const std::string& GetSubTitle() const { return mSubTitle; }
	const std::string& GetAuthor() const { return mAuthor; }
	const std::string& GetComment() const { return mComment; }
	uint32_t GetNumSubSongs() const { return mSubSongs; }
	uint32_t GetDefaultSong() const { return mDefaultSong; }

	int GetLengthMs() const { return mLengthMs; }

	void SetTitle(char *str) { mTitle = str; }
	void SetSubTitle(char *str) { mSubTitle = str; }
	void SetAuthor(char *str) { mAuthor = str; }
	void SetComment(char *str) { mComment = str; }
	void SetNumSubSongs(uint32_t subSongs) { mSubSongs = subSongs; }
	void SetDefaultSong(uint32_t defaultSong) { mDefaultSong = defaultSong; }
	void SetLengthMs(int lengthMs) { mLengthMs = lengthMs; }

private:
	std::string mTitle, mSubTitle, mAuthor, mComment;
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

	/**
	 * Checks whether fileName contains the name of a supported file type (only
	 * looks at the file extension).
	 * @return true if the file is supported, false if it is not.
	 */
	static bool IsSupportedFileType(std::string fileName);

	/**
	 * Creates a new player object suitable for playing the file specified by
	 * fileName.
	 * @return A pointer to the player object, or NULL.
	 */
	static MusicPlayer *MusicPlayerFactory(std::string fileName);

	enum State
	{
		STATE_CREATED,
		STATE_PREPARING,
		STATE_PREPARED,
		STATE_PLAYING,
	};

	enum Result
	{
		OK = 0,
		ERROR_UNKNOWN = -1,
		ERROR_FILE_IO = -2,
		ERROR_OUT_OF_MEMORY = -3,
		ERROR_UNRECOGNIZED_FORMAT = -4,
		ERROR_DECOMPRESSION = -5,
		ERROR_BAD_STATE = -6,
	};

	/**
	 * Open the file specified by fileName.
	 * @param[out] musicFile An readable binary ifstream.
	 * @param[in] fileName The name of the file to open.
	 * @param[out] fileSize Size of the file in bytes.
	 * @return A MusicPlayer::Result set to MusicPlayer::OK on success, or an error code on failure.
	 */
	virtual MusicPlayer::Result OpenFile(std::ifstream& musicFile, std::string fileName, size_t& fileSize);

	/**
	 * Prepare playback of the file specified by fileName
	 * @return a MusicPlayer::Result containing MusicPlayer::OK on success, or an error code on failure.
	 */
	virtual MusicPlayer::Result Prepare(std::string fileName);
#ifdef __TIZEN__
	virtual MusicPlayer::Result Prepare(std::wstring fileName) { return Prepare(std::string(fileName.begin(), fileName.end())); }
#endif

	/**
	 * Run the player for numSamples samples and store the output in buffer.
	 * @return a MusicPlayer::Result containing MusicPlayer::OK on success, or an error code on failure.
	 */
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer) = 0;

	/**
	 * Reset the player to an initialized state (typically destroys the associated CPU/APU/DSP
	 * emulation objects).
	 */
	virtual MusicPlayer::Result Reset() = 0;

	/**
	 * @return A MusicPlayer::State containing the current state of the player.
	 */
	virtual MusicPlayer::State GetState() const { return mState; }

	const std::string& GetTitle() const { return mMetaData.GetTitle(); }
	const std::string& GetSubTtitle() const { return mMetaData.GetSubTitle(); }
	const std::string& GetAuthor() const { return mMetaData.GetAuthor(); }
	const std::string& GetComment() const { return mMetaData.GetComment(); }

	/**
	 * Get the number of sub-songs in the prepared file.
	 * @return Number of sub-songs.
	 */
	uint32_t GetNumSubSongs() const { return mMetaData.GetNumSubSongs(); }

	uint32_t GetDefaultSong() const { return mMetaData.GetDefaultSong(); }

	/**
	 * Get the length of the current (sub)song in milliseconds.
	 * @return (Sub)song length in milliseconds.
	 */
	int GetLengthMs() const { return mMetaData.GetLengthMs(); }

	/**
	 * Set the sub-song to play.
	 */
	virtual void SetSubSong(uint32_t subSong) {}

protected:
	Blip_Buffer *mBlipBuf;
	Blip_Synth<blip_low_quality,4096> *mSynth;
	MetaData mMetaData;
	MusicPlayer::State mState;
	static std::map<std::string, PlayerFactory> mSupportedFormats;
};


#endif /* MUSICPLAYER_H_ */
