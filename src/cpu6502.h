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


/*
*
* All instructions supported by 6502
*
*/
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

	INS_ROR_ACC		= 0x6A, // Rotate A right
	INS_ROR_ZP		= 0x66, // Rotate zero page value right
	INS_ROR_ZPX		= 0x76, // Rotate zero page + X value right
	INS_ROR_ABS		= 0x6E, // Rotate absolute address value right
	INS_ROR_ABSX	= 0x7E, // Rotate absolute address + X value right

	INS_ROL_ACC		= 0x2A, // Rotate A right
	INS_ROL_ZP		= 0x26, // Rotate zero page value right
	INS_ROL_ZPX		= 0x36, // Rotate zero page + X value right
	INS_ROL_ABS		= 0x2E, // Rotate absolute address value right
	INS_ROL_ABSX	= 0x3E, // Rotate absolute address + X value right

	INS_ORA_IMM 	= 0x09, // Bitwise OR A with immediate value
	INS_ORA_ZP		= 0x05, // Bitwise OR A with zero page value
	INS_ORA_ZPX		= 0x15, // Bitwise OR A with zero page + X value
	INS_ORA_ABS		= 0x0D, // Bitwise OR A with absolute address value
	INS_ORA_ABSX	= 0x1D, // Bitwise OR A with absolute address + X value
	INS_ORA_ABSY	= 0x19, // Bitwise OR A with absolute address + Y value
	INS_ORA_INDX	= 0x01, // Bitwise OR A with indirext address + X value
	INS_ORA_INDY	= 0x11, // Bitwise OR A with indirext address + Y value

	INS_LSR_ACC		= 0x4A, // Logical shift right A
	INS_LSR_ZP		= 0xA4, // Logical shift right zero page value
	INS_LSR_ZPX		= 0xB4, // Logical shift right zero page value + X
	INS_LSR_ABS		= 0xAE, // Logical shift right absolute addrress value
	INS_LSR_ABSX	= 0xAE, // Logical shift right absolute addrress + X value

	INS_NOP			= 0xEA, // No operation

	INS_RTI			= 0x40, // Return from interrupt

	INS_BRK			= 0x00, // Break

	INS_TAX			= 0xAA, // Transfer A to X
	INS_TXA 		= 0x8A, // Transfer X to A
	INS_DEX			= 0xCA, // Decrement X
	INS_INX 		= 0xE8, // Increment X
	INS_TAY 		= 0xA8, // Transfer A to Y
	INS_TYA 		= 0x98, // Transfer Y to A
	INS_DEY			= 0x88, // Decrement Y
	INS_INY 		= 0xC8, // Increment Y

	INS_EOR_IMM		= 0x49, // Bitwise exclusive OR with immediate value
	INS_EOR_ZP		= 0x45, // Bitwise exclusive OR with zero page value
	INS_EOR_ZPX		= 0x55, // Bitwise exclusive OR with zero page + X value
	INS_EOR_ABS		= 0x4D, // Bitwise exclusive OR with absolute address value
	INS_EOR_ABSX	= 0x5D, // Bitwise exclusive OR with absolute address + X value
	INS_EOR_ABSY	= 0x59, // Bitwise exclusive OR with absolute address + Y value
	INS_EOR_INDX	= 0x41, // Bitwise exclusive OR with indirect address + X value
	INS_EOR_INDY	= 0x51, // Bitwise exclusive OR with indirect address + Y value

	INS_DEC_ZP		= 0xC6, // Decrement memory at zero page
	INS_DEC_ZPX		= 0xD6, // Decrement memory at zero page + X
	INS_DEC_ABS		= 0xCE, // Decrement memory at absolute address
	INS_DEC_ABSX	= 0xDE, // Decrement memory at absolute address + X

	INS_CPY_IMM		= 0xC0, // Compare Y immediate
	INS_CPY_ZP		= 0xC4, // Compare Y with zero page address value
	INS_CPY_ABS		= 0xCC, // Compare Y with absolute address value

	INS_CPX_IMM		= 0xE0, // Compare X immediate
	INS_CPX_ZP		= 0xE4, // Compare X with zero page address value
	INS_CPX_ABS		= 0xEC, // Compare X with absolute address value

	INS_CMP_IMM		= 0xC9, // Compare A immediate
	INS_CMP_ZP		= 0xC5, // Compare A zero page
	INS_CMP_ZPX		= 0xD5, // Compare A zero page + X
	INS_CMP_ABS		= 0xCD, // Compare A absolute address
	INS_CMP_ABSX	= 0xDD, // Compare A absolute + X
	INS_CMP_ABSY	= 0xD9, // Compare A absolute + Y
	INS_CMP_INDX	= 0xC1, // Compare A indirect address + X
	INS_CMP_INDY	= 0xD1, // Compare A indirect address + Y

	INS_BPL			= 0x10, // Brnach on plus
	INS_BMI			= 0x30, // Branch on minus
	INS_BVC			= 0x50, // Branch on overflow clear
	INS_BVS			= 0x70, // Branch on overflow set
	INS_BCC			= 0x90, // Branch on carry clear
	INS_BCS			= 0xB0, // Branch on carry set
	INS_BNE			= 0xD0, // Branch on not equal
	INS_BEQ			= 0xF0, // Branch on equal
} opcode;


