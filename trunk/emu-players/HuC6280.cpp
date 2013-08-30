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

#include <string>
#include <stdio.h>
#include "NativeLogger.h"
#include "HuC6280.h"


void HuC6280::Reset()
{
	// ToDo: implement
}


void HuC6280::Run(uint32_t maxCycles)
{
	// ToDo: implement
}


void HuC6280PsgChannel::Reset()
{
	// ToDo: implement
}

void HuC6280PsgChannel::Step()
{
	// ToDo: implement
}


void HuC6280Psg::Reset()
{
	// ToDo: implement
}


void HuC6280Psg::Step()
{
	// ToDo: implement
	for (int i = 0; i < 6; i++) {
		mChannels[i].Step();
	}
}


void HuC6280Psg::Write(uint32_t addr, uint8_t data)
{
	// ToDo: implement
}
