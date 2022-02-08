import struct
import sys

def swap_bytes(i):
	return struct.unpack("<I", struct.pack(">I", i))[0]
	
'''
Big endian to little endian
'''
def btol(data):
	if sys.byteorder == "little":
		return data
	elif sys.byteorder == "big":
		return swap_bytes(data)
	else:
		print("Endianess not supported")

'''
Little endian to big endian
'''
def ltob(data):
	if sys.byteorder == "little":
		return swap_bytes(data)
	elif sys.byteorder == "big":
		return data
	else:
		print("Endianess not supported")
