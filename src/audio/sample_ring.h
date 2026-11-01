#pragma once
// Bounded single-producer/single-consumer ring of 16-bit samples over caller-owned
// storage (no heap). Producer = I2S capture task, consumer = voice/upload task.
// Overflow is counted, never silent.
#include <atomic>
#include <stddef.h>
#include <stdint.h>

namespace weby {

class SampleRing {
 public:
  SampleRing(int16_t* storage, size_t slots)
      : buf_(storage), cap_(slots), head_(0), tail_(0), droppedSamples_(0), overflowEvents_(0) {}

  size_t capacity() const { return cap_ > 1 ? cap_ - 1 : 0; }  // one slot is kept free
  size_t available() const {
    if (cap_ < 2) return 0;
    size_t h = head_.load(std::memory_order_acquire);
    size_t t = tail_.load(std::memory_order_acquire);
    return (h + cap_ - t) % cap_;
  }
  size_t freeSpace() const { return capacity() - available(); }

  // Producer side. Writes as many samples as fit; the rest are dropped and counted.
  size_t write(const int16_t* src, size_t n) {
    if (cap_ < 2 || !src || n == 0) return 0;
    size_t h = head_.load(std::memory_order_relaxed);
    size_t t = tail_.load(std::memory_order_acquire);
    size_t freeN = capacity() - ((h + cap_ - t) % cap_);
    size_t w = n < freeN ? n : freeN;
    for (size_t i = 0; i < w; ++i) buf_[(h + i) % cap_] = src[i];
    head_.store((h + w) % cap_, std::memory_order_release);
    if (w < n) { droppedSamples_ += (uint32_t)(n - w); ++overflowEvents_; }
    return w;
  }

  // Consumer side.
  size_t read(int16_t* dst, size_t n) {
    if (cap_ < 2 || !dst || n == 0) return 0;
    size_t t = tail_.load(std::memory_order_relaxed);
    size_t h = head_.load(std::memory_order_acquire);
    size_t avail = (h + cap_ - t) % cap_;
    size_t r = n < avail ? n : avail;
    for (size_t i = 0; i < r; ++i) dst[i] = buf_[(t + i) % cap_];
    tail_.store((t + r) % cap_, std::memory_order_release);
    return r;
  }

  // Call only while neither producer nor consumer is running.
  void clear() { head_.store(0); tail_.store(0); }
  uint32_t droppedSamples() const { return droppedSamples_; }
  uint32_t overflowEvents() const { return overflowEvents_; }

 private:
  int16_t* buf_;
  size_t cap_;
  std::atomic<size_t> head_, tail_;
  uint32_t droppedSamples_;   // written by producer only
  uint32_t overflowEvents_;
};

}  // namespace weby
