#ifndef CPU_6502_H
#define CPU_6502_H

#include "bytes.h"
#include "ram.h"

#define STACK_BEGIN 	0x0100
#define STACK_END 		0x01FF
#define PROG_BEGIN 		0xFFFC
#define PAGE_SIZE 		0xFF

// 6502 CPU
typedef struct cpu6502
{
	byte A, X, Y; 	// register a, x and y
	byte SP; 		// stack pointer
	word PC; 		// program counter
	byte status; 	// status bits
} cpu6502_t;

// cpu flags
typedef enum cpu_flag
{
	N = 1 << 7, // negative
	V = 1 << 6, // overflow
	U = 1 << 5, // unused
	B = 1 << 4, // break
	D = 1 << 3, // decimal
	I = 1 << 2, // interrupt
	Z = 1 << 1, // zero
	C = 1 << 0  // carry
} cpu_flag_t;

#define CPU_RESET_FLAGS(cpu, flags) cpu->status &= ~((flags))
#define CPU_SET_FLAGS(cpu, flags) cpu->status |= (flags)

void cpu_reset(cpu6502_t *cpu, ram_t *rm);

byte cpu_fetch_byte(cpu6502_t *cpu, ram_t *ram);

word cpu_fetch_word(cpu6502_t *cpu, ram_t *ram);

byte cpu_read_byte(ram_t *ram, word addr);

word cpu_read_word(ram_t *ram, word addr);

void cpu_write_byte(ram_t *ram, word addr, byte data);

void cpu_write_word(ram_t *ram, word addr, word data);

void cpu_push_stack_byte(cpu6502_t *cpu, ram_t *ram, byte data);

void cpu_push_stack_word(cpu6502_t *cpu, ram_t *ram, word data);

byte cpu_pop_stack_byte(cpu6502_t *cpu, ram_t *ram);

word cpu_pop_stack_word(cpu6502_t *cpu, ram_t *ram);


