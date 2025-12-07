#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

#include <Arduino.h>

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
    enum Region : uint8_t { Below, InBandBelow, AtSetPoint, InBandAbove, Above };

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
    void selfTest();
    void allOff();

    // ---- Setter/Getter Functions -------------------------------------------
    Region region() const;

  private:
    uint8_t _abovePin, _inBandPin, _belowPin;
    float _hysteresisF;
    unsigned long _updateIntervalMs, _lastUpdate;
    Region _region;
    Region _lastRegion;
};

#endif // STATUS_LEDS_H
