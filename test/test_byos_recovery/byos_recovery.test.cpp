#include <byos_recovery.h>
#include <memory_persistence.h>
#include <refresh_interval.h>
#include <unity.h>

void test_good_results_do_not_trigger_recovery(void) {
  TEST_ASSERT_FALSE(byosKeepFrameOnError(HTTPS_SUCCESS));
  TEST_ASSERT_FALSE(byosKeepFrameOnError(HTTPS_NO_ERR));
  TEST_ASSERT_FALSE(byosKeepFrameOnError(HTTPS_NO_REGISTER));
  TEST_ASSERT_FALSE(byosKeepFrameOnError(HTTPS_RESET));
  TEST_ASSERT_FALSE(byosKeepFrameOnError(HTTPS_PLUGIN_NOT_ATTACHED));
}

void test_every_failure_keeps_the_frame(void) {
  const https_request_err_e failures[] = {
      HTTPS_CLIENT_FAILED,       HTTPS_UNABLE_TO_CONNECT, HTTPS_CONNECTION_FAILED, HTTPS_RESPONSE_CODE_INVALID,
      HTTPS_JSON_PARSING_ERR,    HTTPS_WRONG_IMAGE_SIZE,  HTTPS_WRONG_IMAGE_FORMAT, HTTPS_IMAGE_FILE_TOO_BIG,
      HTTPS_BAD_CLIENT,          HTTPS_OUT_OF_MEMORY,     HTTPS_TIMED_OUT,          HTTPS_IMAGE_DOWNLOAD_FAILED,
  };
  for (auto f : failures)
    TEST_ASSERT_TRUE_MESSAGE(byosKeepFrameOnError(f), https_request_err_str(f));
}

void test_retry_counter_saturates_instead_of_wrapping(void) {
  TEST_ASSERT_EQUAL_UINT8(2, byosNextRetryCount(1));
  TEST_ASSERT_EQUAL_UINT8(BYOS_RETRY_COUNT_MAX, byosNextRetryCount(BYOS_RETRY_COUNT_MAX - 1));
  TEST_ASSERT_EQUAL_UINT8(BYOS_RETRY_COUNT_MAX, byosNextRetryCount(BYOS_RETRY_COUNT_MAX));
  TEST_ASSERT_EQUAL_UINT8(BYOS_RETRY_COUNT_MAX, byosNextRetryCount(255));
}

// A week-long outage: the ladder settles at 5-minute wakes, never escalates, never shows an error.
void test_week_long_outage_stays_on_the_five_minute_ladder(void) {
  MemoryPersistence persistence;
  RefreshInterval refreshInterval(persistence);
  uint8_t count = 1;
  uint32_t total = 0, wakes = 0;
  while (total < 7 * 24 * 3600) {
    uint32_t sleep = refreshInterval.applyQuietRetry(count);
    count = byosNextRetryCount(count);
    total += sleep;
    wakes++;
  }
  TEST_ASSERT_EQUAL_UINT8(BYOS_RETRY_COUNT_MAX, count);
  TEST_ASSERT_EQUAL_UINT32(SHORT_TERM_SLOW_RETRY_INTERVAL, refreshInterval.seconds());
  TEST_ASSERT_LESS_THAN_UINT32(7 * 24 * 12 + 10, wakes); // ~12 wakes/hour
}

void setUp(void) {}
void tearDown(void) {}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_good_results_do_not_trigger_recovery);
  RUN_TEST(test_every_failure_keeps_the_frame);
  RUN_TEST(test_retry_counter_saturates_instead_of_wrapping);
  RUN_TEST(test_week_long_outage_stays_on_the_five_minute_ladder);
  UNITY_END();
}

int main(int argc, char **argv) {
  process();
  return 0;
}
