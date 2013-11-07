/*
 * CpuBase.h
 *
 *  Created on: Nov 7, 2013
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

#ifndef CPUBASE_H_
#define CPUBASE_H_

#include <stdint.h>

class MemoryMapper;

class CpuBase
{
public:

	CpuBase();
	CpuBase(MemoryMapper *mapper);

	virtual ~CpuBase() {}

	virtual void Reset() = 0;
	virtual void Run(uint32_t maxCycles) = 0;
	virtual void SetMapper(MemoryMapper *mapper) { mMemory = mapper; }

	MemoryMapper *mMemory;
};


#endif /* CPUBASE_H_ */
