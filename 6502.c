#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

#define MEM_MAX 0xFFFF

#define ENDIAN_LITTLE \
	({int32_t x = 1; \
		*(char*) &x;})

#define ENDIAN_BIG \
	({int32_t x = 1; \
		((char*) &x)[3];})

typedef uint8_t byte;
typedef uint16_t word;

// cpu flags
typedef enum cpu_flag
{
	N = 1 << 7, // negative
	V = 1 << 6, // 
	U = 1 << 5, // unused
	B = 1 << 4, // 
	D = 1 << 3,
	I = 1 << 2,
	Z = 1 << 1,
	C = 1 << 0
} cpu_flag_t;


typedef struct ram
{
	byte data[MEM_MAX];
} ram_t;


void ram_init(ram_t *r)
{
	for(word i = 0; i < MEM_MAX; i++)
		r->data[i] = 0x0;
}

byte ram_read(ram_t *ram, word addr)
{
	if(addr >= 0 && addr <= MEM_MAX)
		return ram->data[addr];
	return 0;
}

void ram_write(ram_t *ram, word addr, byte data)
{
	if(addr >= 0 && addr <= MEM_MAX)
		ram->data[addr] = data;
}


// 6502 CPU
typedef struct cpu6502
{
	byte A, X, Y; //register a, x and y
	word SP, PC; //stack pointer and program counter
	byte status; // status bits
} cpu6502_t;

void cpu_set_flags(cpu6502_t *cpu, byte flags)
{
	cpu->status |= flags;
}

void cpu_reset_flags(cpu6502_t *cpu, byte flags)
{
	cpu->status &= ~(flags);
}

void cpu_reset(cpu6502_t *cpu, ram_t *rm)
{
	memset(cpu, 0, sizeof *cpu);
	cpu->PC = 0xFFFC;
	cpu->SP = 0x0100;
	cpu->status = 0;
	cpu->A = cpu->X = cpu->Y = 0x0;
	ram_init(rm);
}

void swap_bytes_w(word *w)
{
	byte high = (*w >> 8) & 0xFF;
	byte low = (*w & 0xFF);
	*w = (low << 8) | high;
}

// Fetch byte from RAM increasing program counter once. Takes one clock cycle.
byte cpu_fetch_byte(cpu6502_t *cpu, ram_t *ram, uint32_t *cycles)
{
	byte data = ram_read(ram, cpu->PC++);
	(*cycles)--;
	return data;
}
	
// Fetch word(16 bit) from RAM increasing program counter twice. Takes two clock cycles.
word cpu_fetch_word(cpu6502_t *cpu, ram_t *ram, uint32_t *cycles)
{
	byte high = 0x0, low = 0x0;
	if(ENDIAN_LITTLE)
	{
		low = cpu_fetch_byte(cpu, ram, cycles);
		high = cpu_fetch_byte(cpu, ram, cycles);
	}
	else
	{
		high = cpu_fetch_byte(cpu, ram, cycles);
		low = cpu_fetch_byte(cpu, ram, cycles);
	}
	return (high << 8) | low;
}

byte cpu_read_byte(ram_t *ram, word addr, uint32_t *cycles)
{
	(*cycles)--;
	return ram->data[addr];
}

