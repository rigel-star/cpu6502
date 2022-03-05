#!/usr/bin/env python3

import os
import sys
from collections import namedtuple
from bytes_op import btol, ltob

BYTEORDER = sys.byteorder

class Colors:
	RED = "\033[91m"
	END = "\033[0m"


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
	"sbc" :
		{
			"imm": InstrInfo(opcode=0xE9),
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
	"jsr" :
		{
			"abs": InstrInfo(opcode=0x20)
		},
	"kil" :
		{
			"imp": InstrInfo(opcode=0x02)
		},
	"nop" :
		{
			"imp": InstrInfo(opcode=0xEA)
		},
	"clc" :
		{
			"imp": InstrInfo(opcode=0x18)
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

SUPPORTED_DIRECTIVES = ['org', 'bit']
def parse_directive(line):
	# directive syntax : [.dir_name value]
	global user_defined_directives
	splited_str = line.split(" ", 1)

	if len(splited_str) != 2:
		return ParseError.ERR_SYN

	name = splited_str[0][2:]
	if not name in SUPPORTED_DIRECTIVES:
		return ParseError.ERR_SYN

	end_index = splited_str[1].find("]")
	if end_index < 0:
		return ParseError.ERR_SYN

	value = splited_str[1][:end_index]
	user_defined_directives[name] = value
	return ParseError.ERR_OK


# Preprocessing stuff. Will extract directives and remove comments from the source file.
def preprocess(source_lines):
	processed_source = []
	line_no = 1
	for line in source_lines:
		noc_line = strip_comments(line) # remove comments if any
		if noc_line.startswith("["):
			err = parse_directive(line)
			if err == ParseError.ERR_SYN:
				print("Preprocess: syntax error at line", line_no)
				sys.exit(-1)
		else:
			if line == "":
				continue
			else:
				processed_source.append(noc_line)
		line_no += 1
	return processed_source


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
	try:
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
	except ValueError:
		return ParseError.ERR_SYN


# Assemble single line
def parse_instr_line(instruction_line):
	pure_instr = strip_comments(instruction_line)

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
	if not operands_as_bytes or operands_as_bytes == ParseError.ERR_SYN:
		return ParseError.ERR_SYN

	return [opcode] + [byte for byte in operands_as_bytes]


# Assembling starts here
def assemble(stream, args):
	pure_lines =list(filter(lambda line: line != "",
				list(map(lambda line: line.strip(), stream.split("\n")))))

	output_file_ir = open(args.output + ".ir65", "wb")

	# writing magic number
	output_file_ir.write(ord('E').to_bytes(1, sys.byteorder, signed=False))
	output_file_ir.write(ord('F').to_bytes(1, sys.byteorder, signed=False))

	# size of output file
	output_array = []

	preprocessed = preprocess(pure_lines)

	print("[Generating IR65 file] ", end="")
	for line in preprocessed:
		parsed_line = parse_instr_line(line)
		if parsed_line == ParseError.ERR_OK:
			continue
		if parsed_line == ParseError.ERR_SYN:
			print("failed")
			print("Syntax error " + Colors.RED + line + Colors.END)
			sys.exit(-1)
		else:
			for byte in parsed_line:
				# making ir file
				output_array.append(byte)

	output_size = len(output_array)
	size_high = ((output_size >> 8) & 0xFF).to_bytes(1, BYTEORDER, signed=False)
	size_low = (output_size  & 0xFF).to_bytes(1, BYTEORDER, signed=False)
	if BYTEORDER == "big":
		output_file_ir.write(size_high)
		output_file_ir.write(size_low)
	elif BYTEORDER == "little":
		output_file_ir.write(size_low)
		output_file_ir.write(size_high)

	for byte in output_array:
		output_file_ir.write(byte.to_bytes(1, BYTEORDER, signed=False))

	output_file_ir.close()
	print("done")


def print_as_fatal_error(error):
	print(f"assembler: {Colors.RED} fatal error: {Colors.END} {error}")
	print("assembling terminated.")
	sys.exit(1)


import argparse
def parse_args():
	parser = argparse.ArgumentParser(description="6502 Assembler")
	parser.add_argument('-o', '--output', default='out', type=str, help='Output file name. Defaults to out')
	parser.add_argument('filename', help="File to assemble", type=str)
	return parser.parse_args()


'''
File extension of input source program
'''
FILE_EXTENSION = ".a65"

if __name__ == "__main__":
	args = parse_args()

	filename, fileext = os.path.splitext(args.filename)
	if fileext != FILE_EXTENSION:
		print_as_fatal_error(f"{filename}{fileext}: file format not recognized")

	with open(filename + FILE_EXTENSION, "r") as prog_file:
		program = prog_file.read()
		assemble(program, args)
