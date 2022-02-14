#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "cpu6502.h"
#include "ef.h"

static void dump_cpu_flags(cpu6502_t *cpu)
{
	puts("\nFlags: ");
	printf("Carry: 		%d\n", cpu->status & C);
	printf("Zero: 		%d\n", cpu->status & Z ? 1 : 0);
	printf("Interrupt: 	%d\n", cpu->status & I ? 1 : 0);
	printf("Decimal: 	%d\n", cpu->status & D ? 1 : 0);
	printf("Break:		%d\n", cpu->status & B ? 1 : 0);
	printf("Unused: 	%d\n", cpu->status & U ? 1 : 0);
	printf("Overflow: 	%d\n", cpu->status & V ? 1 : 0);
	printf("Negative: 	%d\n", cpu->status & N ? 1 : 0);
}

static void dump_cpu_regs(cpu6502_t *cpu)
{
	puts("\nRegisters: ");
	printf("A:	0x%x\n", cpu->A);
	printf("X:	0x%x\n", cpu->X);
	printf("Y:	0x%x\n", cpu->Y);
	printf("PC: 	0x%x\n", cpu->PC);
	printf("SP: 	0x%x\n", cpu->SP);
}

void cpu_reset(cpu6502_t *cpu, ram_t *rm)
{
	memset(cpu, 0, sizeof *cpu);
	cpu->PC = PROG_BEGIN;
	cpu->SP = PAGE_SIZE;
	cpu->status = 0;
	cpu->A = cpu->X = cpu->Y = 0x0;
	ram_init(rm);
}

static uint32_t cycles = 0;

// Fetch byte from RAM increasing program counter once. Takes one clock cycle.
byte cpu_fetch_byte(cpu6502_t *cpu, ram_t *ram)
{
	byte data = ram_read(ram, cpu->PC++);
	return data;
}

// Fetch word(16 bit) from RAM increasing program counter twice. Takes two clock cycles.
word cpu_fetch_word(cpu6502_t *cpu, ram_t *ram)
{
	byte high = 0x0, low = 0x0;
	if(__ORDER_LITTLE_ENDIAN__)
	{
		low = cpu_fetch_byte(cpu, ram);
		high = cpu_fetch_byte(cpu, ram);
	}
	else
	{
		high = cpu_fetch_byte(cpu, ram);
		low = cpu_fetch_byte(cpu, ram);
	}
	return (high << 8) | low;
}

byte cpu_read_byte(ram_t *ram, word addr)
{
	return ram_read(ram, addr);
}

word cpu_read_word(ram_t *ram, word addr)
{
	byte high = 0x0, low = 0x0;
	if(__ORDER_LITTLE_ENDIAN__)
	{
		low = ram_read(ram, addr);
		high = ram_read(ram, addr + 1);
	}
	else
	{
		high = ram_read(ram, addr);
		low = ram_read(ram, addr + 1);
	}
	return (high << 8) | low;
}

void cpu_write_byte(ram_t *ram, word addr, byte data)
{
	ram_write(ram, addr, data);
}

void cpu_write_word(ram_t *ram, word addr, word data)
{
	byte high = (data >> 8) & 0xFF;
	byte low = (data & 0xFF);
	if(__ORDER_LITTLE_ENDIAN__)
	{
		ram_write(ram, addr, low);
		ram_write(ram, addr + 1, high);
	}
	else
	{
		ram_write(ram, addr, high);
		ram_write(ram, addr + 1, low);
	}
}

void cpu_push_stack_byte(cpu6502_t *cpu, ram_t *ram, byte data)
{
	cpu_write_byte(ram, STACK_BEGIN + cpu->SP, data);
	cpu->SP--;
}

void cpu_push_stack_word(cpu6502_t *cpu, ram_t *ram, word data)
{
	byte high = (data >> 8) & 0xFF;
	byte low = (data & 0xFF);
	if(__ORDER_LITTLE_ENDIAN__)
	{
		cpu_write_byte(ram, STACK_BEGIN + cpu->SP, low);
		cpu->SP--;
		cpu_write_byte(ram, STACK_BEGIN + cpu->SP, high);
		cpu->SP--;
	}
	else
	{
		cpu_write_byte(ram, STACK_BEGIN + cpu->SP, high);
		cpu->SP--;
		cpu_write_byte(ram, STACK_BEGIN + cpu->SP, low);
		cpu->SP--;
	}
}

