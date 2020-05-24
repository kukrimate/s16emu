# Executable formats
This document describes the executable formats supported by this emulator.

## Flat binary
This format is very basic. The on disk file is just a flat binary containing
16-bit big-endian words. The whole file gets loaded into memory at address zero.
After loading the program counter is set to zero and execution begins.

## S16EXE
S16EXE is a wrapper around big-endian flat binaries, it supports loading code
anywhere in memory and it supports attaching debugging information to the
executable program itself. Multi-byte words are always encoded as big-endian.

### Header
Every S16EXE file has to start with the following header:
<table border=1>
	<tr>
		<td>Offset</td>
		<td>Length</td>
		<td>Description</td>
	</tr>
	<tr>
		<td>0</td>
		<td>4</td>
		<td>Magic number (S16X in ASCII)</td>
	</tr>
	<tr>
		<td>4</td>
		<td>2</td>
		<td>Header length</td>
	</tr>
	<tr>
		<td>6</td>
		<td>2</td>
		<td>Section count</td>
	</tr>
	<tr>
		<td>8</td>
		<td>2</td>
		<td>Entry point address</td>
	</tr>
	<tr>
		<td>10</td>
		<td>(Header length) - 10</td>
		<td>Reserved</td>
	</tr>
</table>

### Sections
The header is directly followed by (Section count) sections. Each section has
the following format.
<table border=1>
	<tr>
		<td>Offset (in words)</td>
		<td>Length (in words)</td>
		<td>Description</td>
	</tr>
	<tr>
		<td>0x0000</td>
		<td>1</td>
		<td>Section type</td>
	</tr>
	<tr>
		<td>0x0001</td>
		<td>1</td>
		<td>Section length (without header, in words)</td>
	</tr>
	<tr>
		<td>0x0002</td>
		<td>(Section length)</td>
		<td>Section content (dependent on section type)</td>
	</tr>
</table>

### Section types
The following section types are supported currently, a loader should ignore a
section whose type is unknown to it:
<table border=1>
	<tr>
		<td>Type</td>
		<td>Description</td>
	</tr>
	<tr>
		<td>0</td>
		<td>Loadable section</td>
	</tr>
	<tr>
		<td>1</td>
		<td>Symbol table</td>
	</tr>
</table>
