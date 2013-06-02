/*
 * NsfMapper.h
 *
 *  Created on: Jun 1, 2013
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

#ifndef NSFMAPPER_H_
#define NSFMAPPER_H_

#include <stdint.h>

class NsfMapper
{
public:
	void Reset();
	uint8_t ReadByte(uint16_t addr);
	void WriteByte(uint16_t addr, uint8_t data);

private:
	void WriteByte_0000(uint16_t addr, uint8_t data);
	void WriteByte_5000(uint16_t addr, uint8_t data);

	typedef void (NsfMapper::*WriteByteFunc)(uint16_t, uint8_t);
	WriteByteFunc mWriteByteFunc[0x10];
	uint8_t *mRomTbl[8];
	uint8_t *mCart;
};

#endif /* NSFMAPPER_H_ */
