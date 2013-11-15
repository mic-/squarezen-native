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
#define NLOG_TAG "SapPlayer"

#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "SapPlayer.h"


SapPlayer::SapPlayer()
	: mFormat(TYPE_UNKNOWN)
{

}


MusicPlayer *SapPlayerFactory()
{
	return new SapPlayer;
}


SapPlayer::~SapPlayer()
{
	delete m6502;
	delete mPokey;
	delete mMemory;

	m6502      = NULL;
	mPokey = NULL;
	mMemory    = NULL;
}


MusicPlayer::Result SapPlayer::Reset()
{
	// ToDo: implement
	NLOGV(NLOG_TAG, "Reset");

	mFormat = TYPE_UNKNOWN;
	mFastplayScanlines = 312;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


static MusicPlayer::Result ReadCrTerminatedTag(std::ifstream& musicFile, char *buffer, bool& noMoreTags)
{
	char *ptr;
	size_t tagLength;

	musicFile.read((char*)buffer, 1);

	if (0xFF == *((uint8_t*)buffer)) {
		musicFile.read((char*)buffer+1, 1);
		if (0xFF == *((uint8_t*)buffer+1)) {
			// We've reached the end of the tags when we find the byte sequence 0xFF 0xFF
			noMoreTags = true;
		} else {
			NLOGE(NLOG_TAG, "Expected 0xFF, 0xFF; got 0xFF %02x", *((uint8_t*)buffer+1));
			return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
		}
	} else {
		ptr = buffer;
		tagLength = 1;
		while (0x0D != *ptr && musicFile.good() && tagLength < 250) {
			ptr++;
			musicFile.read(ptr, 1);
			tagLength++;
		}
	}

	if (tagLength >= 250 || !musicFile.good()) {
		return MusicPlayer::ERROR_FILE_IO;
	}

	*ptr = 0;	// replace the CR character with a NUL terminator in the read buffer
	musicFile.read(ptr+1, 1); 	// consume the LF character

	return MusicPlayer::OK;
}


static char* UnquoteString(char *buffer, size_t stringLength)
{
	char *openingQuote = strchr(buffer, '\"');
	size_t openingQuotePos = (openingQuote - buffer);
	char *closingQuote = NULL;

	if (openingQuote && openingQuotePos < (stringLength-1) && (closingQuote = strchr(openingQuote+1, '\"'))) {
		*closingQuote = 0;
	} else {
		return NULL;
	}

	return openingQuote + 1;
}


MusicPlayer::Result SapPlayer::ParseTags(std::ifstream& musicFile)
{
	static char buffer[256];
	bool allTagsParsed = false;
	bool sapSignatureFound = false;
	MusicPlayer::Result result;

	while (!allTagsParsed) {
		result = ReadCrTerminatedTag(musicFile, buffer, allTagsParsed);
		if (MusicPlayer::OK != result) {
			return result;
		}

		size_t bufLen = strlen(buffer);

		if (strncmp(buffer, "SAP", 3) == 0) {
			sapSignatureFound = true;

		} else if (strncmp(buffer, "AUTHOR", 6) == 0) {
			char *str = UnquoteString(buffer, bufLen);
			if (str) {
				mMetaData.SetAuthor(str);
			} else {
				NLOGE(NLOG_TAG, "Malformed AUTHOR tag");
				return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
			}
		} else if (strncmp(buffer, "NAME", 4) == 0) {
			char *str = UnquoteString(buffer, bufLen);
			if (str) {
				mMetaData.SetTitle(str);
			} else {
				NLOGE(NLOG_TAG, "Malformed NAME tag");
				return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
			}
		} else if (strncmp(buffer, "DATE", 4) == 0) {
			char *str = UnquoteString(buffer, bufLen);
			if (str) {
				mMetaData.SetComment(str);
			} else {
				NLOGE(NLOG_TAG, "Malformed DATE tag");
				return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
			}

		} else if (strncmp(buffer, "SONGS", 5) == 0) {
			int numSongs = (bufLen >= 7) ? atoi(buffer + 6) : 0;
			if (numSongs > 0) {
				mMetaData.SetNumSubSongs(numSongs);
			} else {
				NLOGE(NLOG_TAG, "Malformed SONGS tag");
				return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
			}
		} else if (strncmp(buffer, "DEFSONG", 7) == 0) {
			int defSong = (bufLen >= 9) ? atoi(buffer + 8) : 0;
			if (defSong > 0) {
				mMetaData.SetDefaultSong(defSong);
			} else {
				NLOGE(NLOG_TAG, "Malformed DEFSONG tag");
				return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
			}

		} else if (strncmp(buffer, "INIT", 4) == 0) {
			// ToDo: handle
		} else if (strncmp(buffer, "MUSIC", 5) == 0) {
			// ToDo: handle
		} else if (strncmp(buffer, "PLAYER", 6) == 0) {
			// ToDo: handle

		} else if (strncmp(buffer, "STEREO", 6) == 0) {
			// ToDo: handle
		} else if (strncmp(buffer, "NTSC", 4) == 0) {
			// ToDo: handle

		} else if (strncmp(buffer, "FASTPLAY", 8) == 0) {
			// ToDo: handle

		} else if (strncmp(buffer, "TYPE B", 6) == 0) {
			mFormat = TYPE_B;
		} else if (strncmp(buffer, "TYPE C", 6) == 0) {
			mFormat = TYPE_C;
		} else if (strncmp(buffer, "TYPE D", 6) == 0) {
			mFormat = TYPE_D;
		} else if (strncmp(buffer, "TYPE S", 6) == 0) {
			mFormat = TYPE_S;
		} else if (strncmp(buffer, "TYPE R", 6) == 0) {
			mFormat = TYPE_R;
		}
	}

	if (!sapSignatureFound) {
		NLOGE(NLOG_TAG, "No SAP signature found");
		return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
	}

	return MusicPlayer::OK;
}


MusicPlayer::Result SapPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    mFormat = TYPE_UNKNOWN;
    MusicPlayer::Result parseResult = ParseTags(musicFile);
    if (MusicPlayer::OK != parseResult) {
    	musicFile.close();
    	return parseResult;
    }

    mMemory = new SapMapper(0);		// ToDo: set number of ROM banks
	m6502 = new Emu6502;
	mPokey = new Pokey;

	size_t dataBlocksRead = 0;

    while (musicFile.good()) {
        uint16_t dataDestStart, dataDestEnd;

        musicFile.read((char*)&dataDestStart, 2);
        if (!musicFile.good()) {
        	break;
        }
		musicFile.read((char*)&dataDestEnd, 2);

		if (musicFile.good()) {
			uint8_t c;
			for (uint16_t i = dataDestStart; i <= dataDestEnd; i++) {
				musicFile.read((char*)&c, 1);
				if (!musicFile.good()) {
					break;
				}
				mMemory->WriteByte(i, c);
			}
		}

		if (!musicFile.good()) {
			NLOGE(NLOG_TAG, "Error while reading SAP file");
			musicFile.close();
			return MusicPlayer::ERROR_FILE_IO;
		}
		dataBlocksRead++;
    }

	NLOGV(NLOG_TAG, "File read done");
	musicFile.close();

    if (!dataBlocksRead) {
    	NLOGE(NLOG_TAG, "Unable to read any SAP data blocks");
		return MusicPlayer::ERROR_FILE_IO;
    }

    mScanlineCycles = 114;
    mFastplayCycles = mFastplayScanlines * mScanlineCycles;

    switch (mFormat) {
    case TYPE_B:
    	m6502->mRegs.A = mMetaData.GetDefaultSong();
    	// ToDo: run the 6502 at the INIT address
    	break;
    case TYPE_C:
    	m6502->mRegs.A = 0x70;
    	m6502->mRegs.X = mMusicAddress & 0xFF;
    	m6502->mRegs.Y = mMusicAddress >> 8;
    	// ToDo: run the 6502 at mPlayAddress+3
    	m6502->mRegs.A = 0x00;
    	m6502->mRegs.X = mMetaData.GetDefaultSong();
    	// ToDo: run the 6502 at mPlayAddress+3
    	break;
    default:
    	NLOGE(NLOG_TAG, "Unsupported SAP player type (%d)", mFormat);
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    	break;
    }

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


void SapPlayer::SetSubSong(uint32_t subSong)
{
	// ToDo: implement
}


MusicPlayer::Result SapPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			// ToDo: run the 6502
		}

		mPokey->Step();

		// ToDo: finish

		mCycleCount++;
		if (mCycleCount >= mFastplayCycles) {
			mCycleCount = 0;
		}
	}

	return MusicPlayer::OK;
}


size_t SapPlayer::GetNumChannels() const{
	// ToDo: implement
	return 0;
}


void SapPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}


