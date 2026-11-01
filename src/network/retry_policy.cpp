#include "network/retry_policy.h"

namespace weby {

HttpAction classifyHttpStatus(int s) {
  if (s <= 0) return HttpAction::RETRY;
  if (s >= 200 && s < 300) return HttpAction::SUCCESS;
  if (s == 401) return HttpAction::REAUTH;
  if (s == 408 || s == 425 || s == 429) return HttpAction::RETRY;
  if (s >= 500 && s < 600) return (s == 501 || s == 505) ? HttpAction::FAIL : HttpAction::RETRY;
  return HttpAction::FAIL;
}

uint32_t Backoff::nextDelayMs(uint16_t jitterPermille) {
  if (jitterPermille > 1000) jitterPermille = 1000;
  uint32_t d = cfg_.maxMs;
  if (attempt_ < 20) {
    uint64_t v = (uint64_t)cfg_.baseMs << attempt_;
    if (v < cfg_.maxMs) d = (uint32_t)v;
  }
  if (attempt_ < 255) ++attempt_;
  return d / 2 + (uint32_t)(((uint64_t)(d - d / 2) * jitterPermille) / 1000);
}

}  // namespace weby
