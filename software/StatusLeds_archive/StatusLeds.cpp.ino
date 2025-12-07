/*============================================================================
 DESCRIPTION: ATtiny84A Temperature Controller (Non-blocking Architecture)

 ASSUMPTIONS:
  * Heating element is 200 watts or less. Probably a light bulb.
  * We are using a solid-state relay (SSR) to control the heating element.
  * Temperature sensor is a TI-LM19 analog IC.
==============================================================================*/

/******************************************************************************
 CHANGE LOG:

 2025-12-06: initial creation, based on a ChatGPT session.
*******************************************************************************/


//=============================================================================
//==== INITIALIZATION SECTION =================================================
//     Define and initilize global variables (this section is for the compilier)

// ---------------- Digital Pin Assignments -----------------------------------
const uint8_t SSR_PIN   = PIN_PB0;  // physical pin 2 - SSR control (digital)
const uint8_t LED_PIN   = PIN_PA0;  // physical pin 13 - status LED (digital)

// ---------------- Analog Pin Assignments: using Ax form for ADC -------------
const uint8_t TEMP_PIN  = A7;       // PA7, physical pin 6 - LM19 Vout
const uint8_t POT_PIN   = A3;       // PA3, physical pin 10 - pot wiper

// ---------------- LM19 Constants --------------------------------------------
const float VREF        = 5.0;
const float LM19_V0C    = 1.8663;
const float LM19_SLOPE  = 0.01169;   // V/°C; T = (1.8663 - V)/0.01169

// ---------------- Setpoint Range --------------------------------------------
const float MIN_SET_F   = 50.0;      // bottom of pot
const float MAX_SET_F   = 90.0;      // top of pot
const float MID_SET_F   = 72.0;      // desired midpoint temperature

// ---------------- Control Tuning --------------------------------------------
const float HYST_F      = 2.0;       // hysteresis band (°F)
const float TEMP_ALPHA  = 0.1;       // EMA factor for filtered temp

// ---------------- Task Intervals --------------------------------------------
const unsigned long TEMP_SAMPLE_MS   = 250;   // how often to sample LM19
const unsigned long CONTROL_UPDATE_MS= 1000;  // how often to update heater
const unsigned long LED_BLINK_MS     = 500;   // deadband blink period

// ---------------- Global State ----------------------------------------------
float filteredTempF      = 72.0;     // start, or seed it, near room temp
bool  heaterOn           = false;    // current heater state
bool  inDeadband         = false;    // true if temp is between ON/OFF thresholds

// ---------------- Timing State ----------------------------------------------
unsigned long lastTempSampleMs  = 0;
unsigned long lastControlMs     = 0;
unsigned long lastLedBlinkMs    = 0;
bool          ledBlinkState     = false;



//=============================================================================
//==== SET-UP SECTION =========================================================
//     This runs once at ATTiny boot-up. Configure the ATTiny.

