#!/usr/bin/env python3

import sys
import os

BYTE_ORDER = sys.byteorder

MAGIC_HIGH = 0x45
MAGIC_LOW = 0x46
EF_MAGIC_NUM = (MAGIC_HIGH << 8) | MAGIC_LOW

def bytes_to_int(byte_arr):
	return int.from_bytes(bytes(byte_arr, "ascii"), BYTE_ORDER)

def make_ef(in_fname, out_fname):
	print("[Generating EF file] ", end="")
	try:
		with open(in_fname, "rb") as infile:
			file_size = os.stat(in_fname).st_size
			file_content = infile.read()

			output_size = file_size + 4
			output_array = bytearray([0 for _ in range(output_size)])
			output_index = 0
			if BYTE_ORDER == "big":
				output_array[output_index] = MAGIC_LOW
				output_index += 1
				output_array[output_index] = MAGIC_HIGH
				output_index += 1
				output_array[output_index] = output_size & 0xFF
				output_index += 1
				output_array[output_index] = (output_size >> 8) & 0xFF
				output_index += 1
			else:
				output_array[output_index] = MAGIC_HIGH
				output_index += 1
				output_array[output_index] = MAGIC_LOW
				output_index += 1
				output_array[output_index] = (output_size >> 8) & 0xFF
				output_index += 1
				output_array[output_index] = output_size & 0xFF
				output_index += 1

			for i in range(output_index, output_size):
				output_array[i] = file_content[i - 4]

			with open(out_fname + ".ef", "wb") as outfile:
				for byte in output_array:
					outfile.write(byte.to_bytes(1, BYTE_ORDER))
			print("done")
	except IOError as err:
		print("failed")
		print(err)


if __name__ == "__main__":
	if len(sys.argv) < 2:
		print("No file provided")
		sys.exit(1)

	fullname = sys.argv[1]
	filename, fileext = os.path.splitext(fullname)
	
	if fileext != ".ir65":
		print("Not valid extension")
		sys.exit(2)

	make_ef(fullname, filename)