byte cpu_pop_stack_byte(cpu6502_t *cpu, ram_t *ram)
{
	cpu->SP++;
	return cpu_read_byte(ram, cpu->SP);
}

word cpu_pop_stack_word(cpu6502_t *cpu, ram_t *ram)
{
	byte high = 0x0, low = 0x0;
	cpu->SP++;
	if(__ORDER_LITTLE_ENDIAN__)
	{
		high = cpu_read_byte(ram, STACK_BEGIN + cpu->SP);
		cpu->SP++;
		low = cpu_read_byte(ram, STACK_BEGIN + cpu->SP);
	}
	else
	{
		low = cpu_read_byte(ram, STACK_BEGIN + cpu->SP);
		cpu->SP++;
		high = cpu_read_byte(ram, STACK_BEGIN + cpu->SP);
	}
	return (high << 8) | low;
}

/*
 *
 * Add with carry
 *
 * */

static inline void perform_adc(cpu6502_t *cpu, byte fetched)
{
	word tmp = fetched + cpu->A + (cpu->status & C ? 1 : 0);
	cpu->status |= (tmp & 0x00FF) == 0 ? Z : 0; // zero
	cpu->status |= (tmp & 0x0080) == 1 ? N : 0; // negative
	cpu->status |= (tmp > 0x00FF) == 1 ? C : 0;
	cpu->status |= ((~(cpu->A) ^ fetched) & (cpu->A ^ tmp) & 0x0080) ? V : 0;
	cpu->A = (byte) (tmp & 0x00FF);
}

void ADC_IMM(cpu6502_t *cpu, ram_t *ram)
{
	byte data = cpu_fetch_byte(cpu, ram);
	perform_adc(cpu, data);
}


/*
 *
 *  assembler	opcode
 *  ---------   ------
 * 	ADC zp		65
 *	ADC zp, X	75
 *
 * */
static void perform_adc_zp(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram) + addr_off;
	byte data = cpu_read_byte(ram, zp_addr);
	perform_adc(cpu, data);
}

void ADC_ZP(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_zp(cpu, ram, 0);
}

void ADC_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_zp(cpu, ram, cpu->X);
}

/*
 *
 *  assembler	opcode
 *  ---------   ------
 * 	ADC abs		6D
 *	ADC abs, X	7D
 *	ADC abs, Y	79
 *
 * */
static inline void perform_adc_abs(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + addr_off;
	byte data = cpu_read_byte(ram, abs_addr);
	perform_adc(cpu, data);
}

void ADC_ABS(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_abs(cpu, ram, 0);
}

void ADC_ABSX(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_abs(cpu, ram, cpu->X);
}

void ADC_ABSY(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_abs(cpu, ram, cpu->Y);
}


/*
 *
 *  assembler		opcode
 *  ---------   	------
 * 	ADC (ind, x)	61
 *	ADC (ind, y)	71
 *
 * */
static inline void perform_adc_ind(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	word ind_addr = cpu_read_word(ram, abs_addr) + addr_off;
	byte data = cpu_read_byte(ram, ind_addr);
	perform_adc(cpu, data);
}

void ADC_INDX(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_ind(cpu, ram, cpu->X);
}

void ADC_INDY(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_ind(cpu, ram, cpu->Y);
}

/*
 *
 * Increment by 1
 *
 * */

// Changing status bits related to ASL operation
static inline void inc_set_status(cpu6502_t *cpu)
{
	CPU_SET_FLAGS(cpu, Z | (cpu->A & N ? N : 0));
}

// increment memory at zero page memory location
void INC_ZP(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram);
	cpu_write_byte(ram, zp_addr, ram_read(ram, zp_addr) + 1);
	inc_set_status(cpu);
}

// increment memory at zero page memory location + X
void INC_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram);
	cpu_write_byte(ram, zp_addr + cpu->X, ram_read(ram, zp_addr) + 1);
	inc_set_status(cpu);
}

// increment memory at absolute memory location
void INC_ABS(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	cpu_write_byte(ram, abs_addr, ram_read(ram, abs_addr) + 1);
	inc_set_status(cpu);
}

