from os import path
import sys
from collections import namedtuple
from bytes_op import btol, ltob

# Every .ef file will begin with this magic number as first, second and third byte
MAGIC_NUMBER = 0x076566

InstrInfo = namedtuple("InstrInfo", "opcode")

INS_LOOKUP_TABLE = {
	"lda" :
		{
			"imm":	InstrInfo(opcode=0xA9),
			"zp":	InstrInfo(opcode=0xA5),
			"abs": 	InstrInfo(opcode=0xAD),
		},
	"adc" :
		{
			"imm": 	InstrInfo(opcode=0x69),
		},
	"jmp" :
		{
			"abs":	InstrInfo(opcode=0x4C),
			"ind":	InstrInfo(opcode=0x6C),
		},
	"rts" :
		{
			"imp": InstrInfo(opcode=0x60)
		},
	"jsr":
		{
			"abs": InstrInfo(opcode=0x20)
		},
	"kil" :
		{
			"imp": InstrInfo(opcode=0x02)
		}
}

# user defined directives
user_defined_directives = dict()

# user defined labels
user_defined_labels = dict()

from enum import Enum
class ParseError(Enum):
	ERR_OK = 0x0
	ERR_SYN = 0x1

def strip_comments(line):
	comment_index = line.find(";")
	if comment_index >= 0:
		line = line[:comment_index]
	return line.strip()

def parse_directives(line):
	global errorno
	# directive syntax : [.dir_name value]
	global user_defined_directives
	splited_str = line.split(" ", 1)
	name = splited_str[0][2:]

	end_index = splited_str[1].find("]")
	if end_index < 0:
		return ParseError.ERR_SYN

	value = splited_str[1][:end_index]
	user_defined_directives[name] = value
	return ParseError.ERR_OK


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
	elif operand[0].isdigit() or operand.startswith("0x"):
		if operand.lower().endswith("x"):
			return "absx"
		elif operand.lower().endswith("y"):
			return "absy"
		else:
			return "abs"
	return ParseError.ERR_SYN

def parse_operand_as_bytearr(operand, mode):
	if mode == "imm":
		return [int(operand[1:])]
	elif mode == "zp":
		return [int(operand[1:])]
	elif mode == "zpx" or mode == "zpy":
		comma_index = operand.find(",")
		return [int(operand[1:comma_index])]
	elif mode == "ind":
		closing_brace_index = operand.find("]")
		return [int(operand[1:closing_brace_index])]
	elif mode == "indx" or mode == "indy":
		comma_index = operand.find(",")
		return [int(operand[1:comma_index])]
	elif mode == "abs":
		abs_addr = int(operand, 0)
		high = (abs_addr >> 8) & 0xFF
		low = (abs_addr) & 0xFF
		if sys.byteorder == "big":
			return [high, low]
		else:
			return [low, high]
	elif mode == "absx" or mode == "absy":
		comma_index = operand.find(",")
		return [int(operand[:comma_index])]
	else:
		return ParseError.ERR_SYN

# Assemble single line
def parse_instr_line(instruction_line):
	pure_instr = strip_comments(instruction_line)
	if pure_instr.startswith("["):
		return parse_directives(pure_instr)

	tokens = pure_instr.split(" ", 1)
	mode = parse_addr_mode(pure_instr)

	mnemonic = tokens[0]
	addr_modes = INS_LOOKUP_TABLE.get(mnemonic)
	if not addr_modes:
		return ParseError.ERR_SYN

	addr_mode = addr_modes.get(mode)
	if not addr_mode:
		return ParseError.ERR_SYN

	opcode = addr_mode.opcode
	if mode == "imp":
		return [opcode]

	operands = tokens[1]
	operands_as_bytes = parse_operand_as_bytearr(operands, mode)
	if not operands_as_bytes:
		return ParseError.ERR_SYN

	return [opcode] + [byte for byte in operands_as_bytes]


# Assembling starts here
def assemble(stream):
	instruction_lines = list(map(lambda line: line.strip(), stream.split("\n")))
	pure_lines = [line for line in instruction_lines if line != ""]

	output_file = open("../output.ef", "wb")

	line_no = 1
	for line in pure_lines:
		parsed_line = parse_instr_line(line)
		if parsed_line == ParseError.ERR_OK:
			line_no += 1
			continue
		if parsed_line == ParseError.ERR_SYN:
			print(f"Syntax error at line {line_no}")
			sys.exit(-1)
		else:
			for byte in parsed_line:
				output_file.write(byte.to_bytes(1, sys.byteorder, signed=False))
		line_no += 1
	print(user_defined_directives)

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
