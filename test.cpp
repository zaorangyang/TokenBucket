#include <iostream>
#include <ctime>
#include <thread>
#include <unistd.h>
#include <chrono>
#include <time.h>
#include "TokenBucket.h"

using namespace std::chrono;
using namespace std;


std::atomic<uint64_t> number{0};

std::time_t getTimeStamp()
{
    auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());//获取当前时间点
    std::time_t timestamp =  tp.time_since_epoch().count(); //计算距离1970-1-1,00:00的时间长度
    return timestamp;
}

std::atomic<long> start{getTimeStamp()};

void func(uint64_t rate, TokenBucket& t) {
    while(true)
    {
        if (t.consume(1))
        {
            if (++number % rate == 0 ) {
                std::atomic<long> end{getTimeStamp()};
                printf("used time: %ldms\n", end-start);
                start = getTimeStamp();
            }
        }
        // usleep(2000);
    }
}


int main()
{
    uint64_t rate = 1000;
    uint64_t bs = 1000;
    TokenBucket t(rate, bs);
    
    thread t1(func, rate, std::ref(t));
    thread t2(func, rate, std::ref(t));
    thread t3(func, rate, std::ref(t));
    t1.join();
    t2.join();
    t3.join();

    return 0;
}