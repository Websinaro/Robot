#pragma once
#include <stdint.h>

namespace weby {

enum class HttpAction : uint8_t {
  SUCCESS,   // 2xx
  RETRY,     // transient: transport error, 408/425/429, 5xx (except 501/505)
  REAUTH,    // 401: credential expired/revoked -> controlled re-authentication
  FAIL       // permanent: other 3xx/4xx, 501, 505
};

// status <= 0 means a transport-level failure (no HTTP response).
HttpAction classifyHttpStatus(int status);

struct BackoffConfig {
  uint32_t baseMs;
  uint32_t maxMs;
  uint8_t maxAttempts;
  BackoffConfig() : baseMs(500), maxMs(30000), maxAttempts(5) {}
};

// Exponential backoff with "equal jitter": delay in [d/2, d], d = min(max, base * 2^attempt).
// The caller supplies randomness (0..1000) so the logic is deterministic and testable.
class Backoff {
 public:
  explicit Backoff(const BackoffConfig& cfg = BackoffConfig()) : cfg_(cfg), attempt_(0) {}
  void reset() { attempt_ = 0; }
  bool canRetry() const { return attempt_ < cfg_.maxAttempts; }
  uint8_t attempts() const { return attempt_; }
  uint32_t nextDelayMs(uint16_t jitterPermille);  // consumes one attempt

 private:
  BackoffConfig cfg_;
  uint8_t attempt_;
};

}  // namespace weby
