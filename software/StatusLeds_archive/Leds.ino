/******************************************************************************
 DESCRIPTION: This class is part of the Temperature Controller application.
  It handles displaying heater status to the user via LEDs on the physical
  controller.

 NOTES:
  - For flexibility this module does not make assumptions about the colors of
    the various status LEDs. Instead it uses naming based on the information
    each LED conveys.
  - The above notwithstanding, as of Dec 2025 the color mapping is:
      * abovePIN = Orange color LED
      * inBandPIN = Green color LED
      * belowPIN = Blue color LED
*******************************************************************************/

class StatusLeds {
  public:
    enum Region : uint8_t { Below, InBand, Above };

    // ---- Constructor -------------------------------------------------------
    StatusLeds(
      uint8_t abovePin,
      uint8_t inBandPin,
      uint8_t belowPin,
      float hysteresisF,
      unsigned long updateIntervalMs);

    // ---- General Methods ---------------------------------------------------
    void begin();
    void setDisplayState(float tempF, float setPointF);
    void updateLEDs(unsigned long now);
    void StatusLeds::allOff();

    // ---- Setter/Getter Functions -------------------------------------------
    Region region() const;

  private:
    uint8_t _abovePin, _inBandPin, _belowPin;
    float _hysteresisF;
    unsigned long _updateIntervalMs, _lastUpdate;
    Region _region;
    Region _lastRegion;
};


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
      _region(InBand),
      _lastRegion(InBand)
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
 DESCRIPTION: Getter methods
*******************************************************************************/
StatusLeds::Region StatusLeds::region() const { 
  return _region; 
  }

/******************************************************************************
 DESCRIPTION: Based on the current state of the actual temperature and the 
  set-point this method will determine which LED should be lit in order to
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
*******************************************************************************/
  void StatusLeds::setDisplayState(float tempF, float setPointF) {
    const float halfBand = _hysteresisF * 0.5f;

    if (tempF < setPointF - halfBand) {
      _region = Below;
    } else if (tempF > setPointF + halfBand) {
      _region = Above;
    } else {
      _region = InBand;
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

    bool aboveOn = false;
    bool inBandOn  = false;
    bool belowOn   = false;

    switch (_region) {
      case Below:
        belowOn = true;   // below setpoint
        break;
      case InBand:
        inBandOn = true;  // in hysteresis band
        break;
      case Above:
        aboveOn = true; // above setpoint
        break;
    }

    digitalWrite(_abovePin, aboveOn ? HIGH : LOW);
    digitalWrite(_inBandPin,  inBandOn  ? HIGH : LOW);
    digitalWrite(_belowPin,   belowOn   ? HIGH : LOW);
  }

/******************************************************************************
 DESCRIPTION: Turn all LEDs off.
*******************************************************************************/
void StatusLeds::allOff() {
  digitalWrite(_abovePin, LOW);
  digitalWrite(_inBandPin, LOW);
  digitalWrite(_belowPin, LOW);
}