/* LSR */
void LSR_ACC(cpu6502_t *cpu, ram_t *ram);
void LSR_ZP(cpu6502_t *cpu, ram_t *ram);
void LSR_ZPX(cpu6502_t *cpu, ram_t *ram);
void LSR_ABS(cpu6502_t *cpu, ram_t *ram);
void LSR_ABSX(cpu6502_t *cpu, ram_t *ram);


/* NOP */
void NOP(cpu6502_t *cpu);


/* RTI */
void RTI(cpu6502_t *cpu);


/* Break */
void BRK(cpu6502_t *cpu);


/* Register instructions */
void TAX(cpu6502_t *cpu);
void TXA(cpu6502_t *cpu);
void DEX(cpu6502_t *cpu);
void INX(cpu6502_t *cpu);
void TAY(cpu6502_t *cpu);
void TYA(cpu6502_t *cpu);
void DEY(cpu6502_t *cpu);
void INY(cpu6502_t *cpu);


/* EOR */
void EOR_IMM(cpu6502_t *cpu, ram_t *ram);
void EOR_ZP(cpu6502_t *cpu, ram_t *ram);
void EOR_ZPX(cpu6502_t *cpu, ram_t *ram);
void EOR_ABS(cpu6502_t *cpu, ram_t *ram);
void EOR_ABSX(cpu6502_t *cpu, ram_t *ram);
void EOR_ABSY(cpu6502_t *cpu, ram_t *ram);
void EOR_INDX(cpu6502_t *cpu, ram_t *ram);
void EOR_INDY(cpu6502_t *cpu, ram_t *ram);

/* DEC */
void DEC_ZP(cpu6502_t *cpu, ram_t *ram);
void DEC_ZPX(cpu6502_t *cpu, ram_t *ram);
void DEC_ABS(cpu6502_t *cpu, ram_t *ram);
void DEC_ABSX(cpu6502_t *cpu, ram_t *ram);


/* CPY */
void CPY_IMM(cpu6502_t *cpu, ram_t *ram);
void CPY_ZP(cpu6502_t *cpu, ram_t *ram);
void CPY_ABS(cpu6502_t *cpu, ram_t *ram);


/* CPX */
void CPX_IMM(cpu6502_t *cpu, ram_t *ram);
void CPX_ZP(cpu6502_t *cpu, ram_t *ram);
void CPX_ABS(cpu6502_t *cpu, ram_t *ram);


/* CMP */
void CMP_IMM(cpu6502_t *cpu, ram_t *ram);
void CMP_ZP(cpu6502_t *cpu, ram_t *ram);
void CMP_ZPX(cpu6502_t *cpu, ram_t *ram);
void CMP_ABS(cpu6502_t *cpu, ram_t *ram);
void CMP_ABSX(cpu6502_t *cpu, ram_t *ram);
void CMP_ABSY(cpu6502_t *cpu, ram_t *ram);
void CMP_INDX(cpu6502_t *cpu, ram_t *ram);
void CMP_INDY(cpu6502_t *cpu, ram_t *ram);