// increment memory at absolute memory location + X
void INC_ABSX(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	cpu_write_byte(ram, abs_addr + cpu->X, ram_read(ram, abs_addr) + 1);
	inc_set_status(cpu);
}

/*
 *
 * Test bit
 *
 * */

// Changing status bits related to BIT operation
void bit_set_status(cpu6502_t *cpu, byte data)
{
	// cpu->status |= (data & N); // 7th bit goes to N flag
	// cpu->status |= (data & V); // 6th bit goes to V flag
	CPU_SET_FLAGS(cpu, (data & N) | (data & V)); // 7th bit goes to N flag
												// 6th bit goes to V flag
}

void BIT_ZP(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram);
	byte data = cpu_read_byte(ram, zp_addr);
	bit_set_status(cpu, data);

}

void BIT_ABS(cpu6502_t *cpu, ram_t *ram)
{
	byte abs_addr = cpu_fetch_word(cpu, ram);
	byte data = cpu_read_byte(ram, abs_addr);
	bit_set_status(cpu, data);
}


/*
 *
 * Shift left by 1
 *
 * */

// Changing status bits related to ASL operation
void asl_set_status(cpu6502_t *cpu)
{
	cpu->status |= Z;
	cpu->status |= (cpu->A & N);
	cpu->status |= (cpu->A >> 7) & C;
}

void ASL_A(cpu6502_t *cpu)
{
	cpu->A <<= 1;
	asl_set_status(cpu);
}

void ASL_ZP(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram);
	byte data = cpu_read_byte(ram, zp_addr);
	cpu_write_byte(ram, zp_addr, data << 1);
	asl_set_status(cpu);
}

void ASL_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram);
	byte data = cpu_read_byte(ram, zp_addr + cpu->X);
	cpu_write_byte(ram, zp_addr, data << 1);
	asl_set_status(cpu);
}

void ASL_ABS(cpu6502_t *cpu, ram_t *ram)
{
	byte abs_addr = cpu_fetch_word(cpu, ram);
	byte data = cpu_read_byte(ram, abs_addr);
	cpu_write_byte(ram, abs_addr, data << 1);
	asl_set_status(cpu);
}

void ASL_ABSX(cpu6502_t *cpu, ram_t *ram)
{
	byte abs_addr = cpu_fetch_word(cpu, ram);
	byte data = cpu_read_byte(ram, abs_addr + cpu->X);
	cpu_write_byte(ram, abs_addr, data << 1);
	asl_set_status(cpu);
}

/*
 *
 * AND with accumulator
 *
 * */

// Changing status bits related to AND operation
void and_set_status(cpu6502_t *cpu)
{
	cpu->status |= Z;
	cpu->status |= (cpu->A & N);
}

void AND_IMM(cpu6502_t *cpu, ram_t *ram)
{
	byte data = cpu_fetch_byte(cpu, ram);
	cpu->A &= data;
	and_set_status(cpu);
}

void AND_ZP(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram);
	cpu->A &= cpu_read_byte(ram, zp_addr);
	and_set_status(cpu);
}

void AND_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram) + cpu->X;
	cpu->A &= cpu_read_byte(ram, zp_addr);
	and_set_status(cpu);
}

/*
 *
 * ABS
 *
 * */
static void perform_and_abs(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + addr_off;
	cpu->A &= cpu_read_byte(ram, abs_addr);
	and_set_status(cpu);
}

void AND_ABS(cpu6502_t *cpu, ram_t *ram)
{
	perform_and_abs(cpu, ram, 0);
}

void AND_ABSX(cpu6502_t *cpu, ram_t *ram)
{
	perform_and_abs(cpu, ram, cpu->X);
}

void AND_ABSY(cpu6502_t *cpu, ram_t *ram)
{
	perform_and_abs(cpu, ram, cpu->Y);
}


/*
 *
 * AND (ind, x/y)
 *
 * */
static void perform_and_ind(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + addr_off;
	word ind_addr = cpu_read_word(ram, abs_addr);
	cpu->A &= cpu_read_byte(ram, ind_addr);
	and_set_status(cpu);
}