// instruction mnemonics and opcodes
typedef enum
{
	INS_LDA_IMM 	= 0xA9, // Load accumulator immediate
	INS_LDA_ZP 		= 0xA5, // Load accumulator from zero page i.e memory address 0-255
	INS_LDA_ZPX 	= 0xB5, // Loda accumulator by adding value of X in immediate zero page address.
	INS_LDA_ABS 	= 0xAD, // Load from absolute address
	INS_LDA_ABSX 	= 0xBD, // Load accumulator absoulute addres + X
	INS_LDA_ABSY 	= 0xB9, // Load A absolute address + Y
	INS_LDA_INDX	= 0xA1, // Load accumulator indirect address + X
	INS_LDA_INDY	= 0xB1, // Load accumulator indirect address + Y
	INS_LDX_IMM 	= 0xA2, // Load X immediate
	INS_LDX_ZP		= 0xA6, // Load X zero page
	INS_LDX_ZPY		= 0xB6, // Load X zero page + Y
	INS_LDX_ABS		= 0xAE, // Load X absolute
	INS_LDX_ABSY	= 0xBE, // Load X absolute + Y
	INS_LDY_IMM 	= 0xA0, // Load Y immediate
	INS_LDY_ZP		= 0xA4, // Load Y zero page
	INS_LDY_ZPX		= 0xB4, // Load Y zero page + X
	INS_LDY_ABS		= 0xAC, // Load Y absolute
	INS_LDY_ABSX	= 0xBC, // Load Y absolute + X
	INS_JSR 		= 0x20, // Jump to subroutine
	INS_RTS 		= 0x60, // Return from subroutine
	INS_AND_IMM 	= 0x29, // And immediate with accumulator
	INS_AND_ZP		= 0x25, // And A with given immediate zero page address value
	INS_AND_ZPX		= 0x35, // And A with given immediate zero page address value + X
	INS_AND_ABS		= 0x2D, // And A with given immediate absolute address value
	INS_AND_ABSX 	= 0x3D, // And A with given immediate absolute address value + X
	INS_AND_ABSY 	= 0x39, // And A with given immediate absolute address value + Y
	INS_AND_INDX 	= 0x21, // And A with given indirect address value + X
	INS_AND_INDY 	= 0x31, // And A with given indirect address value + Y
	INS_JMP_ABS		= 0x4C, // Jump to absolute address
	INS_JMP_IND		= 0x6C, // Jump to
	INS_KIL			= 0x02, // Freeze or kill the CPU
	INS_ASL_A 		= 0x0A, // Left Shift A by 1 bit
	INS_ASL_ZP		= 0x06, // Left Shift value from zero page address by 1 bit
	INS_ASL_ZPX		= 0x16, // Left Shift value from zero page address + X by 1 bit
	INS_ASL_ABS		= 0x0E, // Left Shift value from absolute address by 1 bit
	INS_ASL_ABSX	= 0x1E, // Left Shift value from absolute address + X by 1 bit
	INS_BIT_ZP		= 0x24, // Test bit stored at zero page address
	INS_BIT_ABS		= 0x2C, // Test bit stored at absolute address
	INS_CLC			= 0x18, // Clear carry flag
	INS_SEC 		= 0x38, // Set carry flag
	INS_CLI			= 0x58, // Clear interrupt flag
	INS_SEI			= 0x78, // Set interrupt flag
	INS_CLV 		= 0xB8, // Clear overflow flag
	INS_CLD			= 0xD8, // Clear decimal flag
	INS_SED			= 0xF8, // Set decimal flag
	INS_INC_ZP 		= 0xE6, // Increment value at zero page
	INS_INC_ZPX 	= 0xF6, // Increment value at zero page + X
	INS_INC_ABS		= 0xEE, // Increment value at absolute address
	INS_INC_ABSX 	= 0xFE, // Increment value at absolute address + X
	INS_ADC_IMM		= 0x69, // Add immediate value with A and store in A
	INS_ADC_ZP		= 0x65, // Add value from given zero page address
	INS_ADC_ZPX		= 0x75, // Add value from given zero page address  + X
	INS_ADC_ABS		= 0x6D, // Add value from given absolute address
	INS_ADC_ABSX	= 0x7D, // Add value from given absolute address + X
	INS_ADC_ABSY	= 0x79, // Add value from given absolute address + Y
	INS_ADC_INDX	= 0x61, // Add value from given indirect address + X
	INS_ADC_INDY	= 0x71, // Add value from given indirect address + Y
	INS_STA_ZP		= 0x85, // Store A at zero page address
	INS_STA_ZPX		= 0x95, // Store A at zp + X
	INS_STA_ABS		= 0x8D, // store A at absolute address
	INS_STA_ABSX	= 0x9D, // Store A at absolute address + X
	INS_STA_ABSY	= 0x99, // Store A at absolute address + Y
	INS_STA_INDX	= 0x81, // Store A at indirect address + X
	INS_STA_INDY	= 0x91, // Store A at indirect address + Y
	INS_STX_ZP		= 0x86, // Store X at zp address
	INS_STX_ZPY		= 0x96, // Store X at zp address + Y
	INS_STX_ABS		= 0x8E, // Store X at absolute address
	INS_STY_ZP		= 0x84, // Store Y at zp address
	INS_STY_ZPX		= 0x94, // Store Y at zp address + X
	INS_STY_ABS		= 0x8C, // Store Y at absolute address
	INS_TXS			= 0x9A, // Transfer X to Stack ptr
	INS_TSX			= 0xBA, // Transfer Stack ptr to X
	INS_PHA			= 0x48, // Push accumulator
	INS_PLA			= 0x68, // Pull accumulator
	INS_PHP			= 0x08, // Push processor status
	INS_PLP			= 0x28, // Pull processor status
	INS_SBC_IMM		= 0xE9, // Subtract A immediate 
	INS_SBC_ZP		= 0xE5, // Subtract A from zero page address value
	INS_SBC_ZPX		= 0xF5, // Subtract A from zp address + X value
	INS_SBC_ABS		= 0xED, // Subtract A from absolute address value
	INS_SBC_ABSX	= 0xFD, // Subtract A from absolute address + X value
	INS_SBC_ABSY	= 0xF9, // Subtract A from absolute address + Y value
	INS_SBC_INDX	= 0xE1, // Subtract A from indirect address + X value
	INS_SBC_INDY	= 0xF1, // Subtract A from indirect address + Y value
} ins;


/* ADC */
void ADC_IMM(cpu6502_t *cpu, ram_t *ram);
void ADC_ZP(cpu6502_t *cpu, ram_t *ram);
void ADC_ZPX(cpu6502_t *cpu, ram_t *ram);
void ADC_ABS(cpu6502_t *cpu, ram_t *ram);
void ADC_ABSX(cpu6502_t *cpu, ram_t *ram);
void ADC_ABSY(cpu6502_t *cpu, ram_t *ram);
void ADC_INDX(cpu6502_t *cpu, ram_t *ram);
void ADC_INDY(cpu6502_t *cpu, ram_t *ram);


