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
  - As of Dec 2025 the color mapping is:
      * abovePin  = Orange color LED
      * inBandPin = Green color LED
      * belowPin  = Blue color LED
*******************************************************************************/

class StatusLeds {
  public:
    enum Region : uint8_t { Below, InBand, Above };

    StatusLeds(
      uint8_t abovePin,
      uint8_t inBandPin,
      uint8_t belowPin,
      float hysteresisF,
      unsigned long updateIntervalMs
    );

    void begin();
    void setDisplayState(float tempF, float setPointF);
    void updateLEDs(unsigned long now);

    // Optional: getter so the main code can inspect the current region
    Region region() const;

  private:
    uint8_t _abovePin;
    uint8_t _inBandPin;
    uint8_t _belowPin;

    float _hysteresisF;

    unsigned long _updateIntervalMs;
    unsigned long _lastUpdate;

    Region _region;
};

#endif // STATUS_LEDS_H
