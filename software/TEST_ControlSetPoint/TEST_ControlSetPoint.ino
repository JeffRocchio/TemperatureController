// ATtiny84A + LM19 + Pot + SSR/LED
// Simple thermostat with debug LED states

// --- Pin assignments (digital) ---
const uint8_t SSR_PIN   = PIN_PB0;  // physical pin 2 - SSR control
const uint8_t LED_PIN   = PIN_PA0;  // physical pin 13 - status LED

// --- Analog channels ---
// These are the ADC channels, not the digital pin numbers.
// PA7 (physical pin 6) is ADC7.
// PA3 (physical pin 10) is ADC3.
const uint8_t TEMP_PIN  = A7;       // LM19 Vout on PA7
const uint8_t POT_PIN   = A3;       // Pot wiper on PA3

// --- LM19 constants ---
const float VREF        = 5.0;
const float LM19_V0C    = 1.8663;
const float LM19_SLOPE  = 0.01169;

// --- Setpoint range ---
const float MIN_SET_F   = 50.0;
const float MAX_SET_F   = 90.0;
const float MID_SET   = 72.0;
// --- Hysteresis ---
const float HYST_F      = 2.0;  // start with 2°F

// --- Filtered temperature ---
float filteredTempF = 70.0;
bool heaterOn = false;

// --- Read LM19, return °C ---
float readTemperatureC() {
  const uint8_t N_SAMPLES = 8;
  uint32_t sum = 0;

  for (uint8_t i = 0; i < N_SAMPLES; i++) {
    sum += analogRead(TEMP_PIN);
  }

  float raw = sum / (float)N_SAMPLES;
  float v = (raw * VREF) / 1023.0;

  // Clamp to sane LM19 range
  if (v < 0.2) v = 0.2;
  if (v > 2.8) v = 2.8;

  float tC = (LM19_V0C - v) / LM19_SLOPE;
  return tC;
}

// --- Convert °C to °F ---
float readTemperatureF() {
  float c = readTemperatureC();
  return c * 9.0 / 5.0 + 32.0;
}

// --- Read pot and map 0–1023 to 50–90°F linearly ---
float readSetpointF() {
  /*uint16_t raw = analogRead(POT_PIN);
  float fraction = raw / 1023.0;
  return MIN_SET_F + (MAX_SET_F - MIN_SET_F) * fraction; */

  uint16_t rawADC = analogRead(POT_PIN);
  float raw = rawADC;

  const float MID_RAW = 511.5;   // center of 0–1023

  if (raw <= MID_RAW) {
    // Map [0 .. MID_RAW] → [MIN_SET_F .. MID_SET]
    float spanRaw  = MID_RAW - 0.0;                 // ≈ 511.5
    float spanTemp = MID_SET - MIN_SET_F;           // 72 - 50 = 22°F
    float frac = raw / spanRaw;                     // 0.0 → 1.0
    return MIN_SET_F + spanTemp * frac;
  } else {
    // Map [MID_RAW .. 1023] → [MID_SET .. MAX_SET_F]
    float spanRaw  = 1023.0 - MID_RAW;              // ≈ 511.5
    float spanTemp = MAX_SET_F - MID_SET;           // 90 - 72 = 18°F
    float frac = (raw - MID_RAW) / spanRaw;         // 0.0 → 1.0
    return MID_SET + spanTemp * frac;
  }
 
}

// --- Update heater state based on temp and setpoint ---
// Also encode "what the logic thinks" onto the LED:
//   - Too cold (want heat ON): LED solid ON
//   - Too hot (want heat OFF): LED solid OFF
//   - In deadband: LED slow blink
void updateHeater(float tempF, float setF) {
  static unsigned long lastBlink = 0;
  static bool blinkState = false;

  bool wantOn = heaterOn;  // default to current state
  bool inDeadband = false;

  if (tempF <= setF - HYST_F) {
    wantOn = true;              // too cold → turn ON
  } else if (tempF >= setF + HYST_F) {
    wantOn = false;             // too hot → turn OFF
  } else {
    inDeadband = true;          // between thresholds
  }

  heaterOn = wantOn;

  // Drive SSR
  digitalWrite(SSR_PIN, heaterOn ? HIGH : LOW);

  // LED behavior:
  if (inDeadband) {
    // Slow blink to show we're between ON and OFF thresholds
    unsigned long now = millis();
    if (now - lastBlink >= 500) {  // 0.5s
      lastBlink = now;
      blinkState = !blinkState;
    }
    digitalWrite(LED_PIN, blinkState ? HIGH : LOW);
  } else {
    // Solid ON if heater ON, solid OFF if heater OFF
    digitalWrite(LED_PIN, heaterOn ? HIGH : LOW);
  }
}

void setup() {
  pinMode(SSR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(SSR_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  analogReference(DEFAULT);  // Vcc as ADC reference
}

void loop() {
  float tempFraw = readTemperatureF();
  float setF     = readSetpointF();

  // Simple exponential filter on temp
  filteredTempF = filteredTempF * 0.9 + tempFraw * 0.1;

  updateHeater(filteredTempF, setF);

  delay(500);  // 2 updates per second
}
