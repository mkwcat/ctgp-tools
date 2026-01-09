import sys, os, struct
from Crypto.Cipher import AES

blobKey = bytes([0x90, 0x83, 0x00, 0x04, 0x90, 0xA3, 0x00, 0x08, 0x90, 0xC3, 0x00, 0x0C, 0x4E, 0x80, 0x00, 0x20])

encode = False
if sys.argv[1] == "enc":
    encode = True
elif sys.argv[1] == "dec":
    encode = False
else:
    raise Exception("invalid encrypt mode!")

infile = open(sys.argv[2], "rb")
outfile = open(sys.argv[3], "wb")

infile.seek(0, 2)
size = infile.tell()
infile.seek(0, 0)

i = 0
while i < size / 512:
    data = []
    curIv = struct.pack(">IIII", 0x80630004, 0x90830004, i, 0x4E800020)
    cipher = AES.new(blobKey, AES.MODE_CBC, iv=curIv)
    if encode == True:
        data = cipher.encrypt(infile.read(0x8000))
    else:
        data = cipher.decrypt(infile.read(0x8000))
    outfile.write(data)
    i += 64
    