from os import path
import sys
from collections import namedtuple
from bytes_op import btol, ltob

# Every .ef file will begin with this magic number as first, second and third byte
MAGIC_NUMBER = 0x076566

INS_LOOKUP_TABLE = {
	"lda" : 
		{
			"imm":	[0xA9],
			"zp":	[0xA5],
			"abs": 	[0xAD],
		},
	"adc" : 
		{
			"imm": [0x69]
		}
}


def strip_comments(line):
	comment_index = line.find(";")
	if comment_index >= 0:
		line = line[:comment_index]
	return line.strip()


def parse_addr_mode(line):
	tokens = line.split(" ", 1)	
	if len(tokens) == 1:
		return "imp"

	operand = tokens[1]

	if operand.startswith("#"):
		return "imm"
	elif operand.startswith("%"):
		if operand.__contains__(","):
			oprs = operand.split(",", 1)
			if oprs[1].lower().endswith("x"):
				return "zpx"
			elif oprs[1].lower().endswith("y"):
				return "zpy"
		else:
			return "zp"
	elif operand.startswith("["):
		if operand.__contains__(","):
			oprs = operand.split(",", 1)
			if oprs[1].lower().endswith("x]"):
				return "indx"
			elif oprs[1].lower().endswith("y]"):
				return "indy"
		else:
			return "ind"
	else:
		if operand.lower().endswith("x"):
			return "absx"
		elif operand.lower().endswith("y"):
			return "absy"
		else:
			return "abs"
	return None	

def parse_operand_as_bytes(operand, mode):
	if mode == "imp":
		return None
	elif mode == "imm":
		return operand[1:]
	elif mode == "zp":
		return operand[1:]
	elif mode == "zpx" or mode == "zpy":
		comma_index = operand.find(",")
		return operand[1:comma_index]
	elif mode == "ind":
		closing_brace_index = operand.find("]")
		return operand[1:closing_brace_index]
	elif mode == "indx" or mode == "indy":
		comma_index = operand.find(",")
		return operand[1:comma_index]
	elif mode == "abs":
		abs_addr = int(operand)
		return [(abs_addr >> 8) & 0xFF, abs_addr & 0xFF]
	elif mode == "absx" or mode == "absy":
		comma_index = operand.find(",")
		return operand[:comma_index]

# Assemble single line
def parse_instr_line(instruction_line):
	pure_instr = strip_comments(instruction_line)
	tokens = pure_instr.split(" ", 1)
	mnemonic = tokens[0]
	mode = parse_addr_mode(pure_instr)
	operands = tokens[1]
	print(parse_operand_as_bytes(operands, mode))

# Assembling starts here
def assemble(stream):
	instruction_lines = list(map(lambda line: line.strip(), stream.split("\n")))
	pure_lines = [line for line in instruction_lines if line != ""]
	for line in pure_lines:
		parse_instr_line(line)

def print_as_fatal_error(error):
	print(f"assembler: fatal error: {error}")
	print("assembling terminated.")
	sys.exit(1)

'''
File extension of input source program
'''
FILE_EXTENSION = ".a65"

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print_as_fatal_error("no input files")

	filename, fileext = path.splitext(sys.argv[1])
	if fileext != FILE_EXTENSION:
		print_as_fatal_error(f"{filename}{fileext}: file format not recognized")

	with open(filename + FILE_EXTENSION, "r") as prog_file:
		program = prog_file.read()
		assemble(program)
