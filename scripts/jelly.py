import sys, os, struct
from Crypto.Cipher import AES
import math

encode = False

mtime = os.path.getmtime(sys.argv[1])
# we should actually get the real creation time but it's not cross platform, and
# also breaks with copies
ctime = mtime

infile = open(sys.argv[1], "rb")
outfile = open(sys.argv[2], "wb")

infile.seek(0, 2)
size = infile.tell()

blobKey = []

def crypt_block(sector):
    curIv = struct.pack(">IIII", 0x80630004, 0x90830004, sector, 0x4E800020)
    cipher = AES.new(blobKey, AES.MODE_CBC, iv=curIv)
    if encode == True:
        return cipher.encrypt(infile.read(0x8000))
    else:
        return cipher.decrypt(infile.read(0x8000))

for i in range(24):
    blobKey = struct.pack(">IIII", math.floor(mtime + (i - 12) * 3600), 0x90A30008, math.floor(ctime + (i - 12) * 3600), 0x4E800020)
    infile.seek(0, 0)
    data = crypt_block(0)
    if (data[0x52] == 0x46 and data[0x53] == 0x41 and data[0x54] == 0x54 and data[0x55] == 0x33 and data[0x56] == 0x32):
        break
else:
    raise Exception("Could not find key!")

i = 0
infile.seek(0, 0)
while i < size / 512:
    data = crypt_block(i)
    outfile.write(data)
    i += 64
    