/* Branch */
void BPL(cpu6502_t *cpu);
void BMI(cpu6502_t *cpu);
void BVC(cpu6502_t *cpu);
void BVS(cpu6502_t *cpu);
void BCC(cpu6502_t *cpu);
void BCS(cpu6502_t *cpu);
void BNE(cpu6502_t *cpu);
void BEQ(cpu6502_t *cpu);


/* ROL */
void ROL_ACC(cpu6502_t *cpu, ram_t *ram);
void ROL_ZP(cpu6502_t *cpu, ram_t *ram);
void ROL_ZPX(cpu6502_t *cpu, ram_t *ram);
void ROL_ABS(cpu6502_t *cpu, ram_t *ram);
void ROL_ABSX(cpu6502_t *cpu, ram_t *ram);


/* ROR */
void ROR_ACC(cpu6502_t *cpu, ram_t *ram);
void ROR_ZP(cpu6502_t *cpu, ram_t *ram);
void ROR_ZPX(cpu6502_t *cpu, ram_t *ram);
void ROR_ABS(cpu6502_t *cpu, ram_t *ram);
void ROR_ABSX(cpu6502_t *cpu, ram_t *ram);

/* ORA */
void ORA_IMM(cpu6502_t *cpu, ram_t *ram);
void ORA_ZP(cpu6502_t *cpu, ram_t *ram);
void ORA_ZPX(cpu6502_t *cpu, ram_t *ram);
void ORA_ABS(cpu6502_t *cpu, ram_t *ram);
void ORA_ABSX(cpu6502_t *cpu, ram_t *ram);
void ORA_INDX(cpu6502_t *cpu, ram_t *ram);
void ORA_INDY(cpu6502_t *cpu, ram_t *ram);


/* LSR */
void LSR_ACC(cpu6502_t *cpu, ram_t *ram);
void LSR_ZP(cpu6502_t *cpu, ram_t *ram);
void LSR_ZPX(cpu6502_t *cpu, ram_t *ram);
void LSR_ABS(cpu6502_t *cpu, ram_t *ram);
void LSR_ABSX(cpu6502_t *cpu, ram_t *ram);


/* STA */
void STA_ZP(cpu6502_t *cpu, ram_t *ram);
void STA_ZPX(cpu6502_t *cpu, ram_t *ram);
void STA_ABS(cpu6502_t *cpu, ram_t *ram);
void STA_ABSX(cpu6502_t *cpu, ram_t *ram);
void STA_ABSY(cpu6502_t *cpu, ram_t *ram);
void STA_INDX(cpu6502_t *cpu, ram_t *ram);
void STA_INDY(cpu6502_t *cpu, ram_t *ram);


/* STX */
void STX_ZP(cpu6502_t *cpu, ram_t *ram);
void STX_ZPY(cpu6502_t *cpu, ram_t *ram);
void STX_ABS(cpu6502_t *cpu, ram_t *ram);


/* STY */
void STY_ZP(cpu6502_t *cpu, ram_t *ram);
void STY_ZPX(cpu6502_t *cpu, ram_t *ram);
void STY_ABS(cpu6502_t *cpu, ram_t *ram);


/* Stack operations */
void TXS(cpu6502_t *cpu, ram_t *ram);
void TSX(cpu6502_t *cpu, ram_t *ram);
void PHS(cpu6502_t *cpu, ram_t *ram);
void PLA(cpu6502_t *cpu, ram_t *ram);
void PHP(cpu6502_t *cpu, ram_t *ram);
void PLP(cpu6502_t *cpu, ram_t *ram);


/* SBC */
void SBC_IMM(cpu6502_t *cpu, ram_t *ram);
void SBC_ZP(cpu6502_t *cpu, ram_t *ram);
void SBC_ZPX(cpu6502_t *cpu, ram_t *ram);
void SBC_ABS(cpu6502_t *cpu, ram_t *ram);
void SBC_ABSX(cpu6502_t *cpu, ram_t *ram);
void SBC_ABSY(cpu6502_t *cpu, ram_t *ram);
void SBC_INDX(cpu6502_t *cpu, ram_t *ram);
void SBC_INDY(cpu6502_t *cpu, ram_t *ram);


// TODO up from here ^^^^^^^^^^^^^^^^^^^^^^^


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
