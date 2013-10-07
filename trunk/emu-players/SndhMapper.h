/*
 * SndhMapper.h
 *
 *  Created on: Oct 3, 2013
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

#ifndef SNDHMAPPER_H_
#define SNDHMAPPER_H_

#include <stdint.h>
#include "MemoryMapper.h"


class SndhMapper : public MemoryMapper
{
public:
	SndhMapper(uint32_t fileSize);
	virtual ~SndhMapper();

	uint8_t *GetFileImagePointer() const { return mFileImage; }

	virtual void Reset();
	virtual uint8_t ReadByte(uint32_t addr);
	virtual void WriteByte(uint32_t addr, uint8_t data);

private:
	uint8_t *mFileImage;
};

#endif /* SNDHMAPPER_H_ */

