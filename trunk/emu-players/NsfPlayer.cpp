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
#include <string.h>
#include "NativeLogger.h"
#include "NsfPlayer.h"


static float pulseTable[31];
static float tndTable[203];


NsfPlayer::NsfPlayer()
	: m6502(NULL)
	, m2A03(NULL)
	, mVrc6(NULL)
	, mSunsoft5B(NULL)
	, mMemory(NULL)
	, mSongIsBankswitched(false)
{
	for (int i = 0; i < 31; i++) {
		pulseTable[i] = 95.52f / (8128.0f / (float)i + 100.0);
	}
	for (int i = 0; i < 203; i++) {
		tndTable[i] = 163.67f / (24329.0f / (float)i + 100.0);
	}
}

MusicPlayer *NsfPlayerFactory()
{
	return new NsfPlayer;
}

NsfPlayer::~NsfPlayer()
{
	delete m6502;
	delete m2A03;
	delete mVrc6;
	delete mSunsoft5B;
	delete mMemory;

	m6502      = NULL;
	m2A03      = NULL;
	mVrc6      = NULL;
	mSunsoft5B = NULL;
	mMemory    = NULL;
}


MusicPlayer::Result NsfPlayer::Reset()
{
	NLOGV("NsfPlayer", "Reset");

	delete mBlipBuf;
	delete [] mSynth;
	mBlipBuf = NULL;
	mSynth   = NULL;

	delete m6502;
	delete m2A03;
	delete mVrc6;
	delete mSunsoft5B;
	delete mMemory;
	m6502      = NULL;
	m2A03      = NULL;
	mVrc6      = NULL;
	mSunsoft5B = NULL;
	mMemory    = NULL;

	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}


void NsfPlayer::Execute6502(uint16_t address)
{
	//NLOGD("NsfPlayer", "Execute6502(%#x)", address);

	// JSR loadAddress
	mMemory->WriteByte(0x4f80, 0x20);
	mMemory->WriteByte(0x4f81, address & 0xff);
	mMemory->WriteByte(0x4f82, address >> 8);
	// -: JMP -
	mMemory->WriteByte(0x4f83, 0x4c);
	mMemory->WriteByte(0x4f84, 0x83);
	mMemory->WriteByte(0x4f85, 0x4f);
	m6502->mRegs.PC = 0x4f80;
	m6502->mCycles = 0;
	m6502->Run(mFrameCycles);
}


