#!/usr/bin/env python3
"""
Простой сервер для DLP системы
Этот сервер принимает данные от клиентов и обрабатывает их
"""

import socket
import json
import threading
import time
from datetime import datetime

class SimpleDLPServer:
    def __init__(self, host='127.0.0.1', port=8080):
        self.host = host
        self.port = port
        self.running = False
        self.server_socket = None
        self.clients = []
        
    def _respond_http_hint(self, client_socket):
        body = (
            "<!doctype html><html><head><meta charset='utf-8'>"
            "<title>memk demo server</title></head><body>"
            "<h2>memk demo server is running</h2>"
            "<p>This port (8080) is a demo <b>TCP JSON</b> server (not an HTTP website).</p>"
            "<p>Run <code>run.cmd demo-dashboard</code> and open "
            "<a href='http://localhost:3000'>http://localhost:3000</a> for the web UI.</p>"
            "</body></html>"
        )
        resp = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            f"Content-Length: {len(body.encode('utf-8'))}\r\n"
            "Connection: close\r\n"
            "\r\n"
            f"{body}"
        )
        try:
            client_socket.sendall(resp.encode("utf-8"))
        except Exception:
            pass
    def handle_client(self, client_socket, address):
        """Обрабатывает подключение клиента"""
        print(f"Новое подключение от {address}")
        
        try:
            while self.running:
                # Получаем данные от клиента
                raw = client_socket.recv(4096)
                if not raw:
                    break

                # If 127.0.0.1:8080 is opened in a browser, an HTTP request arrives here.
                if raw.startswith((b"GET ", b"POST ", b"HEAD ", b"PUT ", b"OPTIONS ")):
                    self._respond_http_hint(client_socket)
                    break

                data = raw.decode('utf-8', errors='replace')
                
                # Обрабатываем полученные данные
                self.process_data(data, address)
                
        except Exception as e:
            print(f"Ошибка при обработке клиента {address}: {e}")
        finally:
            client_socket.close()
            print(f"Клиент {address} отключен")
    
    def process_data(self, data, address):
        """Обрабатывает полученные данные"""
        try:
            # Разбиваем данные на сообщения (по символу новой строки)
            messages = data.strip().split('\n')
            
            for message in messages:
                if message.strip():
                    # Парсим JSON
                    json_data = json.loads(message)
                    
                    # Обрабатываем в зависимости от типа данных
                    if json_data['type'] == 'keystroke':
                        self.handle_keystroke(json_data, address)
                    elif json_data['type'] == 'window_focus':
                        self.handle_window_focus(json_data, address)
                    elif json_data['type'] == 'file_access':
                        self.handle_file_access(json_data, address)
                    elif json_data['type'] == 'network_activity':
                        self.handle_network_activity(json_data, address)
                    elif json_data['type'] == 'suspicious_activity':
                        self.handle_suspicious_activity(json_data, address)
                    else:
                        print(f"Неизвестный тип данных: {json_data['type']}")
                        
        except json.JSONDecodeError as e:
            print(f"Ошибка парсинга JSON: {e}")
        except Exception as e:
            print(f"Ошибка обработки данных: {e}")
    
    def handle_keystroke(self, data, address):
        """Обрабатывает события нажатия клавиш"""
        print(f"[{datetime.now()}] Клавиатурный ввод от {address[0]}: {data['data']}")
    
    def handle_window_focus(self, data, address):
        """Обрабатывает события фокусировки окон"""
        print(f"[{datetime.now()}] Фокус окна от {address[0]}: {data['data']}")
    
    def handle_file_access(self, data, address):
        """Обрабатывает события доступа к файлам"""
        print(f"[{datetime.now()}] Доступ к файлу от {address[0]}: {data['data']}")
    
    def handle_network_activity(self, data, address):
        """Обрабатывает сетевую активность"""
        print(f"[{datetime.now()}] Сетевая активность от {address[0]}: {data['data']}")
    
    def handle_suspicious_activity(self, data, address):
        """Обрабатывает подозрительную активность"""
        activity_data = data['data']
        print(f"\n🚨 [{datetime.now()}] ПОДОЗРИТЕЛЬНАЯ АКТИВНОСТЬ от {address[0]}!")
        print(f"   Тип: {activity_data['activity']}")
        print(f"   Описание: {activity_data['description']}")
        print(f"   Уровень угрозы: {activity_data['severity']}")
        print("-" * 50)
    
    def start(self):
        """Запускает сервер"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(5)
            
            self.running = True
            print(f"Сервер запущен на {self.host}:{self.port}")
            print("Ожидание подключений...")
            
            while self.running:
                try:
                    client_socket, address = self.server_socket.accept()
                    client_thread = threading.Thread(
                        target=self.handle_client, 
                        args=(client_socket, address)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                    
                except socket.error:
                    break
                    
        except Exception as e:
            print(f"Ошибка сервера: {e}")
        finally:
            self.stop()
    
    def stop(self):
        """Останавливает сервер"""
        self.running = False
        if self.server_socket:
            self.server_socket.close()
        print("Сервер остановлен")

def main():
    print("=== Простой сервер DLP системы ===")
    print("Этот сервер принимает данные от клиентов и обрабатывает их.")
    print("=========================================")
    
    server = SimpleDLPServer()
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("\nПолучен сигнал остановки...")
    finally:
        server.stop()

if __name__ == "__main__":
    main()
