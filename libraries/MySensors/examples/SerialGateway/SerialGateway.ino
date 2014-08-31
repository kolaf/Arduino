  /*
 * Copyright (C) 2013 Henrik Ekblad <henrik.ekblad@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * DESCRIPTION
 * The ArduinoGateway prints data received from sensors on the serial link. 
 * The gateway accepts input on seral which will be sent out on radio network.
 *
 * The GW code is designed for Arduino Nano 328p / 16MHz
 *
 * Wire connections (OPTIONAL):
 * - Inclusion button should be connected between digital pin 3 and GND
 * - RX/TX/ERR leds need to be connected between +5V (anode) and digital ping 6/5/4 with resistor 270-330R in a series
 *
 * LEDs (OPTIONAL):
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error  
 */

#include <SPI.h>  
#include <MySensor.h>  
#include <MyGateway.h>  
#ifdef DRH_NRF24
#include <RH_NRF24.h>
#else
#ifdef DRH_RF69
#include <RH_RF69.h>
#endif
#endif
#include <stdarg.h>

#define INCLUSION_MODE_TIME 1 // Number of minutes inclusion mode is enabled
#define INCLUSION_MODE_PIN 3 // Digital pin used for inclusion mode button

#ifdef DRH_NRF24
RH_NRF24 driver(9 /*ce*/, 10 /*csn*/);
static uint64_t address = BASE_RADIO_ID;
#else
#ifdef DRH_RF69
RH_RF69 driver;
#endif
#endif

MyGateway gw(INCLUSION_MODE_TIME, INCLUSION_MODE_PIN,  A5, A4, A3);

char inputString[MAX_RECEIVE_LENGTH] = "";    // A string to hold incoming commands from serial/ethernet interface
int inputPos = 0;
boolean commandComplete = false;  // whether the string is complete

void setup()  
{ 
  if(gw.setRadio(&driver))
  {
#ifdef DRH_NRF24
    if (    driver.setChannel(RF24_CHANNEL)
         && driver.setNetworkAddress(reinterpret_cast<uint8_t*>(&address), 5)
         && driver.setRF(RF24_DATARATE, RF24_PA_LEVEL_GW)
         && driver.printRegisters() )
#else
#ifdef DRH_RF69
    if (    driver.setFrequency(868)
         && driver.setTxPower(14) )
#endif
#endif
    {
      gw.begin();
    }
    else
    {
      Serial.println(F("Failed!"));
      for (;;) {};
    }
  }
}

void loop()  
{ 
  gw.processRadioMessage();   
  if (commandComplete) {
    // A command wass issued from serial interface
    // We will now try to send it to the actuator
    gw.parseAndSend(inputString);
    commandComplete = false;  
    inputPos = 0;
  }
}


/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inputPos<MAX_RECEIVE_LENGTH-1 && !commandComplete) { 
      if (inChar == '\n') {
        inputString[inputPos] = 0;
        commandComplete = true;
      } else {
        // add it to the inputString:
        inputString[inputPos] = inChar;
        inputPos++;
      }
    } else {
       // Incoming message too long. Throw away 
        inputPos = 0;
    }
  }
}