MusicPlayer::Result NsfPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
    size_t fileSize;
    uint32_t numBanks;

	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    NLOGD("NsfPlayer", "Reading NSF header");
    musicFile.read((char*)&mFileHeader, sizeof(NsfFileHeader));
	if (!musicFile) {
		NLOGE("NsfPlayer", "Reading NSF header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    NLOGD("NsfPlayer", "ID: %c%c%c", mFileHeader.ID[0], mFileHeader.ID[1], mFileHeader.ID[2]);
    NLOGD("NsfPlayer", "Load: %#x\nInit: %#x\nPlay: %#x",
    		mFileHeader.loadAddress, mFileHeader.initAddress, mFileHeader.playAddress);
    if (strncmp(mFileHeader.ID, "NESM", 4)) {
    	NLOGE("NsfPlayer", "Bad NSF header signature");
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    mMetaData.SetTitle((char*)mFileHeader.title);
    mMetaData.SetAuthor((char*)mFileHeader.author);
    mMetaData.SetComment((char*)mFileHeader.copyright);

    //NLOGD("NsfPlayer", "Prepare(): title and author is %s, %s", mMetaData.GetTitle().c_str(), mMetaData.GetAuthor().c_str());

    mSongIsBankswitched = false;
    for (i = 0; i < 8; i++) {
    	if (mFileHeader.initialBanks[i]) {
    		mSongIsBankswitched = true;
    		break;
    	}
    }

	uint32_t numSynths = 2;

	m6502 = new Emu6502;
	m2A03 = new Emu2A03;
	if (mFileHeader.extraChips & NsfPlayer::USES_VRC6) {
		NLOGD("NsfPlayer", "This song uses the Konami VRC6");
		mVrc6 = new KonamiVrc6;
		mVrc6Synth = numSynths;
		numSynths += 2;		// One for the two square wave channels, one for the saw wave channel
	}
	if (mFileHeader.extraChips & NsfPlayer::USES_N163) {
		NLOGD("NsfPlayer", "This song uses the Namco163");
		mN163 = new Namco163;
		mN163Synth = numSynths;
		//numSynths += 8;
	}
	if (mFileHeader.extraChips & NsfPlayer::USES_SUNSOFT_5B) {
		NLOGD("NsfPlayer", "This song uses the Sunsoft-5B");
		mSunsoft5B = new Sunsoft5B(YmChip::YM2149_ENVELOPE_STEPS);
		mSunsoft5BSynth = numSynths;
		numSynths += 3;
	}

	uint32_t offset = mFileHeader.loadAddress & 0x0fff;
	if (!mSongIsBankswitched) {
		offset = mFileHeader.loadAddress - 0x8000;
	}

    numBanks = ((fileSize + offset - sizeof(NsfFileHeader)) + 0xfff) >> 12;
    NLOGD("NsfPlayer", "Trying to allocate %d bytes (file size = %d)", (uint32_t)numBanks << 12, fileSize);
	mMemory = new NsfMapper(numBanks);
	mMemory->SetApu(m2A03);
	mMemory->SetVrc6(mVrc6);
	mMemory->SetN163(mN163);
	mMemory->SetSunsoft5B(mSunsoft5B);

    NLOGD("NsfPlayer", "Loading to offset %#x (%#x..%#x)", offset, 0x8000+offset, 0x7fff+offset+fileSize-sizeof(NsfFileHeader));
	musicFile.read((char*)(mMemory->GetRomPointer()) + offset,
			       fileSize - sizeof(NsfFileHeader));
	if (!musicFile) {
		NLOGE("NsfPlayer", "Read failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	NLOGD("NsfPlayer", "File read done");

	musicFile.close();

	m6502->SetMapper(mMemory);
	m2A03->SetMapper(mMemory);
	mMemory->Reset();
	m6502->Reset();
	m2A03->Reset();
	if (mVrc6) mVrc6->Reset();
	if (mN163) mN163->Reset();
	if (mSunsoft5B) mSunsoft5B->Reset();

	mBlipBuf = new Blip_Buffer();
	mSynth = new Blip_Synth<blip_low_quality,4096>[numSynths];

	if (mBlipBuf->set_sample_rate(44100)) {
    	NLOGE("NsfPlayer", "Failed to set blipbuffer sample rate");
		return MusicPlayer::ERROR_UNKNOWN;
	}
	if (mFileHeader.region & NsfPlayer::REGION_PAL) {
		mBlipBuf->clock_rate(1662607);
		m2A03->SetClock(1662607, 50);
		mFrameCycles = 1662607 / 200;
	} else {
		mBlipBuf->clock_rate(1789773);
		m2A03->SetClock(1789773, 60);
		mFrameCycles = 1789773 / 240;
	}

	mCycleCount = 0;
	mPlayCounter = 0;

	float synthVolume = 0.95f / (float)numSynths;
    // Setup waves
	for (int i = 0; i < numSynths; i++) {
		mSynth[i].volume(synthVolume);
		mSynth[i].output(mBlipBuf);
	}

	mMetaData.SetNumSubSongs(mFileHeader.numSongs);
	mMetaData.SetDefaultSong(mFileHeader.firstSong);

	m6502->mRegs.S = 0xFF;
	SetSubSong(mFileHeader.firstSong - 1);

	NLOGD("NsfPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;

	return MusicPlayer::OK;
}


void NsfPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD("NsfPlayer", "SetSubSong(%d)", subSong);

	mMemory->Reset();

	// Initialize the APU registers
	for (int i = 0x4000; i < 0x4010; i++) {
		m2A03->Write(i, 0);
	}
	m2A03->Write(0x4010, 0x10);
	m2A03->Write(0x4011, 0x0);
	m2A03->Write(0x4012, 0x0);
	m2A03->Write(0x4013, 0x0);
	m2A03->Write(0x4015, 0x0f);

	// Set up ROM mapping
	for (int i = 0; i < 8; i++) {
		if (mSongIsBankswitched) {
			mMemory->WriteByte(0x5FF8 + i, mFileHeader.initialBanks[i]);
		} else {
			mMemory->WriteByte(0x5FF8 + i, i);
		}
	}

	m6502->mRegs.A = subSong;
	m6502->mRegs.X = 0;  // NTSC/PAL
	Execute6502(mFileHeader.initAddress);
}


/*void NsfPlayer::PresentBuffer(int16_t *out, Blip_Buffer *in)
{
	int count = in->samples_avail();

	in->read_samples(out, count, 1);

	// Copy each left channel sample to the right channel
	for (int i = 0; i < count*2; i += 2) {
		out[i+1] = out[i];
	}
}*/


MusicPlayer::Result NsfPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int32_t k;
    int16_t out, pulseOut, tndOut;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	int blipLen = mBlipBuf->count_clocks(numSamples);

	//NLOGV("NsfPlayer", "Run(%d, %p) -> %d clocks", numSamples, buffer, blipLen);

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			if (mPlayCounter == 3) {
				Execute6502(mFileHeader.playAddress);
			}
			mPlayCounter = (mPlayCounter + 1) & 3;
		}

		m2A03->Step();

		pulseOut = (-m2A03->mChannels[Emu2A03::CHN_PULSE1].mPhase)
					& (m2A03->mChannels[Emu2A03::CHN_PULSE1].mVol & 0x0F)
					& m2A03->mChannels[Emu2A03::CHN_PULSE1].mOutputMask;
		pulseOut += (-m2A03->mChannels[Emu2A03::CHN_PULSE2].mPhase)
					& (m2A03->mChannels[Emu2A03::CHN_PULSE2].mVol & 0x0F)
					& m2A03->mChannels[Emu2A03::CHN_PULSE2].mOutputMask;
		pulseOut = (uint16_t)(pulseTable[pulseOut] * 6460.0f); //5460.0f);

		tndOut = ((-m2A03->mChannels[Emu2A03::CHN_TRIANGLE].mPhase)
					& (m2A03->mChannels[Emu2A03::CHN_TRIANGLE].mVol & 0x0F)
					& m2A03->mChannels[Emu2A03::CHN_TRIANGLE].mOutputMask) * 3;
		tndOut += ((-m2A03->mChannels[Emu2A03::CHN_NOISE].mPhase)
					& (m2A03->mChannels[Emu2A03::CHN_NOISE].mVol & 0x0F)
					& m2A03->mChannels[Emu2A03::CHN_NOISE].mOutputMask) * 2;
		tndOut += m2A03->mChannels[Emu2A03::CHN_DMC].mDacOut; /*((m2A03->mChannels[Emu2A03::CHN_DMC].mDuty)
					& m2A03->mChannels[Emu2A03::CHN_DMC].mOutputMask);*/
		tndOut = (uint16_t)(tndTable[tndOut] * 6460.0f); //5460.0f);

		if (pulseOut != m2A03->mChannels[0].mOut) {
			mSynth[0].update(k, pulseOut);
			m2A03->mChannels[0].mOut = pulseOut;
		}
		if (tndOut != m2A03->mChannels[1].mOut) {
			mSynth[1].update(k, tndOut);
			m2A03->mChannels[1].mOut = tndOut;
		}

		if (mVrc6) {
			mVrc6->Step();
			out = mVrc6->mChannels[KonamiVrc6::CHN_PULSE1].mPhase * mVrc6->mChannels[KonamiVrc6::CHN_PULSE1].mVol;
			out += mVrc6->mChannels[KonamiVrc6::CHN_PULSE2].mPhase * mVrc6->mChannels[KonamiVrc6::CHN_PULSE2].mVol;
			out = (uint16_t)(pulseTable[out] * 6460.0f);
			if (out != mVrc6->mChannels[KonamiVrc6::CHN_PULSE1].mOut) {
				mSynth[mVrc6Synth].update(k, out);
				mVrc6->mChannels[KonamiVrc6::CHN_PULSE1].mOut = out;
			}
			out = mVrc6->mChannels[KonamiVrc6::CHN_SAW].mPhase * mVrc6->mChannels[KonamiVrc6::CHN_SAW].mVol;
			out = (out > 30) ? 30 : out;
			out = (uint16_t)(pulseTable[out] * 6460.0f);
			if (out != mVrc6->mChannels[KonamiVrc6::CHN_SAW].mOut) {
				mSynth[mVrc6Synth + 1].update(k, out);
				mVrc6->mChannels[KonamiVrc6::CHN_SAW].mOut = out;
			}
		}

		if (mN163) {
			// ToDo: output Namco163 channels to blip synths
		}

		if (mSunsoft5B) {
			mSunsoft5B->Step();
			for (int i = 0; i < 3; i++) {
				out = (mSunsoft5B->mChannels[i].mPhase | mSunsoft5B->mChannels[i].mToneOff) &
					  (mSunsoft5B->mNoise.mOut         | mSunsoft5B->mChannels[i].mNoiseOff);
				out = (-out) & *(mSunsoft5B->mChannels[i].mCurVol);

				if (out != mSunsoft5B->mChannels[i].mOut) {
					mSynth[mSunsoft5BSynth + i].update(k, out);
					mSunsoft5B->mChannels[i].mOut = out;
				}
			}
		}

		mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mCycleCount = 0;
		}
	}

	mBlipBuf->end_frame(blipLen);
	PresentBuffer(buffer, mBlipBuf);

	return MusicPlayer::OK;
}


size_t NsfPlayer::GetNumChannels() const
{
	size_t numChannels = 2;
	if (mVrc6) numChannels += 3;
	if (mN163) numChannels += 8;
	if (mSunsoft5B) numChannels += 3;
	return numChannels;
}

void NsfPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}

