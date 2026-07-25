#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <windows.h>

// Простой клиент для демонстрации работы системы
// Этот клиент будет имитировать отправку данных на сервер

class SimpleClient {
private:
    bool running;
    std::thread workerThread;
    
    void WorkerThread() {
        int counter = 0;
        while (running) {
            counter++;
            std::cout << "Клиент: Отправляю данные на сервер... (пакет #" << counter << ")" << std::endl;
            
            // Имитация отправки данных
            Sleep(2000);
            
            // Имитация обнаружения подозрительной активности
            if (counter % 5 == 0) {
                std::cout << "Клиент: Обнаружена подозрительная активность! Отправляю алерт..." << std::endl;
            }
        }
    }
    
public:
    SimpleClient() : running(false) {}
    
    void Start() {
        if (running) return;
        running = true;
        workerThread = std::thread(&SimpleClient::WorkerThread, this);
        std::cout << "Клиент запущен. Нажмите Enter для остановки..." << std::endl;
    }
    
    void Stop() {
        if (!running) return;
        running = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
        std::cout << "Клиент остановлен." << std::endl;
    }
};

int main() {
    std::cout << "=== Простой клиент DLP системы ===" << std::endl;
    std::cout << "Этот клиент имитирует работу настоящего агента." << std::endl;
    std::cout << "Он будет периодически отправлять данные на сервер." << std::endl;
    std::cout << "=========================================" << std::endl;
    
    SimpleClient client;
    client.Start();
    
    // Ожидаем нажатия Enter для остановки
    std::cin.get();
    
    client.Stop();
    
    return 0;
}