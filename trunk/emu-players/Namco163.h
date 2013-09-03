/*
 * Namco163.h
 *
 *  Created on: Sep 3, 2013
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

#ifndef NAMCO163_H_
#define NAMCO163_H_

#include <stdint.h>
#include "Oscillator.h"

class Namco163
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t data);
};

#endif 	/* NAMCO163_H_ */

