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
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


MusicPlayer::Result SapPlayer::ParseTags(std::ifstream& musicFile)
{
	static char buffer[256];
	char *ptr;
	size_t tagLength;
	bool allTagsParsed = false;
	bool sapSignatureFound = false;

	while (!allTagsParsed) {
		musicFile.read((char*)buffer, 1);

		if (0xFF == *((uint8_t*)buffer)) {
			musicFile.read((char*)buffer+1, 1);
			if (0xFF == *((uint8_t*)buffer+1)) {
				// We've reached the end of the tags when we find the byte sequence 0xFF 0xFF
				allTagsParsed = true;
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
			if (!musicFile.good()) {
				NLOGE(NLOG_TAG, "Error while reading SAP tag data");
				return MusicPlayer::ERROR_FILE_IO;
			}
			*ptr = 0;	// replace the CR character with a NUL terminator in the read buffer
			musicFile.read(ptr+1, 1); 	// consume the LF character
			size_t bufLen = strlen(buffer);

			if (strncmp(buffer, "SAP", 3) == 0) {
				sapSignatureFound = true;

			} else if (strncmp(buffer, "AUTHOR", 6) == 0) {
				char *openingQuote = strchr(buffer, '\"');
				char *closingQuote = NULL;
				if (openingQuote && (closingQuote = strchr(openingQuote+1, '\"'))) {
					*closingQuote = 0;
					mMetaData.SetAuthor(openingQuote+1);
				} else {
					NLOGE(NLOG_TAG, "Malformed AUTHOR tag");
					return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
				}
			} else if (strncmp(buffer, "NAME", 4) == 0) {
				char *openingQuote = strchr(buffer, '\"');
				char *closingQuote = NULL;
				if (openingQuote && (closingQuote = strchr(openingQuote+1, '\"'))) {
					*closingQuote = 0;
					mMetaData.SetTitle(openingQuote+1);
				} else {
					NLOGE(NLOG_TAG, "Malformed NAME tag");
					return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
				}
			} else if (strncmp(buffer, "DATE", 4) == 0) {
				char *openingQuote = strchr(buffer, '\"');
				char *closingQuote = NULL;
				if (openingQuote && (closingQuote = strchr(openingQuote+1, '\"'))) {
					*closingQuote = 0;
					mMetaData.SetComment(openingQuote+1);
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
    // ToDo: finish

	NLOGV(NLOG_TAG, "File read done");
	musicFile.close();

	m6502 = new Emu6502;
	mPokey = new Pokey;

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
	// ToDo: implement
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


