/*
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

#ifndef GBZ80_H__
#define GBZ80_H__

typedef struct
{
	struct {
		unsigned char F,A,C,B,E,D,L,H;
		unsigned short SP,PC;
	} regs;
	unsigned int cycles;
	int halted,stopped;
} tag_cpu;

extern tag_cpu cpu;

extern int cpu_reset();
extern void cpu_execute(unsigned int);
extern void cpu_rst(unsigned short);

extern void cpu_close();

#endif