void setup() {
  pinMode(SSR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(SSR_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  analogReference(DEFAULT);  // Vcc as ADC reference
}

//=============================================================================
//==== APPLICATION MAIN LOOP ==================================================

void loop() {
  unsigned long now = millis();

  taskSampleTemperature(now);
  taskUpdateControl(now);
  taskUpdateLed(now);
}


//=============================================================================
//==== FUNCTIONS ==============================================================

/******************************************************************************
 PURPOSE: Read LM19 and return °C

 NOTES: 
  - We take an average from multiple readings of the sensor in order to
    minimize noise. 
  - We are also guarding against extreme edge cases by testing for, and 
    setting, min/max possible values.
  - The formula: tC = (LM19_V0C - v) / LM19_SLOPE is what converts the 
    read ADC value read from the LM19, at the ATTiny pin it is connected to,
    into a temperature value. This formula was derived by ChatGPT from the
    LM19 datasheet, which contains a temperature to Vout table and graph.
    ChatGPT also claims that the values for V0C and the Slope are values that
    are commonly seen in application notes and example code across the
    internet.
*******************************************************************************/
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

// Celsius → Fahrenheit
float readTemperatureFOnce() {
  float c = readTemperatureC();
  return c * 9.0 / 5.0 + 32.0;
}

/******************************************************************************
 PURPOSE: Read the position of the potentiometer and use that to establish
  the user's desired temperature set-point.

 NOTES:
  - Global constants are used to establish the range of temperatures the pot
    can span; as well as the temperature set-point that we want the midpoint
    of the pot to represent.
  - We are doing some work here to make sure the pot positions are responsive
    to our desired range and midpoint.
*******************************************************************************/
float readSetpointF() {
  uint16_t rawADC = analogRead(POT_PIN);
  float raw = rawADC;             // convert ADC to float for math to follow

  const float MID_RAW = 511.5f;   // center of the 0–1023 value ADC pins return

  if (raw <= MID_RAW) {
    // Values: [0 .. MID_RAW] map to range: [MIN_SET_F .. MID_SET_F]
    float spanRaw  = MID_RAW - 0.0f;        // ≈ 511.5
    float spanTemp = MID_SET_F - MIN_SET_F; // ex: 72 - 50 = 22°F
    float frac = (spanRaw > 0.0f) ? (raw / spanRaw) : 0.0f;
    return MIN_SET_F + spanTemp * frac;
  } else {
    // Values: [MID_RAW .. 1023] map to range: [MID_SET_F .. MAX_SET_F]
    float spanRaw  = 1023.0f - MID_RAW;     // ≈ 511.5
    float spanTemp = MAX_SET_F - MID_SET_F; // ex: 90 - 72 = 18°F
    float frac = (spanRaw > 0.0f) ? ((raw - MID_RAW) / spanRaw) : 0.0f;
    return MID_SET_F + spanTemp * frac;
  }
}

/******************************************************************************
 PURPOSE: Sample temperature periodically and update filteredTempF

 NOTES:
  - filteredTempF: This variable represents the actual temperature measured by 
    the LM19 after being run through an exponential moving average (EMA) to 
    remove jitter and stabilize the control loop. It is dynamic. 
    It changes continuously as the room or enclosure changes temperature.
  - Our mini-RTS: We continuously call this funtion in the main loop; but it
    only actually execute at a tempo that is given by the sampling rate
    (in milliseconds) that we have set in the global constant for that.
*******************************************************************************/
void taskSampleTemperature(unsigned long now) {
  if (now - lastTempSampleMs < TEMP_SAMPLE_MS) {
    return;
  }
  lastTempSampleMs = now;

  float tempFraw = readTemperatureFOnce();

  // Exponential moving average
  filteredTempF = filteredTempF * (1.0f - TEMP_ALPHA) + tempFraw * TEMP_ALPHA;
}

/******************************************************************************
 PURPOSE: Update heater control based on filtered temp + setpoint

 NOTES:
  - Our mini-RTS: We continuously call this funtion in the main loop; but it
    only actually execute at a tempo that is given by the sampling rate
    (in milliseconds) that we have set in the global constant for that.
*******************************************************************************/
void taskUpdateControl(unsigned long now) {
  if (now - lastControlMs < CONTROL_UPDATE_MS) {
    return;
  }
  lastControlMs = now;

  float setF = readSetpointF();
  float tempF = filteredTempF;

  bool wantOn = heaterOn;  // default: keep current state
  inDeadband = false;

  if (tempF <= setF - HYST_F) {
    // Too cold: turn heater on
    wantOn = true;
  } else if (tempF >= setF + HYST_F) {
    // Too hot: turn heater off
    wantOn = false;
  } else {
    // Between thresholds: deadband
    inDeadband = true;
  }

  heaterOn = wantOn;

  // Drive SSR immediately when state changes
  digitalWrite(SSR_PIN, heaterOn ? HIGH : LOW);
}

/******************************************************************************
 PURPOSE: Update LED according to heater state / deadband

 NOTES:
  - Our mini-RTS: We continuously call this funtion in the main loop; but it
    only actually execute at a tempo that is given by the sampling rate
    (in milliseconds) that we have set in the global constant for that.

 LED STATES: 
  - heaterOn = true, not inDeadband  → LED solid ON
  - heaterOn = false, not inDeadband → LED solid OFF
  - inDeadband = true                → LED blinks
*******************************************************************************/
void taskUpdateLed(unsigned long now) {
  if (!inDeadband) {
    // Not in deadband: solid ON/OFF based on heater state
    digitalWrite(LED_PIN, heaterOn ? HIGH : LOW);
    return;
  }

  // In deadband: blink at LED_BLINK_MS period
  if (now - lastLedBlinkMs >= LED_BLINK_MS) {
    lastLedBlinkMs = now;
    ledBlinkState = !ledBlinkState;
    digitalWrite(LED_PIN, ledBlinkState ? HIGH : LOW);
  }
}
