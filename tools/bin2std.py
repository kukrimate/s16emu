#!/usr/bin/python3
# Convert a binary to a fake "standard" assembler file
import sys
import struct

with open(sys.argv[1], "rb") as f:
	code = f.read()

for i in range(0, len(code), 2):
	print(" data $%04x" %struct.unpack(">H", code[i:i+2]))
