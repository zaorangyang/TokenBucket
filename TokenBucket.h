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

#include <atomic>
#include <chrono>

class TokenBucket {
public:
  TokenBucket() {}

  /*
   * rate: qps
   * burstSize: 令牌桶大小
   */
  TokenBucket(const uint64_t rate, const uint64_t burstSize) {
    /* timePerToken_表示放置一个令牌需要多少微妙 */
    timePerToken_ = 1000000 / rate;
    /* 将令牌桶装满需要多上时间 */
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
    const uint64_t timeNeeded =
        tokens * timePerToken_.load(std::memory_order_relaxed);

    /*
     * oldTime表示上次令牌桶为空的时刻
     * 随着消费的进行，令牌桶为空的时刻沿着时间流逝的方向移动
     */
    uint64_t oldTime = time_.load(std::memory_order_relaxed);
    uint64_t newTime = oldTime;

    /* 在minTime时刻之前开始，一定可以将令牌桶装满 */
    const uint64_t minTime =
        now - timePerBurst_.load(std::memory_order_relaxed);
    /*
     * 注意这里：既然从minTime这里开始就一定可以将令牌桶装满，那么如果 
     * oldTime比minTime更早，则令牌桶一定是满的，此时将newTime设置为
     * minTime
     */
    if (minTime > oldTime) {
      newTime = minTime;
    }


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
  /* time_表示令牌桶为空的时刻 */
  std::atomic<uint64_t> time_ = {0};
  std::atomic<uint64_t> timePerToken_ = {0};
  std::atomic<uint64_t> timePerBurst_ = {0};
};
