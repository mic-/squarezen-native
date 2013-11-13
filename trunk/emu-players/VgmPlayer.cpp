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

//#ifdef __TIZEN__
//#include <FBase.h>
//#endif
#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include "NativeLogger.h"
#include "VgmPlayer.h"
#ifdef __ANDROID__
#include <zlib.h>
#else
#include "../zlib/zlib.h"
#endif

//#define LOG_PCM 1


VgmPlayer::VgmPlayer()
	: mSN76489(NULL)
	, mWait(0)
{
}

MusicPlayer *VgmPlayerFactory()
{
	return new VgmPlayer;
}

MusicPlayer::Result VgmPlayer::Reset()
{
	NLOGV("VgmPlayer", "VgmPlayer::Reset");

	delete mSN76489;
	mSN76489 = NULL;
	delete mBlipBuf;
	mBlipBuf = NULL;
	delete [] mSynth;
	mSynth = NULL;

	mState = MusicPlayer::STATE_CREATED;

	return MusicPlayer::OK;
}


void VgmPlayer::ParseGd3(size_t fileSize)
{
	size_t offset;
	short *gd3w;

	if (mFileHeader->gd3Offset == 0) return;

	offset = mFileHeader->gd3Offset + 0x14 + 12;

	memset(mAsciiTitle, 0, 256);
	memset(mAsciiAuthor, 0, 256);
	memset(mAsciiComment, 0, 256);

	gd3w = (short*)&mVgmData[offset];

	for (int i = 0; *gd3w != 0 && i < 255; i++) {
		if (offset >= fileSize) return;
		mAsciiTitle[i] = *gd3w++;
		offset++;
	}
	gd3w++; offset++;
	mMetaData.SetTitle(mAsciiTitle);

	while (*gd3w++) offset++; offset++;	// Skip japanese title

	for (int i = 0; *gd3w != 0 && i < 255; i++) {
		if (offset >= fileSize) return;
		mAsciiComment[i] = *gd3w++;
		offset++;
	}
	gd3w++; offset++;
	mMetaData.SetComment(mAsciiComment);

	while (*gd3w++) offset++; offset++;	// Skip japanese game name
	while (*gd3w++) offset++; offset++;	// Skip system name
	while (*gd3w++) offset++; offset++;	// Skip japanese system name

	for (int i = 0; *gd3w != 0 && i < 255; i++) {
		if (offset >= fileSize) return;
		mAsciiAuthor[i] = *gd3w++;
		offset++;
	}
	gd3w++;
	mMetaData.SetAuthor(mAsciiAuthor);
}


MusicPlayer::Result VgmPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
    size_t fileSize;

	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

#ifdef LOG_PCM
    pcmFile = fopen("/sdcard/log.pcm", "wb");
    loggedBuffers = 0;
