from linkserv.secureepilinker import SecureEpilinker
from linkserv.linkresult import LinkageResult

class LinkageServer:
    def __init__(self):
        self.epilinker = {}
        self.computations = {}

    def add_epilinker(self, epilinker_id, key, padding):
        self.epilinker[epilinker_id] = SecureEpilinker(epilinker_id, key, padding)

    def generate_ids(self, remote_id, count):
        if remote_id in self.epilinker:
            return self.epilinker[remote_id].generate_id(count)
        raise KeyError('Invalid Remote Id')

    def get_epilinker(self, epilinker_id):
        if epilinker_id in self.epilinker:
            return self.epilinker[epilinker_id]
        raise KeyError('Invalid Remote Id')

    def set_share(self, local_id, remote_id, data):
        if data['role'] == 'client':
            client_id = local_id
            server_id = remote_id
        elif data['role'] == 'server':
            client_id = remote_id
            server_id = local_id
        else:
            raise KeyError('Invalid Role')

        if client_id in self.computations:
            if server_id in self.computations[client_id]:
                self.computations[client_id][server_id].set_share(data)
            else:
                self.computations[client_id][server_id] = LinkageResult(self.get_epilinker(client_id), self.get_epilinker(server_id))
                self.computations[client_id][server_id].set_share(data)
        else:
            self.computations[client_id] = {}
            self.computations[client_id][server_id] = LinkageResult(self.get_epilinker(client_id), self.get_epilinker(server_id))
            self.computations[client_id][server_id].set_share(data)

    def is_result_ready(self, client_id, server_id):
        return self.computations[client_id][server_id].is_ready()

    def get_linkage_result(self, client_id, server_id):
        return self.computations[client_id][server_id].get_linkage_result()

    def clear_result(self, client_id, server_id):
        self.computations[client_id][server_id].clear()
