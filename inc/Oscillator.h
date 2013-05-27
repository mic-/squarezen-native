/*
 * Oscillator.h
 *
 *  Created on: May 23, 2013
 *      Author: Mic
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
	virtual void Write(uint8_t addr, uint8_t val) {}
	virtual void Write(uint8_t val) {}
	virtual void Write(uint8_t *regs) {}

	uint32_t mPos;
	uint32_t mPeriod;
	uint32_t mStep;
};


#endif /* OSCILLATOR_H_ */
