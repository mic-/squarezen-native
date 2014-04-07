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

#include <iostream>
#include <fstream>
#include <stddef.h>
#include <string.h>
#include "NativeLogger.h"
#include "YmPlayer.h"
#ifdef __TIZEN__
#include "../lha/lha_decoder.h"
#endif
#ifdef __ANDROID__
#include "../tizen/serviceapp/lha/lha_decoder.h"
#endif

#ifdef LOG_PCM
FILE *pcmFile;
int loggedBuffers;
#endif

static const char LH5_DECODER_NAME[] = "-lh5-";


YmPlayer::YmPlayer()
	: mYmData(NULL)
{
}

MusicPlayer *YmPlayerFactory()
{
	return new YmPlayer;
}

MusicPlayer::Result YmPlayer::Reset()
{
	NLOGV("YmPlayer", "YmPlayer::Reset");

	mYmData = nullptr;

	delete mBlipBuf;
	delete [] mSynth;
	mBlipBuf = NULL;
	mSynth = NULL;

	mState = MusicPlayer::STATE_CREATED;

	return MusicPlayer::OK;
}


size_t lhDecodeCallback(void *buf, size_t buf_len, void *user_data)
{
	YmPlayer *ymPlayer = (YmPlayer*)user_data;
	int bytesToCopy = ymPlayer->mLhDataAvail - ymPlayer->mLhDataPos;
	if (bytesToCopy < 0) return 0;
	bytesToCopy = (bytesToCopy > buf_len) ? buf_len : bytesToCopy;

	memcpy(buf, &(ymPlayer->mYmData.get()[ymPlayer->mLhDataPos]), bytesToCopy);
	ymPlayer->mLhDataPos += bytesToCopy;
	return bytesToCopy;
}