word cpu_read_word(ram_t *ram, word addr, uint32_t *cycles)
{
	*cycles -= 2;

	byte high = 0x0, low = 0x0;
	if(ENDIAN_LITTLE)
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

void cpu_write_byte(ram_t *ram, word addr, byte data, uint32_t *cycles)
{
	ram->data[addr] = data;
	(*cycles)--;
}

void cpu_write_word(ram_t *ram, word addr, word data, uint32_t *cycles)
{
	byte high = (data >> 8) & 0xFF;
	byte low = (data & 0xFF);
	if(ENDIAN_LITTLE)
	{
		ram->data[addr] = low;
		ram->data[addr + 1] = high;
	}
	else
	{
		ram->data[addr] = high;
		ram->data[addr + 1] = low;
	}
	*cycles -= 2;
}

// instruction mnemonics
typedef enum ins
{
	LDA_IMM = 0xA9, // Load accumulator immediate
	LDA_ZP = 0xA5, // Load accumulator from zero page i.e memory address 0-255
	LDA_ZPX = 0xB5, // Loda accumulator by adding value of X in immediate zero page address. 
					// Syntax: LDA $80, X means Load A from 80 + valueof(X)
	LDA_ABS = 0xAD, // load from absolute address
	LDA_ABSX = 0xBD, // load accumulator absoulute addres + X
	LDA_ABSY = 0xB9, // load A absolute address + Y
	JSR = 0x20 // jump to subroutine
} ins_t;

void lda_set_status(cpu6502_t *cpu)
{
	cpu->status |= Z;
	cpu->status |= (cpu->A & N) > 0;
}
	
void cpu_execute(cpu6502_t *cpu, ram_t *ram, uint32_t cycles)
{
	while(cycles > 0)
	{
		byte data = cpu_fetch_byte(cpu, ram, &cycles);
		switch(data)
		{
		case LDA_IMM:
		{
			byte value = cpu_fetch_byte(cpu, ram, &cycles);
			cpu->A = value;
			lda_set_status(cpu);
		}
		break;

		case LDA_ZP:
		{
			byte zp_addr = cpu_fetch_byte(cpu, ram, &cycles); //zero page address
			cpu->A = cpu_read_byte(ram, zp_addr, &cycles); // read from RAM
			lda_set_status(cpu);
		}
		break;

		case LDA_ZPX:
		{
			byte imm_zp_addr = cpu_fetch_byte(cpu, ram, &cycles);
			imm_zp_addr += cpu->X;
			cycles--; // ^ for addition
			cpu->A = cpu_read_byte(ram, imm_zp_addr, &cycles);
			lda_set_status(cpu);	
		}
		break;

		case LDA_ABS:
		{
			word addr = cpu_fetch_word(cpu, ram, &cycles); // creating address
			cpu->A = cpu_read_byte(ram, addr, &cycles);
			lda_set_status(cpu);
		}
		break;

		case LDA_ABSX:
		{
			word addr = cpu_fetch_word(cpu, ram, &cycles) + cpu->X; 
							// creating address by adding X
			cpu->A = cpu_read_byte(ram, addr, &cycles);
			cycles -= 2;// for RAM access and addition of X
			lda_set_status(cpu);
		}
		break;

		case LDA_ABSY:
		{
			word addr = cpu_fetch_word(cpu, ram, &cycles) + cpu->Y;
			cpu->A = cpu_read_byte(ram, addr, &cycles);
			cycles--; // one cycle for addition
			lda_set_status(cpu);
		}
		break;

		case JSR:
		{
			word sub_addr = cpu_fetch_word(cpu, ram, &cycles);	
			word to_write = cpu->PC - 1; // to push return address onto stack
			
			cpu_write_word(ram, cpu->SP, to_write, &cycles);
			cpu->SP += 2; 		// ^^^^^^^^^^^^^^^^^^^  is written onto the stack 
			cpu->PC = sub_addr; // copy subroutine address to program counter
			cycles--; 			// 1 cycle to set program counter to subroutine address
		}
		break;
	
		default:
			printf("Invalid instruction: 0x%x\n", data);	
			exit(1);
			break;
		}
	}
}

int main(void)
{
	ram_t ram;
	ram_init(&ram);
	cpu6502_t cpu;
	cpu_reset(&cpu, &ram);

	ram.data[0xFFFC] = JSR;
	ram.data[0xFFFD] = 0x42;
	ram.data[0xFFFE] = 0x42;
	ram.data[0x4242] = LDA_IMM;
	ram.data[0x4243] = 0xc;

	cpu_execute(&cpu, &ram, 8);

	printf("%d\n", cpu.A);
	return 0;
}
