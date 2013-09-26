/*
 * HuC6280Mapper.h
 *
 *  Created on: Sep 11, 2013
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

#ifndef HUC6280MAPPER_H_
#define HUC6280MAPPER_H_

#include <stdint.h>
#include "MemoryMapper.h"

class HuC6280Mapper : public MemoryMapper
{
public:
	virtual void SetMpr(uint8_t mprNum, uint8_t val) { mMPR[mprNum & 7] = val; }

	enum {
		MPR_RAM_PAGE = 0xF8,
		MPR_IO_PAGE = 0xFF,
	};

	uint8_t mMPR[8];
};

#endif	/* HUC6280MAPPER_H_ */
