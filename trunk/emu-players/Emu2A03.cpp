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

#include "Emu2A03.h"

const uint16_t Emu2A03::VOL_TB[] = {
	0,5,10,15,
	20,25,30,35,
	40,45,50,55,
	60,65,70,75
};


void Emu2A03Channel::Reset()
{
	// TODO: fill out
}


void Emu2A03Channel::Step()
{
	// TODO: fill out
}

void Emu2A03Channel::Write(uint32_t addr, uint8_t val)
{
	// TODO: fill out
}


void Emu2A03::Step()
{
	for (int i = 0; i < 4; i++) {
		mChannels[i].Step();
	}
}

