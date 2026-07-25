#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple demo DLP client.

Connects to 127.0.0.1:8080 (see analysis-backend/simple_server.py) and sends
newline-delimited JSON events every 2 seconds.
"""

from __future__ import annotations

import json
import random
import socket
import threading
import time
from dataclasses import dataclass


@dataclass
class DLPClientConfig:
    server_host: str = "127.0.0.1"
    server_port: int = 8080


class DLPClient:
    def __init__(self, config: DLPClientConfig | None = None) -> None:
        self.config = config or DLPClientConfig()
        self.running = False
        self.client_socket: socket.socket | None = None
        self.worker_thread_obj: threading.Thread | None = None

    def generate_random_event(self) -> dict:
        now = time.time()
        candidates = [
            {"type": "keystroke", "data": "user_typed_text", "timestamp": now},
            {"type": "window_focus", "data": "Slack - Team Chat", "timestamp": now},
            {"type": "file_access", "data": r"C:\Users\user\Documents\secret.txt", "timestamp": now},
            {"type": "network_activity", "data": "https://example.com/upload", "timestamp": now},
        ]
        return random.choice(candidates)

    def generate_suspicious_event(self) -> dict:
        now = time.time()
        return {
            "type": "suspicious_activity",
            "data": {
                "activity": "potential_data_leak",
                "description": "User attempted to copy sensitive data",
                "severity": "HIGH",
                "timestamp": now,
            },
            "timestamp": now,
        }

    def connect_to_server(self) -> bool:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.config.server_host, self.config.server_port))
            self.client_socket = sock
            print(f"Connected to {self.config.server_host}:{self.config.server_port}")
            return True
        except Exception as exc:
            print(f"Failed to connect to {self.config.server_host}:{self.config.server_port}: {exc}")
            return False

    def send_event(self, event: dict) -> None:
        try:
            if not self.client_socket:
                return
            payload = (json.dumps(event, ensure_ascii=False) + "\n").encode("utf-8")
            self.client_socket.sendall(payload)
            print(f"Sent: {event.get('type')}")
        except Exception as exc:
            print(f"Send failed: {exc}")

    def worker_thread(self) -> None:
        counter = 0
        while self.running:
            counter += 1
            self.send_event(self.generate_random_event())
            if counter % 5 == 0:
                self.send_event(self.generate_suspicious_event())
            time.sleep(2)

    def start(self) -> bool:
        if not self.connect_to_server():
            return False
        self.running = True
        self.worker_thread_obj = threading.Thread(target=self.worker_thread, daemon=True)
        self.worker_thread_obj.start()
        print("Client running. Press Ctrl+C to stop.")
        return True

    def stop(self) -> None:
        self.running = False
        if self.worker_thread_obj:
            self.worker_thread_obj.join(timeout=2)
        if self.client_socket:
            try:
                self.client_socket.close()
            except Exception:
                pass
        print("Client stopped.")


def main() -> None:
    client = DLPClient()
    try:
        if client.start():
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        pass
    finally:
        client.stop()


if __name__ == "__main__":
    main()

