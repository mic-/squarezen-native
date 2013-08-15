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

#define NLOG_LEVEL_VERBOSE 0

#ifdef __TIZEN__
#include <FBase.h>
#endif
#include "GbMemory.h"
#include "GbZ80.h"
#include "GbsPlayer.h"
#include "NativeLogger.h"

#define	FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10

#define H_BIT_SHIFT 2  //1


#define LOAD_IMM_8(r) r = mem_read_8(cpu.regs.PC + 1);\
	                      cpu.cycles += 8;\
						  cpu.regs.PC += 2

#define LOAD_IMM_16(r1,r2) r2 = mem_read_8(cpu.regs.PC + 1);\
						   r1 = mem_read_8(cpu.regs.PC + 2);\
						   cpu.cycles += 12;\
						   cpu.regs.PC += 3

#define MOVE_REG_REG_8(r1,r2) r1 = r2;\
	                          cpu.cycles += 4;\
							  cpu.regs.PC++

#define ADD_REG_REG_8(r1,r2) temp16 = (uint16_t)(r1) + (uint16_t)(r2);\
							 cpu.regs.F = 0;\
							 if ((uint8_t)temp16 == 0) cpu.regs.F |= FLAG_Z;\
							 if (temp16 >= 0x100) cpu.regs.F |= FLAG_C;\
							 cpu.regs.F |= ((temp8 & 0x0F) < (r1 & 0x0F)) ? FLAG_H : 0;\
							 r1 = (uint8_t)temp16; \
							 cpu.cycles += 4;\
							 cpu.regs.PC++

#define ADC_REG_REG_8(r1,r2) temp9 = ((cpu.regs.F & FLAG_C) >> 4);\
                             temp8 = r1 + r2 + temp9; \
	                         temp16 = (unsigned short)r1 + r2 + temp9; \
							 cpu.regs.F = 0;\
							 if (temp8 == 0) cpu.regs.F |= FLAG_Z;\
							 if (temp16 >= 0x100) cpu.regs.F |= FLAG_C;\
							 cpu.regs.F |= ((temp8 & 0x0F) < (r1 & 0x0F))?FLAG_H:0;\
							 cpu.regs.F |= (((temp8 & 0xF) == (r1 & 0xF))&&temp9)?FLAG_H:0;\
							 r1 = temp8; \
							 cpu.cycles += 4;\
							 cpu.regs.PC++

#define SUB_REG_REG_8(r1,r2) temp8 = r1 - r2;\
							 cpu.regs.F = FLAG_N;\
							 if (r1 == r2) cpu.regs.F |= FLAG_Z;\
							 if (r1 < r2) cpu.regs.F |= FLAG_C;\
							 cpu.regs.F |= ((temp8&0x0F)>(r1&0x0F))?FLAG_H:0;\
							 r1 = temp8;\
							 cpu.cycles += 4;\
							 cpu.regs.PC++

#define SBC_REG_REG_8(r1,r2) temp8 = (r2 + ((cpu.regs.F & FLAG_C) >> 4));\
							 cpu.regs.F = FLAG_N;\
							 if (r1 == temp8) cpu.regs.F |= FLAG_Z;\
							 if (r1 < temp8) cpu.regs.F |= FLAG_C;\
							 cpu.regs.F |= ((temp8 & 0x0F) > (r1 & 0x0F))?FLAG_H:0;\
							 r1 -= temp8;\
							 cpu.cycles += 4;\
							 cpu.regs.PC++

#define AND_REG_REG_8(r1,r2) r1 &= r2;\
							 cpu.regs.F = (cpu.regs.F & 0x0) | FLAG_H;\
							 if (r1==0) cpu.regs.F |= FLAG_Z;\
							 cpu.cycles += 4;\
							 cpu.regs.PC++

#define OR_REG_REG_8(r1,r2) r1 |= r2;\
							cpu.regs.F &= 0x0;\
							if (r1==0) cpu.regs.F |= FLAG_Z;\
							cpu.cycles += 4;\
							cpu.regs.PC++

#define XOR_REG_REG_8(r1,r2) r1 ^= r2;\
							 cpu.regs.F &= 0x0;\
							 if (r1 == 0) cpu.regs.F |= FLAG_Z;\
							 cpu.cycles += 4;\
							 cpu.regs.PC++

#define CP_REG_REG_8(r1,r2) temp8 = r1 - r2;\
							cpu.regs.F = (cpu.regs.F&0x0)|FLAG_N;\
							if (r1 == r2) cpu.regs.F |= FLAG_Z;\
							if (r1 < r2) cpu.regs.F |= FLAG_C;\
							if ((temp8 & 0xF) > (r1&0xF)) cpu.regs.F |= FLAG_H;\
							cpu.cycles += 4;\
							cpu.regs.PC++

#define INC_REG_8(r) temp8 = r + 1;\
					 cpu.regs.F &= FLAG_C;\
					 if (temp8 == 0) cpu.regs.F |= FLAG_Z;\
					 cpu.regs.F |= ((r ^ 1 ^ temp8)<<H_BIT_SHIFT)&FLAG_H;\
					 r = temp8;\
					 cpu.cycles += 4;\
					 cpu.regs.PC++

#define DEC_REG_8(r) temp8 = r - 1;\
	                 cpu.regs.F = (cpu.regs.F & FLAG_C) | FLAG_N;\
					 if (temp8 == 0) cpu.regs.F |= FLAG_Z;\
					 cpu.regs.F |= ((r ^ 1 ^ temp8)<<H_BIT_SHIFT)&FLAG_H;\
					 r = temp8;\
					 cpu.cycles += 4;\
                     cpu.regs.PC++


#define ADD_REG_REG_16(r1,r2,r3,r4) temp16 = (uint16_t)r1 + r3;\
                                    temp9 = 0;\
							        cpu.regs.F &= FLAG_Z;\
									if (((uint16_t)r2 + (uint16_t)r4) > 0xFF) temp9++;\
									tempa = r2 + r4; \
									temp16 += temp9;\
							        if (temp16 >= 0x100) cpu.regs.F |= FLAG_C;\
							        if ((temp16 & 0xF) < (r1 & 0xF)) cpu.regs.F |= FLAG_H;\
							        if (((temp16 & 0xF) == (r1&0xF))&&temp9) cpu.regs.F |= FLAG_H;\
							        r1 = (uint8_t)temp16; \
							        r2 = tempa; \
							        cpu.cycles += 8; \
							        cpu.regs.PC++

#define INC_REG_16(r1,r2) r2++;\
	                      if (r2 == 0) r1++;\
						  cpu.cycles += 8;\
						  cpu.regs.PC++

#define DEC_REG_16(r1,r2) r2--;\
	                      if (r2 == 0xFF) r1--;\
						  cpu.cycles += 8;\
						  cpu.regs.PC++

#define SWAP_REG_8(r) r = ((r&0xF)<<4) | (r>>4);\
	                  cpu.regs.F = 0;\
                      if (r==0) cpu.regs.F |= FLAG_Z;\
					  cpu.cycles += 8;\
					  cpu.regs.PC++

#define RLC_REG_8(r) cpu.regs.F = (r&0x80)?FLAG_C:0;\
                     r = (r<<1)|((r&0x80)>>7);\
                     if (r==0) cpu.regs.F |= FLAG_Z;\
                     cpu.cycles += 8;\
                     cpu.regs.PC++

#define RRC_REG_8(r) temp8 = (r & 0x01) ? FLAG_C : 0;\
                     r = (r>>1) | ((r&0x01)<<7);\
					 cpu.regs.F = temp8;\
                     if (r == 0) cpu.regs.F |= FLAG_Z;\
                     cpu.cycles += 8;\
                     cpu.regs.PC++

#define RL_REG_8(r)  temp8 = (r & 0x80) ? FLAG_C : 0;\
                     r = (r << 1) | ((cpu.regs.F&FLAG_C) >> 4);\
					 cpu.regs.F = temp8;\
                     if (r == 0) cpu.regs.F |= FLAG_Z;\
                     cpu.cycles += 8;\
                     cpu.regs.PC++

#define RR_REG_8(r)  temp8 = (r & 0x01) ? FLAG_C : 0;\
                     r = (r >> 1) | ((cpu.regs.F & FLAG_C) << 3);\
					 cpu.regs.F = temp8;\
                     if (r == 0) cpu.regs.F |= FLAG_Z;\
                     cpu.cycles += 8;\
                     cpu.regs.PC++

#define PUSH_REG_16(r1,r2) cpu.regs.SP -= 2;\
	                       mem_write_8(cpu.regs.SP,r2);\
						   mem_write_8(cpu.regs.SP+1,r1);\
						   cpu.cycles += 16;\
                           cpu.regs.PC++

#define POP_REG_16(r1,r2) r2 = mem_read_8(cpu.regs.SP);\
	                      r1 = mem_read_8(cpu.regs.SP+1);\
						  cpu.regs.SP += 2;\
                          cpu.cycles += 12;\
                          cpu.regs.PC++

#define SLA_REG_8(r) cpu.regs.F = (r&0x80)?FLAG_C:0;\
                     r <<= 1;\
                     if (r==0) cpu.regs.F |= FLAG_Z;\
                     cpu.cycles += 8;\
                     cpu.regs.PC++