void AND_INDX(cpu6502_t *cpu, ram_t *ram)
{
	perform_and_ind(cpu, ram, cpu->X);
}

void AND_INDY(cpu6502_t *cpu, ram_t *ram)
{
	perform_and_ind(cpu, ram, cpu->Y);
}


/*
 *
 * Unconditional Jump
 *
 * JMP does not change any status bit
 * */

void JMP_ABS(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	printf("[PC] From: 0x%x to: ", cpu->PC);
	cpu->PC = abs_addr;
	printf("0x%x\n", cpu->PC);
}

void JMP_IND(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	word ind_addr = cpu_read_word(ram, abs_addr);
	cpu->PC = ind_addr;
}

/*
 *
 * Load Accumulator
 *
 * */

// Changing status bits related to LDA operation
void lda_set_status(cpu6502_t *cpu)
{
	cpu->status |= Z;
	cpu->status |= (cpu->A & N) ? N : 0;
}

void LDA_IMM(cpu6502_t *cpu, ram_t *ram)
{
	byte data = cpu_fetch_byte(cpu, ram);
	cpu->A = data;
	lda_set_status(cpu);
}

void LDA_ZP(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram); //zero page address
	cpu->A = cpu_read_byte(ram, zp_addr); // read from RAM
	lda_set_status(cpu);
}

void LDA_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	byte imm_zp_addr = cpu_fetch_byte(cpu, ram);
	imm_zp_addr += cpu->X;
	cycles++; // ^ for addition
	cpu->A = cpu_read_byte(ram, imm_zp_addr);
	lda_set_status(cpu);
}

void LDA_ABS(cpu6502_t *cpu, ram_t *ram)
{
	word addr = cpu_fetch_word(cpu, ram); // creating address
	cpu->A = cpu_read_byte(ram, addr);
	lda_set_status(cpu);
}

void LDA_ABSX(cpu6502_t *cpu, ram_t *ram)
{
	word addr = cpu_fetch_word(cpu, ram) + cpu->X; // creating address by adding X
	cpu->A = cpu_read_byte(ram, addr);
	cycles++;
	lda_set_status(cpu);
}

void LDA_ABSY(cpu6502_t *cpu, ram_t *ram)
{
	word addr = cpu_fetch_word(cpu, ram) + cpu->Y; // creating address by adding Y
	cpu->A = cpu_read_byte(ram, addr);
	cycles++;
	lda_set_status(cpu);
}

void LDA_INDX(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + cpu->X; // creating address by adding X
	word ind_addr = cpu_read_word(ram, abs_addr);
	cpu->A = cpu_read_byte(ram, ind_addr);
	cycles++;
	lda_set_status(cpu);
}

void LDA_INDY(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + cpu->Y; // creating address by adding Y
	word ind_addr = cpu_read_word(ram, abs_addr);
	cpu->A = cpu_read_byte(ram, ind_addr);
	cycles++;
	lda_set_status(cpu);
}

/*
 *
 * Load X
 *
 * LDX affects same flags as LDA
 *
 * */
void LDX_IMM(cpu6502_t *cpu, ram_t *ram)
{
	byte data = cpu_fetch_byte(cpu, ram);
	cpu->X = data;
	lda_set_status(cpu); //TODO::: same as LDA for now
}

void LDX_ZP(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram); //zero page address
	cpu->X = cpu_read_byte(ram, zp_addr); // read from RAM
	lda_set_status(cpu);
}

void LDX_ZPY(cpu6502_t *cpu, ram_t *ram)
{
	byte imm_zp_addr = cpu_fetch_byte(cpu, ram);
	imm_zp_addr += cpu->Y;
	cycles++; // ^ for addition
	cpu->X = cpu_read_byte(ram, imm_zp_addr);
	lda_set_status(cpu);
}

void LDX_ABS(cpu6502_t *cpu, ram_t *ram)
{
	word addr = cpu_fetch_word(cpu, ram); // creating address
	cpu->X = cpu_read_byte(ram, addr);
	lda_set_status(cpu);
}

void LDX_ABSY(cpu6502_t *cpu, ram_t *ram)
{
	word addr = cpu_fetch_word(cpu, ram) + cpu->Y; // creating address by adding Y
	cpu->X = cpu_read_byte(ram, addr);
	cycles++;
	lda_set_status(cpu);
}


