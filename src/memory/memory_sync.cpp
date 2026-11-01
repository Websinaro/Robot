#include "memory/memory_sync.h"
#include <string.h>

namespace weby {

bool memoryKeyValid(const char* k) {
  if (!k) return false;
  size_t n = strlen(k);
  if (n == 0 || n >= kMemKeyMax) return false;
  for (size_t i = 0; i < n; ++i) {
    char c = k[i];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.')) return false;
  }
  return true;
}

bool memoryValueValid(const char* v) {
  if (!v) return false;
  size_t n = strlen(v);
  if (n >= kMemValueMax) return false;
  for (size_t i = 0; i < n; ++i)
    if ((unsigned char)v[i] < 0x20 || (unsigned char)v[i] == 0x7F) return false;
  return true;
}

MemoryStore::MemoryStore() { clear(); }

void MemoryStore::clear() { memset(slots_, 0, sizeof slots_); }

int MemoryStore::find(const char* key) const {
  for (size_t i = 0; i < kMemSlots; ++i)
    if (slots_[i].used && strcmp(slots_[i].key, key) == 0) return (int)i;
  return -1;
}

ApplyResult MemoryStore::applyServerUpdate(const char* key, const char* value, uint32_t version) {
  if (!memoryKeyValid(key) || !memoryValueValid(value)) return ApplyResult::INVALID;
  int idx = find(key);
  if (idx >= 0) {
    if (version <= slots_[idx].version) return ApplyResult::STALE;
    if (value[0] == 0) { slots_[idx].used = false; return ApplyResult::APPLIED; }   // delete
    strncpy(slots_[idx].value, value, kMemValueMax - 1);
    slots_[idx].value[kMemValueMax - 1] = 0;
    slots_[idx].version = version;
    return ApplyResult::APPLIED;
  }
  if (value[0] == 0) return ApplyResult::APPLIED;   // deleting a key we never had
  for (size_t i = 0; i < kMemSlots; ++i) {
    if (!slots_[i].used) {
      slots_[i].used = true;
      strncpy(slots_[i].key, key, kMemKeyMax - 1);
      slots_[i].key[kMemKeyMax - 1] = 0;
      strncpy(slots_[i].value, value, kMemValueMax - 1);
      slots_[i].value[kMemValueMax - 1] = 0;
      slots_[i].version = version;
      return ApplyResult::APPLIED;
    }
  }
  return ApplyResult::FULL;
}

const char* MemoryStore::get(const char* key) const {
  if (!key) return 0;
  int i = find(key);
  return i >= 0 ? slots_[i].value : 0;
}

size_t MemoryStore::count() const {
  size_t c = 0;
  for (size_t i = 0; i < kMemSlots; ++i) if (slots_[i].used) ++c;
  return c;
}

int PendingQueue::find(const char* key) const {
  for (size_t i = 0; i < n_; ++i) if (strcmp(items_[i].key, key) == 0) return (int)i;
  return -1;
}

bool PendingQueue::enqueue(const char* key, const char* value) {
  if (!memoryKeyValid(key) || !memoryValueValid(value)) return false;
  int i = find(key);
  if (i < 0) {
    if (n_ >= kPendingSlots) return false;
    i = (int)n_++;
    strncpy(items_[i].key, key, kMemKeyMax - 1);
    items_[i].key[kMemKeyMax - 1] = 0;
  }
  strncpy(items_[i].value, value, kMemValueMax - 1);
  items_[i].value[kMemValueMax - 1] = 0;
  return true;
}

bool PendingQueue::at(size_t i, const char*& key, const char*& value) const {
  if (i >= n_) return false;
  key = items_[i].key;
  value = items_[i].value;
  return true;
}

bool PendingQueue::acknowledge(const char* key, const char* sentValue) {
  if (!key || !sentValue) return false;
  int i = find(key);
  if (i < 0 || strcmp(items_[i].value, sentValue) != 0) return false;
  for (size_t j = (size_t)i; j + 1 < n_; ++j) items_[j] = items_[j + 1];
  --n_;
  return true;
}

}  // namespace weby
