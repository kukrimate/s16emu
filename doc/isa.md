# Sigma16 ISA supported by this emulator

## Introduction

While this emulator is based on the Sigma16 architecture, it is only strictly
conforming to the ISA specified in this file.

Sigma16 is 16-bit RISC-style ISA. It has 16 general purpose registers.
It has a 16-bit address bus with a word of memory corresponding to each address,
this results in 64K words (or 128K bytes) of addressable memory. Sigma16 does
not support byte addressing memory, thus byte-endianness is implementation
defined, and not exposed to software in any way whatsoever. Bits of words are
numbered in PowerPC style, aka the most significant bit is bit 0, and the least significant bit is bit 15. Signed integers are represented in 16-bit two's
complement.

Registers are numbered R0 through R15, the register R0 always contains 0 and
writes to it are discarded. Instructions that address memory use effective
addresses specified in the following format: Displacement[Ra]. The address is caclulated
by taking the Displacement and adding the content of Ra to it. If said addition
overflows, it wraps around to address 0.

## Flags
The register R15 functions as a flag output from arithmetic and compare
operations. The following flags are synonyms for the corresponding bit of R15:
<table border=1>
	<tr>
		<td>Flag</td>
		<td>Bit</td>
		<td>Description</td>
	</tr>
	<tr>
		<td>ccG</td>
		<td>0</td>
		<td>Unsigned greater than (or greater than 0)</td>
	</tr>
	<tr>
		<td>ccg</td>
		<td>1</td>
		<td>Signed greater than (or greater than 0)</td>
	</tr>
	<tr>
		<td>ccE</td>
		<td>2</td>
		<td>Equal to (or equal to 0)</td>
	</tr>
	<tr>
		<td>ccl</td>
		<td>3</td>
		<td>Signed less than (or less than 0)</td>
	</tr>
	<tr>
		<td>ccL</td>
		<td>4</td>
		<td>Unsigned less than (or less than 0)</td>
	</tr>
	<tr>
		<td>ccV</td>
		<td>5</td>
		<td>Unsigned overflow</td>
	</tr>
	<tr>
		<td>ccv</td>
		<td>6</td>
		<td>Signed overflow</td>
	</tr>
	<tr>
		<td>ccC</td>
		<td>7</td>
		<td>Carry propagation (always matches ccV)</td>
	</tr>
</table>

## Assembly instructions

### add Rd,Ra,Rb
- Calculate Ra + Rb.
- Stores the Ra + Rb in Rd.
- The ccV, ccv, and ccC flags are set accordingly.

### sub Rd,Ra,Rb
- Calculate Ra - Rb, by taking the two's complement of Rb and adding it to Ra.
- Stores the Ra - Rb in Rd.
- The ccV, ccv, and ccC flags are set accordingly.

### mul Rd,Ra,Rb
- Calculate Ra * Rb, treating both as 16-bit two's complement signed integers.
- Stores Ra * Rb in Rd.
- If Ra * Rb would overflow Rd, the lower 16-bits of the result is stored in Rd.
- If said overflow happens, the ccv flag will be set.

### div Rd,Ra,Rb
- Caculates Ra / Rb and Ra % Rb, treating both as 16-bit two's complement signed
integers.
- Stores Ra / Rb in Rd.
- If Rd is R15 than Ra % Rb is discarded, if not Ra % Rb is stored in R15.
- Division is understood as floor division, e.g. 10 / 4 = 2 and 10 / -4 = -3.
- Ra % Rb is defined as Ra - Rb * (Ra / Rb), where Ra / Rb is the result of the
floor divsion.
- Division by zero is interpreted as a no-op.
- This instruction does **not** set any flags, as R15 is used for Ra % Rb.

### cmp Ra,Rb
- Compares Ra and Rb.
- The ccG, ccg, ccE, ccl and ccL flags are set according to the result of said
comperison.

### inv Rd,Ra
- Calculates the bitwise inverse of Ra.
- Stores ~Ra in Rd.
- This instruction does not set any flags.

### and Rd,Ra,Rb
- Calculates the bitwise and of Ra and Rb.
- Stores Ra & Rb in Rd.
- This instruction does not set any flags.

### or Rd,Ra,Rb
- Calculates the bitwise or of Ra and Rb.
- Stores Ra | Rb in Rd.
- This instruction does not set any flags.

### xor Rd,Ra,Rb
- Calculates the bitwise exclusive-or of Ra and Rb.
- Stores Ra ^ Rb in Rd.
- This instruction does not set any flags.

### addc Rd,Ra,Rb
- Calculate Ra + Rb plus R15.ccC.
- Stores the result in Rd.
- The ccV, ccv, and ccC flags are set accordingly.

### cmplt Rd,Ra,Rb
- Check if Ra < Rb, treating both as two's complement signed integers.
- If Ra < Rb than Rd is set to 1, otherwise Rd is set to 0.
- This instruction does not set any flags.

### cmpeq Rd,Ra,Rb
- Compare Ra and Rb for equality.
- If Ra and Rb are equal set Rd to 1, otherwise set Rd to 0.
- This instruction does not set any flags.

### cmpgt Rd,Ra,Rb
- Check if Ra > Rb, treating both as two's complement signed integers.
- If Ra > Rb than Rd is set to 1, otherwise Rd is set to 0.
- This instruction does not set any flags.