/*
 *
 * Load Y
 *
 * LDY affects same flags as LDA
 *
 * */
 void LDY_IMM(cpu6502_t *cpu, ram_t *ram)
{
	byte data = cpu_fetch_byte(cpu, ram);
	cpu->Y = data;
	lda_set_status(cpu); //TODO::: same as LDA for now
}

void LDY_ZP(cpu6502_t *cpu, ram_t *ram)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram); //zero page address
	cpu->Y = cpu_read_byte(ram, zp_addr); // read from RAM
	lda_set_status(cpu);
}

void LDY_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	byte imm_zp_addr = cpu_fetch_byte(cpu, ram) + cpu->X;
	cpu->Y = cpu_read_byte(ram, imm_zp_addr);
	lda_set_status(cpu);
}

void LDY_ABS(cpu6502_t *cpu, ram_t *ram)
{
	word addr = cpu_fetch_word(cpu, ram); // creating address
	cpu->Y = cpu_read_byte(ram, addr);
	lda_set_status(cpu);
}

void LDY_ABSX(cpu6502_t *cpu, ram_t *ram)
{
	word addr = cpu_fetch_word(cpu, ram) + cpu->X; // creating address by adding Y
	cpu->Y = cpu_read_byte(ram, addr);
	lda_set_status(cpu);
}

/*
 *
 * Jump to subroutine
 *
 * */

void JSR(cpu6502_t *cpu, ram_t *ram)
{
	word sub_addr = cpu_fetch_word(cpu, ram);
	cpu_push_stack_word(cpu, ram, cpu->PC - 1);
	cpu->PC = sub_addr; // copy subroutine address to program counter
	cycles++; 			// 1 cycle to set program counter to subroutine address
}

/*
 *
 * Return from subroutine
 *
 * */

void RTS(cpu6502_t *cpu, ram_t *ram)
{
	word addr = cpu_pop_stack_word(cpu, ram);
	cpu->PC = addr + 1;
	cycles += 2; // for setting PC to return address
}


/*
 * SBC
*/
static inline void perform_sbc(cpu6502_t *cpu, byte fetched)
{
	/*
	Source:: https://www.reddit.com/r/EmuDev/comments/k5hzuo/6502_sbc/
	-----------------------------------------------------------------
	"The 6502 probably uses the same adder circuit for addition as it
	does for subtraction. When subtracting, the subtrahend is first
	converted to twos compliment (invert all bits + 1), then added to
	the minuend. When you add two numbers like this, it effectively
	subtracts them. It works because the sum overflows and leaves the
	remainder, which is why the carry flag is set."
	*/
	#if 1
	word negate = (((word) fetched) ^ 0x00FF) + 1; // to two's compliment
	word tmp = ((word) cpu->A) + negate + ((cpu->status & C) ? 1 : 0);
	cpu->status |= (tmp >> 0x7) & 0x1 ? C : 0;
	cpu->status |= (tmp & 0x0080) ? N : 0;
	cpu->status |= (tmp ^ (word) cpu->A) & (tmp ^ negate) & 0x0080 ? V : 0;
	cpu->A = tmp & 0x00FF;
	#else
	perform_adc(cpu, ~(fetched));
	#endif
}

void SBC_IMM(cpu6502_t *cpu, ram_t *ram)
{
	byte data = cpu_fetch_byte(cpu, ram);
	perform_sbc(cpu, data);
}

static inline void perform_sbc_zp(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	byte zp_addr = cpu_fetch_byte(cpu, ram) + addr_off;
	byte data = cpu_read_byte(ram, zp_addr);
	perform_sbc(cpu, data);
}

void SBC_ZP(cpu6502_t *cpu, ram_t *ram)
{
	perform_sbc_zp(cpu, ram, 0);
}

void SBC_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	perform_sbc_zp(cpu, ram, cpu->X);
}

static inline void perform_sbc_abs(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + addr_off;
	byte data = cpu_read_byte(ram, abs_addr);
	perform_sbc(cpu, data);
}

void SBC_ABS(cpu6502_t *cpu, ram_t *ram)
{
	perform_sbc_abs(cpu, ram, 0);
}

