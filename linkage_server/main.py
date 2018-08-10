from flask import Flask
from flask import request
from linkserv.secureepilinker import SecureEpilinker
from Crypto import Random
import simplejson as json

app = Flask("LinkageService")

A = SecureEpilinker("TUDA1", Random.new().read(32), 15)
B = SecureEpilinker("TUDA2", Random.new().read(32), 13)

@app.route('/')
def running():
    return 'Server is running!'

@app.route('/freshIds/<string:remote_id>')
def get_ids(remote_id):
    count = int(request.args.get('count', 1))
    if remote_id == "TUDA1":
        ids = A.generate_id(count)
    if remote_id == "TUDA2":
        ids = B.generate_id(count)
    return json.dumps({'linkageIds': ids.tolist()})
