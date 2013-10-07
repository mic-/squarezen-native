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

#define NLOG_LEVEL_DEBUG 0

#include <algorithm>
#include "NativeLogger.h"
#include "MusicPlayer.h"
#include "GbsPlayer.h"
#include "HesPlayer.h"
#include "KssPlayer.h"
#include "NsfPlayer.h"
#include "SgcPlayer.h"
#include "SidPlayer.h"
#include "SpcPlayer.h"
#include "VgmPlayer.h"
#include "YmPlayer.h"

std::map<std::string, PlayerFactory> CreateSupportedFormatsMap()
{
	std::map<std::string, PlayerFactory> formatsMap;

	formatsMap.insert( std::make_pair(".gbs", &GbsPlayerFactory) );
	formatsMap.insert( std::make_pair(".nsf", &NsfPlayerFactory) );
	formatsMap.insert( std::make_pair(".sid", &SidPlayerFactory) );
	formatsMap.insert( std::make_pair(".vgm", &VgmPlayerFactory) );
	formatsMap.insert( std::make_pair(".vgz", &VgmPlayerFactory) );
	formatsMap.insert( std::make_pair(".ym",  &YmPlayerFactory)  );

	return formatsMap;
}

std::map<std::string, PlayerFactory> MusicPlayer::mSupportedFormats = CreateSupportedFormatsMap();


bool MusicPlayer::IsSupportedFileType(std::string fileName)
{
	std::string lowerCaseName = fileName;
	std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);

	for (std::map<std::string, PlayerFactory>::iterator it = mSupportedFormats.begin(); it != mSupportedFormats.end(); ++it) {
		size_t pos = lowerCaseName.rfind(it->first);
		if (pos != std::string::npos) {
			return true;
		}
	}

	return false;
}


MusicPlayer *MusicPlayer::MusicPlayerFactory(std::string fileName)
{
	std::string lowerCaseName = fileName;
	std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);

	for (std::map<std::string, PlayerFactory>::iterator it = mSupportedFormats.begin(); it != mSupportedFormats.end(); ++it) {
		size_t pos = lowerCaseName.rfind(it->first);
		if (pos != std::string::npos) {
			return (it->second)();
		}
	}

	return NULL;
}


MusicPlayer::Result MusicPlayer::Prepare(std::string fileName)
{
    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    mState = MusicPlayer::STATE_PREPARING;
    return MusicPlayer::OK;
}


MusicPlayer::Result MusicPlayer::OpenFile(std::ifstream& musicFile, std::string fileName, size_t& fileSize)
{
	fileSize = 0;

    musicFile.open(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NLOGE("MusicPlayer", "Failed to open file %S", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    return MusicPlayer::OK;
}
