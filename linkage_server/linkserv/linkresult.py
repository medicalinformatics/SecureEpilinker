import simplejson as json
from linkserv.secureepilinker import SecureEpilinker
from flask import make_response
import base64
import binascii
import sys
import operator

class LinkageResult:
    def __init__(self, client, server):
        self.client = client
        self.client_share_match = False
        self.client_share_tmatch = False
        self.client_share_id = ""
        self.server = server
        self.server_share_match = False
        self.server_share_tmatch = False
        self.server_share_id = ""
        self.client_data = False
        self.server_data = False

    def clear(self):
        self.client_share_match = False
        self.client_share_tmatch = False
        self.client_share_id = ""
        self.server_share_match = False
        self.server_share_tmatch = False
        self.server_share_id = ""
        self.client_data = False
        self.server_data = False

    def set_share(self, json_data):
        # data = json.load(json_data)
        data = json_data
        if (data['role'] == 'server'):
            self.server_share_match = bool(data['result']['match'])
            self.server_share_tmatch = bool(data['result']['tentative_match'])
            self.server_share_id = binascii.a2b_base64(data['result']['bestId'])
            self.server_data = True
        elif (data['role'] == 'client'):
            self.client_share_match = bool(data['result']['match'])
            self.client_share_tmatch = bool(data['result']['tentative_match'])
            self.client_share_id = binascii.a2b_base64(data['result']['bestId'])
            self.client_data = True
        else:
            return make_response('Invalid role', 400)

    def get_linkage_result(self):
        # All inputs set?
        assert self.server_data and self.client_data is True
        # Compute clear match bits
        match = self.client_share_match ^ self.server_share_match
        tmatch = self.client_share_tmatch ^ self.server_share_tmatch
        if match or tmatch:
            # Compute best ID
            assert len(self.client_share_id) == len(self.server_share_id)
            client_id_int = int.from_bytes(self.client_share_id, sys.byteorder)
            server_id_int = int.from_bytes(self.server_share_id, sys.byteorder)
            bestId = (client_id_int^server_id_int).to_bytes(len(self.client_share_id), sys.byteorder)
            # Decrypt ID with server key and encrypt with client key prepend
            # tmatch bit
            print(self.server.validate_id(bestId,base64=False))
            client_linkage_id = self.client.encrypt(bytes(tmatch)+self.server.decrypt(bestId,False))
            self.clear()
            return json.dumps({'linkageId': client_linkage_id})
        else:
            self.clear()
            return json.dumps({'linkageId': self.client.generate_id()})

    def is_ready(self):
        return self.server_data and self.client_data
