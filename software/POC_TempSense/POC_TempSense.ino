/*********************************************************************
 Themperature Sensing 

 DESCRIPTION: Wire up the LM19 and temp-sense-pot and just see if
 I can get something working.

 ASSUMPTIONS:
    SpenceKonde ATTinyCore assumed
    Temperature Sensor is a TI LM19 analog sensor IC

 12-05-2025: Initial attempt
*********************************************************************/

#include <Arduino.h>

/**************************************************************************
 * Initiization Section
 *************************************************************************/

  // --- Pin assignments (your choices) ---
  const uint8_t SSR_PIN   = PIN_PB0;  // physical pin 2 - SSR trigger
  const uint8_t TEMP_PIN  = PIN_PA7;       // physical pin 6 - LM19 output
  const uint8_t POT_PIN   = PIN_PA3;       // physical pin 10 - setpoint pot
  const uint8_t LED_PIN   = PIN_PA0;  // physical pin 13 - status LED

  // --- Config Global Variables ---
  const float VREF        = 5.0;      // using Vcc as ADC reference
  const float LM19_V0C    = 1.8663;   // approx Vout at 0°C (from datasheet)
  const float LM19_SLOPE  = 0.01169;  // V/°C (output decreases as temp rises)

  // Setpoint range in °F (you can tweak these later)
  const float MIN_SET_F   = 30.0;
  const float MAX_SET_F   = 70.0;

  // Hysteresis band in °F
  const float HYST_F      = 3.0;

  bool heaterOn = false;


/**************************************************************************
  PURPOSE: Perform setup work. This function is called automatically 
  on startup.
/**************************************************************************/
void setup(void)
{
  pinMode(SSR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(SSR_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

}

/**************************************************************************
  PURPOSE: Do the work
/**************************************************************************/
void loop(void)
{

  float tempF = readTemperatureF();
  float setF  = readSetpointF();
  updateHeater(tempF, setF);
  delay(1000);  // 1-second update period for testing

}

/****************************************************************************
  PURPOSE: Get the temperature from the sensor, in it's native units
*****************************************************************************/
float readTemperatureC() {
  const uint8_t N_SAMPLES = 8;  // Take 8 reading samples and average them.
  uint32_t sum = 0;

  for (uint8_t i = 0; i < N_SAMPLES; i++) {
    sum += analogRead(TEMP_PIN);
  }

  float raw = sum / (float)N_SAMPLES;
  float v = (raw * VREF) / 1023.0;

  // Clamp to sane range for LM19
  if (v < 0.2) v = 0.2;
  if (v > 2.8) v = 2.8;

  float tC = (LM19_V0C - v) / LM19_SLOPE;
  return tC;
}

/****************************************************************************
  PURPOSE: Convert sensor's native units to farenheit units
*****************************************************************************/
float readTemperatureF() {

  float c = readTemperatureC();
  return c * 9.0 / 5.0 + 32.0;

}

/****************************************************************************
  PURPOSE: Get the temperature Set-Point as input by user
*****************************************************************************/
float readSetpointF() {
  uint16_t raw = analogRead(POT_PIN);

  // Map 0–1023 → MIN_SET_F–MAX_SET_F
  float fraction = raw / 1023.0;
  float setF = MIN_SET_F + (MAX_SET_F - MIN_SET_F) * fraction;
  return setF;
}

/****************************************************************************
  PURPOSE: Turn heater device On/Off as appropriate
*****************************************************************************/
void updateHeater(float tempF, float setF) {
  // Simple hysteresis control
  if (tempF <= setF - HYST_F) {
    heaterOn = true;
  } else if (tempF >= setF + HYST_F) {
    heaterOn = false;
  }

  digitalWrite(SSR_PIN, heaterOn ? HIGH : LOW);
  digitalWrite(LED_PIN, heaterOn ? HIGH : LOW);
}


