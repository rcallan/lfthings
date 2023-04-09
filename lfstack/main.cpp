#include <iostream>
#include <vector>
#include <thread>
#include "lfs.h"

int main() {
    LockFreeStack<int> lfs;

    std::vector<std::thread> threads;

    // would probably like to add a thread pool here

    int numElements = 10000;

    for (unsigned i = 0; i < numElements; ++i) {
        // lfs.push(i);
        threads.emplace_back(std::thread(&LockFreeStack<int>::push, &lfs, i));
    }

    for (auto& t : threads) {
        t.join();
    }

    threads.clear();

    for (unsigned i = 0; i < numElements; ++i) {
        threads.emplace_back(std::thread(&LockFreeStack<int>::pop, &lfs));
    }

    for (auto& t : threads) {
        t.join();
    }

    // auto temp = lfs.pop();

    // std::cout << *temp << std::endl;

    return 0;
}