#include "stdio.h"
#include "stdint.h"

/*
 * virtual-machine architecture
 * ++++++++++++++++++++++++++++
 *
 * segmented memory model with 16-byte segment size (notation seg:offset)
 *
 * 4 general-purpose registers (r0-r3)
 * 2 segment registers (cs, ds equiv. to r4, r5)
 * 1 flags register (fl)
 *
 *
 * instruction encoding
 * ++++++++++++++++++++
 *
 *            byte 1               byte 2 (optional)
 * bits      [ 7 6 5 4 3 2 1 0 ]  [ 7 6 5 4 3 2 1 0 ]
 * opcode      - - -             
 * mod               -           
 * operand1            - - - -
 * operand2                         - - - - - - - -
 *
 * operand1 is always a register index
 * operand2 is optional, depending upon the instruction set specified below
 * the value of mod alters the meaning of any operand2
 *   0: operand2 = register index
 *   1: operand2 = fixed immediate value or target segment (depending on instruction)
 *
 *
 * instruction set
 * +++++++++++++++
 * 
 * opcode | instruction | operands (mod 0) | operands (mod 1)
 * -------+-------------+------------------+-----------------
 * 0x00   | jmp         | r1               | r2:r1
 * 0x01   | movr        | r1, r2           | rx,   imm 
 * 0x02   | movm        | r1, [ds:r2]      | [ds:r1], r2
 * 0x03   | add         | r1, r2           | r1,   imm
 * 0x04   | xor         | r1, r2           | r1,   imm 
 * 0x05   | cmp         | r1, r2           | r1,   imm 
 * 0x06   | jmpe        | r1               | r2:r1
 * 0x07   | hlt         | N/A              | N/A
 *
 *
 * flags
 * +++++
 *
 *   r1, r2 => operand 1 is register 1, operand 2 is register 2
 *   movr r1, r2 => move contents of register r2 into register r1
 *
 *   cmp r1, r2
 *     if (r1 == r2) => flag = 0
 *     if (r1  < r2) => flag = 0xff
 *     if (r1  > r2) => flag = 1
 *   jmpe r1
 *      if (flag == 0) jmp r1
 *      else nop
 */

