#pragma once

#include <stdint.h>

// Robust reduction of a burst of ADC millivolt samples (pure, host-testable).
// A freshly powered ADC can return a few zero/garbage conversions before it settles; averaging
// those halves the result (seen on a TRMNL OG: 2.05 V reported for a 4.11 V battery). The median
// of the burst ignores such outliers; `adcSamplesUnstable` flags a burst worth logging.

#define ADC_SAMPLES_COUNT          16
#define ADC_SAMPLES_WARMUP         2     // discarded conversions before the burst
#define ADC_SAMPLES_SPREAD_PERCENT 10    // (max - min) above this share of the median = unstable

inline void adcSamplesSort(uint32_t *s, uint8_t n) {
  for (uint8_t i = 1; i < n; i++) { // insertion sort, n is tiny
    uint32_t v = s[i];
    int8_t j = (int8_t)i - 1;
    while (j >= 0 && s[j] > v) {
      s[j + 1] = s[j];
      j--;
    }
    s[j + 1] = v;
  }
}

// Upper median of the samples (sorts in place): with an even count the higher middle value is used,
// so up to n/2 low outliers (zero conversions) cannot pull the result down. n == 0 → 0.
inline uint32_t adcSamplesMedian(uint32_t *s, uint8_t n) {
  if (n == 0) return 0;
  adcSamplesSort(s, n);
  return s[n / 2];
}

// True when the burst contains a zero or its spread exceeds ADC_SAMPLES_SPREAD_PERCENT of the median
// (expects a sorted burst, i.e. call after adcSamplesMedian).
inline bool adcSamplesUnstable(const uint32_t *sorted, uint8_t n, uint32_t median) {
  if (n == 0 || median == 0) return true;
  if (sorted[0] == 0) return true;
  return (sorted[n - 1] - sorted[0]) * 100 > median * ADC_SAMPLES_SPREAD_PERCENT;
}
