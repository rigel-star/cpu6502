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

static void dump_cpu_flags(cpu6502_t *cpu)
{
	printf("Carry: 		%d\n", cpu->status & C);
	printf("Zero: 		%d\n", cpu->status & Z ? 1 : 0);
	printf("Interrupt: 	%d\n", cpu->status & I ? 1 : 0);
	printf("Decimal: 	%d\n", cpu->status & D ? 1 : 0);
	printf("Break:		%d\n", cpu->status & B ? 1 : 0);
	printf("Unused: 	%d\n", cpu->status & U ? 1 : 0);
	printf("Overflow: 	%d\n", cpu->status & V ? 1 : 0);
	printf("Negative: 	%d\n", cpu->status & N ? 1 : 0);
}


void cpu_reset(cpu6502_t *cpu, ram_t *rm)
{
	memset(cpu, 0, sizeof *cpu);
	cpu->PC = PROG_START;
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
	cycles++;
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
	cycles++;
	return ram_read(ram, addr);
}

word cpu_read_word(ram_t *ram, word addr)
{
	cycles += 2;
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
	cycles++;
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
	cycles += 2; // for writing 2 bytes on RAM
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

// Changing status bits related to ADC operation

//MAKE THIS FUNCTION INLINE
static void adc_set_status(cpu6502_t *cpu, word res)
{
	cpu->status |= Z; // zero
	cpu->status |= (cpu->A & N); // negative
	cpu->status |= ((res > 255 || res < 0) ? C : 0); // carry
	cpu->status |= ((res > 255 || res < 0) ? V : 0); // overflow
}

void ADC_IMM(cpu6502_t *cpu, ram_t *ram)
{
	byte data = cpu_fetch_byte(cpu, ram);
	word ans = cpu->A + data + ((cpu->status & C) ? 1 : 0); // if carry flag is set, add 1
	cpu->A = (ans & 0xFF);
	adc_set_status(cpu, ans);
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
	word ans = cpu->A + data + ((cpu->status & C) ? 1 : 0); // if carry flag is set, add 1
	cpu->A = (ans & 0xFF);
	adc_set_status(cpu, ans);
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
static void perform_adc_abs(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word abs_addr = cpu_fetch_word(cpu, ram) + addr_off;
	byte data = cpu_read_byte(ram, abs_addr);
	word ans = cpu->A + data + ((cpu->status & C) ? 1 : 0); // if carry flag is set, add 1
	cpu->A = (ans & 0xFF);
	adc_set_status(cpu, ans);
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
static void perform_adc_ind(cpu6502_t *cpu, ram_t *ram, off_t addr_off)
{
	word abs_addr = cpu_fetch_word(cpu, ram);
	word ind_addr = cpu_read_word(ram, abs_addr) + addr_off;
	byte data = cpu_read_byte(ram, ind_addr);
	word ans = cpu->A + data + ((cpu->status & C) ? 1 : 0); // if carry flag is set, add 1
	cpu->A = (ans & 0xFF);
	adc_set_status(cpu, ans);
}

void ADC_INDX(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_ind(cpu, ram, cpu->X);
}

void ADC_INDY(cpu6502_t *cpu, ram_t *ram)
{
	perform_adc_ind(cpu, ram, cpu->Y);
}


static inline void cpu_set_flags(cpu6502_t *cpu, byte flags)
{
	cpu->status |= flags;
}

static inline void cpu_reset_flags(cpu6502_t *cpu, byte flags)
{
	cpu->status &= ~(flags);
}

/*
 * 
 * Increment by 1
 * 
 * */

// Changing status bits related to ASL operation
static void inc_set_status(cpu6502_t *cpu)
{
	cpu->status |= Z;
	cpu->status |= (cpu->A & N);
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
	cpu->status |= (data & N); // 7th bit goes to N flag
	cpu->status |= (data & V); // 6th bit goes to V flag
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
	cpu->PC = abs_addr;
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
	cpu->status |= (cpu->A & N);
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
	byte imm_zp_addr = cpu_fetch_byte(cpu, ram);
	imm_zp_addr += cpu->X;
	cycles++; // ^ for addition
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
	cycles++;
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

		case INS_AND_IMM:
			AND_IMM(cpu, ram);
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

		case INS_KIL:
			process_complete = true;
		break;
	
		default:
			printf("Invalid instruction: 0x%x\n", opcode);
			exit(1);
			break;
		}
	}
}

void load_into_memory(ram_t *ram, const char *fname)
{
	// (void) ram;
	int32_t fd = open(fname, O_RDONLY);
	if(fd < 0)
	{
		fprintf(stderr, "File not found %s\n", fname);
		exit(1);
	}
	
	struct stat stat_buf;
	if(fstat(fd, &stat_buf) == -1)
	{
		perror("stat");
		exit(2);
	}
	
	size_t LEN = stat_buf.st_size; // length of file in bytes
	byte buffer[LEN];
	memset(buffer, 0, sizeof buffer);

	if(read(fd, buffer, LEN) < 0)
	{
		perror("read");
		exit(3);
	}
	
	word bidx = 0;
	for(word i = PROG_START; i < PROG_START + LEN; i++)
		ram->data[i] = buffer[bidx++];

	close(fd);
}

int main(void)
{
	ram_t ram;
	ram_init(&ram);
	cpu6502_t cpu;
	cpu_reset(&cpu, &ram);

	load_into_memory(&ram, "./src/small.ef");

	cpu_execute(&cpu, &ram);
	printf("A: %d\n", cpu.A);
	printf("Number of cycles: %d\n", cycles);
	dump_cpu_flags(&cpu);
	return 0;
}
