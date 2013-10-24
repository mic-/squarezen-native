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
#define NLOG_TAG "FcPlayer"

#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "FcPlayer.h"


FcPlayer::FcPlayer()
	: mIsFc14(false)
	, mFileHeader(NULL)
{

}


MusicPlayer *FcPlayerFactory()
{
	return new FcPlayer;
}


FcPlayer::~FcPlayer()
{

}


MusicPlayer::Result FcPlayer::Reset()
{
	// ToDo: implement
	NLOGV(NLOG_TAG, "Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


MusicPlayer::Result FcPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    char id[4];
    NLOGV(NLOG_TAG, "Reading signature");
    musicFile.read(id, 4);
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading FC header ID failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	if (strncmp(id, "SMOD", 4) == 0) {
		mFileHeader = new Fc13FileHeader;
		 musicFile.read((char*)&mFileHeader, sizeof(Fc13FileHeader) - 4);
	} else if (strncmp(id, "FC14", 4) == 0) {
		mIsFc14 = true;
		mFileHeader = new Fc14FileHeader;
		musicFile.read((char*)&mFileHeader, sizeof(Fc14FileHeader) - 4);
	} else {
		NLOGE(NLOG_TAG, "Unsupported Future Composer type: %c%c%c%c", id[0], id[1], id[2], id[3]);
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
	}

	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading the FC header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}


    // ToDo: finish

	NLOGV(NLOG_TAG, "File read done");
	musicFile.close();

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


MusicPlayer::Result FcPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}

void FcPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}
