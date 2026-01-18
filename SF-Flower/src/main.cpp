#include <Arduino.h>
#include <ServoEasing.hpp>

#include "pins.h"
#include "config.h"

ServoEasing xServo;
ServoEasing zServo;
ServoEasing winchServo;

// Current position of petals and thus winch
int winchPosition = 0;

//Make petals hit the limit switch
void homePetals();

void openPetals();

void angleHead(int x, int z);

void setup(){
	zServo.attach(MOT_AX0, 90);
	xServo.attach(MOT_AX1, 90);

	winchServo.attach(MOT_AX1, 0);

	pinMode(ONBOARD_LED, OUTPUT);
	pinMode(SW_LIMIT, INPUT);
	pinMode(SW_UI, INPUT);

}

void loop(){

}

void homePetals(){
	winchServo.setEaseTo(WINCH_RETRACT_RATE);

	unsigned long startMillis = millis();

	while(!digitalRead(SW_LIMIT)){
		
		if(winchServo.isMoving()){ //This way in case limit is hit during ease in
			winchServo.update();
		}

		if((millis() - startMillis) > 5000){ // watchdog incase of limit switch not working 
			break;
		}
	}

	winchServo.write(0); //Stop winch
}