MusicPlayer::Result YmPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
	uint32_t sampleBytes;
    size_t fileSize;
    uint16_t numDigiDrums;

    NLOGV("YmPlayer", "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    if (fileSize < 7) {
    	musicFile.close();
    	NLOGE("YmPlayer", "File is too small (%d bytes)", fileSize);
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

#ifdef LOG_PCM
    pcmFile = fopen("/sdcard/log.pcm", "wb");
    loggedBuffers = 0;
#endif

  	mYmData = std::shared_ptr<uint8_t>(new uint8_t[fileSize]);
    if (mYmData == nullptr) {
    	NLOGE("YmPlayer", "Failed to allocate memory");
		return MusicPlayer::ERROR_OUT_OF_MEMORY;
    }

	NLOGV("YmPlayer","Allocation ok");

	musicFile.read(reinterpret_cast<char*>(mYmData.get()), fileSize);
	if (!musicFile) {
		NLOGE("YmPlayer", "Failed to read data from file");
		musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}
	musicFile.close();

	if (strncmp((char*)&(mYmData.get()[2]), "-lh5", 4) == 0) {
		NLOGD("YmPlayer", "File is compressed; decoding..");
		LHADecoderType *decoderType;
		LHADecoder *decoder;
		//mLhHeader = (LhFileHeader*)mYmData;
		LhFileHeader *lhHeader = reinterpret_cast<LhFileHeader*>(mYmData.get());
		mLhDataPos = 24 + lhHeader->filenameLength;
		mLhDataAvail = fileSize;
		NLOGD("YmPlayer", "Decompressed size: %d bytes", lhHeader->uncompressedSize);
		if (!(decoderType = lha_decoder_for_name(const_cast<char*>(LH5_DECODER_NAME)))) {
			NLOGE("YmPlayer", "Failed to get decoder type for \"-lh5-\"");
			return MusicPlayer::ERROR_DECOMPRESSION;
		}
		if (!(decoder = lha_decoder_new(decoderType, lhDecodeCallback, this, lhHeader->uncompressedSize))) {
			NLOGE("YmPlayer", "Failed to get decoder for \"-lh5-\"");
			return MusicPlayer::ERROR_DECOMPRESSION;
		}
		std::shared_ptr<uint8_t> decoded = std::shared_ptr<uint8_t>(new uint8_t[lhHeader->uncompressedSize]);
		uint8_t* p = decoded.get();
		while (lha_decoder_read(decoder, p, 4096) == 4096) {
			p += 4096;
		}
		NLOGD("YmPlayer", "Number of bytes decoded: %d", lha_decoder_get_length(decoder));
		lha_decoder_free(decoder);
		mYmData = decoded;
	}

	numDigiDrums = ((uint16_t)mYmData.get()[0x14]) << 8;
	numDigiDrums |= mYmData.get()[0x15];

	mYmRegStream = (uint8_t*)mYmData.get() + 0x22;

	i = 0;
	NLOGD("YmPlayer", "Found %d digidrums", numDigiDrums);
	while (numDigiDrums) {
		sampleBytes  = ((uint32_t)mYmRegStream[0]) << 24;
		sampleBytes |= ((uint32_t)mYmRegStream[1]) << 16;
		sampleBytes |= ((uint32_t)mYmRegStream[2]) << 8;
		sampleBytes |= ((uint32_t)mYmRegStream[3]);
		mYmRegStream += 4;
		if (i < 256) {
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

	if (mYmData.get()[0x02] == '5') {
		mChip.mSfx[0].SetYmFormat(YmSoundFX::YM_FORMAT_5);
		mChip.mSfx[1].SetYmFormat(YmSoundFX::YM_FORMAT_5);
	} else if (mYmData.get()[0x02] == '6') {
		mChip.mSfx[0].SetYmFormat(YmSoundFX::YM_FORMAT_6);
		mChip.mSfx[1].SetYmFormat(YmSoundFX::YM_FORMAT_6);
	} else {
		mChip.mSfx[0].SetYmFormat(YmSoundFX::YM_FORMAT_UNKNOWN);
		mChip.mSfx[1].SetYmFormat(YmSoundFX::YM_FORMAT_UNKNOWN);
	}

	mChip.mEG.mEnvTable  = (uint16_t*)YmChip::YM2149_ENVE_TB;
	mFrameCycles = ATARI_ST_CLOCK / 50;
	mChip.mEG.mMaxCycle = YmChip::YM2149_ENVELOPE_STEPS - 1;

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,4096>[3];

	if (mBlipBuf->set_sample_rate(44100)) {
		NLOGE("YmPlayer", "Failed to set blipbuffer sample rate");
		return MusicPlayer::ERROR_UNKNOWN;
	}
	mBlipBuf->clock_rate(ATARI_ST_CLOCK);

	if (mYmData.get()[0x17] == 0x1B) {
		// This is probably a ZX Spectrum tune with an AY clock of 1773400 Hz
		// (0x001B0F58 big-endian). Adjust the emulation speed accordingly.
		mChip.mEG.mMaxCycle = YmChip::AY_3_8910_ENVELOPE_STEPS - 1;
		mChip.mEG.mEnvTable = (uint16_t*)YmChip::YM2149_VOL_TB;
		mBlipBuf->clock_rate(ZX_SPECTRUM_CLOCK);
		mFrameCycles = ZX_SPECTRUM_CLOCK / 50;
	}

	mChip.Reset();

	if (mYmData.get()[0x13] & 0x04) {
		mChip.mDigiDrumFormat = YmChip::DIGI_DRUM_U4;
	} else if (mYmData.get()[0x13] & 0x02) {
		mChip.mDigiDrumFormat = YmChip::DIGI_DRUM_S8;
	} else {
		mChip.mDigiDrumFormat = YmChip::DIGI_DRUM_U8;
	}

	mFrame = 0;
	mNumFrames = (uint32_t)(mYmData.get()[0x0E]) << 8;
	mNumFrames += mYmData.get()[0x0F];

	mMetaData.SetLengthMs(mNumFrames * 20);

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

	return MusicPlayer::OK;
}


MusicPlayer::Result YmPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	//NLOGV("YmPlayer", "Run(%d, %p) -> %d clocks", numSamples, buffer, blipLen);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			mChip.Write(mYmRegs);
		}

		mChip.Step();

		for (i = 0; i < 3; i++) {
			if ((mChip.mChannels[i].mSfxActive & YmSoundFX::SFX_DIGI_DRUM) == 0) {
				out = (mChip.mChannels[i].mPhase | mChip.mChannels[i].mToneOff) &
					  (mChip.mNoise.mOut         | mChip.mChannels[i].mNoiseOff);
				out = (-out) & *(mChip.mChannels[i].mCurVol);
			} else {
				out = *(mChip.mChannels[i].mCurVol);
			}

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

	return MusicPlayer::OK;
}


void YmPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}