#define SRA_REG_8(r) cpu.regs.F = (r&0x01)?FLAG_C:0;\
                     r = (r>>1)|(r&0x80);\
                     if (r==0) cpu.regs.F |= FLAG_Z;\
                     cpu.cycles += 8;\
                     cpu.regs.PC++

#define SRL_REG_8(r) cpu.regs.F = (r&0x01)?FLAG_C:0;\
                     r >>= 1;\
                     if (r==0) cpu.regs.F |= FLAG_Z;\
                     cpu.cycles += 8;\
                     cpu.regs.PC++

#define BIT_REG_8(r,b) cpu.regs.F = (cpu.regs.F&FLAG_C)|FLAG_H;\
	                   cpu.regs.F |= (r & (1<<(b)))?0:FLAG_Z;\
					   cpu.cycles += 8;\
					   cpu.regs.PC++

#define RES_REG_8(r,b) r &= ~(1<<(b));\
					   cpu.cycles += 8;\
					   cpu.regs.PC++


#define SET_REG_8(r,b) r |= (1<<(b));\
					   cpu.cycles += 8;\
					   cpu.regs.PC++


unsigned char	*cart;
int		pendingDI,pendingEI;
int 		speedShift;
tag_cpu		cpu;
extern GbsPlayer *gbsPlayer;  // located in GbMemory.cpp

int exec_count[512];
int exec_samples = 1000000;


int cpu_reset()
{
	if (MACH_TYPE & 0x80) {
		cpu.regs.A = 0x11;
	} else {
		cpu.regs.A = 0x01;
	}
	cpu.regs.B = 0x00;
	cpu.regs.C = 0x13;
	cpu.regs.D = 0x00;
	cpu.regs.E = 0xD8;
	cpu.regs.H = 0x01;
	cpu.regs.L = 0x4D;
	cpu.regs.F = 0xB0;
	cpu.regs.PC = 0x100;
	cpu.regs.SP = 0xFFFE;

	speedShift = 0;

	/*for (int i=0; i<512; i++)
	{
	    exec_count[i] = 0;
	}
	exec_samples = 1000000;

	log = (FILE*)fopen("./gbz80.txt","w");*/

	cpu.halted = cpu.stopped = 0;
	pendingDI = pendingEI = 0;
	return 1;
}


void cpu_rst(unsigned short address)
{
	cpu.halted = 0;
	cpu.regs.SP -= 2;
	mem_write_16(cpu.regs.SP, cpu.regs.PC);
	cpu.regs.PC = address + gbsPlayer->GetLoadAddress();
}

void cpu_close()
{
	/*int i;

     FILE *fp;
     fp = fopen("c:\\exec_cnt.txt","wb");

     for (int i=0; i<256; i++)
     {
         fprintf(fp,"%02X: %d\n",i,exec_count[i]);
     }
     for (int i=0; i<256; i++)
     {
         fprintf(fp,"CB %02X: %d\n",i,exec_count[i+256]);
     }
     fclose(fp);*/

    //fclose(log);
}

