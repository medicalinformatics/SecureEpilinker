#!/usr/bin/env python
# Reflects the requests from HTTP methods GET, POST, PUT, and DELETE
# Written by Nathan Hamiel (2010)

from http.server import HTTPServer, BaseHTTPRequestHandler
from optparse import OptionParser

class RequestHandler(BaseHTTPRequestHandler):
    
    def do_POST(self):
        
        request_path = self.path
        
        print("\n----- Request Start ----->\n")
        print(request_path)
        print((self.headers))
        print("<----- Request End -----\n")

        self.send_response(200)
        self.send_header("Connection", "Close")
        self.send_header('Content-Length', str(0))
        self.end_headers()
        
    def do_GET(self):
        
        request_path = self.path
        
        print("\n----- Request Start ----->\n")
        print(request_path)
        
        request_headers = self.headers
        print(request_headers)
        if 'Authorization' in request_headers:
            print("Authorization:", request_headers['Authorization'])
        else:
            print("Kein Auth-Header!")
        #length = 1755

        
        #print((self.rfile.read(length)))
        print("<----- Request End -----\n")
        
        if(request_path.find("page=2")== -1 and request_path.find("page=3")== -1):
            print("First Page")
            with open('database/page1.json', 'r') as myfile:
                msg=myfile.read().replace('\n', '')
        if(request_path.find("page=2")!= -1):
            print("Second Page")
            with open('database/page2.json', 'r') as myfile:
                msg=myfile.read().replace('\n', '')
        if(request_path.find("page=3")!= -1):
            with open('database/page3.json', 'r') as myfile:
                msg=myfile.read().replace('\n', '')
        self.send_response(200,'')
        self.send_header('Content-Length', str(len(msg)))
        self.end_headers()
        self.wfile.write((msg+"\n").encode("utf-8"))
 
    do_PUT = do_POST
    do_DELETE = do_GET
        
def main():
    port = 8800
    print(('Listening on localhost:%s' % port))
    server = HTTPServer(('', port), RequestHandler)
    server.serve_forever()

        
if __name__ == "__main__":
    parser = OptionParser()
    parser.usage = ("Creates an http-server that will echo out any GET or POST parameters\n"
                    "Run:\n\n"
                    "   reflect")
    (options, args) = parser.parse_args()
    
main()
