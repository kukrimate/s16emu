#!/usr/bin/python3
# Converts "standard" Sigma16 assembly to the "flexible" syntax used by s16asm

import argparse
import re

parser = argparse.ArgumentParser(
	description="Convert standard Sigma16 assembly to s16asm syntax")
parser.add_argument("file", help="Input assembly file")
args = parser.parse_args()

# Read
with open(args.file) as file:
	data = file.read()

# Convert
data = data.replace("\r", "")
for line in data.split("\n"):
	comment_start = line.find(";")
	if comment_start != -1:
		line = line[:comment_start]
	if len(line) == 0:
		continue

	parts = []

	tokens = re.split("\\s+", line, maxsplit=2)
	for i, token in enumerate(tokens):
		token = token.strip()
		if len(token) == 0:
			continue

		if i == 0:
			parts.append("%s:" %token)
		elif i == 1:
			parts.append(token)
		elif i == 2:
			operands = token.split(",")
			if len(operands) > 0:
				parts.append(', '.join(map(lambda x: x.strip(), operands)))

	if len(parts) > 0:
		print(' '.join(parts))
