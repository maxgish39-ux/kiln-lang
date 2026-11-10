"""Tiny local HTTP server used by tests/native/test-network.sh.

Usage: http_server.py [CERT KEY]

Prints the port it chose on its first line, then serves until killed. With a
certificate and key it serves HTTPS instead.
"""
import json
import ssl
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def _send(self, status, body, content_type="text/plain", extra=None, chunked=False):
        data = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        for name, value in (extra or {}).items():
            self.send_header(name, value)
        if chunked:
            self.send_header("Transfer-Encoding", "chunked")
            self.end_headers()
            for piece in (data[:5], data[5:]):
                self.wfile.write(b"%x\r\n%s\r\n" % (len(piece), piece))
            self.wfile.write(b"0\r\n\r\n")
        else:
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

    def do_GET(self):
        if self.path == "/hello":
            self._send(200, "hello from server", extra={"X-Test": "yes"})
        elif self.path == "/json":
            self._send(200, json.dumps({"name": "Kiln", "n": [1, 2]}), "application/json")
        elif self.path == "/chunked":
            self._send(200, "chunk one and two", chunked=True)
        elif self.path == "/headers":
            self._send(200, "client=" + (self.headers.get("X-Client") or "none"))
        elif self.path == "/broken-json":
            self._send(500, "server error")
        else:
            self._send(404, "not here")

    def do_POST(self):
        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length).decode("utf-8")
        self._send(201, "echo:" + body + ":" + (self.headers.get("Content-Type") or ""))

    def log_message(self, *args):
        pass


if __name__ == "__main__":
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    if len(sys.argv) == 3:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(certfile=sys.argv[1], keyfile=sys.argv[2])
        server.socket = context.wrap_socket(server.socket, server_side=True)
    print(server.server_address[1], flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    sys.exit(0)
