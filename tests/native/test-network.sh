#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
PIDS=""
cleanup() { for p in $PIDS; do kill "$p" 2>/dev/null || true; done; rm -rf "$TMP"; }
trap cleanup EXIT
if ! command -v python3 >/dev/null 2>&1; then
    echo "SKIP: python3 is needed for the local test server"
    exit 0
fi
cc -std=c11 -O2 -Wall -Wextra -Werror "$ROOT/runtime/kiln.c" -lm -o "$TMP/kiln"

# start_server [CERT KEY]: starts the test server and prints its base URL.
start_server() {
    python3 "$ROOT/tests/native/http_server.py" "$@" > "$TMP/port.$$" 2>"$TMP/server.err" &
    PIDS="$PIDS $!"
    i=0
    while [ ! -s "$TMP/port.$$" ] && [ $i -lt 50 ]; do sleep 0.1; i=$((i+1)); done
    PORT="$(head -1 "$TMP/port.$$")"
    rm -f "$TMP/port.$$"
    [ -n "$PORT" ] || { echo "local test server did not start" >&2; cat "$TMP/server.err" >&2; exit 1; }
}

start_server
BASE="http://127.0.0.1:$PORT"
cat > "$TMP/net.kiln" <<KILN
import "http" as http
let r = http.get("$BASE/hello")
print(str(r["status"]) + " " + str(r["ok"]) + " " + r["body"] + " " + r["headers"]["x-test"])
let j = http.get_json("$BASE/json")
print(j["name"] + " " + str(j["n"][1]))
print(http.get("$BASE/chunked")["body"])
let p = http.post("$BASE/echo", "payload")
print(str(p["status"]) + " " + p["body"])
let h = http.request("GET", "$BASE/headers", null, {"X-Client": "kiln-test"})
print(h["body"])
let missing = http.get("$BASE/nothing")
print(str(missing["status"]) + " " + str(missing["ok"]))
try {
    http.get_json("$BASE/broken-json")
} catch (e) {
    print("get_json refused: " + e)
}
try {
    http.get("http://127.0.0.1:1/")
} catch (e) {
    print("unreachable caught")
}
KILN
OUT="$("$TMP/kiln" "$TMP/net.kiln")"
EXPECTED="200 true hello from server yes
Kiln 2
chunk one and two
201 echo:payload:text/plain; charset=utf-8
client=kiln-test
404 false
get_json refused: http.get_json: server replied with status 500
unreachable caught"
if [ "$OUT" != "$EXPECTED" ]; then
    echo "network test output mismatch" >&2
    printf 'expected:\n%s\ngot:\n%s\n' "$EXPECTED" "$OUT" >&2
    exit 1
fi

# HTTPS needs curl and openssl; skip just the HTTPS part if either is missing.
if ! command -v curl >/dev/null 2>&1 || ! command -v openssl >/dev/null 2>&1; then
    echo "SKIP: curl and openssl are needed for the HTTPS tests"
    echo "Native network tests passed (HTTP only)."
    exit 0
fi
openssl req -x509 -newkey rsa:2048 -nodes -days 1 -keyout "$TMP/key.pem" -out "$TMP/cert.pem" \
    -subj "/CN=127.0.0.1" -addext "subjectAltName=IP:127.0.0.1" >/dev/null 2>&1 || {
    echo "SKIP: could not create a test certificate"; echo "Native network tests passed (HTTP only)."; exit 0; }
start_server "$TMP/cert.pem" "$TMP/key.pem"
SBASE="https://127.0.0.1:$PORT"
cat > "$TMP/tls.kiln" <<KILN
import "http" as http
let r = http.get("$SBASE/hello")
print(str(r["status"]) + " " + r["body"])
let p = http.post("$SBASE/echo", "secure")
print(p["body"])
let j = http.get_json("$SBASE/json")
print(j["name"])
KILN
cat > "$TMP/untrusted.kiln" <<KILN
import "http" as http
try {
    http.get("$SBASE/hello")
    print("accepted an untrusted certificate")
} catch (e) {
    print("untrusted certificate refused")
}
KILN
# Trust the test certificate through curl's standard CA-bundle variable.
OUT="$(CURL_CA_BUNDLE="$TMP/cert.pem" "$TMP/kiln" "$TMP/tls.kiln" 2>"$TMP/tls.err")" || { echo "https test failed" >&2; cat "$TMP/tls.err" >&2; exit 1; }
EXPECTED="$(printf '%s\n%s\n%s' "200 hello from server" "echo:secure:text/plain; charset=utf-8" "Kiln")"
if [ "$OUT" != "$EXPECTED" ]; then
    echo "https output mismatch" >&2; printf 'got:\n%s\n' "$OUT" >&2; exit 1
fi
# Without the trust setting, curl refuses the self-signed certificate and Kiln reports it.
UNTRUSTED="$(env -u CURL_CA_BUNDLE "$TMP/kiln" "$TMP/untrusted.kiln" 2>/dev/null)"
[ "$UNTRUSTED" = "untrusted certificate refused" ] || { echo "untrusted certificate was not refused: $UNTRUSTED" >&2; exit 1; }
echo "Native network tests passed (HTTP and HTTPS)."
