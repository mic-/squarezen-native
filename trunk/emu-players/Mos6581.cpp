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

void Mos6581::Reset()
{
	// TODO: implement
}


void Mos6581::Step()
{
	// TODO: implement
}


void Mos6581::Write(uint32_t addr, uint8_t data)
{
	NLOGD("Mos6581", "Write(%#x, %#x)", addr, data);

	// TODO: implement
}
