import sys

options = list()
arguments = list()
opt_args: dict

from enum import Enum
class ArgParseError(Enum):
	ERR_INV_ARG = 0x5
	ERR_INV_OP = 0x7
	ERR_OK = 0x2

def parse_args(arg_list):
	global options
	global arguments
	global opt_args

	for i in range(len(arg_list)):
		arg = arg_list[i]
		if arg.startswith("-"):
			options.append(arg)
			i += 1
			if arg_list[i].startswith("-"):
				print("Mismatch of option and argument")
				return ArgParseError.ERR_INV_ARG
			else:
				arguments.append(arg_list[i])

	opt_args = dict(zip(options, arguments))
	return ArgParseError.ERR_OK 


if __name__ == "__main__":
	status = parse_args(sys.argv[1:])
	if status == ArgParseError.ERR_OK:
		print(options)
		print(arguments)
		print(opt_args)
