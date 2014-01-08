/*
 * HesMapper.h
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

#ifndef HESMAPPER_H_
#define HESMAPPER_H_

#include <stdint.h>
#include "HuC6280Mapper.h"
#include "HuC6280.h"

class HesPlayer;


class HesMapper : public HuC6280Mapper
{
public:
	explicit HesMapper(uint32_t numRomPages);
	virtual ~HesMapper();

	uint8_t *GetRomPointer() const { return mCart; }

	void SetCpu(HuC6280 *cpu) { m6280 = cpu; }
	void SetPsg(HuC6280Psg *psg) { mPsg = psg; }
	void SetPlayer(HesPlayer *player) { mPlayer = player; }

	virtual void Reset();
	virtual uint8_t ReadByte(uint32_t addr);
	virtual void WriteByte(uint32_t addr, uint8_t data);

	virtual void Irq(uint8_t irqSource);

private:
	HuC6280 *m6280;
	HuC6280Psg *mPsg;
	HesPlayer *mPlayer;

	uint8_t *mCart;
	uint8_t *mRam;
	uint32_t mNumRomPages;
};

#endif	/* HESMAPPER_H_ */
