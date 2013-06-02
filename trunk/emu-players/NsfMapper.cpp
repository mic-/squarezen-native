#include "NsfMapper.h"

uint8_t NsfMapper::ReadByte(uint16_t addr)
{
	// TODO: fill out
	return 0;
}


void NsfMapper::WriteByte_0000(uint16_t addr, uint8_t data)
{
}

void NsfMapper::WriteByte_5000(uint16_t addr, uint8_t data)
{
	if (addr >= 0x5FF8 && addr <= 0x5FFF) {
		mRomTbl[addr - 0x5FF8] = mCart + data * 0x1000;
	}
}


void NsfMapper::WriteByte(uint16_t addr, uint8_t data)
{
	(this->*mWriteByteFunc[addr >> 12])(addr, data);
}


void NsfMapper::Reset()
{
	mWriteByteFunc[0x00] = &NsfMapper::WriteByte_0000;
	mWriteByteFunc[0x05] = &NsfMapper::WriteByte_5000;
}
