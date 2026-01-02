#include <Arduino.h>
#include <ServoEasing.hpp>
#include <Adafruit_VEML7700.h>

#include "pins.h"

/**
 * Tracker side gadget
 */

ServoEasing xServo;
ServoEasing yServo;

//max ADC 1.1V
float readBatteryVoltage();

/*
Scanning for sun and find its angle coords
*/
int scanForSun();

void setup(){

	// Wait for servos to get to their start positions
	synchronizeAllServosStartAndWaitForAllServosToStop();

	// Set servo operation to be non-blocking
	synchronizeAllServosAndStartInterrupt(false);
}

void loop(){

	updateAllServos();
}