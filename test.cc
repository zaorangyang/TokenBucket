#include "TokenBucket.h"
#include "unistd.h"
#include <iostream>
#include <thread>
#include <vector>
#include <functional>

using namespace std;

atomic_llong success_count;

void printCount() {
    while (true) {
        std::cout << success_count.load() << std::endl;
        usleep(1000000);
    }
}

void doConsume(TokenBucket& bucket) {
    while (true) {
        if (bucket.consume(1)) {
            success_count++;
        }
    }
}

int main() {
    thread t1(printCount);
    TokenBucket bucket(5000, 5000);
    int thread_num = 10;
    vector<thread> threads;
    threads.reserve(thread_num);
    for (int i=0; i<thread_num; i++) {
        threads[i] = thread(doConsume, std::ref(bucket));
    }
    t1.join();
}