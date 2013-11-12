#include "Pokey.h"

void Pokey::Write(uint32_t addr, uint8_t data)
{
	if (addr < 0xD200 || addr > 0xD2FF)
		return;

	addr &= 0x0F;

	switch (addr) {
	case Pokey::R_AUDF1:
	case Pokey::R_AUDF2:
	case Pokey::R_AUDF3:
	case Pokey::R_AUDF4:
		break;

	case Pokey::R_AUDC1:
	case Pokey::R_AUDC2:
	case Pokey::R_AUDC3:
	case Pokey::R_AUDC4:
		break;

	case Pokey::R_AUDCTL:
		break;
	}
}
