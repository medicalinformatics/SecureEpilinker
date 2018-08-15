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

    def encrypt(self, plain, b64=True):
        assert len(self.key) == self.key_length
        iv = Random.new().read(AES.block_size)
        iv_int = int(binascii.hexlify(iv), 16)
        ctr = Counter.new(AES.block_size * 8, initial_value= iv_int)
        cipher = AES.new(self.key, AES.MODE_CTR, counter=ctr)
        if b64:
            return base64.b64encode(iv + cipher.encrypt(plain))
        return iv+cipher.encrypt(plain)

    def decrypt(self, enc, b64=True):
        assert len(self.key) == self.key_length
        if b64:
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
            result[i] = self.cipher.encrypt(b'0'+bytes(self.idcounter)+self.pad*b'0')
            self.idcounter += 1
        if n is 1:
            return result[0]
        else:
            return result

    def validate_id(self, lid, base64 = True):
        m = self.cipher.decrypt(lid, base64)
        return m[-self.pad:] == self.pad*b'0'

    def decrypt(self, enc, base64=True):
        return self.cipher.decrypt(enc, base64)

    def encrypt(self, msg, base64=True):
        return self.cipher.encrypt(msg,base64)
