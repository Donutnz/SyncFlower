#include <Arduino.h>
#include <ServoEasing.hpp>
#include <Adafruit_VEML7700.h>

#include "pins.h"
#include "config.h"

/**
 * Tracker side gadget
 */

ServoEasing xServo;
ServoEasing zServo;

Adafruit_VEML7700 veml = Adafruit_VEML7700();

float sunPosX = 90;
float sunPosZ = 90;

//max ADC 1.1V
float readBatteryVoltage();

/*
Scanning for sun and find its angle coords
*/
int scanForSun();

void setup(){

	Serial.begin(9600);

	while(!Serial);

	Serial.println("Tracker Start...");

	// Init Servos

	xServo.attach(SERVO_X, 90);
	zServo.attach(SERVO_Z, 90);

	xServo.setSpeed(SERVO_SPEED);

	setEasingTypeForAllServos(EASE_QUADRATIC_IN_OUT);

	// Wait for servos to get to their start positions
	synchronizeAllServosStartAndWaitForAllServosToStop();

	// Set servo operation to be non-blocking
	synchronizeAllServosAndStartInterrupt(false);

	// Init VEML

	if(!veml.begin()){

	}
}

void loop(){

	updateAllServos();
}

int scanForSun(){
	int lumMaxX = 0;
	int lumMaxZ = 0;

	int xGrads = (SCAN_X_MAX - SCAN_X_MIN) / SCAN_X_GRADS_DIV;
	int zGrads = (SCAN_Z_MAX - SCAN_Z_MIN) / SCAN_Z_GRADS_DIV;

	float inputLuxVal = 0;

	for(int u = SCAN_Z_MIN; u < xGrads; u++){ // X Axis
		for(int v = SCAN_X_MIN; v < zGrads; v++){ // Z Axis
			inputLuxVal = veml.readLux(); // readALS() for raw intensity

			if(inputLuxVal > lumMaxX){
				lumMaxX = inputLuxVal;
				sunPosX = v;
			}
		}

		if(inputLuxVal > lumMaxZ){
			lumMaxZ = inputLuxVal;
			sunPosZ = u;
		}
	}
}