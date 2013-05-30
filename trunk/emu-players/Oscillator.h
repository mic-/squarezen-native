/*
 * Oscillator.h
 *
 *  Created on: May 23, 2013
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

#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include <stdint.h>

class Oscillator
{
public:
	virtual ~Oscillator() {}

	virtual void Reset() = 0;
	virtual void Step() = 0;
	virtual void Write(uint32_t addr, uint8_t val) {}
	virtual void Write(uint8_t val) {}
	virtual void Write(uint8_t *regs) {}

	uint32_t mPos;
	uint32_t mPeriod;
	uint32_t mStep;
};


#endif /* OSCILLATOR_H_ */
