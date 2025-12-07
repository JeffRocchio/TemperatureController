#include "StatusLeds.h"

/******************************************************************************
 DESCRIPTION: Constructor, using initializer list.
*******************************************************************************/
StatusLeds::StatusLeds(
        uint8_t abovePin,
        uint8_t inBandPin,
        uint8_t belowPin,
        float hysteresisF,
        unsigned long updateIntervalMs)
      :
      _abovePin(abovePin),
      _inBandPin(inBandPin),
      _belowPin(belowPin),
      _hysteresisF(hysteresisF),
      _updateIntervalMs(updateIntervalMs),
      _lastUpdate(0),
      _region(AtSetPoint),
      _lastRegion(AtSetPoint)
      {}


/******************************************************************************
 DESCRIPTION: Call this before using any other methods of this object.
  This configures the chip for use of this object.
*******************************************************************************/
void StatusLeds::begin() {
    pinMode(_abovePin, OUTPUT);
    pinMode(_inBandPin,  OUTPUT);
    pinMode(_belowPin,   OUTPUT);

    digitalWrite(_abovePin, LOW);
    digitalWrite(_inBandPin,  LOW);
    digitalWrite(_belowPin,   LOW);
}

/******************************************************************************
 DESCRIPTION: Based on the current state of the actual temperature and the 
  set-point this method will determine which LEDs should be lit in order to
  inform the user.

 NOTES: 
  - This method determines the state, but does not actually affect the LEDs'
    On/Off state. I am keeping that in a seperate method in order to have
    flexibility. E.g., future revisions may want to incorporate some add'l 
    status or whatnot. It may also be the case that future iterations may
    make more complex use of the LEDs such that having where they get turned
    On/Off be seperate will better enable that.
  - Given the above note, remember that it is important to call the 
    updateLEDs() function in the main program loop (or in some function that
    gets called during each loop iteration).
  - The behavior of the LEDs is: If we are right at the Set Point then only
    green LED is on. If we are within the hysteresis band, but above the set
    point then both the orange and green LED is on. If in band and below, then
    both the green and blue LED is on. If outside of the band then the blue or
    orange LED is on, respectively; but green is off.
*******************************************************************************/
void StatusLeds::setDisplayState(float tempF, float setPointF) {
  const float halfBand = _hysteresisF * 0.5f;

  if (tempF < setPointF - halfBand) {
    _region = Below;
  } else if ((tempF >= setPointF - halfBand) && (tempF <= setPointF)) {
    _region = InBandBelow;
  } else if ((tempF <= setPointF + halfBand) && (tempF >= setPointF)) {
    _region = InBandAbove;
  } else if (tempF > setPointF + halfBand) {
    _region = Above;
  } else {
    _region = AtSetPoint;
  }
}

/******************************************************************************
 DESCRIPTION: Based on the current state of _region make sure the correct 
  status LED is lit.

 NOTES:
  - The method setDisplayState() must be called periodically in order to set
    the state of _region.
  - This is intended to be a non-blocking function that gets called as part of
    the RTS structure in the application's main loop() function.
*******************************************************************************/
void StatusLeds::updateLEDs(unsigned long now) {
  if (now - _lastUpdate < _updateIntervalMs) {
    return;
  }
  _lastUpdate = now;

  if (_region == _lastRegion) return; // No status change, nothing to do

  _lastRegion = _region;

  bool belowOn    = false;
  bool inBandOn   = false;
  bool aboveOn    = false;

  switch (_region) {
    case Below:
      belowOn = true;   // below lower bound of band
      break;
    case InBandBelow:
      inBandOn = true;  // in hysteresis band
      belowOn = true;   // and below the setpoint
      break;
    case AtSetPoint:
      inBandOn = true;  // right at the set point
      break;
    case InBandAbove:
      inBandOn = true;  // in hysteresis band
      aboveOn = true;   // and above the setpoint
      break;
    case Above:
      aboveOn = true;   // above upper bound of setpoint
      break;
  }

  digitalWrite(_abovePin, aboveOn ? HIGH : LOW);
  digitalWrite(_inBandPin,  inBandOn  ? HIGH : LOW);
  digitalWrite(_belowPin,   belowOn   ? HIGH : LOW);
}



/******************************************************************************
 DESCRIPTION: Self test. Use at boot-up to show that all LEDs are working.
*******************************************************************************/
void StatusLeds::selfTest() {
  uint8_t pins[3] = { _belowPin, _inBandPin, _abovePin };

  // Step each LED
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(pins[i], HIGH);
    delay(250);
    digitalWrite(pins[i], LOW);
  }
  // All on
  digitalWrite(_abovePin, HIGH);
  digitalWrite(_inBandPin, HIGH);
  digitalWrite(_belowPin, HIGH);
  delay(250);
  // All off
  digitalWrite(_abovePin, LOW);
  digitalWrite(_inBandPin, LOW);
  digitalWrite(_belowPin, LOW);
  delay(250);
  // Step each LED
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(pins[i], HIGH);
    delay(250);
    digitalWrite(pins[i], LOW);
  }
}


/******************************************************************************
 DESCRIPTION: Turn all LEDs off.
*******************************************************************************/
void StatusLeds::allOff() {
  digitalWrite(_abovePin, LOW);
  digitalWrite(_inBandPin, LOW);
  digitalWrite(_belowPin, LOW);
}

/******************************************************************************
 DESCRIPTION: Getter methods
*******************************************************************************/
StatusLeds::Region StatusLeds::region() const { 
  return _region; 
}

