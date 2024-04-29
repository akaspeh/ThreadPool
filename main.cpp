
#include <thread>
#include <functional>
#include <chrono>
#include "ThreadPool.h"
#include <cstdlib>
#include <string>
#include <random>
#include <algorithm>
#include <barrier>
//Пул потоків обслуговується 4-ма робочими потоками й має одну чергу
//виконання. Задачі додаються відразу в кінець черги виконання. Черга
//задач виконується з інтервалом в 40 секунд (буфер наповнюється
//задачами на протязі 40-ти секунд, котрі потім виконуються), або
//відкидаються, якщо час виконання всіх задач в черзі та потенційної задачі
//для додавання більший ніж 60 секунд. Задачі, що додаються під час
//виконання черги задач відкладаються на наступну ітерацію (подвійна
//буферизація). Задача займає випадковий час від 6 до 14 секунд.

char getRandomChar() {
    static constexpr char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    static constexpr size_t max_index = (sizeof(charset) - 1);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, max_index);

    return charset[dis(gen)];
}

// Генерирует случайную строку заданной длины
std::string generateRandomString(size_t length) {
    std::string str(length, 0);
    std::generate_n(str.begin(), length, getRandomChar);
    return str;
}

void task(int time_s){
    ZoneScoped;
    const char* text = generateRandomString(time_s).c_str();
    size_t size = strlen(text);
    ZoneText(text, size);
    // Задержка
    std::this_thread::sleep_for(std::chrono::seconds (time_s));
}

void add_tasks_in_thread(ThreadPool& myThreadPool, int num_of_tasks_in_cycle, int cycles){
    for(int i = 0; i < cycles; i++){
        std::this_thread::sleep_for(std::chrono::seconds(rand() % 10 + 10));
        for(int j = 0; j < num_of_tasks_in_cycle; j++){
            myThreadPool.add_task(std::move(task), rand() % 9 + 6);
            std::this_thread::sleep_for(std::chrono::seconds(rand() % 6));
        }
    }
}

int main() {
    std::srand(std::time(nullptr));
    ThreadPool myThreadPool;
    myThreadPool.initialize(4);

    int numThreads = 2;
    std::vector<std::thread> threads;

    // Создание и запуск каждого потока
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread(add_tasks_in_thread, std::ref(myThreadPool), 10, 5));
    }

    // Ожидание завершения каждого потока
    for (auto& thread : threads) {
        thread.join();
    }

    //std::rand() % 9 + 6
//    myThreadPool.add_task(std::move(task), 7);
//    myThreadPool.add_task(std::move(task), 9);
//    myThreadPool.add_task(std::move(task), 6);
//    myThreadPool.add_task(std::move(task), 7);
//    myThreadPool.add_task(std::move(task), 11);
//    myThreadPool.add_task(std::move(task), 12);
//
//    std::this_thread::sleep_for(std::chrono::seconds(42));
//
//    myThreadPool.add_task(std::move(task), 14);
//    myThreadPool.add_task(std::move(task), 12);
//    myThreadPool.add_task(std::move(task), 8);
//
//    std::this_thread::sleep_for(std::chrono::seconds(25));
//
//    myThreadPool.add_task(std::move(task), 8);
//    myThreadPool.add_task(std::move(task), 13);
//    myThreadPool.add_task(std::move(task), 10);
//
//    std::this_thread::sleep_for(std::chrono::seconds(20));
//
//    myThreadPool.add_task(std::move(task), 14);
//    myThreadPool.add_task(std::move(task), 12);
//    myThreadPool.add_task(std::move(task), 8);
//    myThreadPool.add_task(std::move(task), 14);
//    myThreadPool.add_task(std::move(task), 11);
//    myThreadPool.add_task(std::move(task), 10);
//    myThreadPool.add_task(std::move(task), 12);
//
//    std::this_thread::sleep_for(std::chrono::seconds(60));

    std::cout << "Mean time wait" << myThreadPool.sv.mean_wait_ms()/1000.0 << "\n";

    myThreadPool.terminate();
    std::cout<< "end\n";
    return 0;
}