void SBC_ABSX(cpu6502_t *cpu, ram_t *ram)
{
	perform_sbc_abs(cpu, ram, cpu->X);
}

void SBC_ABSY(cpu6502_t *cpu, ram_t *ram)
{
	perform_sbc_abs(cpu, ram, cpu->Y);
}

static inline void perform_sbc_ind(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word ind_addr = cpu_fetch_word(cpu, ram) + addr_off;
	word abs_addr = cpu_read_word(ram, ind_addr);
	byte data = cpu_read_byte(ram, abs_addr);
	perform_sbc(cpu, data);
}

void SBC_INDX(cpu6502_t *cpu, ram_t *ram)
{
	perform_sbc_ind(cpu, ram, cpu->X);
}

void SBC_INDY(cpu6502_t *cpu, ram_t *ram)
{
	perform_sbc_ind(cpu, ram, cpu->Y);
}


/*
 *
 * Stack operations
 *
 */
void TXS(cpu6502_t *cpu)
{
	cpu->SP = cpu->X;
}

void TSX(cpu6502_t *cpu)
{
	cpu->X = cpu->SP;
}

void PHA(cpu6502_t *cpu, ram_t *ram)
{
	cpu_push_stack_byte(cpu, ram, cpu->A);
}

void PLA(cpu6502_t *cpu, ram_t *ram)
{
	cpu->A = cpu_pop_stack_byte(cpu, ram);
}

void PHP(cpu6502_t *cpu, ram_t *ram)
{
	cpu_push_stack_byte(cpu, ram, cpu->status);
}

void PLP(cpu6502_t *cpu, ram_t *ram)
{
	cpu->status = cpu_pop_stack_byte(cpu, ram);
}

/*
 *
 * STA - Store Accumulator in Memory
 *
 */
void STA_ZP(cpu6502_t *cpu, ram_t *ram)
{
	word zp_addr = cpu_fetch_byte(cpu, ram);
	cpu_write_byte(ram, zp_addr, cpu->A);
}

void STA_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	word zp_addr = cpu_fetch_byte(cpu, ram) + cpu->X;
	cpu_write_byte(ram, zp_addr, cpu->A);
}

void STA_ABS(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	cpu_write_byte(ram, abs_addr, cpu->A);
}

void STA_ABSX(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + cpu->X;
	cpu_write_byte(ram, abs_addr, cpu->A);
}

void STA_ABSY(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + cpu->Y;
	cpu_write_byte(ram, abs_addr, cpu->A);
}

/* STA indirect */
static inline void perform_sta_ind(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word ind_addr = cpu_fetch_word(cpu, ram) + addr_off;
	word abs_addr = cpu_read_word(ram, ind_addr);
	cpu_write_byte(ram, abs_addr, cpu->A);
}

void STA_INDX(cpu6502_t *cpu, ram_t *ram)
{
	perform_sta_ind(cpu, ram, cpu->X);
}

void STA_INDY(cpu6502_t *cpu, ram_t *ram)
{
	perform_sta_ind(cpu, ram, cpu->Y);
}

/* STX */
void STX_ZP(cpu6502_t *cpu, ram_t *ram)
{
	word zp_addr = cpu_fetch_byte(cpu, ram);
	cpu_write_byte(ram, zp_addr, cpu->X);
}

void STX_ZPY(cpu6502_t *cpu, ram_t *ram)
{
	word zp_addr = cpu_fetch_byte(cpu, ram) + cpu->Y;
	cpu_write_byte(ram, zp_addr, cpu->X);
}

void STX_ABS(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	cpu_write_byte(ram, abs_addr, cpu->X);
}


/* STY */
void STY_ZP(cpu6502_t *cpu, ram_t *ram)
{
	word zp_addr = cpu_fetch_byte(cpu, ram);
	cpu_write_byte(ram, zp_addr, cpu->Y);
}

void STY_ZPX(cpu6502_t *cpu, ram_t *ram)
{
	word zp_addr = cpu_fetch_byte(cpu, ram) + cpu->X;
	cpu_write_byte(ram, zp_addr, cpu->Y);
}

void STY_ABS(cpu6502_t *cpu, ram_t *ram)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	cpu_write_byte(ram, abs_addr, cpu->Y);
}

