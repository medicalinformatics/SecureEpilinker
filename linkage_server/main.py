from flask import Flask
from flask import request
from flask import make_response
from linkserv.secureepilinker import SecureEpilinker
from linkserv.linkresult import LinkageResult
from linkserv.linkageserver import LinkageServer
from Crypto import Random
import simplejson as json
import time

app = Flask("LinkageService")

LinServ = LinkageServer()
LinServ.add_epilinker("TUDA1", Random.new().read(32), 15)
LinServ.add_epilinker("TUDA2", Random.new().read(32), 13)
timeouttime = 20

@app.route('/', methods=['GET'])
def running():
    return 'Server is running!'

@app.route('/freshIds/<string:remote_id>', methods=['GET'])
def get_ids(remote_id):
    count = int(request.args.get('count', 1))
    ids = LinServ.generate_ids(remote_id, count)
    return json.dumps({'linkageIds': ids.tolist()})

@app.route('/testConnection/<string:remote_id>')
def test_connection(remote_id):
    resp = make_response('Connection correctly established', 200)
    return resp

@app.route('/linkageResult/<string:local_id>/<string:remote_id>', methods=['POST'])
def linkageResult(local_id, remote_id):
    length = request.content_length
    if (length != 0):
        try:
            data = request.get_json()
            if (data != None):
                LinServ.set_share(local_id, remote_id, data)
                if data['role'] == 'client':
                    print("Client")
                    timeout_counter = 0
                    timeout = False
                    while not LinServ.is_result_ready(local_id, remote_id):
                        print("Waiting for Server")
                        time.sleep(1)
                        timeout_counter += 1
                        if timeout_counter >= timeouttime:
                            timeout = True
                            break
                    if not timeout:
                        result = LinServ.get_linkage_result(local_id, remote_id)
                        return result
                    else:
                        LinServ.clear_result(local_id, remote_id)
                        return make_response('No result from server', 408)
                return 'Recieved server result'
            else:
                return make_response('Invalid JSON', 400)
        except BadRequest:
            return make_response('Invalid JSON', 400)
    else:
        return make_response('No result sent', 411)


