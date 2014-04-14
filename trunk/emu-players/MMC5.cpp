/*
 * Copyright 2014 Mic
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

#define NLOG_LEVEL_DEBUG 0
#define NLOG_TAG "MMC5"

#include "NativeLogger.h"
#include "MMC5.h"


void MMC5::Reset()
{
	// ToDo: implement
}


void MMC5::Step()
{
	mChannels[MMC5::CHN_PULSE1].Step();
	mChannels[MMC5::CHN_PULSE2].Step();
}


void MMC5::Write(uint32_t addr, uint8_t data)
{
	uint32_t reg = addr - 0x5000;

	switch (reg) {
	// Pulse 1
	case MMC5::R_PULSE1_DUTY_ENVE:
		break;
	case MMC5::R_PULSE1_PERLO:
		break;
	case MMC5::R_PULSE1_PERHI_LEN:
		break;

	// Pulse 2
	case MMC5::R_PULSE2_DUTY_ENVE:
		break;
	case MMC5::R_PULSE2_PERLO:
		break;
	case MMC5::R_PULSE2_PERHI_LEN:
		break;

	}
}

