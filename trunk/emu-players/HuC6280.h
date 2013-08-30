/*
 * HuC6280.h
 *
 *  Created on: Aug 30, 2013
 *
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

#ifndef HUC62802_H_
#define HUC62802_H_

#include <stdint.h>
#include "MemoryMapper.h"
#include "Oscillator.h"


class HuC6280PsgChannel : public Oscillator
{
public:
	virtual ~HuC6280PsgChannel() {}

	virtual void Reset();
	virtual void Step();
};


class HuC6280Psg
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);

	HuC6280PsgChannel mChannels[6];
};


class HuC6280
{
public:
	void Reset();
	void Run(uint32_t maxCycles);
};

#endif

