#pragma once

#include <stdint.h>
#include <types.h>

// BYOS outage recovery (fork, BYOS_PROTOCOL_V1): the panel keeps the last frame while the server or the
// network is unreachable and the device retries quietly for as long as it takes. Pure helpers so the
// decision is unit-testable on the host (see test/test_byos_recovery).

// Retry counters live in NVS as uint8_t; stop incrementing here so they never wrap back to a "fresh" state.
#define BYOS_RETRY_COUNT_MAX 250

// A /api/display result that must NOT replace the frame on the panel with an error screen.
// Everything except a good answer, the setup/registration flow, a reset request and the
// "plugin not attached" fast-poll state counts as an outage (server down, DNS, timeouts, bad body,
// image host down, ...).
inline bool byosKeepFrameOnError(https_request_err_e result) {
  switch (result) {
  case HTTPS_SUCCESS:
  case HTTPS_NO_ERR:
  case HTTPS_NO_REGISTER:
  case HTTPS_RESET:
  case HTTPS_PLUGIN_NOT_ATTACHED:
    return false;
  default:
    return true;
  }
}

inline uint8_t byosNextRetryCount(uint8_t count) {
  return count < BYOS_RETRY_COUNT_MAX ? (uint8_t)(count + 1) : (uint8_t)BYOS_RETRY_COUNT_MAX;
}