void cpu_execute(unsigned int max) {
	uint8_t opcode, opcode2,temp8,temp9,tempa;
	uint16_t temp16, oldPC;
	int debg=0,oldcyc;

	//cpu.cycles = 0;

	if (cpu.halted || cpu.stopped) {
		temp8 = divcnt - (max-cpu.cycles);
		if (temp8 > divcnt) REG_DIV++;
		divcnt = temp8;
		if (REG_TAC & 4) {
			tmcnt -= (max-cpu.cycles);
			if (tmcnt < 0) {
				temp8 = REG_TIMA + (max - cpu.cycles) / tmrld;
				if (temp8 < REG_TIMA) {
					REG_TIMA = REG_TMA;
					REG_IF |= 4;
					if ((IME != 0) && ((REG_IE & 4) != 0)) {
						//sprintf(szBuffer,"RST50(1)");
						REG_IF &= ~4;
						cpu_rst(0x50);
						IME = 0;
					}
				} else {
					REG_TIMA = temp8;
				}
				tmcnt = tmrld;
			}
		}
		cpu.cycles = max;
		return;
	}


	while (cpu.cycles < max) {
		opcode = mem_read_8(cpu.regs.PC);
		oldcyc = cpu.cycles;
        oldPC = cpu.regs.PC;

		if (debg) {
			NLOGV("GbZ80", "PC=%#x, A=%#x, B=%#x, SP=%#x, opcode=%#x", cpu.regs.PC, cpu.regs.A, cpu.regs.B, cpu.regs.SP, opcode);
			/*fprintf(stderr,"PC=%04X: A=%02X, B=%02X, C=%02X, D=%02X, E=%02X, H=%02X, L=%02X, F=%02X, SP=%02X, opcode=%02X\n",
					cpu.regs.PC,cpu.regs.A,cpu.regs.B,cpu.regs.C,cpu.regs.D,cpu.regs.E,cpu.regs.H,cpu.regs.L,cpu.regs.F,
					cpu.regs.SP,opcode);*/
			debg--;
		}

		switch (opcode) {
			// NOP
			case 0x00:
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// LD BC,nn
			case 0x01:
				LOAD_IMM_16(cpu.regs.B,cpu.regs.C);
				break;

			// LD (BC),A
			case 0x02:
				mem_write_8(cpu.regs.C + ((uint16_t)cpu.regs.B<<8), cpu.regs.A);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// INC BC
			case 0x03:
				INC_REG_16(cpu.regs.B,cpu.regs.C);
				break;

			// INC B
			case 0x04:
				INC_REG_8(cpu.regs.B);
				break;
			// DEC B
			case 0x05:
				DEC_REG_8(cpu.regs.B);
				break;
			// LD B,n
			case 0x06:
				LOAD_IMM_8(cpu.regs.B);
				break;

			// RLCA
			case 0x07:
				cpu.regs.F = (cpu.regs.A & 0x80) ? FLAG_C : 0;
				cpu.regs.A = (cpu.regs.A<<1)|(cpu.regs.A>>7);
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// LD (nn),SP
			case 0x08:
				mem_write_16(mem_read_16(cpu.regs.PC+1), cpu.regs.SP);
				cpu.cycles += 20;
				cpu.regs.PC += 3;
				break;

			// ADD HL,BC
			case 0x09:
				ADD_REG_REG_16(cpu.regs.H,cpu.regs.L, cpu.regs.B, cpu.regs.C);
				break;

			// LD A,(BC)
			case 0x0A:
				cpu.regs.A = mem_read_8(cpu.regs.C + ((uint16_t)cpu.regs.B<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// DEC BC
			case 0x0B:
				DEC_REG_16(cpu.regs.B, cpu.regs.C);
				break;

			// INC C
			case 0x0C:
				INC_REG_8(cpu.regs.C);
				break;
			// DEC C
			case 0x0D:
				DEC_REG_8(cpu.regs.C);
				break;
			// LD C,n
			case 0x0E:
				LOAD_IMM_8(cpu.regs.C);
				break;

			// RRCA
			case 0x0F:
				cpu.regs.F = /*(cpu.regs.F&FLAG_Z) |*/ ((cpu.regs.A & 0x01) ? FLAG_C : 0);
				cpu.regs.A = (cpu.regs.A>>1)|((cpu.regs.A&1)<<7);
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// STOP
			case 0x10:
				if ((MACH_TYPE & 0x80) && (IOREGS[0x4D] & 1)) {
				      // TODO: Switch speed
				      IOREGS[0x4D] &= 0x80;
				      IOREGS[0x4D] ^= 0x80;
				      speedShift = (IOREGS[0x4D] >> 7) & 1;
				} else {
				      cpu.stopped = 1;
				}
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// LD DE,nn
			case 0x11:
				LOAD_IMM_16(cpu.regs.D,cpu.regs.E);
				break;
			// LD (DE),A
			case 0x12:
				mem_write_8(cpu.regs.E + ((uint16_t)cpu.regs.D<<8), cpu.regs.A);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// INC DE
			case 0x13:
				INC_REG_16(cpu.regs.D, cpu.regs.E);
				break;

			// INC D
			case 0x14:
				INC_REG_8(cpu.regs.D);
				break;
			// DEC D
			case 0x15:
				DEC_REG_8(cpu.regs.D);
				break;
			// LD D,n
			case 0x16:
				LOAD_IMM_8(cpu.regs.D);
				break;

			// RLA
			case 0x17:
				temp8 = (cpu.regs.A & 0x80) ? FLAG_C : 0;
				cpu.regs.A = (cpu.regs.A<<1)|((cpu.regs.F&FLAG_C)>>4);
				cpu.regs.F = temp8;
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// JR n
			case 0x18:
				cpu.regs.PC += 2;
				cpu.regs.PC += (signed char)mem_read_8(cpu.regs.PC-1);
				cpu.cycles += 12;
				break;

			// ADD HL,DE
			case 0x19:
				ADD_REG_REG_16(cpu.regs.H, cpu.regs.L, cpu.regs.D, cpu.regs.E);
				break;

			// LD A,(DE)
			case 0x1A:
				cpu.regs.A = mem_read_8(cpu.regs.E + ((unsigned short)cpu.regs.D<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// DEC DE
			case 0x1B:
				DEC_REG_16(cpu.regs.D, cpu.regs.E);
				break;

			// INC E
			case 0x1C:
				INC_REG_8(cpu.regs.E);
				break;
			// DEC E
			case 0x1D:
				DEC_REG_8(cpu.regs.E);
				break;
			case 0x1E:
				LOAD_IMM_8(cpu.regs.E);
				break;

			// RRA
			case 0x1F:
				temp8 = /*(cpu.regs.F&FLAG_Z) |*/ ((cpu.regs.A&0x01)?FLAG_C:0);
				cpu.regs.A = (cpu.regs.A>>1)|((cpu.regs.F&FLAG_C)<<3);
				cpu.regs.F = temp8;
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// JR NZ,n
			case 0x20:
				cpu.regs.PC += 2;
				if ((cpu.regs.F&FLAG_Z)==0)
				{
					cpu.regs.PC += (signed char)mem_read_8(cpu.regs.PC-1);
					cpu.cycles += 4;
                }
				cpu.cycles += 8;
				break;

			// LD HL,nn
			case 0x21:
				LOAD_IMM_16(cpu.regs.H, cpu.regs.L);
				break;
			// LD (HL+),A
			case 0x22:
				mem_write_8(cpu.regs.L + ((uint16_t)cpu.regs.H<<8), cpu.regs.A);
				cpu.regs.L++;
				if (cpu.regs.L==0) cpu.regs.H++;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// INC HL
			case 0x23:
				INC_REG_16(cpu.regs.H, cpu.regs.L);
				break;

			// INC H
			case 0x24:
				INC_REG_8(cpu.regs.H);
				break;
			// DEC H
			case 0x25:
				DEC_REG_8(cpu.regs.H);
				break;
			case 0x26:
				LOAD_IMM_8(cpu.regs.H);
				break;

			// DAA
			case 0x27:
				temp8 = cpu.regs.A;

                temp16 = (unsigned short)cpu.regs.A;
                if ( !(cpu.regs.F & FLAG_N) )
                {
                    if ( (cpu.regs.F & FLAG_H) || (temp16 & 0x0F) > 9 )
                        temp16 += 6;

                    if ( (cpu.regs.F & FLAG_C) || temp16 > 0x9F )
                        temp16 += 0x60;
                }
                else
                {
                    if ( cpu.regs.F & FLAG_H )
                        temp16 = (temp16 - 6) & 0xFF;

                    if ( cpu.regs.F & FLAG_C )
                        temp16 -= 0x60;
                }

                cpu.regs.F &= ~(FLAG_H | FLAG_Z);
                if ( temp16 & 0x100 )
                    cpu.regs.F |= FLAG_C;

                cpu.regs.A = (unsigned char)(temp16 & 0xFF);
                if ( !cpu.regs.A )
                    cpu.regs.F |= FLAG_Z;

				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// JR Z,n
			case 0x28:
				cpu.regs.PC += 2;
				if ((cpu.regs.F & FLAG_Z)!=0)
				{
					cpu.regs.PC += (int8_t)mem_read_8(cpu.regs.PC-1);
					cpu.cycles += 4;
                }
				cpu.cycles += 8;
				break;

			// ADD HL,HL
			case 0x29:
				ADD_REG_REG_16(cpu.regs.H, cpu.regs.L, cpu.regs.H, cpu.regs.L);
				break;

			// LD A,(HL+)
			case 0x2A:
				cpu.regs.A = mem_read_8(cpu.regs.L + ((uint16_t)cpu.regs.H<<8));
				cpu.regs.L++;
				if (cpu.regs.L==0) cpu.regs.H++;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// DEC HL
			case 0x2B:
				DEC_REG_16(cpu.regs.H, cpu.regs.L);
				break;


			// INC L
			case 0x2C:
				INC_REG_8(cpu.regs.L);
				break;
			// DEC L
			case 0x2D:
				DEC_REG_8(cpu.regs.L);
				break;
			// LD L,n
			case 0x2E:
				LOAD_IMM_8(cpu.regs.L);
				break;

			// CPL
			case 0x2F:
				cpu.regs.A = ~cpu.regs.A;
				cpu.regs.F |= FLAG_H|FLAG_N;
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// JR NC,n
			case 0x30:
				cpu.regs.PC += 2;
				if ((cpu.regs.F&FLAG_C)==0)
				{
					cpu.regs.PC += (signed char)mem_read_8(cpu.regs.PC-1);
					cpu.cycles += 4;
                }
				cpu.cycles += 8;
				break;

			// LD SP,nn
			case 0x31:
				cpu.regs.SP = mem_read_16(cpu.regs.PC+1);
				cpu.cycles += 12;
				cpu.regs.PC += 3;
				break;

			// LD (HL-),A
			case 0x32:
				mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8), cpu.regs.A);
				cpu.regs.L--;
				if (cpu.regs.L==0xFF) cpu.regs.H--;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// INC SP
			case 0x33:
				cpu.regs.SP++;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// INC (HL)
			case 0x34:
				temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				temp9 = temp8+1;
	            cpu.regs.F = (cpu.regs.F&FLAG_C);
				if (temp9==0) cpu.regs.F |= FLAG_Z;
				if ((temp9&0xF) < (temp8&0xF)) cpu.regs.F |= FLAG_H;
				mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp9);
				cpu.cycles += 12;
                cpu.regs.PC++;
				break;

			// DEC (HL)
			case 0x35:
				temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				temp9 = temp8-1;
	            cpu.regs.F = (cpu.regs.F&FLAG_C)|FLAG_N;
				if (temp9==0) cpu.regs.F |= FLAG_Z;
				if ((temp9&0xF) > (temp8&0xF)) cpu.regs.F |= FLAG_H;
				mem_write_8(cpu.regs.L + (cpu.regs.H<<8),temp9);
				cpu.cycles += 12;
                cpu.regs.PC++;
				break;

			// LD (HL),n
			case 0x36:
				mem_write_8((unsigned short)cpu.regs.L + ((unsigned short)cpu.regs.H<<8), mem_read_8(cpu.regs.PC+1));
				cpu.cycles += 12;
				cpu.regs.PC += 2;
				break;

			// SCF
			case 0x37:
				//cpu.regs.F |= FLAG_C;
				cpu.regs.F = (cpu.regs.F&FLAG_Z)|FLAG_C;
                cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// JR C,n
			case 0x38:
				cpu.regs.PC += 2;
				if ((cpu.regs.F&FLAG_C)!=0)
				{
					cpu.regs.PC += (signed char)mem_read_8(cpu.regs.PC-1);
					cpu.cycles += 4;
                }
				cpu.cycles += 8;
				break;

			// ADD HL,SP
			case 0x39:
				ADD_REG_REG_16(cpu.regs.H,cpu.regs.L,(cpu.regs.SP>>8),(cpu.regs.SP&0xFF));
				break;

			// LD A,(HL-)
			case 0x3A:
				cpu.regs.A = mem_read_8(cpu.regs.L + (cpu.regs.H<<8));
				cpu.regs.L--;
				if (cpu.regs.L==0xFF) cpu.regs.H--;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// DEC SP
			case 0x3B:
				cpu.regs.SP--;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// INC A
			case 0x3C:
				INC_REG_8(cpu.regs.A);
				break;
			// DEC A
			case 0x3D:
				DEC_REG_8(cpu.regs.A);
				break;
			// LD A,n
			case 0x3E:
				LOAD_IMM_8(cpu.regs.A);
				break;

			// CCF
			case 0x3F:
				//cpu.regs.F ^= FLAG_C;
				cpu.regs.F = (cpu.regs.F&(FLAG_Z|FLAG_C))^FLAG_C;
				//cpu.regs.F = ((cpu.regs.F&FLAG_C)<<1) | ((cpu.regs.F&(FLAG_Z|FLAG_C))^FLAG_C);
                cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			case 0x40:
				MOVE_REG_REG_8(cpu.regs.B,cpu.regs.B);
				break;
			case 0x41:
				MOVE_REG_REG_8(cpu.regs.B,cpu.regs.C);
				break;
			case 0x42:
				MOVE_REG_REG_8(cpu.regs.B,cpu.regs.D);
				break;
			case 0x43:
				MOVE_REG_REG_8(cpu.regs.B,cpu.regs.E);
				break;
			case 0x44:
				MOVE_REG_REG_8(cpu.regs.B,cpu.regs.H);
				break;
			case 0x45:
				MOVE_REG_REG_8(cpu.regs.B,cpu.regs.L);
				break;
			case 0x46:
				cpu.regs.B = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x47:
				MOVE_REG_REG_8(cpu.regs.B,cpu.regs.A);
				break;

			case 0x48:
				MOVE_REG_REG_8(cpu.regs.C,cpu.regs.B);
				break;
			case 0x49:
				MOVE_REG_REG_8(cpu.regs.C,cpu.regs.C);
				break;
			case 0x4A:
				MOVE_REG_REG_8(cpu.regs.C,cpu.regs.D);
				break;
			case 0x4B:
				MOVE_REG_REG_8(cpu.regs.C,cpu.regs.E);
				break;
			case 0x4C:
				MOVE_REG_REG_8(cpu.regs.C,cpu.regs.H);
				break;
			case 0x4D:
				MOVE_REG_REG_8(cpu.regs.C,cpu.regs.L);
				break;
			case 0x4E:
				cpu.regs.C = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x4F:
				MOVE_REG_REG_8(cpu.regs.C,cpu.regs.A);
				break;

			case 0x50:
				MOVE_REG_REG_8(cpu.regs.D,cpu.regs.B);
				break;
			case 0x51:
				MOVE_REG_REG_8(cpu.regs.D,cpu.regs.C);
				break;
			case 0x52:
				MOVE_REG_REG_8(cpu.regs.D,cpu.regs.D);
				break;
			case 0x53:
				MOVE_REG_REG_8(cpu.regs.D,cpu.regs.E);
				break;
			case 0x54:
				MOVE_REG_REG_8(cpu.regs.D,cpu.regs.H);
				break;
			case 0x55:
				MOVE_REG_REG_8(cpu.regs.D,cpu.regs.L);
				break;
			case 0x56:
				cpu.regs.D = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x57:
				MOVE_REG_REG_8(cpu.regs.D,cpu.regs.A);
				break;

			case 0x58:
				MOVE_REG_REG_8(cpu.regs.E,cpu.regs.B);
				break;
			case 0x59:
				MOVE_REG_REG_8(cpu.regs.E,cpu.regs.C);
				break;
			case 0x5A:
				MOVE_REG_REG_8(cpu.regs.E,cpu.regs.D);
				break;
			case 0x5B:
				MOVE_REG_REG_8(cpu.regs.E,cpu.regs.E);
				break;
			case 0x5C:
				MOVE_REG_REG_8(cpu.regs.E,cpu.regs.H);
				break;
			case 0x5D:
				MOVE_REG_REG_8(cpu.regs.E,cpu.regs.L);
				break;
			case 0x5E:
				cpu.regs.E = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x5F:
				MOVE_REG_REG_8(cpu.regs.E,cpu.regs.A);
				break;

			case 0x60:
				MOVE_REG_REG_8(cpu.regs.H,cpu.regs.B);
				break;
			case 0x61:
				MOVE_REG_REG_8(cpu.regs.H,cpu.regs.C);
				break;
			case 0x62:
				MOVE_REG_REG_8(cpu.regs.H,cpu.regs.D);
				break;
			case 0x63:
				MOVE_REG_REG_8(cpu.regs.H,cpu.regs.E);
				break;
			case 0x64:
				MOVE_REG_REG_8(cpu.regs.H,cpu.regs.H);
				break;
			case 0x65:
				MOVE_REG_REG_8(cpu.regs.H,cpu.regs.L);
				break;
			case 0x66:
				cpu.regs.H = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x67:
				MOVE_REG_REG_8(cpu.regs.H,cpu.regs.A);
				break;

			case 0x68:
				MOVE_REG_REG_8(cpu.regs.L,cpu.regs.B);
				break;
			case 0x69:
				MOVE_REG_REG_8(cpu.regs.L,cpu.regs.C);
				break;
			case 0x6A:
				MOVE_REG_REG_8(cpu.regs.L,cpu.regs.D);
				break;
			case 0x6B:
				MOVE_REG_REG_8(cpu.regs.L,cpu.regs.E);
				break;
			case 0x6C:
				MOVE_REG_REG_8(cpu.regs.L,cpu.regs.H);
				break;
			case 0x6D:
				MOVE_REG_REG_8(cpu.regs.L,cpu.regs.L);
				break;
			case 0x6E:
				cpu.regs.L = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x6F:
				MOVE_REG_REG_8(cpu.regs.L, cpu.regs.A);
				break;

			case 0x70:
				mem_write_8(cpu.regs.L + ((uint16_t)cpu.regs.H<<8),cpu.regs.B);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x71:
				mem_write_8(cpu.regs.L + ((uint16_t)cpu.regs.H<<8), cpu.regs.C);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x72:
				mem_write_8(cpu.regs.L + ((uint16_t)cpu.regs.H<<8), cpu.regs.D);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x73:
				mem_write_8(cpu.regs.L + ((uint16_t)cpu.regs.H<<8), cpu.regs.E);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x74:
				mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8), cpu.regs.H);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// LD (HL),L
			case 0x75:
				mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8), cpu.regs.L);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// HALT
			case 0x76:
				cpu.halted = 1;
				cpu.cycles = max;
				cpu.regs.PC++;
				break;

			// LD (HL),A
			case 0x77:
				mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8), cpu.regs.A);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			case 0x78:
				MOVE_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0x79:
				MOVE_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0x7A:
				MOVE_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0x7B:
				MOVE_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0x7C:
				MOVE_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0x7D:
				MOVE_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0x7E:
				cpu.regs.A = mem_read_8(cpu.regs.L + ((uint16_t)cpu.regs.H<<8));
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			case 0x7F:
				MOVE_REG_REG_8(cpu.regs.A,cpu.regs.A);
				break;

			// ADD A,B
			case 0x80:
				ADD_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0x81:
				ADD_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0x82:
				ADD_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0x83:
				ADD_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0x84:
				ADD_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0x85:
				ADD_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0x86:
				temp16 = (uint16_t)cpu.regs.A + mem_read_8(cpu.regs.L + ((uint16_t)cpu.regs.H<<8));
				temp8 = (uint8_t)temp16;
				cpu.regs.F = 0;
				if (temp8 == 0) cpu.regs.F |= FLAG_Z;
				if (temp16 >= 0x100) cpu.regs.F |= FLAG_C;
				if ((temp8 & 0xF) < (cpu.regs.A & 0xF)) cpu.regs.F |= FLAG_H;
				cpu.regs.A = temp8;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// ADD A,A
			case 0x87:
				ADD_REG_REG_8(cpu.regs.A, cpu.regs.A);
				break;

			// ADC A,B
			case 0x88:
				ADC_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0x89:
				ADC_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0x8A:
				ADC_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0x8B:
				ADC_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0x8C:
				ADC_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0x8D:
				ADC_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0x8E:
                temp9 = (cpu.regs.F&FLAG_C)>>4; // Old carry
				temp8 = cpu.regs.A + mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8)) + temp9;
				temp16 = (uint16_t)cpu.regs.A + mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8)) + temp9;
				cpu.regs.F = 0;
				if (temp8==0) cpu.regs.F |= FLAG_Z;
				if (temp16 >= 0x100) cpu.regs.F |= FLAG_C;
				if ((temp8 & 0xF) < (cpu.regs.A&0xF)) cpu.regs.F |= FLAG_H;
				if (((temp8 & 0xF) == (cpu.regs.A&0xF)) && temp9) cpu.regs.F |= FLAG_H;
				cpu.regs.A = temp8;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// ADC A,A
			case 0x8F:
				ADC_REG_REG_8(cpu.regs.A, cpu.regs.A);
				break;

			// SUB A,B
			case 0x90:
				SUB_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0x91:
				SUB_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0x92:
				SUB_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0x93:
				SUB_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0x94:
				SUB_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0x95:
				SUB_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0x96:
				temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.regs.F = FLAG_N;
				if (cpu.regs.A < temp8) cpu.regs.F |= FLAG_C;
				temp8 = cpu.regs.A - temp8;
				if (temp8 == 0) cpu.regs.F |= FLAG_Z;
				if ((temp8 & 0xF) > (cpu.regs.A & 0xF)) cpu.regs.F |= FLAG_H;
				cpu.regs.A = temp8;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// SUB A,A
			case 0x97:
				SUB_REG_REG_8(cpu.regs.A, cpu.regs.A);
				break;

			// SBC A,B
			case 0x98:
				SBC_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0x99:
				SBC_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0x9A:
				SBC_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0x9B:
				SBC_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0x9C:
				SBC_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0x9D:
				SBC_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0x9E:
				temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8)) + ((cpu.regs.F&FLAG_C)>>4);
				cpu.regs.F = FLAG_N;
				if (cpu.regs.A < temp8) cpu.regs.F |= FLAG_C;
				temp8 = cpu.regs.A - temp8;
				if (temp8 == 0) cpu.regs.F |= FLAG_Z;
				if ((temp8 & 0xF) > (cpu.regs.A & 0xF)) cpu.regs.F |= FLAG_H;
				cpu.regs.A = temp8;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// SBC A,A
			case 0x9F:
				SBC_REG_REG_8(cpu.regs.A, cpu.regs.A);
				break;

			// AND A,B
			case 0xA0:
				AND_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0xA1:
				AND_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0xA2:
				AND_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0xA3:
				AND_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0xA4:
				AND_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0xA5:
				AND_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0xA6:
				cpu.regs.A &= mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.regs.F = FLAG_H;
				if (cpu.regs.A==0) cpu.regs.F |= FLAG_Z;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// AND A,A
			case 0xA7:
				AND_REG_REG_8(cpu.regs.A, cpu.regs.A);
				break;

			// XOR A,B
			case 0xA8:
				XOR_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0xA9:
				XOR_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0xAA:
				XOR_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0xAB:
				XOR_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0xAC:
				XOR_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0xAD:
				XOR_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0xAE:
				cpu.regs.A ^= mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.regs.F = 0;
				if (cpu.regs.A==0) cpu.regs.F |= FLAG_Z;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// XOR A,A
			case 0xAF:
				XOR_REG_REG_8(cpu.regs.A, cpu.regs.A);
				break;

			// OR A,B
			case 0xB0:
				OR_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0xB1:
				OR_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0xB2:
				OR_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0xB3:
				OR_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0xB4:
				OR_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0xB5:
				OR_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0xB6:
				cpu.regs.A |= mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.regs.F = 0;
				if (cpu.regs.A == 0) cpu.regs.F |= FLAG_Z;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// OR A,A
			case 0xB7:
				OR_REG_REG_8(cpu.regs.A, cpu.regs.A);
				break;

			// CP A,B
			case 0xB8:
				CP_REG_REG_8(cpu.regs.A, cpu.regs.B);
				break;
			case 0xB9:
				CP_REG_REG_8(cpu.regs.A, cpu.regs.C);
				break;
			case 0xBA:
				CP_REG_REG_8(cpu.regs.A, cpu.regs.D);
				break;
			case 0xBB:
				CP_REG_REG_8(cpu.regs.A, cpu.regs.E);
				break;
			case 0xBC:
				CP_REG_REG_8(cpu.regs.A, cpu.regs.H);
				break;
			case 0xBD:
				CP_REG_REG_8(cpu.regs.A, cpu.regs.L);
				break;
			case 0xBE:
				temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				cpu.regs.F = FLAG_N;
				if (cpu.regs.A < temp8) cpu.regs.F |= FLAG_C;
				temp8 = cpu.regs.A - temp8;
				if (temp8 == 0) cpu.regs.F |= FLAG_Z;
				if ((temp8&0xF) > (cpu.regs.A&0xF)) cpu.regs.F |= FLAG_H;
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;
			// CP A,A
			case 0xBF:
				CP_REG_REG_8(cpu.regs.A, cpu.regs.A);
				break;

			// RET NZ
			case 0xC0:
				if ((cpu.regs.F & FLAG_Z)==0) {
					cpu.regs.PC = mem_read_16(cpu.regs.SP);
					cpu.regs.SP += 2;
					cpu.cycles += 12;
				} else
					cpu.regs.PC++;
				cpu.cycles += 8;
				break;

			// POP BC
			case 0xC1:
				POP_REG_16(cpu.regs.B,cpu.regs.C);
				break;

			// JP NZ,nn
			case 0xC2:
				if ((cpu.regs.F & FLAG_Z)==0)
				{
					cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
					cpu.cycles += 4;
				}
                else
					cpu.regs.PC += 3;
				cpu.cycles += 12;
				break;

			// JP nn
			case 0xC3:
				cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
				cpu.cycles += 16;
				break;

			// CALL NZ,nn
			case 0xC4:
				if ((cpu.regs.F & FLAG_Z)==0) {
					cpu.regs.SP -= 2;
					mem_write_16(cpu.regs.SP, cpu.regs.PC+3);
					cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
					cpu.cycles += 12;
				} else
					cpu.regs.PC += 3;
				cpu.cycles += 12;
				break;

			// PUSH BC
			case 0xC5:
				PUSH_REG_16(cpu.regs.B,cpu.regs.C);
				break;

			// ADD A,n
			case 0xC6:
				temp16 = (uint16_t)cpu.regs.A + mem_read_8(cpu.regs.PC+1);
				temp8 = (uint8_t)temp16;
				cpu.regs.F = 0;
				if (temp8 == 0) cpu.regs.F |= FLAG_Z;
				if (temp16 >= 0x100) cpu.regs.F |= FLAG_C;
				if ((temp8 & 0xF) < (cpu.regs.A & 0xF)) cpu.regs.F |= FLAG_H;
				cpu.regs.A = temp8;
				cpu.cycles += 8;
				cpu.regs.PC += 2;
				break;

			// RST 0x00
			case 0xC7:
				cpu.regs.PC++;
				cpu_rst(0x00);
				cpu.cycles += 32;
				break;

			// RET Z
			case 0xC8:
				if ((cpu.regs.F & FLAG_Z)!=0) {
					cpu.regs.PC = mem_read_16(cpu.regs.SP);
					cpu.regs.SP += 2;
					cpu.cycles += 12;
				} else
					cpu.regs.PC++;
				cpu.cycles += 8;
				break;

			// RET
			case 0xC9:
				cpu.regs.PC = mem_read_16(cpu.regs.SP);
				cpu.regs.SP += 2;
				cpu.cycles += 16;
				break;

			// JP Z,nn
			case 0xCA:
				if ((cpu.regs.F & FLAG_Z)!=0)
				{
					cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
					cpu.cycles += 4;
				}
                else
					cpu.regs.PC += 3;
				cpu.cycles += 12;
				break;

			case 0xCB:
				cpu.regs.PC++;
				opcode2 = mem_read_8(cpu.regs.PC);
				if (debg>0)
                 {
					/*sprintf(szBuffer,"CB: PC=%04X: A=%02X, B=%02X, D=%02X, E=%02X, H=%02X, L=%02X, F=%02X, SP=%02X, opcode=%02X\n",
							cpu.regs.PC,cpu.regs.A,cpu.regs.B,cpu.regs.D,cpu.regs.E,cpu.regs.H,cpu.regs.L,cpu.regs.F,
							cpu.regs.SP,opcode2);*/
					//WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);
					debg--;
				}

        /*if (exec_samples)
        {
            exec_samples--;
            exec_count[opcode2+256]++;
        }*/

				switch (opcode2) {
					case 0x00:
						RLC_REG_8(cpu.regs.B);
						break;
					case 0x01:
						RLC_REG_8(cpu.regs.C);
						break;
					case 0x02:
						RLC_REG_8(cpu.regs.D);
						break;
					case 0x03:
						RLC_REG_8(cpu.regs.E);
						break;
					case 0x04:
						RLC_REG_8(cpu.regs.H);
						break;
					case 0x05:
						RLC_REG_8(cpu.regs.L);
						break;
					case 0x06:
						//sprintf(szBuffer,"RLC (hl)\n");
						//WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);
						temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
                        cpu.regs.F = (temp8&0x80)?FLAG_C:0;
                        temp8 = (temp8<<1)|((temp8&0x80)>>7);
                        if (temp8==0) cpu.regs.F |= FLAG_Z;
                        mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp8);
						cpu.regs.PC++;
						cpu.cycles += 16;
						break;
					case 0x07:
						RLC_REG_8(cpu.regs.A);
						break;

					case 0x08:
						RRC_REG_8(cpu.regs.B);
						break;
					case 0x09:
						RRC_REG_8(cpu.regs.C);
						break;
					case 0x0A:
						RRC_REG_8(cpu.regs.D);
						break;
					case 0x0B:
						RRC_REG_8(cpu.regs.E);
						break;
					case 0x0C:
						RRC_REG_8(cpu.regs.H);
						break;
					case 0x0D:
						RRC_REG_8(cpu.regs.L);
						break;
					case 0x0E:
						/*sprintf(szBuffer,"RRC (hl)\n");
						WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);*/
						temp9 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
				        temp8 = (temp9&0x01)?FLAG_C:0;
                        temp9 = (temp9>>1)|((temp9&0x01)<<7);
					    cpu.regs.F = temp8;
                        if (temp9==0) cpu.regs.F |= FLAG_Z;
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp9);
						cpu.regs.PC++;
						cpu.cycles += 16;
						break;
					case 0x0F:
						RRC_REG_8(cpu.regs.A);
						break;

					case 0x10:
						RL_REG_8(cpu.regs.B);
						break;
					case 0x11:
						RL_REG_8(cpu.regs.C);
						break;
					case 0x12:
						RL_REG_8(cpu.regs.D);
						break;
					case 0x13:
						RL_REG_8(cpu.regs.E);
						break;
					case 0x14:
						RL_REG_8(cpu.regs.H);
						break;
					case 0x15:
						RL_REG_8(cpu.regs.L);
						break;
					case 0x16:
						temp9 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
						temp8 = (temp9&0x80)?FLAG_C:0;
						temp9 = (temp9<<1)|((cpu.regs.F&FLAG_C)>>4);
						cpu.regs.F = temp8;
						if (temp9==0) cpu.regs.F |= FLAG_Z;
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp9);
						cpu.cycles += 16;
						cpu.regs.PC++;
						break;
					case 0x17:
						RL_REG_8(cpu.regs.A);
						break;

					case 0x18:
						RR_REG_8(cpu.regs.B);
						break;
					case 0x19:
						RR_REG_8(cpu.regs.C);
						break;
					case 0x1A:
						RR_REG_8(cpu.regs.D);
						break;
					case 0x1B:
						RR_REG_8(cpu.regs.E);
						break;
					case 0x1C:
						RR_REG_8(cpu.regs.H);
						break;
					case 0x1D:
						RR_REG_8(cpu.regs.L);
						break;
					case 0x1E:
						temp9 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
						temp8 = (temp9&0x01)?FLAG_C:0;
						temp9 = (temp9>>1)|((cpu.regs.F&FLAG_C)<<3);
						cpu.regs.F = temp8;
						if (temp9==0) cpu.regs.F |= FLAG_Z;
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp9);
						cpu.cycles += 16;
						cpu.regs.PC++;
						break;
					case 0x1F:
						RR_REG_8(cpu.regs.A);
						break;

					case 0x20:
						SLA_REG_8(cpu.regs.B);
						break;
					case 0x21:
						SLA_REG_8(cpu.regs.C);
						break;
					case 0x22:
						SLA_REG_8(cpu.regs.D);
						break;
					case 0x23:
						SLA_REG_8(cpu.regs.E);
						break;
					case 0x24:
						SLA_REG_8(cpu.regs.H);
						break;
					case 0x25:
						SLA_REG_8(cpu.regs.L);
						break;
					case 0x26:
						/*sprintf(szBuffer,"SLA (hl)\n");
						WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);*/
						temp9 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
                        cpu.regs.F = (temp9&0x80)?FLAG_C:0;
                        temp9 <<= 1;
                        if (temp9==0) cpu.regs.F |= FLAG_Z;
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp9);
						cpu.regs.PC++;
						cpu.cycles += 16;
						break;
					case 0x27:
						SLA_REG_8(cpu.regs.A);
						break;

					case 0x28:
						SRA_REG_8(cpu.regs.B);
						break;
					case 0x29:
						SRA_REG_8(cpu.regs.C);
						break;
					case 0x2A:
						SRA_REG_8(cpu.regs.D);
						break;
					case 0x2B:
						SRA_REG_8(cpu.regs.E);
						break;
					case 0x2C:
						SRA_REG_8(cpu.regs.H);
						break;
					case 0x2D:
						SRA_REG_8(cpu.regs.L);
						break;
					case 0x2E:
						/*sprintf(szBuffer,"SRA (hl)\n");
						WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);*/
						temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
                        cpu.regs.F = (temp8&0x01)?FLAG_C:0;
                        temp8 = (temp8>>1)|(temp8&0x80);
                        if (temp8==0) cpu.regs.F |= FLAG_Z;
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp8);
						cpu.regs.PC++;
						cpu.cycles += 16;
						break;
					case 0x2F:
						SRA_REG_8(cpu.regs.A);
						break;

					case 0x30:
						SWAP_REG_8(cpu.regs.B);
						break;
					case 0x31:
						SWAP_REG_8(cpu.regs.C);
						break;
					case 0x32:
						SWAP_REG_8(cpu.regs.D);
						break;
					case 0x33:
						SWAP_REG_8(cpu.regs.E);
						break;
					case 0x34:
						SWAP_REG_8(cpu.regs.H);
						break;
					case 0x35:
						SWAP_REG_8(cpu.regs.L);
						break;
					case 0x36:
						/*sprintf(szBuffer,"SWAP (hl)\n");
						WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);*/
						temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
                        temp8 = ((temp8&0xF)<<4) | (temp8>>4);
	                    cpu.regs.F = 0;
                        if (temp8==0) cpu.regs.F |= FLAG_Z;
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp8);
						cpu.regs.PC++;
						cpu.cycles += 16;
						break;
					case 0x37:
						SWAP_REG_8(cpu.regs.A);
						break;

					case 0x38:
						SRL_REG_8(cpu.regs.B);
						break;
					case 0x39:
						SRL_REG_8(cpu.regs.C);
						break;
					case 0x3A:
						SRL_REG_8(cpu.regs.D);
						break;
					case 0x3B:
						SRL_REG_8(cpu.regs.E);
						break;
					case 0x3C:
						SRL_REG_8(cpu.regs.H);
						break;
					case 0x3D:
						SRL_REG_8(cpu.regs.L);
						break;
					case 0x3E:
						/*sprintf(szBuffer,"SRL (hl)\n");
						WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);*/
						temp8 = mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8));
                        cpu.regs.F = (temp8&0x01)?FLAG_C:0;
                        temp8 >>= 1;
                        if (temp8==0) cpu.regs.F |= FLAG_Z;
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),temp8);
						cpu.regs.PC++;
						cpu.cycles += 16;
						break;
					case 0x3F:
						SRL_REG_8(cpu.regs.A);
						break;

					case 0x40: case 0x48: case 0x50: case 0x58:
					case 0x60: case 0x68: case 0x70: case 0x78:
						BIT_REG_8(cpu.regs.B,(opcode2-0x40)>>3);
						break;

					case 0x41: case 0x49: case 0x51: case 0x59:
					case 0x61: case 0x69: case 0x71: case 0x79:
						BIT_REG_8(cpu.regs.C,(opcode2-0x41)>>3);
						break;

					case 0x42: case 0x4A: case 0x52: case 0x5A:
					case 0x62: case 0x6A: case 0x72: case 0x7A:
						BIT_REG_8(cpu.regs.D,(opcode2-0x42)>>3);
						break;

					case 0x43: case 0x4B: case 0x53: case 0x5B:
					case 0x63: case 0x6B: case 0x73: case 0x7B:
						BIT_REG_8(cpu.regs.E,(opcode2-0x43)>>3);
						break;

					case 0x44: case 0x4C: case 0x54: case 0x5C:
					case 0x64: case 0x6C: case 0x74: case 0x7C:
						BIT_REG_8(cpu.regs.H,(opcode2-0x44)>>3);
						break;

					case 0x45: case 0x4D: case 0x55: case 0x5D:
					case 0x65: case 0x6D: case 0x75: case 0x7D:
						BIT_REG_8(cpu.regs.L,(opcode2-0x45)>>3);
						break;

					case 0x47: case 0x4F: case 0x57: case 0x5F:
					case 0x67: case 0x6F: case 0x77: case 0x7F:
						BIT_REG_8(cpu.regs.A,(opcode2-0x47)>>3);
						break;

					case 0x46: case 0x4E: case 0x56: case 0x5E:
					case 0x66: case 0x6E: case 0x76: case 0x7E:
						cpu.regs.F = (cpu.regs.F&FLAG_C)|FLAG_H;
						cpu.regs.F |= (mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8)) & (1<<((opcode2-0x46)>>3)))?0:FLAG_Z;
						cpu.cycles += 12;
						cpu.regs.PC++;
						break;


					case 0x80: case 0x88: case 0x90: case 0x98:
					case 0xA0: case 0xA8: case 0xB0: case 0xB8:
						RES_REG_8(cpu.regs.B,(opcode2-0x80)>>3);
						break;

					case 0x81: case 0x89: case 0x91: case 0x99:
					case 0xA1: case 0xA9: case 0xB1: case 0xB9:
						RES_REG_8(cpu.regs.C,(opcode2-0x81)>>3);
						break;

					case 0x82:
						RES_REG_8(cpu.regs.D,0);
						break;
					case 0x83:
						RES_REG_8(cpu.regs.E,0);
						break;
					case 0x84:
						RES_REG_8(cpu.regs.H,0);
						break;
					case 0x85:
						RES_REG_8(cpu.regs.L,0);
						break;
					case 0x87:
						RES_REG_8(cpu.regs.A,0);
						break;

					case 0x8A:
						RES_REG_8(cpu.regs.D,1);
						break;
					case 0x8B:
						RES_REG_8(cpu.regs.E,1);
						break;
					case 0x8C:
						RES_REG_8(cpu.regs.H,1);
						break;
					case 0x8D:
						RES_REG_8(cpu.regs.L,1);
						break;
					case 0x8F:
						RES_REG_8(cpu.regs.A,1);
						break;

					case 0x92:
						RES_REG_8(cpu.regs.D,2);
						break;
					case 0x93:
						RES_REG_8(cpu.regs.E,2);
						break;
					case 0x94:
						RES_REG_8(cpu.regs.H,2);
						break;
					case 0x95:
						RES_REG_8(cpu.regs.L,2);
						break;
					case 0x97:
						RES_REG_8(cpu.regs.A,2);
						break;

					case 0x9A:
						RES_REG_8(cpu.regs.D,3);
						break;
					case 0x9B:
						RES_REG_8(cpu.regs.E,3);
						break;
					case 0x9C:
						RES_REG_8(cpu.regs.H,3);
						break;
					case 0x9D:
						RES_REG_8(cpu.regs.L,3);
						break;

					case 0x9F:
						RES_REG_8(cpu.regs.A,3);
						break;

					case 0xA2:
						RES_REG_8(cpu.regs.D,4);
						break;
					case 0xA3:
						RES_REG_8(cpu.regs.E,4);
						break;
					case 0xA4:
						RES_REG_8(cpu.regs.H,4);
						break;
					case 0xA5:
						RES_REG_8(cpu.regs.L,4);
						break;
					case 0xA7:
						RES_REG_8(cpu.regs.A,4);
						break;

					case 0xAA:
						RES_REG_8(cpu.regs.D,5);
						break;
					case 0xAB:
						RES_REG_8(cpu.regs.E,5);
						break;
					case 0xAC:
						RES_REG_8(cpu.regs.H,5);
						break;
					case 0xAD:
						RES_REG_8(cpu.regs.L,5);
						break;
					case 0xAF:
						RES_REG_8(cpu.regs.A,5);
						break;

					case 0xB2:
						RES_REG_8(cpu.regs.D,6);
						break;
					case 0xB3:
						RES_REG_8(cpu.regs.E,6);
						break;
					case 0xB4:
						RES_REG_8(cpu.regs.H,6);
						break;
					case 0xB5:
						RES_REG_8(cpu.regs.L,6);
						break;
					case 0xB7:
						RES_REG_8(cpu.regs.A,6);
						break;

					case 0xBA:
						RES_REG_8(cpu.regs.D,7);
						break;
					case 0xBB:
						RES_REG_8(cpu.regs.E,7);
						break;
					case 0xBC:
						RES_REG_8(cpu.regs.H,7);
						break;
					case 0xBD:
						RES_REG_8(cpu.regs.L,7);
						break;
					case 0xBF:
						RES_REG_8(cpu.regs.A,7);
						break;

					case 0x86: case 0x8E: case 0x96: case 0x9E:
					case 0xA6: case 0xAE: case 0xB6: case 0xBE:
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8))&~(1<<((opcode2-0x86)>>3)));
						cpu.cycles += 16;
						cpu.regs.PC++;
						break;

					case 0xC0: case 0xC8: case 0xD0: case 0xD8:
					case 0xE0: case 0xE8: case 0xF0: case 0xF8:
						SET_REG_8(cpu.regs.B,(opcode2-0xC0)>>3);
						break;

					case 0xC1: case 0xC9: case 0xD1: case 0xD9:
					case 0xE1: case 0xE9: case 0xF1: case 0xF9:
						SET_REG_8(cpu.regs.C,(opcode2-0xC1)>>3);
						break;

					case 0xC2: case 0xCA: case 0xD2: case 0xDA:
					case 0xE2: case 0xEA: case 0xF2: case 0xFA:
						SET_REG_8(cpu.regs.D,(opcode2-0xC2)>>3);
						break;

					case 0xC3: case 0xCB: case 0xD3: case 0xDB:
					case 0xE3: case 0xEB: case 0xF3: case 0xFB:
						SET_REG_8(cpu.regs.E,(opcode2-0xC3)>>3);
						break;

					case 0xC4: case 0xCC: case 0xD4: case 0xDC:
					case 0xE4: case 0xEC: case 0xF4: case 0xFC:
						SET_REG_8(cpu.regs.H,(opcode2-0xC4)>>3);
						break;

					case 0xC5: case 0xCD: case 0xD5: case 0xDD:
					case 0xE5: case 0xED: case 0xF5: case 0xFD:
						SET_REG_8(cpu.regs.L,(opcode2-0xC5)>>3);
						break;

					case 0xC7:
						SET_REG_8(cpu.regs.A,0);
						break;

					case 0xCF:
						SET_REG_8(cpu.regs.A,1);
						break;

					case 0xD7:
						SET_REG_8(cpu.regs.A,2);
						break;

					case 0xC6: case 0xCE: case 0xD6: case 0xDE:
					case 0xE6: case 0xEE: case 0xF6: case 0xFE:
						mem_write_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8),mem_read_8(cpu.regs.L + ((unsigned short)cpu.regs.H<<8))|(1<<((opcode2-0xC6)>>3)));
						cpu.cycles += 16;
						cpu.regs.PC++;
						break;

					case 0xDF:
						SET_REG_8(cpu.regs.A,3);
						break;

					case 0xE7:
						SET_REG_8(cpu.regs.A,4);
						break;

					case 0xEF:
						SET_REG_8(cpu.regs.A,5);
						break;

					case 0xF7:
						SET_REG_8(cpu.regs.A,6);
						break;

					case 0xFF:
						SET_REG_8(cpu.regs.A,7);
						break;

					default:
						fprintf(stderr,"Unhandled at PC=%04X: A=%02X, B=%02X, D=%02X, E=%02X, H=%02X, L=%02X, F=%02X, SP=%02X, opcode=%02X\n",
								cpu.regs.PC,cpu.regs.A,cpu.regs.B,cpu.regs.D,cpu.regs.E,cpu.regs.H,cpu.regs.L,cpu.regs.F,
								cpu.regs.SP,opcode2);
						//WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);
						cpu.halted = 1;
						while (1) {}
						break;
				}
				break;

			// CALL Z,nn
			case 0xCC:
				if ((cpu.regs.F & FLAG_Z)!=0) {
					cpu.regs.SP -= 2;
					mem_write_16(cpu.regs.SP, cpu.regs.PC+3);
					cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
					cpu.cycles += 12;
				} else
					cpu.regs.PC += 3;
				cpu.cycles += 12;
				break;

			// CALL nn
			case 0xCD:
				cpu.regs.SP -= 2;
				mem_write_16(cpu.regs.SP, cpu.regs.PC+3);
				cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
				cpu.cycles += 24;
				break;

			// ADC A,n
			case 0xCE:
                temp9 = (cpu.regs.F & FLAG_C)>>4; // Old carry
				temp8 = cpu.regs.A + mem_read_8(cpu.regs.PC+1) + temp9;
				temp16 = (uint16_t)cpu.regs.A + mem_read_8(cpu.regs.PC+1) + temp9;
				cpu.regs.F = 0;
				if (temp8==0) cpu.regs.F |= FLAG_Z;
				if (temp16 >= 0x100) cpu.regs.F |= FLAG_C;
				if ((temp8&0xF) < (cpu.regs.A&0xF)) cpu.regs.F |= FLAG_H;
				if (((temp8&0xF) == (cpu.regs.A&0xF)) && temp9) cpu.regs.F |= FLAG_H;
				cpu.regs.A = temp8;
				cpu.cycles += 8;
				cpu.regs.PC += 2;
				break;

			// RST 0x08
			case 0xCF:
				cpu.regs.PC++;
				cpu_rst(0x08);
				cpu.cycles += 16;
				break;

			// RET NC
			case 0xD0:
				if ((cpu.regs.F & FLAG_C)==0) {
					cpu.regs.PC = mem_read_16(cpu.regs.SP);
					cpu.regs.SP += 2;
					cpu.cycles += 12;
				} else
					cpu.regs.PC++;
				cpu.cycles += 8;
				break;

			// POP DE
			case 0xD1:
				POP_REG_16(cpu.regs.D,cpu.regs.E);
				break;

			// JP NC,nn
			case 0xD2:
				if ((cpu.regs.F & FLAG_C)==0)
				{
					cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
					cpu.cycles += 4;
				}
                else
					cpu.regs.PC += 3;
				cpu.cycles += 12;
				break;

			// CALL NC,nn
			case 0xD4:
				if ((cpu.regs.F & FLAG_C)==0) {
					cpu.regs.SP -= 2;
					mem_write_16(cpu.regs.SP, cpu.regs.PC+3);
					cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
					cpu.cycles += 12;
				} else
					cpu.regs.PC += 3;
				cpu.cycles += 12;
				break;

			// PUSH DE
			case 0xD5:
				PUSH_REG_16(cpu.regs.D,cpu.regs.E);
				break;

			// SUB A,n
			case 0xD6:
				temp8 = mem_read_8(cpu.regs.PC+1);
				cpu.regs.F = (cpu.regs.F&0xF)|FLAG_N;
				if (cpu.regs.A < temp8) cpu.regs.F |= FLAG_C;
				temp8 = cpu.regs.A - temp8;
				if (temp8 == 0) cpu.regs.F |= FLAG_Z;
				if ((temp8&0xF) > (cpu.regs.A&0xF)) cpu.regs.F |= FLAG_H;
				cpu.regs.A = temp8;
				cpu.cycles += 8;
				cpu.regs.PC += 2;
				break;

			// RST 0x10
			case 0xD7:
				cpu.regs.PC++;
				cpu_rst(0x10);
				cpu.cycles += 16;
				break;

			// RET C
			case 0xD8:
				if ((cpu.regs.F & FLAG_C)!=0) {
					cpu.regs.PC = mem_read_16(cpu.regs.SP);
					cpu.regs.SP += 2;
					cpu.cycles += 12;
				} else
					cpu.regs.PC++;
				cpu.cycles += 8;
				break;

			// RETI
			case 0xD9:
				cpu.regs.PC = mem_read_16(cpu.regs.SP);
				cpu.regs.SP += 2;
				cpu.cycles += 16;
				pendingEI = 1;
				break;

			// JP C,nn
			case 0xDA:
				if ((cpu.regs.F & FLAG_C)!=0)
				{
					cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
					cpu.cycles += 4;
				}
                else
					cpu.regs.PC += 3;
				cpu.cycles += 12;
				break;

			// CALL C,nn
			case 0xDC:
				if ((cpu.regs.F & FLAG_C)!=0) {
					cpu.regs.SP -= 2;
					mem_write_16(cpu.regs.SP, cpu.regs.PC+3);
					cpu.regs.PC = mem_read_16(cpu.regs.PC+1);
					cpu.cycles += 12;
				} else
					cpu.regs.PC += 3;
				cpu.cycles += 12;
				break;

			// SBC A,n
			case 0xDE:
                temp9 = ((cpu.regs.F&FLAG_C)>>4); // Old carry
				temp8 = mem_read_8(cpu.regs.PC+1) + temp9;
				cpu.regs.F = FLAG_N;
				if (cpu.regs.A < temp8) cpu.regs.F |= FLAG_C;
				if ((cpu.regs.A&0xF) < (temp8&0xF)) cpu.regs.F |= FLAG_H;
				//if ((cpu.regs.A + ((temp8^0xFF)+1)) > 0xFF) cpu.regs.F |= FLAG_C;
				temp8 = cpu.regs.A - temp8;
				if (temp8 == 0) cpu.regs.F |= FLAG_Z;
				//if ((temp8&0xF) > (cpu.regs.A&0xF)) cpu.regs.F |= FLAG_H;
				//if (((temp8&0xF) == (cpu.regs.A&0xF)) && temp9) cpu.regs.F |= FLAG_H;
				//if (((cpu.regs.A&0xF) + (((temp8&0xF)^0xF)+1)) > 0xF) cpu.regs.F |= FLAG_H;
				cpu.regs.A = temp8;
				cpu.cycles += 8;
				cpu.regs.PC += 2;
				break;

			// RST 0x18
			case 0xDF:
				cpu.regs.PC++;
				cpu_rst(0x18);
				cpu.cycles += 16;
				break;

			// LDH (n),A
			case 0xE0:
				mem_write_8(0xFF00 + mem_read_8(cpu.regs.PC+1), cpu.regs.A);
				cpu.cycles += 12;
				cpu.regs.PC += 2;
				break;

			// POP HL
			case 0xE1:
				POP_REG_16(cpu.regs.H,cpu.regs.L);
				break;

			// LD (C),A
			case 0xE2:
			/*fprintf(stderr,"PC=%04X: A=%02X, B=%02X, C=%02X, D=%02X, E=%02X, H=%02X, L=%02X, F=%02X, SP=%02X, opcode=%02X\n",
					cpu.regs.PC,cpu.regs.A,cpu.regs.B,cpu.regs.C,cpu.regs.D,cpu.regs.E,cpu.regs.H,cpu.regs.L,cpu.regs.F,
					cpu.regs.SP,opcode);
			fprintf(stderr,"LD (%04X),%02X\n",0xFF00+cpu.regs.C,cpu.regs.A);*/
				mem_write_8(0xFF00 + cpu.regs.C, cpu.regs.A);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// PUSH HL
			case 0xE5:
				PUSH_REG_16(cpu.regs.H,cpu.regs.L);
				break;

			// AND A,n
			case 0xE6:
				cpu.regs.A &= mem_read_8(cpu.regs.PC+1);
				cpu.regs.F = FLAG_H;
				if (cpu.regs.A==0) cpu.regs.F |= FLAG_Z;
				cpu.cycles += 8;
				cpu.regs.PC += 2;
				break;

			// RST 0x20
			case 0xE7:
				cpu.regs.PC++;
				cpu_rst(0x20);
				cpu.cycles += 16;
				break;

			// ADD SP,n
			case 0xE8:
				temp16 = mem_read_8(cpu.regs.PC+1);
				if (temp16&0x80) temp16|=0xFF00; else temp16&=0x007F;
				cpu.regs.F &= ~(FLAG_Z|FLAG_N);
				cpu.regs.SP += temp16;
				cpu.cycles += 16;
				cpu.regs.PC += 2;
				break;

			// JP (HL)
			case 0xE9:
				cpu.regs.PC = cpu.regs.L + ((unsigned short)cpu.regs.H<<8);
				cpu.cycles += 4;
				break;

			// LD (nn),A
			case 0xEA:
				mem_write_8(mem_read_16(cpu.regs.PC+1), cpu.regs.A);
				cpu.cycles += 16;
				cpu.regs.PC += 3;
				break;

			// XOR A,n
			case 0xEE:
				cpu.regs.A ^= mem_read_8(cpu.regs.PC+1);
				cpu.regs.F &= 0xF;
				if (cpu.regs.A==0) cpu.regs.F |= FLAG_Z;
				cpu.cycles += 8;
				cpu.regs.PC += 2;
				break;

			// RST 0x28
			case 0xEF:
				cpu.regs.PC++;
				cpu_rst(0x28);
				cpu.cycles += 16;
				break;

			// LDH A,(n)
			case 0xF0:
				cpu.regs.A = mem_read_8(0xFF00 + mem_read_8(cpu.regs.PC+1));
				cpu.cycles += 12;
				cpu.regs.PC += 2;
				break;

			// POP AF
			case 0xF1:
				POP_REG_16(cpu.regs.A,cpu.regs.F);
				//debg = 10;
				break;

			// LD A,(C)
			case 0xF2:
				cpu.regs.A = mem_read_8(0xFF00 + cpu.regs.C);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// DI
			case 0xF3:
				pendingDI = 1;
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// PUSH AF
			case 0xF5:
                //debg = 10;
				PUSH_REG_16(cpu.regs.A,(cpu.regs.F&0xF0));
				break;

			// OR A,n
			case 0xF6:
				cpu.regs.A |= mem_read_8(cpu.regs.PC+1);
				cpu.regs.F &= 0xF;
				if (cpu.regs.A==0) cpu.regs.F |= FLAG_Z;
				cpu.cycles += 8;
				cpu.regs.PC += 2;
				break;

			// RST 0x30
			case 0xF7:
				cpu.regs.PC++;
				cpu_rst(0x30);
				cpu.cycles += 16;
				break;

			// LD HL,SP+n
			case 0xF8:
				temp16 = mem_read_8(cpu.regs.PC+1);
				if (temp16 & 0x80) temp16 |= 0xFF00; else temp16 &= 0x007F;
				temp16 += cpu.regs.SP;
				cpu.regs.L = temp16 & 0xFF;
				cpu.regs.H = (temp16 >> 8) & 0xFF;
				cpu.regs.F &= ~(FLAG_Z|FLAG_N);
				cpu.cycles += 12;
				cpu.regs.PC += 2;
				break;

			// LD SP,HL
			case 0xF9:
				cpu.regs.SP = cpu.regs.L + ((uint16_t)cpu.regs.H<<8);
				cpu.cycles += 8;
				cpu.regs.PC++;
				break;

			// LD A,(nn)
			case 0xFA:
				cpu.regs.A = mem_read_8(mem_read_16(cpu.regs.PC+1));
				cpu.cycles += 16;
				cpu.regs.PC += 3;
				break;

			// EI
			case 0xFB:
				pendingEI = 1;
				cpu.cycles += 4;
				cpu.regs.PC++;
				break;

			// CP n
			case 0xFE:
				temp9 = mem_read_8(cpu.regs.PC+1);
				temp8 = cpu.regs.A - temp9;
				cpu.regs.F = FLAG_N;
				if (cpu.regs.A == temp9) cpu.regs.F |= FLAG_Z;
				if (cpu.regs.A < temp9) cpu.regs.F |= FLAG_C;
				if ((temp8&0xF) > (cpu.regs.A&0xF)) cpu.regs.F |= FLAG_H;
			    //cpu.regs.F |= ((cpu.regs.A ^ temp9 ^ temp8)<<H_BIT_SHIFT)&FLAG_H;
				cpu.cycles += 8;
				cpu.regs.PC += 2;
				break;

			// RST 0x38
			case 0xFF:
				cpu.regs.PC++;
				cpu_rst(0x38);
				cpu.cycles += 16;
				break;

			default:
				fprintf(stderr,"PC=%04X: A=%02X, B=%02X, D=%02X, E=%02X, H=%02X, L=%02X, F=%02X, SP=%02X, opcode=%02X\n",
					cpu.regs.PC,cpu.regs.A,cpu.regs.B,cpu.regs.D,cpu.regs.E,cpu.regs.H,cpu.regs.L,cpu.regs.F,
					cpu.regs.SP,opcode);
				fflush(stderr);
				cpu.halted = 1;
				cpu.cycles++;
				while (1) {}
				break;
		}


		if ((IME!=0) && (REG_IE & REG_IF)) {
			temp8 = REG_IE&REG_IF;
			if (temp8&1) {
				REG_IF &= ~1;
				cpu.halted = 0;
				cpu_rst(0x40);
				IME = 0;
			} else if (temp8&2) {
				REG_IF &= ~2;
				cpu.halted = 0;
				cpu_rst(0x48);
				IME = 0;
			} else if (temp8&4) {
					//sprintf(szBuffer,"RST50(2)");
					//WriteConsole(hout,szBuffer,strlen(szBuffer),&templ,NULL);
				REG_IF &= ~4;
				cpu.halted = 0;
				cpu_rst(0x50);
				IME = 0;
			} else if (temp8&0x10) {
				REG_IF &= ~0x10;
				cpu.halted = 0;
				cpu_rst(0x60);
				IME = 0;
			}
		}

        temp8 = divcnt;
		temp8 -= (cpu.cycles-oldcyc);
		if (temp8 > divcnt) REG_DIV++;
		divcnt = temp8;

		if (REG_TAC & 4) {
			tmcnt -= (cpu.cycles-oldcyc);
			if (tmcnt<0) {
				temp8 = REG_TIMA+1;
				if (!temp8) { //(temp8 < REG_TIMA) {
					REG_TIMA = REG_TMA;
					REG_IF |= 4;
					//fprintf(stderr,"Timer overflow: IE=%02X, IF=%02X, IME=%02X, TIMA=%02X\n",
					//		REG_IE,REG_IF,IME,REG_TIMA);
					if ((IME!=0)&&((REG_IE&4)!=0)) {
						REG_IF &= ~4;
						cpu_rst(0x50);
						IME = 0;
					}
				} else {
					REG_TIMA = temp8;
				}
				tmcnt += tmrld;
			}
		}

		if (pendingDI) { IME = 0; pendingDI = 0; }
		if (pendingEI) { IME = 1; pendingEI = 0; }
		if (cpu.halted || cpu.stopped) return;
	}
}
