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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "Sunsoft5B.h"


const uint16_t Sunsoft5B::SUNSOFT5B_VOL_TB[] = {
	21, 30, 43, 60, 86, 121, 171, 242, 342, 483, 683, 965, 1363, 1925, 2718, 3840
};

const uint16_t Sunsoft5B::SUNSOFT5B_ENVE_TB[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 5, 7, 10, 15,
	21, 30, 43, 60, 86, 121, 171, 242, 342, 483, 683, 965, 1363, 1925, 2718, 3840
};


void Sunsoft5B::Reset()
{
	for (int i = 0; i < 3; i++) {
		mChannels[i].SetChip(this);
		mChannels[i].SetIndex(i);
		mChannels[i].Reset();
	}

	//mEG.SetChip(this);
	mEG.Reset();
	mNoise.Reset();
}


void Sunsoft5B::Step()
{
	mChannels[0].Step();
	mChannels[1].Step();
	mChannels[2].Step();
	mEG.Step();
	mNoise.Step();
}


void Sunsoft5B::Write(uint32_t addr, uint8_t val)
{
	mChannels[0].Write(addr, val);
	mChannels[1].Write(addr, val);
	mChannels[2].Write(addr, val);
	mEG.Write(addr, val);
	mNoise.Write(addr, val);
}


