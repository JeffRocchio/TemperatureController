/*********************************************************************
 Button press test

 Description: Connect a push button to an ATTiny input PIN and use it
 to turn the SSR on and off.
 
 12-03-2025: Initial attempt
*********************************************************************/

#include <Arduino.h>

/**************************************************************************
 * Initiization Section
 *************************************************************************/

  const int  ledPin = PIN_PA1;    // the pin that the led is wired to

  #define ButtonPin PIN_PB1   // Physical pin #3
  #define SSR_outPin PIN_PB2   // Physical pin #5
  int LastButtonState = 0;
  unsigned long currMillis = 0;
  unsigned long LastButtonChangeTime = currMillis;


/**************************************************************************/
/*!
    @brief  Perform setup work. This function is called
            automatically on startup.
*/
/**************************************************************************/
void setup(void)
{
  while (!Serial);  // Give a bit of time for the serial object to start up
  delay(500);

  Serial.begin(9600);

  Serial.println("In setup() function");

  pinMode(ButtonPin, INPUT_PULLUP);   // Set button pin for input
  pinMode(SSR_outPin, OUTPUT);        // initialize SST trigger pin as an output

  Serial.println("Pressing button should turn light on");

}

/**************************************************************************/
/*!
    @brief  Constantly poll for button press
*/
/**************************************************************************/
void loop(void)
{

  pollTestSwitch();

}


/**************************************************************************/
/*!
    @brief  Polls the test switch. It includes a switch de-bounce
    process as well.

*/
/**************************************************************************/
void pollTestSwitch()
{
  currMillis = millis();
  for (int i=0; i<15; i++) {
    boolean state = !digitalRead(ButtonPin);
    //check for state and do debounce
    if (state != LastButtonState && currMillis-LastButtonChangeTime > 50) {
      LastButtonState = state;
      LastButtonChangeTime = currMillis;
      if (state) {
        digitalWrite(SSR_outPin, HIGH);
        Serial.println("Button Pressed");
      }
      else {
        digitalWrite(SSR_outPin, LOW);
        Serial.println("Button Released");
      }
    }
  }
}

