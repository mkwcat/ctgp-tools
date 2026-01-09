import struct

def ibm_bit(n):
	assert n >= 0
	assert n <= 31

	return 1 << (31 - n)

assert ibm_bit(0) == 0x8000_0000
assert ibm_bit(31) == 1

def decode_srr1(srr1):
	BITS = {
		13: 'POW: Power management enabled (reduced power mode)',
		14: '14: RESERVED',
		15: 'ILE: Exception little-endian mode. When an exception occurs, this bit is copied into MSR[LE] to select the endian mode for the context established by the exception.',
		16: 'EE: External interrupt enable',
		17: 'PR: Privilege level ',
		18: 'FP: Floating-point available ',
		19: 'ME: Machine check enable ',
		20: 'FE0',
		21: 'SE',
		22: 'BE',
		23: 'FE1',
		24: '24: RESERVED',
		25: 'IP',
		26: 'IR: Instruction address translation is enabled.',
		27: 'DR: 1 Data address translation is enabled.',
		28: '28: RESERVED',
		29: 'PM: Process is a marked process',
		30: 'RI: Exception is recoverable',
		31: 'LE: Little-endian mode enable'
	}

	enabled_bits = [i for i in range(31) if (ibm_bit(i) & srr1)]
	
	return [(i, BITS[i] if i in BITS else 'RESERVED') for i in enabled_bits]

read_f64 = lambda f: struct.unpack(">d", f.read(8))[0]
read_f32 = lambda f: struct.unpack(">f", f.read(4))[0]

read_u32 = lambda f: struct.unpack(">L", f.read(4))[0]
read_u16 = lambda f: struct.unpack(">H", f.read(2))[0]
read_u8 = lambda f: struct.unpack(">B", f.read(1))[0]
read_bool = lambda f: struct.unpack(">?", f.read(1))[0]

write_u32 = lambda d: struct.pack(">L", d)
write_u16 = lambda d: struct.pack(">H", d)
write_u8 = lambda d: struct.pack(">B", d)
write_bool = lambda d: struct.pack(">?", d)

def unk_field(f, ofs):
	f.seek(ofs)
	return ('?', read_u32(f))

def to_offset(n):
	return n - 0x93000008

LAYOUT = {
	to_offset(0x93000008): ('UNKNOWN', read_u32),
	to_offset(0x930001C0): ('cr', read_u32),
    to_offset(0x930001C4): ('lr', read_u32),
    to_offset(0x930001C8): ('ctr', read_u32),
    to_offset(0x930001CC): ('xer', read_u32),
    to_offset(0x930001D0): ('srr0', read_u32),
    to_offset(0x930001D4): ('srr1', read_u32, decode_srr1),

}

ADD_FLOATS = False

for i in range(32):
	LAYOUT[0x38 + i*4] = ('r%s' % i, read_u32)

if ADD_FLOATS:
	for i in range(32):
		LAYOUT[0x38 + 32*4 + i*8] = ('f%s' % i, read_f64)

END_OF_REGS = 0x1b8 # 0x38 + 32 * 4 + 32 * 8

# 0x930002F8: Stack [+0x2F0]
# if (u32)(r1 + 0x8000) > 0x93400000: len= 0x93400000-r1
# 2kb if in MEM1?
#
# MAX SIZE 0x8000
#
#
# Stack layout:
# +00 NextStackItem
# +04 LR SAVE

# IS R1 AT +3C

def virtual_stack_to_dump(addr, OFFS):
	ofs = addr - OFFS

	assert ofs < 0x8000

	return ofs + 0x2F0


print(LAYOUT)

with open('CTGPException2_Decrypted.bin', 'rb') as f:
	f.seek(0,2)
	file_size = f.tell()

	f.seek(0, 0)


	body = {}

	for ofs, field in LAYOUT.items():
		f.seek(ofs, 0)

		name = field[0]
		reader = field[1]

		val = reader(f)

		if len(field) > 2:
			handler = field[2]
			body[name] = (val, hex(val), handler(val))
		else:
			try:
				as_u = struct.unpack(">L", struct.pack(">L", val))[0]
			except:
				as_u = 0
			body[name] = (val, hex(as_u), 'raw')

	LR = body['lr'][0]

	R1 = body['r1'][0]
	R2 = body['r2'][0]
	R13 = body['r13'][0]

	assert R2 == 0x8038EFA0
	assert R13 == 0x8038CC00

	# start is R1
	f.seek(virtual_stack_to_dump(R1, R1))
	for i in range(30):
		NEXT_CHAIN = read_u32(f)
		PC_AT = read_u32(f)

		print("STACK: %x (next=%x, r1 = %x, delta=%x)" % (PC_AT, NEXT_CHAIN, R1, R1-NEXT_CHAIN))
		#print("--> %x, %x" % (PC_AT ^ 0xB96F367C, PC_AT ^ 0))

		if NEXT_CHAIN-R1 >= 0x8000:
			break

		f.seek(virtual_stack_to_dump(NEXT_CHAIN, R1))

	import json

	#body = decode_srr1(0xb032)
	print(json.dumps(body, indent=4))

	REGS = ['PC']
	for i in range(3, 32):
		REGS += ['R%s' % i]

	def format_slice(index):
		size = 3072
		low = 0x82F0 + size * index
		mid = low + 1024
		top = low + 3072 # exclusive

		return f"{hex(mid)} (-1024: {hex(low)}, +2048: {hex(top)})"

	print('\n'.join(f"({i}) {reg} = {format_slice(i)}" for i, reg in enumerate(REGS)))

	end_of_slices = len(REGS) * 3072 + 0x82F0

	# TODO: Probs last 32 bytes are something else
	print('MEM80 SLICE: %x-%x (%s bytes)' % (end_of_slices, file_size, file_size - end_of_slices))

# 0 - 0x20: Undecodable
# ???
# 0x38-0xb8: R0 .. R31
# 0xb8-0x1b8: ???? (Probably FPRs)
# 0x1b8-0x1d0: CR, LR, CTR, XER, SRR0, SRR1
# ????
# 0x2f0-0x82f0: STACK
# 0x82f0-0x1eaf0: SLICES
# 1eaf0-1ebf0: MEM80
# 1ebf0-1ec10: ??? (Hash?)

CTGPEXCEPTION_KEY = '43FCEA877EBD8B9BCB6FD1CC1E3F64D3'
CTGPEXCEPTION_IV = '0' # TODO: This isn't right. Means we can't decode the first 32 bytes of the file

#from Crypto.Cipher import AES


# openssl enc -d -aes-128-cbc -K 43FCEA877EBD8B9BCB6FD1CC1E3F64D3 -iv 0 -nopad -in CTGPException.bin -out CTGPException_Decrypted.bin