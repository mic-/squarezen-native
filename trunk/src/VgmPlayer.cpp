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
#include <string.h>
#include "VgmPlayer.h"


//#define LOG_PCM 1


VgmPlayer::VgmPlayer() :
	mWait(0), mSN76489(NULL)
{
}

int VgmPlayer::Reset()
{
	//AppLog("VgmPlayer::Reset");

	if (mSN76489) delete mSN76489;
	mSN76489 = NULL;
	if (mBlipBuf) delete mBlipBuf;
	mBlipBuf = NULL;
	if (mSynth) delete [] mSynth;
	mSynth = NULL;

	mState = MusicPlayer::STATE_CREATED;

	return 0;
}


int VgmPlayer::Prepare(std::wstring fileName)
{
	uint32_t  i;
    size_t fileSize, readBytes;
    int retVal = 0;

    if (MusicPlayer::STATE_CREATED != GetState()) {
    	Reset();
    }

    std::ifstream musicFile(std::string(fileName.begin(), fileName.end()).c_str(),
    		std::ios::in | std::ios::binary);
    if (!musicFile) {
    	// AppLog("Failed to open file %S", fileName.c_str());
    	return -1;
    }
    musicFile.seekg(0, musicFile.end);
    fileSize = musicFile.tellg();
    musicFile.seekg(0, musicFile.beg);

#ifdef LOG_PCM
    pcmFile = fopen("/sdcard/log.pcm", "wb");
    loggedBuffers = 0;
#endif

    //AppLog("Trying to allocate %d bytes", fileSize);

    char *buf = new char[fileSize];
	musicFile.read(buf, fileSize);
	if (!musicFile) {
		//AppLog("Read failed");
		musicFile.close();
		return -1;
	}

	musicFile.close();

	//Tizen::Base::ByteBuffer bbuf;
	//bbuf.Construct(buf, 0, fileSize, fileSize);
#if 0
	// TODO: handle zipped VGMs
	Tizen::Base::ByteBuffer *inflatedBuf = Tizen::Base::Utility::Inflator::InflateN(bbuf);
	if (null == inflatedBuf) {
        // Error case handling...
		return E_FAILURE;
	}
    AppLog("Inflated size: %d bytes", inflatedBuf->GetRemaining());
	mVgmData.Clear();
	mVgmData.CopyFrom(*inflatedBuf);
#else
	mVgmData = new uint8_t[fileSize];
	memcpy(mVgmData, buf, fileSize);
#endif

	mDataPos = 0x40;
	mDataLen = fileSize;

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,82>[4];

	if (mBlipBuf->set_sample_rate(44100)) {
    	//AppLog("Failed to set blipbuffer sample rate");
		return -1;
	}
	mBlipBuf->clock_rate(3579545);

	mSampleCycles = 3579545 / 44100;
	mCycleCount = 0;

    // Setup waves
	for (i = 0; i < 4; i++) {
		mSynth[i].volume(0.22);
		mSynth[i].output(mBlipBuf);
	}

	mSN76489 = new SnChip();
	mSN76489->Reset();

	//AppLog("Prepare done");
	mState = MusicPlayer::STATE_PREPARED;

	return 0;
}


uint8_t VgmPlayer::GetData()
{
	uint8_t c = 0;

	if (mDataPos < mDataLen) {
		c = mVgmData[mDataPos++];
		if (mDataPos >= mDataLen)
			mDataPos = 0; // TODO: loop?
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

	//AppLog("Step got %#x from VGM file", c);

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
		mDataPos = 0x40;
		break;
	case 0x70: case 0x71: case 0x72: case 0x73:
	case 0x74: case 0x75: case 0x76: case 0x77:
	case 0x78: case 0x79: case 0x7A: case 0x7B:
	case 0x7C: case 0x7D: case 0x7E: case 0x7F:
		mWait = (c & 0x0F) + 1;
		break;
	}
}


void VgmPlayer::PresentBuffer(int16_t *out, Blip_Buffer *in)
{
	int count = in->samples_avail();

	in->read_samples(out, count, 1);

	// Copy each left channel sample to the right channel
	for (int i = 0; i < count*2; i += 2) {
		out[i+1] = out[i];
	}
}


int VgmPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t *writebuf;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return -1;
    }

    if (!mTempBuffer) {
        mTempBuffer = new int16_t[numSamples];
    }
    writebuf = mTempBuffer;


	int blipLen = mBlipBuf->count_clocks(numSamples);

	//AppLog("Run(%d, %p) -> %d clocks", numSamples, buffer, blipLen);

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

	return 0;
}
