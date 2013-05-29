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

#ifndef GBCOMMON_H__
#define GBCOMMON_H__

#include <stdio.h>
#include <malloc.h>

extern unsigned char *cart;
extern int run,quit,romLoaded;
extern unsigned char cgbBgp[64],cgbObp[64];
extern unsigned short cgbBgpRgb[32],cgbObpRgb[32];
extern unsigned short dmgPalRgb[4];
extern char szBuffer[256];
extern int	run,quit;
extern unsigned long templ;
extern unsigned char keys,divcnt;
extern unsigned char numBanks;
extern int debg,tmcnt,tmrld,speedShift;

extern int load_rom(char*);
extern void reset_machine();
extern unsigned short color16(int r, int g, int b);

#endif

