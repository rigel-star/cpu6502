#!/usr/bin/env python3

import os
import sys
import struct

MAGIC_NUMBER = ((ord('o') << 8) | (ord('k')))

def make_bootable(full_fname, output_fname):
	try:
		with open(full_fname, "rb") as effile:
			file_size = os.stat(full_fname).st_size
			file_content = effile.read()

			if file_size > 0x80 or file_size < 0:
				print("File size not valid")
				sys.exit(3)
			elif file_size == 0x80:
				magic0 = file_content[126]
				magic1 = file_content[127]
				if ((magic0 << 8) | magic1) == MAGIC_NUMBER:
					print("Already bootable")
					sys.exit(0)

			output_array = bytearray([0 for _ in range(128)])
			for i in range(file_size):
				output_array[i] = file_content[i]

			output_array[126] = ord('o')
			output_array[127] = ord('k')
			
			with open(output_fname + ".ef", "wb") as output:
				for byte in output_array:
					output.write(byte.to_bytes(1, sys.byteorder))
	except IOError as err:
		print(err)


def print_usage():
	print(f"Usage: {sys.argv[0]} <filename>")

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print_usage()
		sys.exit(1)

	fname, fileext = os.path.splitext(sys.argv[1])
	full_fname = sys.argv[1]

	if fileext != ".ef":
		print("File ext not recognized")
		sys.exit(2)

	make_bootable(full_fname, fname)