/* Flag set/reset
void CLC(cpu6502_t *cpu);
void SEC(cpu6502_t *cpu);
void CLI(cpu6502_t *cpu);
void SEI(cpu6502_t *cpu);
void CLV(cpu6502_t *cpu);
void CLD(cpu6502_t *cpu);
void SED(cpu6502_t *cpu);
*/

/*
 *
 * Clearing and setting status flags
 *
 * */
 /*
__attribute__((always_inline))
inline
*/
static inline void CLC(cpu6502_t *cpu)
{
	cpu->status &= ~(C);
}

static inline void SEC(cpu6502_t *cpu)
{
	cpu->status |= C;
}

static inline void CLI(cpu6502_t *cpu)
{
	cpu->status &= ~(I);
}

static inline void SEI(cpu6502_t *cpu)
{
	cpu->status |= I;
}

static inline void CLV(cpu6502_t *cpu)
{
	cpu->status &= ~(V);
}

static inline void CLD(cpu6502_t *cpu)
{
	cpu->status &= ~(D);
}

static inline void SED(cpu6502_t *cpu)
{
	cpu->status |= D;
}


/* INC */
void INC_ZP(cpu6502_t *cpu, ram_t *ram);
void INC_ZPX(cpu6502_t *cpu, ram_t *ram);
void INC_ABS(cpu6502_t *cpu, ram_t *ram);
void INC_ABSX(cpu6502_t *cpu, ram_t *ram);


/* BIT */
void BIT_ZP(cpu6502_t *cpu, ram_t *ram);
void BIT_ABS(cpu6502_t *cpu, ram_t *ram);


/* ASL */
void ASL_A(cpu6502_t *cpu);
void ASL_ZP(cpu6502_t *cpu, ram_t *ram);
void ASL_ZPX(cpu6502_t *cpu, ram_t *ram);
void ASL_ABS(cpu6502_t *cpu, ram_t *ram);
void ASL_ABSX(cpu6502_t *cpu, ram_t *ram);


/* AND */
void AND_IMM(cpu6502_t *cpu, ram_t *ram);
void AND_ZP(cpu6502_t *cpu, ram_t *ram);
void AND_ZPX(cpu6502_t *cpu, ram_t *ram);
void AND_ABS(cpu6502_t *cpu, ram_t *ram);
void AND_ABSX(cpu6502_t *cpu, ram_t *ram);
void AND_ABSY(cpu6502_t *cpu, ram_t *ram);
void AND_INDX(cpu6502_t *cpu, ram_t *ram);
void AND_INDY(cpu6502_t *cpu, ram_t *ram);


/* JMP */
void JMP_ABS(cpu6502_t *cpu, ram_t *ram);
void JMP_IND(cpu6502_t *cpu, ram_t *ram);


/* LDA */
void LDA_IMM(cpu6502_t *cpu, ram_t *ram);
void LDA_ZP(cpu6502_t *cpu, ram_t *ram);
void LDA_ZPX(cpu6502_t *cpu, ram_t *ram);
void LDA_ABS(cpu6502_t *cpu, ram_t *ram);
void LDA_ABSX(cpu6502_t *cpu, ram_t *ram);
void LDA_ABSY(cpu6502_t *cpu, ram_t *ram);
void LDA_INDX(cpu6502_t *cpu, ram_t *ram);
void LDA_INDY(cpu6502_t *cpu, ram_t *ram);


/* LDX */
void LDX_IMM(cpu6502_t *cpu, ram_t *ram);
void LDX_ZP(cpu6502_t *cpu, ram_t *ram);
void LDX_ZPY(cpu6502_t *cpu, ram_t *ram);
void LDX_ABS(cpu6502_t *cpu, ram_t *ram);
void LDX_ABSY(cpu6502_t *cpu, ram_t *ram);


/* LDY */
void LDY_IMM(cpu6502_t *cpu, ram_t *ram);
void LDY_ZP(cpu6502_t *cpu, ram_t *ram);
void LDY_ZPX(cpu6502_t *cpu, ram_t *ram);
void LDY_ABS(cpu6502_t *cpu, ram_t *ram);
void LDY_ABSX(cpu6502_t *cpu, ram_t *ram);


/* JSR */
void JSR(cpu6502_t *cpu, ram_t *ram);


/* RTS */
void RTS(cpu6502_t *cpu, ram_t *ram);

#endif
