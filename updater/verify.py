from sys import argv
import struct
from Crypto.PublicKey import DSA
from Crypto.Signature import DSS
from Crypto.Hash import SHA1

diffile = open(argv[1], "rb")
dif = bytearray(diffile.read())
diffile.close()

sig_len = struct.unpack(">I", dif[0x20:0x24])[0]
sig = bytes(dif[0x24:0x24+sig_len])

dif[0x20:0x54] = 0x00, 0x00, 0x00, 0x00, 0x11, 0x12, 0x13, \
     0x14, 0x15, 0x16, 0x17, 0x18, 0x01, 0x23, 0x45, 0x67, \
     0x89, 0xAB, 0xCD, 0xEF, 0x55, 0xAA, 0x55, 0xAA, 0x55, \
     0xAA, 0x55, 0xAA, 0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, \
     0x69, 0x78, 0xDE, 0xAD, 0xBE, 0xEF, 0x0B, 0xAD, 0xC0, \
     0xDE, 0xDE, 0xFA, 0xCE, 0xD0, 0xDA, 0xD0, 0x00, 0x00

hash_obj = SHA1.new(dif)

# Load the public key
pub_key = DSA.import_key(\
    "-----BEGIN PUBLIC KEY-----\n\
    MIHyMIGpBgcqhkjOOAQBMIGdAkEAzgzNc26mTIP8zdBM/TsYVhlbzAmV3xBmIqJQ\
    riA/VUleEY+waI5df29XLh4nX3vkOqN3+8OotDpm/ZW9rFDrlQIVAN62YLaxanX9\
    1ogPoVDWj1qkBHuVAkEAhrBdXMSp3mKRZIUkmT13xhDfTkbwzbv3FEFv3SWlCOS1\
    j+ZtuDXKFy/OPBJ3fRdTY36x1qfqGaE5WD7F18fX0gNEAAJBAMbWjAOX44qexHQu\
    qoTV8YebQypnCDwhp6qLpkNJ482BUHWTpNsakcboEkYdver89MeMbqDdzN4VHsGh\
    dLoTjuo=\n\
    -----END PUBLIC KEY-----"\
)
verifier = DSS.new(pub_key, 'deterministic-rfc6979', encoding='der')

# Verify the authenticity of the message
try:
    verifier.verify(hash_obj, sig)
    print("The signature is valid.")
except ValueError:
    print("The signature is invalid.")
