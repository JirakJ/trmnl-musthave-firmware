#include <adc_samples.h>
#include <unity.h>

void test_median_of_clean_burst(void) {
  uint32_t s[8] = {2051, 2049, 2050, 2052, 2050, 2051, 2049, 2050};
  TEST_ASSERT_EQUAL_UINT32(2050, adcSamplesMedian(s, 8));
  TEST_ASSERT_FALSE(adcSamplesUnstable(s, 8, 2050));
}

// The 2.05 V bug: half of the conversions come back as 0 before the ADC settles. An average
// gives 1025 (halved); the median must still give the real value.
void test_median_ignores_settling_zeros(void) {
  uint32_t s[8] = {0, 0, 0, 0, 2050, 2051, 2049, 2050};
  TEST_ASSERT_EQUAL_UINT32(2049, adcSamplesMedian(s, 8)); // upper median: half zeros still give a real sample
  TEST_ASSERT_TRUE(adcSamplesUnstable(s, 8, 2049)); // but the burst is reported
  uint32_t t[16] = {0, 0, 0, 2050, 2051, 2049, 2050, 2050, 2050, 2051, 2049, 2050, 2050, 2050, 2050, 2050};
  TEST_ASSERT_EQUAL_UINT32(2050, adcSamplesMedian(t, 16));
}

void test_median_survives_a_single_spike(void) {
  uint32_t s[16] = {2050, 2050, 4000, 2050, 2050, 2050, 2050, 2050, 2050, 2050, 2050, 2050, 2050, 2050, 2050, 2050};
  TEST_ASSERT_EQUAL_UINT32(2050, adcSamplesMedian(s, 16));
  TEST_ASSERT_TRUE(adcSamplesUnstable(s, 16, 2050));
}

void test_small_spread_is_stable_large_is_not(void) {
  uint32_t ok[4] = {2000, 2100, 2050, 2050};   // spread 100 = 4.9 % of 2050
  TEST_ASSERT_EQUAL_UINT32(2050, adcSamplesMedian(ok, 4));
  TEST_ASSERT_FALSE(adcSamplesUnstable(ok, 4, 2050));
  uint32_t bad[4] = {1800, 2300, 2050, 2050};  // spread 500 = 24 %
  TEST_ASSERT_EQUAL_UINT32(2050, adcSamplesMedian(bad, 4));
  TEST_ASSERT_TRUE(adcSamplesUnstable(bad, 4, 2050));
}

void test_empty_and_all_zero(void) {
  uint32_t z[4] = {0, 0, 0, 0};
  TEST_ASSERT_EQUAL_UINT32(0, adcSamplesMedian(z, 0));
  TEST_ASSERT_TRUE(adcSamplesUnstable(z, 0, 0));
  TEST_ASSERT_EQUAL_UINT32(0, adcSamplesMedian(z, 4));
  TEST_ASSERT_TRUE(adcSamplesUnstable(z, 4, 0));
}

void setUp(void) {}
void tearDown(void) {}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_median_of_clean_burst);
  RUN_TEST(test_median_ignores_settling_zeros);
  RUN_TEST(test_median_survives_a_single_spike);
  RUN_TEST(test_small_spread_is_stable_large_is_not);
  RUN_TEST(test_empty_and_all_zero);
  UNITY_END();
}

int main(int argc, char **argv) {
  process();
  return 0;
}
