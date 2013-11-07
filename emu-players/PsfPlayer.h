/*
 * PsfPlayer.h
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

#ifndef PSFPLAYER_H_
#define PSFPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"

class PsfPlayer : public MusicPlayer
{
	PsfPlayer();
	virtual ~PsfPlayer();

	typedef struct __attribute__ ((__packed__))
	{
		char signature[3];					// "PSF"
		uint8_t version;
		uint32_t reservedAreaSize;			// little-endian
		uint32_t compressedProgramSize;		// little-endian
		uint32_t compressedProgramCrc32;	// little-endian
	} PsfFileHeader;

	enum
	{
		VERSION_PSF1 = 0x01,	// PS1
		VERSION_PSF2 = 0x02,	// PS2
		VERSION_SSF  = 0x11,	// Sega Saturn
		VERSION_DSF  = 0x12,	// Sega Dreamcast
		VERSION_GENESIS = 0x13,
		VERSION_USF  = 0x21,	// Nintendo 64
		VERSION_GSF  = 0x22,	// Gameboy Advance
		VERSION_SNSF = 0x23,	// Super NES
		VERSION_QSF  = 0x41,	// Campcom QSound
	};
};

#endif /* PSFPLAYER_H_ */