### trap Rd,Ra,Rb
- Call the operating system service routine with the number stored in Rd,
passing Ra and Rb as arguments to said routine.
- Currently the following routines are supported:
<table border=1>
	<tr>
		<td>Routine</td>
		<td>Number</td>
		<td>Argument A</td>
		<td>Argument B</td>
	</tr>
	<tr>
		<td>Exit</td>
		<td>0</td>
		<td>Ignored (0 recommended for compatibility)</td>
		<td>Ignored (0 recommended for compatibility)</td>
	</tr>
	<tr>
		<td>Read</td>
		<td>1</td>
		<td>Buffer address</td>
		<td>Buffer size</td>
	</tr>
	<tr>
		<td>Write</td>
		<td>2</td>
		<td>Buffer address</td>
		<td>Buffer size</td>
	</tr>
</table>

### lea Rd,Displacement[Ra]
- Caculate the effective address and load it into Rd.
- This instruction does not set any flags.

### load Rd,Displacement[Ra]
- Caculate the effective address and load a word from memory from said address.
- This instruction does not set any flags.

### store Rd,Displacement[Ra]
- Caculate the effective address and store the contents of Rd at said address.
- This instruction does not set any flags.

### jump Rd,Displacement[Ra]
- Caculate the effective address and set the program counter to said address.
- This instruction does not set any flags.

### jumpc0 k,Displacement[Ra]
- Check bit k of R15, if it is zero caculate the effective address and set the
program counter to said address.
- This instruction does not set any flags.

### jumpc1 k,Displacement[Ra]
- Check bit k of R15, if it is one caculate the effective address and set the
program counter to said address.
- This instruction does not set any flags.

### jumpf Rd,Displacement[Ra]
- If Rd contains zero, caculate the effective address and set the program
counter to said address.
- This instruction does not set any flags.

### jumpt Rd,Displacement[Ra]
- If Rd contains non-zero, caculate the effective address and set the program
counter to said address.
- This instruction does not set any flags.

### jal Rd,Displacement[Ra]
- Store the address of the next instruction in Rd, caclulate the effective
address and set the program counter to set address.
- This instruction does not set any flags.

## Encoding types
Sigma16 supports the following instruction encodings. Please not that each
column under a word represent a nibble.
<table border=1>
	<tr>
		<td>Name</td>
		<td colspan=4>Word 0</td>
		<td colspan=4>Word 1</td>
	</tr>
	<tr>
		<td>RRR</td>
		<td>Opcode</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>RX</td>
		<td>0xf</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Opcode</td>
		<td colspan=4>Displacement</td>
	</tr>
</table>

## Encoding matrix
The following table lists the specific encoding of each instruction.
<table border=1>
	<tr>
		<td>Mnemonic</td>
		<td>Type</td>
		<td colspan=4>Word 0</td>
		<td colspan=4>Word 1</td>
	</tr>
	<!-- RRR -->
	<tr>
		<td>add</td>
		<td>RRR</td>
		<td>0</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>sub</td>
		<td>RRR</td>
		<td>1</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>mul</td>
		<td>RRR</td>
		<td>2</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>div</td>
		<td>RRR</td>
		<td>2</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>cmp</td>
		<td>RRR</td>
		<td>4</td>
		<td>0</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>cmplt</td>
		<td>RRR</td>
		<td>5</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>cmpeq</td>
		<td>RRR</td>
		<td>6</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>cmpgt</td>
		<td>RRR</td>
		<td>7</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>inv</td>
		<td>RRR</td>
		<td>8</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>0</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>and</td>
		<td>RRR</td>
		<td>9</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>or</td>
		<td>RRR</td>
		<td>a</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>xor</td>
		<td>RRR</td>
		<td>b</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>addc</td>
		<td>RRR</td>
		<td>c</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<tr>
		<td>trap</td>
		<td>RRR</td>
		<td>d</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>Rb</td>
		<td colspan=4>Not present</td>
	</tr>
	<!-- RX -->
	<tr>
		<td>lea</td>
		<td>RX</td>
		<td>f</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>0</td>
		<td colspan=4>Displacement</td>
	</tr>
	<tr>
		<td>load</td>
		<td>RX</td>
		<td>f</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>1</td>
		<td colspan=4>Displacement</td>
	</tr>
	<tr>
		<td>store</td>
		<td>RX</td>
		<td>f</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>2</td>
		<td colspan=4>Displacement</td>
	</tr>
	<tr>
		<td>jump</td>
		<td>RX</td>
		<td>f</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>3</td>
		<td colspan=4>Displacement</td>
	</tr>
	<tr>
		<td>jumpc0</td>
		<td>RX</td>
		<td>f</td>
		<td>k</td>
		<td>Ra</td>
		<td>4</td>
		<td colspan=4>Displacement</td>
	</tr>
	<tr>
		<td>jumpc1</td>
		<td>RX</td>
		<td>f</td>
		<td>k</td>
		<td>Ra</td>
		<td>5</td>
		<td colspan=4>Displacement</td>
	</tr>
	<tr>
		<td>jumpf</td>
		<td>RX</td>
		<td>f</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>6</td>
		<td colspan=4>Displacement</td>
	</tr>
	<tr>
		<td>jumpt</td>
		<td>RX</td>
		<td>f</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>7</td>
		<td colspan=4>Displacement</td>
	</tr>
	<tr>
		<td>jal</td>
		<td>RX</td>
		<td>f</td>
		<td>Rd</td>
		<td>Ra</td>
		<td>8</td>
		<td colspan=4>Displacement</td>
	</tr>
</table>
