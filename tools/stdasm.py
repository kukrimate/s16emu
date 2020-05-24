#!/usr/bin/python3
# Sigma16 assembler in Python, supports the "standard" Sigma16 assembly syntax

import argparse
import re
from enum import Enum
import struct

class TokenKind(Enum):
	LABL = 0
	INSN = 1
	DATA = 2

class Token:
	def __init__(self, kind, string):
		self.kind   = kind
		self.string = string
	def __repr__(self):
		return self.string
	def __str__(self):
		return self.string

def tokenize(string):
	tokens = []

	string.replace("\r", "") # Just in case file is from Windoze

	for line in string.split("\n"):
		comment_start = line.find(";")

		if comment_start != -1:
			line = line[:comment_start] # Remove comment

		if len(line) == 0: # Skip empty lines
			continue

		# We want the first 3 space separated tokens
		for idx, string in enumerate(re.split("\\s+", line)[:3]):
			if len(string) == 0:
				continue
			tokens.append(Token(TokenKind(idx), string))

	return tokens

# Encode constant
def encode_const(data):
	if data.startswith("$"):
		data = int(data[1:], 16)
		return struct.pack(">H", data)
	elif data.startswith("#"):
		data = int(data[1:], 2)
		return struct.pack(">H", data)
	else:
		data = int(data, 10)
		if data < 0:
			return struct.pack(">h", data)
		else:
			return struct.pack(">H", data)

# Encode register
def encode_reg(regname):
	if regname[0] != "R":
		raise Exception("Unknown register %s" %regname)

	regnum = int(regname[1:])
	if regnum < 0 or regnum > 15:
		raise Exception("Unknown register number %d" %regnum)

	return regnum


# Size of an RRR instruction (in words)
SIZE_RRR = 1

# Assemble an RRR instruction
def encode_rrr(opcode, data):
	args = data.split(",")

	rd = 0
	ra = 0
	rb = 0

	if len(args) == 2:
		ra = encode_reg(args[0])
		rb = encode_reg(args[1])
	elif len(args) == 3:
		rd = encode_reg(args[0])
		ra = encode_reg(args[1])
		rb = encode_reg(args[2])
	else:
		raise Exception("Bad data for RRR instruction")

	return struct.pack(">H", opcode << 12 | rd << 8 | ra << 4 | rb)

# Enocde effective address
def encode_ea(ea):
	m = re.match("(.+)\\[(.+)\\]", ea)
	return encode_reg(m.group(2)), encode_const(m.group(1))

# Size of an RX instruction (in words)
SIZE_RX = 2

# Assemble an RX instruction
def encode_rx(opcode, data, fixd):
	args = data.split(",")

	rd = 0
	ra = 0
	disp = 0

	if len(args) == 1:
		ra, disp = encode_ea(args[0])
	elif len(args) == 2:
		rd = encode_reg(args[0])
		ra, disp = encode_ea(args[1])
	else:
		raise Exception("Bad data for RX instruction")

	if fixd > 0:
		rd = fixd

	return struct.pack(">H", 0xf << 12 | rd << 8 | ra << 4 | opcode) + disp

# Instruction sizes
insn_size = {
	"data"  : 1,
	"add"   : SIZE_RRR,
	"sub"   : SIZE_RRR,
	"mul"   : SIZE_RRR,
	"div"   : SIZE_RRR,
	"cmp"   : SIZE_RRR,
	"cmplt" : SIZE_RRR,
	"cmpeq" : SIZE_RRR,
	"cmpgt" : SIZE_RRR,
	"trap"  : SIZE_RRR,
	"lea"   : SIZE_RX,
	"load"  : SIZE_RX,
	"store" : SIZE_RX,
	"jump"  : SIZE_RX,
	"jumpc0": SIZE_RX,
	"jumpc1": SIZE_RX,
	"jumplt": SIZE_RX,
	"jumple": SIZE_RX,
	"jumpne": SIZE_RX,
	"jumpeq": SIZE_RX,
	"jumpge": SIZE_RX,
	"jumpgt": SIZE_RX,
	"jumpf" : SIZE_RX,
	"jumpt" : SIZE_RX,
	"jal"   : SIZE_RX
}

# Instruction functions
insn_func = {
	"data"  : encode_const,
	"add"   : lambda x: encode_rrr(0x0, x),
	"sub"   : lambda x: encode_rrr(0x1, x),
	"mul"   : lambda x: encode_rrr(0x2, x),
	"div"   : lambda x: encode_rrr(0x3, x),
	"cmp"   : lambda x: encode_rrr(0x4, x),
	"cmplt" : lambda x: encode_rrr(0x5, x),
	"cmpeq" : lambda x: encode_rrr(0x6, x),
	"cmpgt" : lambda x: encode_rrr(0x7, x),
	"trap"  : lambda x: encode_rrr(0xd, x),

	"lea"   : lambda x: encode_rx(0x0, x, 0),
	"load"  : lambda x: encode_rx(0x1, x, 0),
	"store" : lambda x: encode_rx(0x2, x, 0),
	"jump"  : lambda x: encode_rx(0x3, x, 0),
	"jumpc0": lambda x: encode_rx(0x4, x, 0),
	"jumpc1": lambda x: encode_rx(0x5, x, 0),

	"jumplt": lambda x: encode_rx(0x5, x, 3),
	"jumple": lambda x: encode_rx(0x4, x, 1),
	"jumpne": lambda x: encode_rx(0x4, x, 2),
	"jumpeq": lambda x: encode_rx(0x5, x, 2),
	"jumpge": lambda x: encode_rx(0x4, x, 3),
	"jumpgt": lambda x: encode_rx(0x5, x, 1),

	"jumpf" : lambda x: encode_rx(0x6, x, 0),
	"jumpt" : lambda x: encode_rx(0x7, x, 0),
	"jal"   : lambda x: encode_rx(0x8, x, 0)
}

def assemble(tokens):
	code = bytearray()

	addr = 0
	symbtab = {}

	# First stage (we build the symbol table)
	for tok in tokens:
		if tok.kind == TokenKind.LABL:
			symbtab[tok.string] = addr
		elif tok.kind == TokenKind.INSN:
			if tok.string in insn_size:
				addr += insn_size[tok.string]
			else:
				raise Exception("Unknown instruction %s" %tok.string)

	# Second stage (we actually assemble instructions)
	while len(tokens) > 0:

		# Get instruction of stack
		insn = tokens.pop(0)
		if insn.kind != TokenKind.INSN:
			continue

		# Get and verify data for it
		if len(tokens) == 0:
			raise Exception("Instruction %s requires data" %insn.string)
		data = tokens.pop(0)
		if data.kind != TokenKind.DATA:
			raise Exception("Instruction %s requires data" %insn.string)


		# Assemble and append instruction
		args = data.string
		for s, addr in sorted(symbtab.items(),
						key=lambda x: len(x[0]), reverse=True):
			args = args.replace(s, str(addr))

		code += insn_func[insn.string](args)

	return bytes(code), symbtab

parser = argparse.ArgumentParser(description="Python assembler for Sigma16")
parser.add_argument("IASM", help="Input assembly file")
parser.add_argument("-o", metavar="OBIN", help="Output binary")
parser.add_argument('-s', metavar="OSYM", help="Output symbol table")
args = parser.parse_args()

# Assemble file
with open(args.IASM) as f:
	code, symtab = assemble(tokenize(f.read()))

if args.o:
	with open(args.o, "wb") as f:
		f.write(code)

if args.s:
	with open(args.s, "w") as f:
		f.write("\n".join(["%s:%s" %(k,v) for k,v in symtab.items()]))