#endif

    //AppLog("Trying to allocate %d bytes", fileSize);

    char *buf = new char[fileSize];
	musicFile.read(buf, fileSize);
	if (!musicFile) {
		NLOGE("VgmPlayer", "ifstream::read failed");
		musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	musicFile.close();

	mFileHeader = (VgmFileHeader*)buf;

	if (strncmp(mFileHeader->ID, "Vgm", 3)) {
		NLOGD("VgmPlayer", "Unzipping file");

		delete [] buf;
		gzFile inFileZ = gzopen(std::string(fileName.begin(), fileName.end()).c_str(), "rb");
		if (inFileZ == NULL) {
			NLOGE("VgmPlayer", "Error: Failed to gzopen %s\n", std::string(fileName.begin(), fileName.end()).c_str());
			return MusicPlayer::ERROR_DECOMPRESSION;
		}
		NLOGD("VgmPlayer", "Opened file successfully");
		std::vector<uint8_t> unzippedData;
		int numUnzippedBytes = 0;
		uint8_t *unzipBuffer = new uint8_t[8192];
		while (unzipBuffer) {
			numUnzippedBytes = gzread(inFileZ, unzipBuffer, 8192);
			NLOGD("VgmPlayer", "gzread returned %d bytes", numUnzippedBytes);
			if (numUnzippedBytes > 0) {
				for (i = 0; i < numUnzippedBytes; i++) {
					unzippedData.push_back(unzipBuffer[i]);
				}
			} else {
				break;
			}
		}
		gzclose(inFileZ);
		delete [] unzipBuffer;
		fileSize = unzippedData.size();
		NLOGD("VgmPlayer", "Unzipped size: %d bytes", fileSize);
		mVgmData = new uint8_t[fileSize];
		memcpy(mVgmData, unzippedData.data(), fileSize);
		if (strncmp((char*)mVgmData, "Vgm", 3)) {
			NLOGE("VgmPlayer", "Bad VGM header signature");
			return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
		}
	} else {
		mVgmData = new uint8_t[fileSize];
		memcpy(mVgmData, buf, fileSize);
		delete [] buf;
	}
	mFileHeader = (VgmFileHeader*)mVgmData;

	if (mFileHeader->sn76489Clock == 0) {
		NLOGE("VgmPlayer", "Only the SN76489 is supported, but is not used by this song");
		return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
	}

	ParseGd3(fileSize);

	if ((mFileHeader->version & 0xFF) >= 0x50) {
		mDataPos = mFileHeader->dataOffset;
	} else {
		mDataPos = 0x40;
	}
	mDataLen = fileSize;

	if (mFileHeader->loopOffset) {
		mLoopPos = mFileHeader->loopOffset + 0x1C;
	} else {
		mLoopPos = mDataPos;
	}

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,4096>[4];

	if (mBlipBuf->set_sample_rate(44100)) {
    	NLOGE("VgmPlayer", "Failed to set blipbuffer sample rate");
		return MusicPlayer::ERROR_UNKNOWN;
	}
	mBlipBuf->clock_rate(3579545);

	mSampleCycles = 3579545 / 44100;
	mCycleCount = 0;

    // Setup waves
	for (i = 0; i < 4; i++) {
		mSynth[i].volume(0.22);
		mSynth[i].output(mBlipBuf);
	}

	mMetaData.SetLengthMs((mFileHeader->totalSamples * 10) / 441);

	mSN76489 = new SnChip();
	mSN76489->Reset();

	NLOGD("VgmPlayer", "Prepare done");
	mState = MusicPlayer::STATE_PREPARED;

	return MusicPlayer::OK;
}


uint8_t VgmPlayer::GetData()
{
	uint8_t c = 0;

	if (mDataPos < mDataLen) {
		c = mVgmData[mDataPos++];
		if (mDataPos >= mDataLen)
			mDataPos = 0x40; // TODO: loop?
	}

	return c;
}


void VgmPlayer::Step()
{
	if (mWait) {
		mWait--;
		return;
	}

	uint8_t c = GetData();

	//NLOGV("VgmPlayer", "Step got %#x from VGM file", c);

	switch (c) {
	case 0x4F:
		mSN76489->Write(0x06, GetData());
		break;
	case 0x50:
		mSN76489->Write(0x7F, GetData());
		break;
	case 0x51: case 0x52: case 0x53: case 0x54:
		// FM chips; ignore
		GetData();
		GetData();
		break;
	case 0x61:
		mWait = GetData();
		mWait += (uint16_t)GetData() << 8;
		break;
	case 0x62:
		// Wait 1/60 s
		mWait = 735;
		break;
	case 0x63:
		// Wait 1/50 s
		mWait = 882;
		break;
	case 0x66:
		// TODO: loop song
		mDataPos = mLoopPos;
		break;
	case 0x70: case 0x71: case 0x72: case 0x73:
	case 0x74: case 0x75: case 0x76: case 0x77:
	case 0x78: case 0x79: case 0x7A: case 0x7B:
	case 0x7C: case 0x7D: case 0x7E: case 0x7F:
		mWait = (c & 0x0F) + 1;
		break;
	}
}


MusicPlayer::Result VgmPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	//NLOGV("VgmPlayer", "Run(%d, %p) -> %d clocks", numSamples, buffer, blipLen);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			Step();
		}

		mSN76489->Step();

		for (i = 0; i < 4; i++) {
			out = (-mSN76489->mChannels[i].mPhase) & SnChip::SN76489_VOL_TB[mSN76489->mChannels[i].mVol & 0x0F];

			if (out != mSN76489->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				mSN76489->mChannels[i].mOut = out;
			}
		}

		mCycleCount++;
		if (mCycleCount == mSampleCycles) {
			mCycleCount = 0;
		}
	}

	mBlipBuf->end_frame(blipLen);
	PresentBuffer(buffer, mBlipBuf);

#ifdef LOG_PCM
	if (loggedBuffers < 5) {
		loggedBuffers++;
		fwrite(buffer, 1, numSamples<<2, pcmFile);
		if (loggedBuffers == 5) fclose(pcmFile);
	}
#endif

	return MusicPlayer::OK;
}


size_t VgmPlayer::GetNumChannels() const
{
	// ToDo: implement
	return 0;
}

void VgmPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}
