#!/usr/bin/env python3
# Reflects the requests from HTTP methods GET, POST, PUT, and DELETE
# Written by Nathan Hamiel (2010)

import re
from http.server import HTTPServer, BaseHTTPRequestHandler
from jinja2 import Template

class RequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):

        print("\n----- Request Start ----->")
        request_path = self.path
        request_headers = self.headers
        print("Path:", request_path)
        print(request_headers)

        if 'Authorization' not in request_headers:
            print("[Kein Authorization-Header]")

        content_len = int(self.headers.get('Content-Length', 0))
        post_body = self.rfile.read(content_len).decode("utf-8")

        msg, code = "", 200
        if "getAllRecords" in request_path:
            page = extract_page(request_path)
            remote = extract_remote(request_path)
            print("Page {} requested for remote {}".format(page, remote))
            msg = render_page(page, remote)

        elif "Callback" in request_path:
            print("Callback recieved:\n", post_body)

        else:
            print("Unknown request received:\n", post_body)
            code = 401

        print("<----- Request End -----")

        self.send_response(code, '')
        self.send_header('Content-Length', str(len(msg)))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header("Connection", "Close")
        self.end_headers()
        self.wfile.write((msg+"\n").encode("utf-8"))

    do_POST = do_GET
    do_DELETE = do_GET
    do_PUT = do_POST

def extract_page(path):
    match = re.search(r'(?<=page=)\d+', path)
    if not match:
        return '1'
    return match[0]

def extract_remote(path):
    match = re.search(r'(?<=getAllRecords/)\w+', path)
    return match[0]

def render_page(page, remote):
    with open('database/page{}.json'.format(page), 'r') as myfile:
        t = Template(myfile.read().replace('\n', ''))
        return t.render(local="http-dummy", remote=remote)

def main():
    port = 8800
    print(('Listening on localhost:%s' % port))
    server = HTTPServer(('', port), RequestHandler)
    server.serve_forever()


if __name__ == "__main__":
    main()
