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

unsigned long previousMillis = 0;

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
	unsigned long currentMillis = millis();

	if (currentMillis - previousMillis >= SCAN_DELAY) {
		// save the last time you blinked the LED
		previousMillis = currentMillis;

	}
}

int scanForSun(){
	int lumMaxX = 0;
	int lumMaxZ = 0;

	int xGrad = (SCAN_X_MAX - SCAN_X_MIN) / SCAN_X_SAMPLES;
	int zGrad = (SCAN_Z_MAX - SCAN_Z_MIN) / SCAN_Z_SAMPLES;

	float inputLuxVal = 0;
	float nextSample = 0;

	for(int u = SCAN_X_MIN; u < SCAN_X_MAX; u+=xGrad){ // X Axis
		xServo.easeTo(u); // X servo into position

		nextSample = SCAN_Z_MIN;

		zServo.easeTo(SCAN_Z_MIN); // Go to start of scan
		zServo.setEaseTo(SCAN_Z_MAX); // Set end of scan

		while(zServo.isMoving()){
			if(zServo.getCurrentAngle() >= nextSample){
				inputLuxVal = veml.readLux(); // Maybe readALS()?

				if(inputLuxVal > lumMaxZ){
					lumMaxZ = inputLuxVal; // Set the top Z lux for this sweep
					sunPosZ = zServo.getCurrentAngle(); // Save Z angle of top Lux
				}

				nextSample += zGrad; // Set next sample angle
			}

			updateAllServos();
		}

		if(lumMaxZ > lumMaxX){ // Update top lux X angle
			lumMaxX = lumMaxZ;
			sunPosX = xServo.getCurrentAngle();
		}

		/*
		for(int v = SCAN_Z_MIN; v <= SCAN_Z_MAX; v+=zGrad){ // Z Axis
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
		*/
	}
}