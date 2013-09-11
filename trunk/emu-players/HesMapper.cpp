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

#include <string.h>
#include <stddef.h>
#include "NativeLogger.h"
#include "HesMapper.h"
#include "HuC6280.h"


HesMapper::HesMapper(uint32_t numRomBanks)
{
	// ToDo: implement
}

HesMapper::~HesMapper()
{
	// ToDo: implement
}

void HesMapper::Reset()
{
	// ToDo: implement
}

uint8_t HesMapper::ReadByte(uint16_t addr)
{
	uint32_t page = (addr >> 13) & 7;
	uint32_t mpr = MPR[page];
	uint32_t offset = addr & 0x1FFF;

	if (mpr < 0x80) {
		// ToDo: handle ROM reads
	} else if (HuC6280Mapper::MPR_RAM_PAGE == mpr) {
		// ToDo: handle RAM reads
	} else if (HuC6280Mapper::MPR_IO_PAGE == mpr) {
		// ToDo: handle IO reads
		if (HuC6280::R_TIMER_COUNT == offset) {
		}
	}

	return 0;
}

void HesMapper::WriteByte(uint16_t addr, uint8_t data)
{
	uint32_t page = (addr >> 13) & 7;
	uint32_t offset = addr & 0x1FFF;

	switch (MPR[page]) {
	case HuC6280Mapper::MPR_RAM_PAGE:
		// ToDo: handle
		break;
	case HuC6280Mapper::MPR_IO_PAGE:
		if (offset >= HuC6280Psg::R_CHN_SELECT && offset <= HuC6280Psg::R_LFO_CTRL) {
			mPsg->Write(offset, data);
		} else if (HuC6280::R_TIMER_COUNT == offset) {

		} else if (HuC6280::R_TIMER_CTRL == offset) {

		}
		break;
	}
}

