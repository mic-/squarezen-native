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

#include <FBase.h>
#include "NsfPlayer.h"

NsfPlayer::NsfPlayer()
{
	// TODO: fill out
}


int NsfPlayer::Reset()
{
	// TODO: fill out
	mState = MusicPlayer::STATE_CREATED;
	return 0;
}

int NsfPlayer::Prepare(std::wstring fileName)
{
	// TODO: fill out

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,82>[4];

	if (mBlipBuf->set_sample_rate(44100)) {
    	//AppLog("Failed to set blipbuffer sample rate");
		return -1;
	}
	if (mFileHeader.region & NsfPlayer::REGION_PAL) {
		mBlipBuf->clock_rate(1662607);
		mFrameCycles = 1662607 / 200;
	} else {
		mBlipBuf->clock_rate(1789773);
		mFrameCycles = 1789773 / 240;
	}

	mCycleCount = 0;
	mCurFrame = 0;
	mPlayCounter = 0;

    // Setup waves
	for (int i = 0; i < 4; i++) {
		mSynth[i].volume(0.22);
		mSynth[i].output(mBlipBuf);
	}

	AppLog("Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;

	return 0;
}


int NsfPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t i, k;
    int16_t out;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return -1;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	//AppLog("Run(%d, %p) -> %d clocks", numSamples, buffer, blipLen);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			if (mCurFrame < 4) {
				if ((mCurFrame & 1) == (m2A03->mMaxFrameCount & 1)) {
					// TODO: update length counter and sweep
				}
				// TODO: update envelopes
			}
			if (mCurFrame == 3 && m2A03->mMaxFrameCount == 3 && m2A03->mGenerateFrameIRQ) {
				// TODO: generate frame IRQ
			}
			mCurFrame++;
			if (mCurFrame > m2A03->mMaxFrameCount) {
				mCurFrame = 0;
			}
			if (mPlayCounter == 3) {
				// TODO: call NSF PLAY routine
			}
			mPlayCounter = (mPlayCounter + 1) & 3;
		}

		m2A03->Step();

		for (i = 0; i < 4; i++) {
			out = (-m2A03->mChannels[i].mPhase) & Emu2A03::VOL_TB[m2A03->mChannels[i].mVol & 0x0F];

			if (out != m2A03->mChannels[i].mOut) {
				mSynth[i].update(k, out);
				m2A03->mChannels[i].mOut = out;
			}
		}

		mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mCycleCount = 0;
		}
	}

	mBlipBuf->end_frame(blipLen);
	//PresentBuffer(buffer, mBlipBuf);

	return 0;
}

