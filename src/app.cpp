#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include "event.h"
#include "config_loader.h"

void render_thread();
void input_thread();

int main() {
    // load config – silently falls back to defaults on failure
    load_config("snake.cfg");

    std::mutex mtx;
    std::condition_variable cv;
    bool should_exit = false;
    
    std::thread renderThread(render_thread);
    std::thread inputThread(input_thread);
    
    // monitor thread to check for quit request or timeout
    std::thread monitor([&]() {
        auto start = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(10);
        
        while (!EventServer::is_quit_requested()) {
            auto now = std::chrono::steady_clock::now();
            if (now - start > timeout) {
                std::cout << "Timeout reached, forcing quit..." << std::endl;
                EventServer::request_quit();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        {
            std::lock_guard<std::mutex> lock(mtx);
            should_exit = true;
        }
        cv.notify_one();
    });
    
    // wait without busy waiting
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return should_exit; });
    }
    
    renderThread.join();
    inputThread.join();
    monitor.join();

    // save config on clean exit
    save_config("snake.cfg");

    return 0;
}