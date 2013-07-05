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

#include "NativeLogger.h"
#include "Mos6581.h"

void Mos6581Channel::Reset()
{
	// TODO: implement
}

void Mos6581Channel::Step()
{
	// TODO: implement
}

void Mos6581Channel::Write(uint32_t addr, uint8_t val)
{
	// TODO: implement
}


void Mos6581::Reset()
{
	NLOGD("Mos6581", "Reset");
	// TODO: implement
}


void Mos6581::Step()
{
	// TODO: implement
}


void Mos6581::Write(uint32_t addr, uint8_t data)
{
	NLOGD("Mos6581", "Write(%#x, %#x)", addr, data);

	uint8_t reg = addr - Mos6581::REGISTER_BASE;

	if (reg >= Mos6581::R_VOICE1_FREQ_LO && reg <= Mos6581::R_VOICE1_SR) {
		mChannels[0].Write(addr, data);

	} else if (reg >= Mos6581::R_VOICE2_FREQ_LO && reg <= Mos6581::R_VOICE2_SR) {
		mChannels[1].Write(addr, data);

	} else if (reg >= Mos6581::R_VOICE3_FREQ_LO && reg <= Mos6581::R_VOICE3_SR) {
		mChannels[2].Write(addr, data);

	} else if (Mos6581::R_FILTER_FC_LO == reg) {

	} else if (Mos6581::R_FILTER_FC_HI == reg) {

	} else if (Mos6581::R_FILTER_RESFIL == reg) {

	} else if (Mos6581::R_FILTER_MODEVOL == reg) {

	}
}