void NOP()
{
	// no operation
}

/*
 *
 * Register instructions
 *
 */
void TAX(cpu6502_t *cpu)
{
	cpu->A = cpu->X;
}

void TXA(cpu6502_t *cpu)
{
	cpu->X = cpu->A;
}

void DEX(cpu6502_t *cpu)
{
	cpu->X--;
}

void INX(cpu6502_t *cpu)
{
	cpu->X++;
}

void TAY(cpu6502_t *cpu)
{
	cpu->A = cpu->Y;
}

void TYA(cpu6502_t *cpu)
{
	cpu->Y = cpu->A;
}

void DEY(cpu6502_t *cpu)
{
	cpu->Y--;
}

void INY(cpu6502_t *cpu)
{
	cpu->Y++;
}



/*
 *
 * Execute instruction from memory
 *
 * */

void cpu_execute(cpu6502_t *cpu, ram_t *ram)
{
	bool process_complete = false;
	while(!process_complete)
	{
		byte opcode = cpu_fetch_byte(cpu, ram);
		switch(opcode)
		{
		case INS_LDA_IMM:
			LDA_IMM(cpu, ram);
		break;

		case INS_LDA_ZP:
			LDA_ZP(cpu, ram);
		break;

		case INS_LDA_ZPX:
			LDA_ZPX(cpu, ram);
		break;

		case INS_LDA_ABS:
			LDA_ABS(cpu, ram);
		break;

		case INS_LDA_ABSX:
			LDA_ABSX(cpu, ram);
		break;

		case INS_LDA_ABSY:
			LDA_ABSY(cpu, ram);
		break;

		case INS_LDA_INDX:
			LDA_INDX(cpu, ram);
			break;

		case INS_LDA_INDY:
			LDA_INDY(cpu, ram);
			break;

		case INS_LDX_IMM:
			LDX_IMM(cpu, ram);
			break;

		case INS_LDX_ZP:
			LDX_ZP(cpu, ram);
			break;

		case INS_LDX_ZPY:
			LDX_ZPY(cpu, ram);
			break;

		case INS_LDX_ABS:
			LDX_ABS(cpu, ram);
			break;

		case INS_LDX_ABSY:
			LDX_ABSY(cpu, ram);
			break;

		case INS_LDY_IMM:
			LDY_IMM(cpu, ram);
			break;

		case INS_LDY_ZP:
			LDY_ZP(cpu, ram);
			break;

		case INS_LDY_ZPX:
			LDY_ZPX(cpu, ram);
			break;

		case INS_LDY_ABS:
			LDY_ABS(cpu, ram);
			break;

		case INS_LDY_ABSX:
			LDY_ABSX(cpu, ram);
			break;

		case INS_JSR:
			JSR(cpu, ram);
			break;

		case INS_RTS:
			RTS(cpu, ram);
			break;

		case INS_ADC_IMM:
			ADC_IMM(cpu, ram);
			break;

		case INS_ADC_ZP:
			ADC_ZP(cpu, ram);
			break;

		case INS_ADC_ZPX:
			ADC_ZPX(cpu, ram);
			break;

		case INS_ADC_ABS:
			ADC_ABS(cpu, ram);
			break;

		case INS_ADC_ABSX:
			ADC_ABSX(cpu, ram);
			break;

		case INS_ADC_ABSY:
			ADC_ABSY(cpu, ram);
			break;

		case INS_ADC_INDX:
			ADC_INDX(cpu, ram);
			break;

		case INS_INC_ZP:
			INC_ZP(cpu, ram);
			break;

		case INS_INC_ZPX:
			INC_ZPX(cpu, ram);
			break;

		case INS_INC_ABS:
			INC_ABS(cpu, ram);
			break;

		case INS_INC_ABSX:
			INC_ABSX(cpu, ram);
			break;

		case INS_CLC:
			CLC(cpu);
			break;

		case INS_SEC:
			SEC(cpu);
			break;

		case INS_CLI:
			CLI(cpu);
			break;

		case INS_SEI:
			SEI(cpu);
			break;

		case INS_CLV:
			CLV(cpu);
			break;

		case INS_CLD:
			CLD(cpu);
			break;

		case INS_SED:
			SED(cpu);
			break;

		case INS_BIT_ZP:
			BIT_ZP(cpu, ram);
			break;

		case INS_BIT_ABS:
			BIT_ABS(cpu, ram);
			break;

		case INS_AND_IMM:
			AND_IMM(cpu, ram);
			break;

		case INS_AND_ZP:
			AND_ZP(cpu, ram);
			break;

		case INS_AND_ZPX:
			AND_ZPX(cpu, ram);
			break;

		case INS_AND_ABS:
			AND_ABS(cpu, ram);
			break;

		case INS_AND_ABSX:
			AND_ABSX(cpu, ram);
			break;

		case INS_AND_ABSY:
			AND_ABSY(cpu, ram);
			break;

		case INS_AND_INDX:
			AND_INDX(cpu, ram);
			break;

		case INS_AND_INDY:
			AND_INDY(cpu, ram);
			break;

		case INS_JMP_ABS:
			JMP_ABS(cpu, ram);
			break;

		case INS_JMP_IND:
			JMP_IND(cpu, ram);
			break;

		case INS_ASL_A:
			ASL_A(cpu);
			break;

		case INS_ASL_ZP:
			ASL_ZP(cpu, ram);
			break;

		case INS_ASL_ZPX:
			ASL_ZPX(cpu, ram);
			break;

		case INS_ASL_ABS:
			ASL_ABS(cpu, ram);
			break;

		case INS_ASL_ABSX:
			ASL_ABSX(cpu, ram);
			break;

		case INS_SBC_IMM:
			SBC_IMM(cpu, ram);
			break;

		case INS_SBC_ZP:
			SBC_ZP(cpu, ram);
			break;

		case INS_SBC_ZPX:
			SBC_ZPX(cpu, ram);
			break;

		case INS_SBC_ABS:
			SBC_ABS(cpu, ram);
			break;

		case INS_SBC_ABSX:
			SBC_ABSX(cpu, ram);
			break;

		case INS_SBC_ABSY:
			SBC_ABSY(cpu, ram);
			break;

		case INS_SBC_INDX:
			SBC_INDX(cpu, ram);
			break;

		case INS_SBC_INDY:
			SBC_INDY(cpu, ram);
			break;

		case INS_KIL:
			cpu->PC++;
			process_complete = true;
			break;

		case INS_NOP:
			NOP(cpu);
			break;

		default:
			printf("Invalid instruction: 0x%x\n", opcode);
			ram_free(ram);
			exit(1);
			break;
		}
	}
}

