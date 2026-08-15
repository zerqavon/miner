import argparse
import json
import socket
import threading
import time


SEED = "00" * 32
FEE_BLOB = "10" + "10" + "00" + ("00" * 32) + ("00" * 4) + ("11" * 32) + "01"
USER_BLOB = "5a515658504f5701" + "10" + "10" + "00" + ("22" * 32) + ("33" * 32) + "01" + ("00" * 4)


class Server:
    def __init__(self, name, port, blob):
        self.name = name
        self.port = port
        self.blob = blob
        self.submits = 0
        self.logins = 0
        self._stop = threading.Event()

    def run(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(("127.0.0.1", self.port))
            server.listen()
            server.settimeout(0.2)
            while not self._stop.is_set():
                try:
                    client, _ = server.accept()
                except socket.timeout:
                    continue
                threading.Thread(target=self.handle, args=(client,), daemon=True).start()

    def handle(self, client):
        with client:
            stream = client.makefile("rwb")
            while not self._stop.is_set():
                line = stream.readline()
                if not line:
                    return
                request = json.loads(line)
                if request.get("method") == "login":
                    self.logins += 1
                    response = {
                        "id": 1,
                        "jsonrpc": "2.0",
                        "result": {
                            "id": f"{self.name}-session",
                            "job": {
                                "job_id": f"{self.name}-job",
                                "blob": self.blob,
                                "target": "ffffffff",
                                "seed_hash": SEED,
                                "height": 1,
                            },
                        },
                    }
                elif request.get("method") == "submit":
                    self.submits += 1
                    response = {"id": request.get("id"), "jsonrpc": "2.0", "result": {"status": "OK"}}
                else:
                    response = {"id": request.get("id"), "jsonrpc": "2.0", "error": {"message": "unknown method"}}
                stream.write((json.dumps(response) + "\n").encode())
                stream.flush()

    def stop(self):
        self._stop.set()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime", type=int, default=24)
    parser.add_argument("--user-delay", type=int, default=13)
    args = parser.parse_args()

    fee = Server("fee", 19090, FEE_BLOB)
    fee_thread = threading.Thread(target=fee.run)
    fee_thread.start()
    user = Server("user", 19091, USER_BLOB)
    user_thread = None

    started = time.monotonic()
    try:
        while time.monotonic() - started < args.runtime:
            if user_thread is None and time.monotonic() - started >= args.user_delay:
                user_thread = threading.Thread(target=user.run)
                user_thread.start()
            time.sleep(0.1)
    finally:
        fee.stop()
        user.stop()
        fee_thread.join()
        if user_thread:
            user_thread.join()
        print(json.dumps({
            "fee_logins": fee.logins,
            "fee_submits": fee.submits,
            "user_logins": user.logins,
            "user_submits": user.submits,
        }))


if __name__ == "__main__":
    main()

