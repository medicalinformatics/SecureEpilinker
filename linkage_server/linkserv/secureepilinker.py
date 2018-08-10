import base64
import binascii
from Crypto.Cipher import AES
from Crypto.Util import Counter
from Crypto import Random
import numpy as np

class AESCipher:
    def __init__(self, key, key_length=32):
        self.key = key
        self.key_length = key_length
        assert len(self.key) == self.key_length

    def encrypt(self, plain):
        assert len(self.key) == self.key_length
        iv = Random.new().read(AES.block_size)
        iv_int = int(binascii.hexlify(iv), 16)
        ctr = Counter.new(AES.block_size * 8, initial_value= iv_int)
        cipher = AES.new(self.key, AES.MODE_CTR, counter=ctr)
        return base64.b64encode(iv + cipher.encrypt(plain))

    def decrypt(self, enc):
        assert len(self.key) == self.key_length
        enc = base64.b64decode(enc)
        iv_int = int(binascii.hexlify(enc[:16]),16)
        ctr = Counter.new(AES.block_size * 8, initial_value= iv_int)
        cipher = AES.new(self.key, AES.MODE_CTR, counter=ctr)
        return cipher.decrypt(enc[16:])

class SecureEpilinker:
    def __init__(self, uid, key, pad, idcounter=0):
        self.uid = uid
        self.cipher = AESCipher(key)
        self.idcounter = idcounter
        self.pad = pad

    def generate_id(self, n=1):
        result = np.empty(n,dtype='S64')
        for i in range(0,n):
            result[i] = self.cipher.encrypt(str(self.idcounter)+self.pad*'0')
            self.idcounter += 1
        return result

    def validate_id(self, lid):
        m = self.cipher.decrypt(lid)
        return m[-self.pad:] == self.pad*b'0'

