/*
 * MemoryMapper.h
 *
 *  Created on: Jun 2, 2013
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

#ifndef MEMORYMAPPER_H_
#define MEMORYMAPPER_H_

#include <stdint.h>

class MemoryMapper
{
public:
	virtual ~MemoryMapper() {}

	virtual void Reset() = 0;
	virtual uint8_t ReadByte(uint16_t addr) = 0;
	virtual void WriteByte(uint16_t addr, uint8_t data) = 0;
};


#endif /* MEMORYMAPPER_H_ */
