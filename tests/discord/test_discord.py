import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import shutil
import socket
import ssl
import struct
import subprocess
import tempfile
import threading
import time


def checked(*args, **kwargs):
    run = subprocess.run(args, capture_output=True, text=True, timeout=25, **kwargs)
    assert run.returncode == 0, (args, run.stdout, run.stderr)
    return run.stdout


def frame(socket, opcode, body, final=True):
    first = opcode | (0x80 if final else 0)
    extended = bytes([len(body)]) if len(body) < 126 else bytes([126]) + struct.pack(">H", len(body))
    socket.sendall(bytes([first]) + extended + body)


def payload(wire):
    head = wire.read(2)
    assert len(head) == 2 and head[1] & 128, head
    length = head[1] & 127
    if length == 126:
        length = struct.unpack(">H", wire.read(2))[0]
    elif length == 127:
        length = struct.unpack(">Q", wire.read(8))[0]
    mask = wire.read(4)
    return head[0] & 15, bytes(value ^ mask[index % 4] for index, value in enumerate(wire.read(length)))


def ws_json(socket, body, final=True, opcode=1):
    frame(socket, opcode, json.dumps(body, separators=(",", ":")).encode(), final)


def handshake(connection, wire, headers):
    challenge = headers[b"sec-websocket-key"]
    accept = base64.b64encode(hashlib.sha1(challenge + b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11").digest())
    connection.sendall(b"HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n"
                       b"Sec-WebSocket-Accept: " + accept + b"\r\n\r\n")
    ws_json(connection, {"op": 10, "d": {"heartbeat_interval": 1000}})
    opcode, body = payload(wire)
    hello = json.loads(body)
    assert opcode == 1 and hello["op"] == 2 and hello["d"]["token"] == "abc.mock.token", hello
    ready = json.dumps({"op": 0, "s": 42, "t": "READY", "d": {
        "session_id": "session-123", "resume_gateway_url": "wss://gateway-us-east.discord.gg"}}).encode()
    frame(connection, 1, ready[:20], False)
    frame(connection, 0, ready[20:])
    while True:
        opcode, body = payload(wire)
        if opcode == 10:
            continue
        beat = json.loads(body)
        assert opcode == 1 and beat["op"] == 1 and beat["d"] in (None, 42), beat
        ws_json(connection, {"op": 11, "d": None})
        if beat["d"] == 42:
            break
    ws_json(connection, {"op": 0, "s": 43, "t": "INTERACTION_CREATE", "d": {"name": "ping"}})
    frame(connection, 8, struct.pack(">H", 4014))


def answer(listener, context, case, errors):
    try:
        with listener.accept()[0] as plain, context.wrap_socket(plain, server_side=True) as connection:
            connection.settimeout(8)
            with connection.makefile("rb") as wire:
                request = wire.readline()
                headers = {}
                for line in iter(wire.readline, b"\r\n"):
                    if not line:
                        raise EOFError("request ended before the HTTP headers")
                    name, _, value = line.partition(b":")
                    headers[name.lower()] = value.strip()
                if case == "https":
                    assert request.startswith(b"PATCH /test "), request
                    assert headers[b"authorization"] == b"Bot test-only", headers
                    assert wire.read(int(headers[b"content-length"])) == b"A\0\xffZ"
                    connection.sendall(b"HTTP/1.1 201 Created\r\nContent-Length: 4\r\n"
                                       b"X-RateLimit-Remaining: 0\r\nConnection: close\r\n\r\nA\0\xffZ")
                else:
                    assert request.startswith(b"GET /test "), request
                    handshake(connection, wire, headers)
    except Exception as error:
        errors.append(error)


def probe_context(cert, key, probe, tls):
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(cert, key)
    for case, scheme in (("https", "https"), ("gateway", "wss")):
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            listener.listen()
            listener.settimeout(15)
            errors = []
            server = threading.Thread(target=answer, args=(listener, context, case, errors), daemon=True)
            server.start()
            url = f"{scheme}://localhost:{listener.getsockname()[1]}/test"
            run = subprocess.run([probe, case, url, tls], capture_output=True, text=True, timeout=25)
            if case == "gateway" and run.returncode == 9 and run.stdout.strip() == "-31007" and os.name != "nt":
                print("skip Linux TLS Gateway fixture: libcurl build has no WSS support")
                continue
            assert run.returncode == 0, (case, run.stdout, run.stderr, errors)
            server.join(10)
            assert not server.is_alive() and not errors, errors
            print(run.stdout.strip())


def check_signature(probe, folder):
    private_key = folder / "signing.pem"
    public_key = folder / "public.der"
    message = folder / "signed-message"
    signature = folder / "signature"
    timestamp = str(int(time.time()))
    payload = "test-discord-body"
    checked("openssl", "genpkey", "-algorithm", "ED25519", "-out", private_key)
    checked("openssl", "pkey", "-in", private_key, "-pubout", "-outform", "DER", "-out", public_key)
    message.write_bytes((timestamp + payload).encode())
    checked("openssl", "pkeyutl", "-sign", "-rawin", "-inkey", private_key, "-in", message, "-out", signature)
    key_hex, signature_hex = public_key.read_bytes()[-32:].hex(), signature.read_bytes().hex()
    checked(probe, "signature", key_hex, signature_hex, timestamp, payload)
    assert subprocess.run([probe, "signature", key_hex, signature_hex, timestamp, payload + "!"], timeout=15).returncode == 12
    print("Discord timestamped Ed25519 signature and tamper rejection")


def install_fixture(compiler, root, folder):
    exe = ".exe" if os.name == "nt" else ""
    driver = compiler.with_name(f"rz{exe}")
    source = folder / "razum-discord"
    shutil.copytree(root / "examples/packages/discord", source, ignore=shutil.ignore_patterns("build"))
    checked("git", "init", "-q", "--template=", "-b", "main", cwd=source)
    checked("git", "add", ".", cwd=source)
    checked("git", "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid",
            "commit", "-qm", "Discord test package", cwd=source)
    checked("git", "tag", "v0.1.0", cwd=source)
    consumer = folder / "consumer"
    consumer.mkdir()
    shutil.copy2(root / "tests/discord/probe.rz", consumer / "main.rz")
    (consumer / "razum.toml").write_text(
        'manifest_version = 1\n[project]\nname = "test-discord"\n'
        '[[target]]\nname = "probe"\nkind = "exe"\nsource = "main.rz"\n'
        f'[dependencies]\ndiscord = {{git = "{source.as_uri()}", tag = "v0.1.0"}}\n',
        encoding="utf8",
    )
    env = {**os.environ, "RZ_CACHE": str(folder / "cache"), "RZC": str(compiler)}
    checked(driver, "build", "--profile", "release", cwd=consumer, env=env)
    source.rename(folder / "unreachable")
    warm = checked(driver, "build", "--profile", "release", "--offline", cwd=consumer, env=env)
    assert "Up to date" in warm, warm
    target = "windows-x64" if os.name == "nt" else "linux-x64"
    print("Discord installed from an exact Git tag and rebuilt offline")
    return consumer / "build" / target / "release" / f"probe{exe}"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("compiler", type=Path)
    parser.add_argument("--trust-local", action="store_true")
    parser.add_argument("--no-tls", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    windows = os.name == "nt"
    with tempfile.TemporaryDirectory(prefix="razum-discord-") as scratch:
        folder = Path(scratch)
        probe = install_fixture(args.compiler.resolve(), root, folder)
        message = json.loads(checked(probe, "helpers"))
        assert message == {"content": "\"@everyone\n", "allowed_mentions": {"parse": []}, "flags": 64}, message
        print("Discord payload escaping, rate-limit state and route keys")
        checked(probe, "multipart")
        print("Discord multipart attachments preserve binary bytes")
        if shutil.which("openssl") is None:
            print("skip generated-signature and TLS fixtures: openssl is unavailable")
            return
        check_signature(probe, folder)
        if args.no_tls or (windows and not args.trust_local):
            return
        cert, key = folder / "localhost.pem", folder / "localhost.key"
        checked("openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
                "-subj", "/CN=localhost", "-addext", "subjectAltName=DNS:localhost",
                "-keyout", key, "-out", cert)
        if windows:
            thumbprint = hashlib.sha1(ssl.PEM_cert_to_DER_cert(cert.read_text())).hexdigest()
            try:
                checked("certutil", "-user", "-addstore", "Root", cert)
                probe_context(cert, key, probe, "")
            finally:
                installed = subprocess.run(["certutil", "-user", "-store", "Root", thumbprint],
                                           capture_output=True, timeout=10)
                if installed.returncode == 0 and b"Cannot find" not in installed.stdout:
                    checked("certutil", "-user", "-delstore", "Root", thumbprint)
        else:
            probe_context(cert, key, probe, cert)


if __name__ == "__main__":
    main()
