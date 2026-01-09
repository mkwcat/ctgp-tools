from sys import argv
import struct

infile = open(argv[1], "rb")
outfile = open(argv[2], "wb")

indata = infile.read()
infile.close()

outdata = b""

if len(indata) & 3:
    raise Exception("bad file size!")

h = struct.unpack_from(">IIIII", indata[:0x14])

if h[0] != 0x44494630:
    raise Exception("magic should be DIF0!")

if h[1] != len(indata):
    raise Exception("wrong file size!")

if h[3] >= h[1] or h[3] & 3:
    raise Exception("bad filename offset!")

if indata[h[3]] != 0x2F:
    raise Exception("filename should start with slash!")

if indata[-1]:
    raise Exception("filename should end with null!")

if h[4] == 0x20:
    h += struct.unpack_from(">III", indata[0x14:0x20])
elif h[4] == 0x30:
    h += struct.unpack_from(">IIIccccccccII", indata[0x14:0x30])
    if h[16] != h[3]:
        raise Exception("bad filename offset in ext. header!")
else:
    raise Exception("bad header length!")

i = h[4]
j = i

while i < h[3]:
    j -= 1
    
    b = indata[i]; i += 1
    c = b & 15

    if c == 0:
        y = b >> 4
        outdata += indata[i:i+y+1]
        i += y + 1
        j = len(outdata)
    elif c == 1:
        if b >> 4 == 15:
            break
        y = indata[i]; i += 1
        if b >> 4:
            y = (y + 1) * 256 + indata[i]; i += 1
            print(y)
        outdata += indata[i:i+y+17]
        i += y + 17
        j = len(outdata)
    elif c == 3:
        y = b >> 4
        while y > -2:
            outdata += outdata[j:j+1]
            j += 1
            y -= 1
        j = len(outdata)
    elif c == 4:
        y = indata[i]; i += 1
        while y > -6:
            outdata += outdata[j:j+1]
            j += 1
            y -= 1
        j = len(outdata)
    elif c == 8:
        y = b >> 4
        j = j - y
    elif c == 9:
        y = indata[i]; i += 1
        j = j - y
    elif c == 10:
        y = indata[i] * 256 + indata[i+1]; i += 2
        j = j - y - 0x100
    else:
        raise Exception(f"unknown command {c:X} at 0x{i-1:X}!")

if i > h[3]:
    raise Exception("DIF0 leaks into filename!")

if len(outdata) != h[6]:
    raise Exception("wrong uncompressed size!")

outfile.write(outdata)
outfile.close()
