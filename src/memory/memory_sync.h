#pragma once
// Bounded local view of server-approved preferences + an outbound change queue.
// Pure logic: persistence to NVS is layered on top in the storage module.
#include <stddef.h>
#include <stdint.h>

namespace weby {

const size_t kMemKeyMax = 24;     // incl. NUL
const size_t kMemValueMax = 64;   // incl. NUL
const size_t kMemSlots = 16;
const size_t kPendingSlots = 8;

enum class ApplyResult : uint8_t { APPLIED, STALE, INVALID, FULL };

bool memoryKeyValid(const char* key);       // [a-z0-9_.], 1..23 chars
bool memoryValueValid(const char* value);   // <= 63 chars, no control characters

class MemoryStore {
 public:
  MemoryStore();
  // Per-key monotonic versions: an update with version <= the stored one is STALE (duplicate/replay).
  // An empty value deletes the key (the version is still remembered via a tombstone-free rule:
  // deletion removes the slot, so a later lower-versioned set is only blocked while the slot lives).
  ApplyResult applyServerUpdate(const char* key, const char* value, uint32_t version);
  const char* get(const char* key) const;
  size_t count() const;
  void clear();

 private:
  struct Slot { bool used; char key[kMemKeyMax]; char value[kMemValueMax]; uint32_t version; };
  Slot slots_[kMemSlots];
  int find(const char* key) const;
};

class PendingQueue {
 public:
  PendingQueue() : n_(0) {}
  // Latest value for a key wins (dedupe). Returns false if invalid or the queue is full.
  bool enqueue(const char* key, const char* value);
  size_t count() const { return n_; }
  bool at(size_t i, const char*& key, const char*& value) const;
  // Remove an entry after the server confirmed it, but ONLY if it was not changed meanwhile.
  bool acknowledge(const char* key, const char* sentValue);

 private:
  struct Item { char key[kMemKeyMax]; char value[kMemValueMax]; };
  Item items_[kPendingSlots];
  size_t n_;
  int find(const char* key) const;
};

}  // namespace weby
