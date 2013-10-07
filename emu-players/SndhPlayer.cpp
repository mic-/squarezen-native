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

#include <cstring>
#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "SndhPlayer.h"


SndhPlayer::SndhPlayer()
	: m68k(NULL)
	, mYm(NULL)
	, mMemory(NULL)
{
}

MusicPlayer *SndhPlayerFactory()
{
	return new SndhPlayer;
}

SndhPlayer::~SndhPlayer()
{
	delete m68k;
	delete mYm;
	delete mMemory;
	m68k = NULL;
	mYm = NULL;
	mMemory = NULL;
}

MusicPlayer::Result SndhPlayer::Reset()
{
	// ToDo: implement
	NLOGV("SndhPlayer", "Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

int SndhPlayer::ReadString(std::ifstream& sndhFile, char *buffer, size_t maxChars)
{
	char c;
	int i = 0;

	while (sndhFile.good()) {
		c = sndhFile.get();
		if (c == 0 || !(sndhFile.good())) {
			break;
		}
		if (i < maxChars) {
			buffer[i++] = c;
		}
	}
	buffer[i] = 0;
	return i;
}

MusicPlayer::Result SndhPlayer::ParseTags(std::ifstream& sndhFile)
{
	static char asciiBuffer[256];
	char tag[4];
	bool headerEnd = false;

	while (!headerEnd) {
		sndhFile.read(tag, 4);
		if (!sndhFile) {
			NLOGE("SndhPlayer", "Reading SNDH tag failed");
			return MusicPlayer::ERROR_FILE_IO;
		}
		if (strncmp(tag, "HDNS", 4) == 0) {
			headerEnd = true;
		} else if (strncmp(tag, "TITL", 4) == 0) {
			(void)ReadString(sndhFile, asciiBuffer, 255);
			mMetaData.SetTitle(asciiBuffer);
		} else if (strncmp(tag, "COMM", 4) == 0) {
			(void)ReadString(sndhFile, asciiBuffer, 255);
			mMetaData.SetAuthor(asciiBuffer);
		} else if (strncmp(tag, "RIPP", 4) == 0) {
			(void)ReadString(sndhFile, asciiBuffer, 255);
			mMetaData.SetComment(asciiBuffer);
		} else if (strncmp(tag, "CONV", 4) == 0) {
			(void)ReadString(sndhFile, asciiBuffer, 255);

		// ToDo: handle all other supported tags

		} else {
			NLOGE("SndhPlayer", "Unsupported tag found: %c%c%c%c", tag[0], tag[1], tag[2], tag[3]);
			return MusicPlayer::ERROR_FILE_IO;
		}
	}

	return MusicPlayer::OK;
}


MusicPlayer::Result SndhPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV("SndhPlayer", "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    NLOGV("SndhPlayer", "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE("SndhPlayer", "Reading SNDH header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    if (strncmp(mFileHeader.signature, "SNDH", 4)) {
    	NLOGE("SndhPlayer", "Bad SNDH header signature");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    if (MusicPlayer::OK != (result = ParseTags(musicFile))) {
        musicFile.close();
		return result;
    }

    // ToDo: finish

	NLOGD("SndhPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


MusicPlayer::Result SndhPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}

