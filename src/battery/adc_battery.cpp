#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include <adc_samples.h>
#include <battery.h>
#include <config.h>
#include <trmnl_log.h>

static uint16_t readReg16(uint8_t u8Addr, uint8_t u8Reg) {
  uint16_t u16;
  Wire.beginTransmission(u8Addr);
  Wire.write(u8Reg);
  Wire.endTransmission();
  Wire.requestFrom(u8Addr, 2);
  u16 = Wire.read();
  u16 |= (Wire.read() << 8);
  return u16;
} /* readReg16() */

// ADCBattery::ADCBattery() {}
float ADCBattery::readVoltage() {
  // stub function
  return 0.0f;
}

float ADCBattery::readVoltage(TRMNL_DEVICE *pDevice) {
  Log.info("%s [%d]: Battery voltage reading...\r\n", __FILE__, __LINE__);
  if (pDevice->batt_type == BATT_NONE) {
    Log.info("No battery defined; read as a fake voltage of 4.2V\r\n");
    return 4.2f;
  } else if (pDevice->batt_type == BATT_ADC) {
    if (pDevice->batt_en_pin != 0xff) {
      pinMode(pDevice->batt_en_pin, OUTPUT);
      digitalWrite(pDevice->batt_en_pin, HIGH);
      delay(10); // Wait for the switch to stabilize
    }
    // Warm-up conversions initialise the ADC; the burst is reduced with a median so a few
    // not-yet-settled zero conversions cannot halve the result (2.05 V for a 4.11 V battery).
    analogRead(pDevice->batt_pin); // needed to properly initialize the ADC BEFORE calling analogReadMilliVolts()
    for (uint8_t i = 0; i < ADC_SAMPLES_WARMUP; i++) {
      analogReadMilliVolts(pDevice->batt_pin);
      delay(2);
    }
    uint32_t samples[ADC_SAMPLES_COUNT];
    for (uint8_t i = 0; i < ADC_SAMPLES_COUNT; i++) {
      samples[i] = analogReadMilliVolts(pDevice->batt_pin);
      delayMicroseconds(250);
    }
    if (pDevice->batt_en_pin != 0xff) {
      digitalWrite(pDevice->batt_en_pin, LOW);
    }
    uint32_t median = adcSamplesMedian(samples, ADC_SAMPLES_COUNT);
    if (adcSamplesUnstable(samples, ADC_SAMPLES_COUNT, median)) {
      Log_error_submit("battery adc unstable: min %u max %u median %u mV (pin)", (unsigned)samples[0],
                       (unsigned)samples[ADC_SAMPLES_COUNT - 1], (unsigned)median);
    }
    int32_t sensorValue = (int32_t)median * 2;
    Log.info("%s [%d]: Battery sensorValue = %d (median of %d, min %u, max %u)\r\n", __FILE__, __LINE__,
             (int)sensorValue, ADC_SAMPLES_COUNT, (unsigned)samples[0], (unsigned)samples[ADC_SAMPLES_COUNT - 1]);
    return (float)sensorValue / 1000.0f;
  } else if (pDevice->batt_type == BATT_BQ27220) { // BQ27220
    Wire.begin(pDevice->sensor_sda, pDevice->sensor_scl);
    int16_t sensorValue = readReg16(0x55, 8); // current battery voltage in millivolts (registers 8+9)
    return (float)sensorValue / 1000.0f;
  } else if (pDevice->batt_type == BATT_AXP2101) {
    Wire.begin(pDevice->sensor_sda, pDevice->sensor_scl);
    int16_t sensorValue = readReg16(0x34, 0x34); // current battery voltage in millivolts (registers 0x34+0x35)
    sensorValue = __builtin_bswap16(sensorValue); // this chip is big-endian
    sensorValue &= 0x3fff; // top 2 bits are config info
    return (float)sensorValue / 1000.0f;
  } else { // BQ2742x
    Wire.begin(pDevice->sensor_sda, pDevice->sensor_scl);
    int16_t sensorValue = readReg16(0x55, 4); // current battery voltage in millivolts (registers 4+5)
    return (float)sensorValue / 1000.0f;
  }
} /* readVoltage() */