uint8_t mem[] = {
	0x31, 0x04, 0x33, 0xaa, 0x40, 0x02, 0x80, 0x03, 0x52, 0x00, 0x72, 0x01, 0x73, 0x01, 0xb2, 0x50,
	0x30, 0x14, 0xc0, 0x01, 0x80, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x98, 0xab, 0xd9, 0xa1, 0x9f, 0xa7, 0x83, 0x83, 0xf2, 0xb1, 0x34, 0xb6, 0xe4, 0xb7, 0xca, 0xb8,
	0xc9, 0xb8, 0x0e, 0xbd, 0x7d, 0x0f, 0xc0, 0xf1, 0xd9, 0x03, 0xc5, 0x3a, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xa9, 0xcd, 0xdf, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0x26, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0x7d, 0x1f, 0x15, 0x60, 0x4d, 0x4d, 0x52, 0x7d, 0x0e, 0x27, 0x6d, 0x10, 0x6d, 0x5a, 0x06, 0x56,
	0x47, 0x14, 0x42, 0x0e, 0xb6, 0xb2, 0xb2, 0xe6, 0xeb, 0xb4, 0x83, 0x8e, 0xd7, 0xe5, 0xd4, 0xd9,
	0xc3, 0xf0, 0x80, 0x95, 0xf1, 0x82, 0x82, 0x9a, 0xbd, 0x95, 0xa4, 0x8d, 0x9a, 0x2b, 0x30, 0x69,
	0x4a, 0x69, 0x65, 0x55, 0x1c, 0x7b, 0x69, 0x1c, 0x6e, 0x04, 0x74, 0x35, 0x21, 0x26, 0x2f, 0x60,
	0x03, 0x4e, 0x37, 0x1e, 0x33, 0x54, 0x39, 0xe6, 0xba, 0xb4, 0xa2, 0xad, 0xa4, 0xc5, 0x95, 0xc8,
	0xc1, 0xe4, 0x8a, 0xec, 0xe7, 0x92, 0x8b, 0xe8, 0x81, 0xf0, 0xad, 0x98, 0xa4, 0xd0, 0xc0, 0x8d,
	0xac, 0x22, 0x52, 0x65, 0x7e, 0x27, 0x2b, 0x5a, 0x12, 0x61, 0x0a, 0x01, 0x7a, 0x6b, 0x1d, 0x67,
	0x75, 0x70, 0x6c, 0x1b, 0x11, 0x25, 0x25, 0x70, 0x7f, 0x7e, 0x67, 0x63, 0x30, 0x3c, 0x6d, 0x6a,
	0x01, 0x51, 0x59, 0x5f, 0x56, 0x13, 0x10, 0x43, 0x19, 0x18, 0xe5, 0xe0, 0xbe, 0xbf, 0xbd, 0xe9,
	0xf0, 0xf1, 0xf9, 0xfa, 0xab, 0x8f, 0xc1, 0xdf, 0xcf, 0x8d, 0xf8, 0xe7, 0xe2, 0xe9, 0x93, 0x8e,
	0xec, 0xf5, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x37, 0x7a, 0x07, 0x11, 0x1f, 0x1d, 0x68, 0x25, 0x32, 0x77, 0x1e, 0x62, 0x23, 0x5b, 0x47, 0x55,
	0x53, 0x30, 0x11, 0x42, 0xf6, 0xf1, 0xb1, 0xe6, 0xc3, 0xcc, 0xf8, 0xc5, 0xe4, 0xcc, 0xc0, 0xd3,
	0x85, 0xfd, 0x9a, 0xe3, 0xe6, 0x81, 0xb5, 0xbb, 0xd7, 0xcd, 0x87, 0xa3, 0xd3, 0x6b, 0x36, 0x6f,
	0x6f, 0x66, 0x55, 0x30, 0x16, 0x45, 0x5e, 0x09, 0x74, 0x5c, 0x3f, 0x29, 0x2b, 0x66, 0x3d, 0x0d,
	0x02, 0x30, 0x28, 0x35, 0x15, 0x09, 0x15, 0xdd, 0xec, 0xb8, 0xe2, 0xfb, 0xd8, 0xcb, 0xd8, 0xd1,
	0x8b, 0xd5, 0x82, 0xd9, 0x9a, 0xf1, 0x92, 0xab, 0xe8, 0xa6, 0xd6, 0xd0, 0x8c, 0xaa, 0xd2, 0x94,
	0xcf, 0x45, 0x46, 0x67, 0x20, 0x7d, 0x44, 0x14, 0x6b, 0x45, 0x6d, 0x54, 0x03, 0x17, 0x60, 0x62,
	0x55, 0x5a, 0x4a, 0x66, 0x61, 0x11, 0x57, 0x68, 0x75, 0x05, 0x62, 0x36, 0x7d, 0x02, 0x10, 0x4b,
	0x08, 0x22, 0x42, 0x32, 0xba, 0xe2, 0xb9, 0xe2, 0xd6, 0xb9, 0xff, 0xc3, 0xe9, 0x8a, 0x8f, 0xc1,
	0x8f, 0xe1, 0xb8, 0xa4, 0x96, 0xf1, 0x8f, 0x81, 0xb1, 0x8d, 0x89, 0xcc, 0xd4, 0x78, 0x76, 0x61,
	0x72, 0x3e, 0x37, 0x23, 0x56, 0x73, 0x71, 0x79, 0x63, 0x7c, 0x08, 0x11, 0x20, 0x69, 0x7a, 0x14,
	0x68, 0x05, 0x21, 0x1e, 0x32, 0x27, 0x59, 0xb7, 0xcf, 0xab, 0xdd, 0xd5, 0xcc, 0x97, 0x93, 0xf2,
	0xe7, 0xc0, 0xeb, 0xff, 0xe9, 0xa3, 0xbf, 0xa1, 0xab, 0x8b, 0xbb, 0x9e, 0x9e, 0x8c, 0xa0, 0xc1,
	0x9b, 0x5a, 0x2f, 0x2f, 0x4e, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
  

int main() {
	// registers
	uint8_t regs[8] = {0};
	// cmp-flag
	uint8_t flag = 0;

	// program counter
	uint32_t pc = 0x0;

	enum {
		INSTR_JUMP = 0x0,
		INSTR_MOVR = 0x1,
		INSTR_MOVM = 0x2,
		INSTR_ADD  = 0x3,
		INSTR_XOR  = 0x4,
		INSTR_CMP  = 0x5,
		INSTR_JMPE = 0x6,
		INSTR_HALT = 0x7,
	};

	regs[5] = 0x10;

	do {
		const uint8_t byte0 = mem[pc    ];
		const uint8_t byte1 = mem[pc + 1];
		const uint8_t instr = byte0 >> 5;

		// jump offset
		uint8_t pc_jmp = 0;

		switch (instr) {
			case INSTR_JUMP: {
				pc_jmp = regs[byte0 & 7];

				if ((byte0 & 16) != 0) {
					// mod --> segmented
					pc = (byte1 * 16) + pc_jmp;
					regs[4] = byte1;
				} else {
					pc = (regs[4] * 16) + pc_jmp;
				}
			} break;

			case INSTR_MOVR: {
				if ((byte0 & 16) != 0) {
					// mod --> immediate
					regs[byte0 & 7] = byte1;
					pc += 2;
				} else {
					// r1,r2
					regs[byte0 & 7] = regs[byte1 & 7];
					pc += 2;
				}
			} break;

			case INSTR_MOVM: {
				if ((byte0 & 16) == 0) {
					// mod --> r1,[ds:r2]
					regs[byte0 & 7] = mem[(regs[5] * 16) + regs[(byte1 & 7)]];
					pc += 2;
				} else {
					// [ds:r1], r2
					mem[(regs[5] * 16) + regs[(byte0 & 7)]] = regs[byte1 & 7];
					pc += 2;
				}
			} break;

			case INSTR_ADD: {
				if ((byte0 & 16) == 0) {
					regs[byte0 & 7] += regs[byte1 & 7];
					pc += 2;
				} else {
					regs[byte0 & 7] += byte1;
					pc += 2;
				}
			} break;

			case INSTR_XOR: {
				if ((byte0 & 16) == 0) {
					regs[byte0 & 7] ^= regs[byte1 & 7];
					pc += 2;
				} else {
					regs[byte0 & 7] ^= byte1;
					pc += 2;
				}
			} break;

			case INSTR_CMP: {
				if ((byte0 & 16) == 0) {
					if (regs[byte0 & 7] == regs[byte1 & 7]) flag = 0;
					if (regs[byte0 & 7]  < regs[byte1 & 7]) flag = 0xff;
					if (regs[byte0 & 7]  > regs[byte1 & 7]) flag = 0x1;

					pc += 2;
				} else {
					if (regs[byte0 & 7] == byte1) flag = 0;
					if (regs[byte0 & 7]  < byte1) flag = 0xff;
					if (regs[byte0 & 7]  > byte1) flag = 0x1;

					pc += 2;
				}
			} break;

			case INSTR_JMPE: {
				pc_jmp = regs[byte0 & 7];

				if ((byte0 & 16) == 0) {
					if (flag == 0) {
						pc = (regs[4] * 16) + pc_jmp;
					} else {
						pc += 1;
					}
				} else {
					if (flag == 0) {
						pc = (byte1 * 16) + pc_jmp;
						regs[4] = byte1;
					} else {
						pc += 2;
					}
				}
			} break;

			case INSTR_HALT: {
				// finished, dump memory contents (never reached)
				for (size_t n = 0; n < sizeof(mem); n++)
					printf("%c ", mem[n]);

				return 0;
			} break;
		}
	} while (1);

	#if 0
	// VM code executes these two operations on its memory
	// when finished, memory dump contains URL to an .exe
	// (for stage 3)
	for (uint32_t i = 0; i < 80; i++) { mem[0x100 + i] ^= (i   + 0xAA); }
	for (uint32_t i = 0; i < 51; i++) { mem[0x1C0 + i] ^= (i*3 + 0x32); }
	return 0;
	#endif

	for (size_t n = 0; n < sizeof(mem); n++)
		printf("%c ", mem[n]);

    return 0;
}

