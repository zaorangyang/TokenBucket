/*
Copyright (c) 2017 Erik Rigtorp <erik@rigtorp.se>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>

class TokenBucket {
public:
  TokenBucket() {}

  TokenBucket(const uint64_t rate, const uint64_t burstSize) {
    // 间隔多少微秒，往令牌桶放入一个token
    timePerToken_ = 1000000 / rate;
    // burst的概念未知，但是从这里的计算方式可以猜到，burstSize表示一批令牌桶的size
    // burstSize表示桶的容量，即表示可以装下多少令牌
    // timePerBurst_表示将burstSize个令牌放入令牌桶需要多少时间
    timePerBurst_ = burstSize * timePerToken_;
  }

  TokenBucket(const TokenBucket &other) {
    timePerToken_ = other.timePerToken_.load();
    timePerBurst_ = other.timePerBurst_.load();
  }

  TokenBucket &operator=(const TokenBucket &other) {
    timePerToken_ = other.timePerToken_.load();
    timePerBurst_ = other.timePerBurst_.load();
    return *this;
  }

  bool consume(const uint64_t tokens) {
    const uint64_t now =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
    /*
     * minTime：令牌桶的下边界 
     */
    const uint64_t minTime =
        now - timePerBurst_.load(std::memory_order_relaxed);
    uint64_t oldTime = time_.load(std::memory_order_relaxed);
    /*
     * 初始化newTime 
     */
    uint64_t newTime = std::max(minTime, oldTime);

    const uint64_t timeNeeded =
    tokens * timePerToken_.load(std::memory_order_relaxed);
    for (;;) {
      newTime += timeNeeded;
      if (newTime > now) {
        return false;
      }
      if (time_.compare_exchange_weak(oldTime, newTime,
                                      std::memory_order_relaxed,
                                      std::memory_order_relaxed)) {
        return true;
      }
      newTime = oldTime;
    }

    return false;
  }

private:
  /*
   * time_表示当前令牌桶推进的进度 
   */
  std::atomic<uint64_t> time_ = {0};
  std::atomic<uint64_t> timePerToken_ = {0};
  std::atomic<uint64_t> timePerBurst_ = {0};
};
