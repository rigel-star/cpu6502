import argparse as argsp

parser = argsp.ArgumentParser(description="Run 6502 CPU emulator")
parser.add_argument("filename", help="Exectuable name", type=str, 
					default="out.ef")

args = parser.parse_args()

import subprocess
import os

if os.path.exists("./main"):
	subprocess.run(["./main", args.filename])
else:
	print("CPU not found")