void load_into_memory(ram_t *ram, const char *fname)
{
	ef_file hdr = read_ef(fname);
	const word MN = (hdr.ef_magic[0] << 8) | hdr.ef_magic[1];

#define MAGIC_NUMBER (word) (('E' << 8) | 'F')
	if(MN != MAGIC_NUMBER)
	{
		fprintf(stderr, "Invalid EF file\n");
		exit(1);
	}

	printf("EF file info:\n");
	printf("Magic: %c %c\n", hdr.ef_magic[0], hdr.ef_magic[1]);
	printf("Size: %d\n", hdr.ef_size);
	printf("_start:\n \t%s\n", hdr.ef_data);

	// putting jump instruction manually for debugging purpose
	word begin = PROG_BEGIN;
	ram->data[begin++] = INS_JMP_ABS;
	ram->data[begin++] = 0x00;
	ram->data[begin] = 0x10;

#define EXEC_START 0x1000

	word bidx = 0;
	word i = EXEC_START;
	for(; i < EXEC_START + hdr.ef_size; i++)
	{
		ram->data[i] = hdr.ef_data[bidx++];
	}

	// putting exit instruction manually for debugging purpose
	// ram->data[i] = INS_KIL;
	free_ef(&hdr);
}

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Insufficient arguments\n");
		exit(1);
	}

	ram_t ram;
	cpu6502_t cpu;

	ram_init(&ram);
	cpu_reset(&cpu, &ram);
	load_into_memory(&ram, argv[1]);
	cpu_execute(&cpu, &ram);
	dump_cpu_flags(&cpu);
	dump_cpu_regs(&cpu);
	ram_free(&ram);
	return 0;
}
