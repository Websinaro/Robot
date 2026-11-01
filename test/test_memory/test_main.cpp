#include <unity.h>
#include <string.h>
#include "memory/memory_sync.h"

using namespace weby;
void setUp(void) {}
void tearDown(void) {}
#define EQ(e, a) TEST_ASSERT_EQUAL_INT((int)(e), (int)(a))

void test_key_and_value_validation() {
  TEST_ASSERT_TRUE(memoryKeyValid("pref.volume"));
  TEST_ASSERT_FALSE(memoryKeyValid(""));
  TEST_ASSERT_FALSE(memoryKeyValid("Has Space"));
  TEST_ASSERT_FALSE(memoryKeyValid("UPPER"));
  TEST_ASSERT_FALSE(memoryKeyValid("this_key_is_way_too_long_for_slot"));
  TEST_ASSERT_FALSE(memoryKeyValid(0));
  TEST_ASSERT_TRUE(memoryValueValid(""));
  TEST_ASSERT_FALSE(memoryValueValid("line\nbreak"));
  char big[80]; memset(big, 'a', 79); big[79] = 0;
  TEST_ASSERT_FALSE(memoryValueValid(big));
}

void test_server_update_apply_and_replay_rejected() {
  MemoryStore m;
  EQ(ApplyResult::APPLIED, m.applyServerUpdate("pref.color", "blue", 5));
  TEST_ASSERT_EQUAL_STRING("blue", m.get("pref.color"));
  EQ(ApplyResult::STALE, m.applyServerUpdate("pref.color", "red", 5));    // duplicate
  EQ(ApplyResult::STALE, m.applyServerUpdate("pref.color", "red", 4));    // older
  TEST_ASSERT_EQUAL_STRING("blue", m.get("pref.color"));
  EQ(ApplyResult::APPLIED, m.applyServerUpdate("pref.color", "green", 6));
  TEST_ASSERT_EQUAL_STRING("green", m.get("pref.color"));
}

void test_invalid_updates_rejected_without_side_effects() {
  MemoryStore m;
  EQ(ApplyResult::INVALID, m.applyServerUpdate("BAD KEY", "x", 1));
  EQ(ApplyResult::INVALID, m.applyServerUpdate("ok", "bad\x01value", 1));
  EQ(ApplyResult::INVALID, m.applyServerUpdate(0, "x", 1));
  EQ(0, m.count());
}

void test_delete_and_capacity() {
  MemoryStore m;
  m.applyServerUpdate("a", "1", 1);
  EQ(ApplyResult::APPLIED, m.applyServerUpdate("a", "", 2));              // delete
  TEST_ASSERT_TRUE(m.get("a") == 0);
  char k[8];
  for (int i = 0; i < (int)kMemSlots; ++i) { sprintf(k, "k%d", i); EQ(ApplyResult::APPLIED, m.applyServerUpdate(k, "v", 1)); }
  EQ(ApplyResult::FULL, m.applyServerUpdate("overflow", "v", 1));
  EQ((int)kMemSlots, m.count());
  m.clear(); EQ(0, m.count());
}

void test_pending_dedupes_latest_wins() {
  PendingQueue q;
  TEST_ASSERT_TRUE(q.enqueue("pref.volume", "40"));
  TEST_ASSERT_TRUE(q.enqueue("pref.volume", "70"));
  EQ(1, q.count());
  const char *k, *v; TEST_ASSERT_TRUE(q.at(0, k, v));
  TEST_ASSERT_EQUAL_STRING("70", v);
  TEST_ASSERT_FALSE(q.at(1, k, v));
}

void test_pending_ack_keeps_changes_made_during_sync() {
  PendingQueue q; q.enqueue("pref.volume", "40");
  q.enqueue("pref.volume", "55");                         // user changed it while "40" was in flight
  TEST_ASSERT_FALSE(q.acknowledge("pref.volume", "40"));  // stale ack must not drop the newer value
  EQ(1, q.count());
  TEST_ASSERT_TRUE(q.acknowledge("pref.volume", "55"));
  EQ(0, q.count());
}

void test_pending_bounded_and_ordered() {
  PendingQueue q; char k[8];
  for (int i = 0; i < (int)kPendingSlots; ++i) { sprintf(k, "k%d", i); TEST_ASSERT_TRUE(q.enqueue(k, "v")); }
  TEST_ASSERT_FALSE(q.enqueue("one_too_many", "v"));
  TEST_ASSERT_TRUE(q.enqueue("k3", "changed"));           // existing key still updatable when full
  TEST_ASSERT_TRUE(q.acknowledge("k0", "v"));
  const char *key, *val; q.at(0, key, val);
  TEST_ASSERT_EQUAL_STRING("k1", key);                    // order preserved after removal
  TEST_ASSERT_FALSE(q.enqueue("bad key", "v"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_key_and_value_validation);
  RUN_TEST(test_server_update_apply_and_replay_rejected);
  RUN_TEST(test_invalid_updates_rejected_without_side_effects);
  RUN_TEST(test_delete_and_capacity);
  RUN_TEST(test_pending_dedupes_latest_wins);
  RUN_TEST(test_pending_ack_keeps_changes_made_during_sync);
  RUN_TEST(test_pending_bounded_and_ordered);
  return UNITY_END();
}
