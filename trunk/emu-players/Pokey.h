/*
 * Pokey.h
 *
 *  Created on: Nov 12, 2013
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

#ifndef POKEY_H_
#define POKEY_H_

#include "Oscillator.h"

class PokeyChannel : public Oscillator
{

};

class Pokey
{
public:
	void Reset();
	void Step();
	void Write(uint8_t addr, uint8_t val);
};

#endif /* POKEY_H_ */

