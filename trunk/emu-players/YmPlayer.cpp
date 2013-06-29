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

#include <iostream>
#include <fstream>
#include <stddef.h>
#include <string.h>
#include "NativeLogger.h"
#include "YmPlayer.h"
#include "../lha/lha_decoder.h"

#ifdef LOG_PCM
FILE *pcmFile;
int loggedBuffers;
#endif


YmPlayer::YmPlayer()
	: mYmData(NULL)
{
}


int YmPlayer::Reset()
{
	//NativeLog(0, "YmPlayer", "YmPlayer::Reset");

	delete [] mYmData;
	mYmData = NULL;

	delete mBlipBuf;
	delete [] mSynth;
	mBlipBuf = NULL;
	mSynth = NULL;

	mState = MusicPlayer::STATE_CREATED;

	return 0;
}


size_t lhDecodeCallback(void *buf, size_t buf_len, void *user_data)
{
	YmPlayer *ymPlayer = (YmPlayer*)user_data;
	int bytesToCopy = ymPlayer->mLhDataAvail - ymPlayer->mLhDataPos;
	if (bytesToCopy < 0) return 0;
	bytesToCopy = (bytesToCopy > buf_len) ? buf_len : bytesToCopy;

	memcpy(buf, &(ymPlayer->mYmData[ymPlayer->mLhDataPos]), bytesToCopy);
	ymPlayer->mLhDataPos += bytesToCopy;
	return bytesToCopy;
}


int YmPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
	uint32_t sampleBytes;
    size_t fileSize;
    uint16_t  numDigiDrums;

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    std::ifstream musicFile(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!musicFile) {
    	NativeLog(0, "YmPlayer", "Failed to open file %s", fileName.c_str());
    	return MusicPlayer::ERROR_FILE_IO;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

    if (fileSize < 7) {
    	musicFile.close();
    	NativeLog(0, "YmPlayer", "File is too small (%d bytes)", fileSize);
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

#ifdef LOG_PCM
    pcmFile = fopen("/sdcard/log.pcm", "wb");
    loggedBuffers = 0;
#endif

    //AppLog("Trying to allocate %d bytes", fileSize);

    mYmData = new unsigned char[fileSize];
    if (!mYmData) {
    	//AppLog("Failed to allocate memory");
		return MusicPlayer::ERROR_OUT_OF_MEMORY;
    }

	//AppLog("Allocation ok");

	musicFile.read((char*)mYmData, fileSize);
	if (!musicFile) {
		NativeLog(0, "YmPlayer", "Failed to read data from file");
		musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}
	musicFile.close();

	if (strncmp((char*)&mYmData[2], "-lh5", 4) == 0) {
		NativeLog(0, "YmPlayer", "File is compressed; decoding..");
		LHADecoderType *decoderType;
		LHADecoder *decoder;
		mLhHeader = (LhFileHeader*)mYmData;
		mLhDataPos = 24 + mLhHeader->filenameLength;
		mLhDataAvail = fileSize;
		NativeLog(0, "YmPlayer", "Decompressed size: %d bytes", mLhHeader->uncompressedSize);
		if (!(decoderType = lha_decoder_for_name("-lh5-"))) {
			NativeLog(0, "YmPlayer", "Failed to get decoder type for \"-lh5-\"");
			return MusicPlayer::ERROR_DECOMPRESSION;
		}
		if (!(decoder = lha_decoder_new(decoderType, lhDecodeCallback, this, mLhHeader->uncompressedSize))) {
			NativeLog(0, "YmPlayer", "Failed to get decoder for \"-lh5-\"");
			return MusicPlayer::ERROR_DECOMPRESSION;
		}
		uint8_t *decoded = new uint8_t[mLhHeader->uncompressedSize];
		uint8_t *p = decoded;
		while (lha_decoder_read(decoder, p, 4096) == 4096) {
			p += 4096;
		}
		NativeLog(0, "YmPlayer", "Number of bytes decoded: %d", lha_decoder_get_length(decoder));
		lha_decoder_free(decoder);
		delete [] mYmData;
		mYmData = decoded;
	}

	numDigiDrums = ((uint16_t)mYmData[0x14]) << 8;
	numDigiDrums |= mYmData[0x15];

	mYmRegStream = (uint8_t*)mYmData + 0x22;

	i = 0;
	while (numDigiDrums) {
		sampleBytes  = ((uint32_t)mYmRegStream[0]) << 24;
		sampleBytes |= ((uint32_t)mYmRegStream[1]) << 16;
		sampleBytes |= ((uint32_t)mYmRegStream[2]) << 8;
		sampleBytes |= ((uint32_t)mYmRegStream[3]);
		mYmRegStream += 4;
		if (i < 16) {
			mChip.mDigiDrumPtr[i] = mYmRegStream;
			mChip.mDigiDrumLen[i] = sampleBytes;
			i++;
		}
		mYmRegStream += sampleBytes;
		numDigiDrums--;
	}

	mMetaData.SetTitle((char*)mYmRegStream);
	while (*mYmRegStream++);		// Skip song name
	mMetaData.SetAuthor((char*)mYmRegStream);
	while (*mYmRegStream++);		// Skip author name
	mMetaData.SetComment((char*)mYmRegStream);
	while (*mYmRegStream++);		// Skip song comment

	mChip.mEG.mEnvTable  = (uint16_t*)YmChip::YM2149_ENVE_TB;
	mFrameCycles = 2000000/50;
	mChip.mEG.mMaxCycle = 31;

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,82>[3];

	if (mBlipBuf->set_sample_rate(44100)) {
		NativeLog(0, "YmPlayer", "Failed to set blipbuffer sample rate");
		return MusicPlayer::ERROR_UNKNOWN;
	}
	mBlipBuf->clock_rate(2000000);

	if (mYmData[0x17] == 0x1B) {
		// This is probably a ZX Spectrum tune with an AY clock of 1773400 Hz
		// (0x001B0F58 big-endian). Adjust the emulation speed accordingly.
		mChip.mEG.mMaxCycle = 15;
		mChip.mEG.mEnvTable = (uint16_t*)YmChip::YM2149_VOL_TB;
		mBlipBuf->clock_rate(1773400);
		mFrameCycles = 1773400/50;
	}

	mChip.Reset();

	mFrame = 0;
	mNumFrames = (uint32_t)mYmData[0x0E] << 8;
	mNumFrames += mYmData[0x0F];


    // Setup waves
	for (i = 0; i < 3; i++) {
		mSynth[i].volume(0.30);
		mSynth[i].output(mBlipBuf);
	}

	mDataOffs = mFrame;
	for (i = 0; i < 16; i++) {
		mYmRegs[i] = mYmRegStream[mDataOffs];
		mDataOffs += mNumFrames;
	}

	mCycleCount = 0;

	mState = MusicPlayer::STATE_PREPARED;

	return 0;
}


void YmPlayer::PresentBuffer(int16_t *out, Blip_Buffer *in)
{
	int count = in->samples_avail();

	in->read_samples(out, count, 1);

	// Copy each left channel sample to the right channel
	for (int i = 0; i < count*2; i += 2) {
		out[i+1] = out[i];
	}
}


int YmPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return -1;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	//NativeLog(0, "YmPlayer", "Run(%d, %p) -> %d clocks", numSamples, buffer, blipLen);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			mChip.Write(mYmRegs);
		}

		mChip.Step();

		for (i = 0; i < 3; i++) {
			out = (mChip.mChannels[i].mPhase | mChip.mChannels[i].mToneOff) &
				  (mChip.mNoise.mOut         | mChip.mChannels[i].mNoiseOff);
			out = (-out) & *(mChip.mChannels[i].mCurVol);

			if (out != mChip.mChannels[i].mOut) {
				mSynth[i].update(k, out);
				mChip.mChannels[i].mOut = out;
			}
		}

		mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mFrame++;
			if (mFrame >= mNumFrames) mFrame = 0;
			mDataOffs = mFrame;
			for (i = 0; i < 16; i++) {
				mYmRegs[i] = mYmRegStream[mDataOffs];
				mDataOffs += mNumFrames;
			}
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

	return 0;
}

