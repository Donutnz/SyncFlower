#include <Arduino.h>
#include <ServoEasing.hpp>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>

#include "pins.h"
#include "config.h"
#include "creds.h"

ServoEasing xServo;
ServoEasing zServo;
ServoEasing winchServo;

// Current position of petals and thus winch
int winchPosition = 0;

WiFiClientSecure netClient;
PubSubClient hmqClient(netClient);

//Char buffer for MQTT pub
char msgBuffer[12];

Adafruit_NeoPixel neoStrip(NEO_COUNT, NEO_PIN); // May need other arg

int headFinalX = 90;
int headFinalZ = 90;
int headFinalLux = 0;

PETALS_STATE petalsStatus = PETALS_OPEN;

// Time batt disp started
unsigned long battDispMillis = 0;

// Time petals motor started
unsigned long petalsStartMillis = 0;

// Steps of brightness for neo ring
float brightnessStep = 255.0f / PETALS_TRAVEL_DURATION;

//Make petals hit the limit switch
void homePetals();

void setPetals(bool isOpening);

void updatePetals();

void setXZ();

void updateHead();

void updateRing();

void setNeoToSun();

void setNeoToSun(byte brightness);

void readBattery();

void setStatusRing(int statusColour);

// Wifi setup stuff
void setup_wifi();

// MQTT Connect
void reconnect();

// MQTT Callback function
void callback(char* topic, byte* message, unsigned int length);

void setup(){
	Serial.begin(115200);

	#ifdef APP_DEBUG //Delay if debugging for serial port to warm up
	delay(5000);
	#endif

	pinMode(ONBOARD_LED, OUTPUT);
	pinMode(SW_LIMIT, INPUT);
	pinMode(SW_UI, INPUT);

	// Init Neopixel ring
	neoStrip.begin();
	neoStrip.show();

	setStatusRing(COLOUR_STARTING);

	pinMode(BATT_ADC_A, INPUT);
	pinMode(BATT_ADC_B, INPUT);

	zServo.attach(MOT_Z, 90);
	xServo.attach(MOT_X, 90);

	winchServo.attach(MOT_PETAL, 0);

	zServo.setSpeed(HEAD_SPEED);
	xServo.setSpeed(HEAD_SPEED);

	winchServo.setSpeed(WINCH_SPEED);

	setEasingTypeForAllServos(EASE_QUADRATIC_IN_OUT);

	// Wait for servos to get to their start positions
	synchronizeAllServosStartAndWaitForAllServosToStop();
	
	// Set servo operation to be non-blocking
	synchronizeAllServosAndStartInterrupt(false);

	//homePetals();
	Serial.print("Endstop Read: ");
	Serial.println(digitalRead(SW_LIMIT));
	homePetals();

	// Init WiFi
	setup_wifi();

	// Init HiveMQ MQTT
	hmqClient.setServer(hiveMQ_Addr, HIVEMQ_PORT);
	hmqClient.setClient(netClient);
	hmqClient.setCallback(callback);

	Serial.println("Set up done, running...");
}

void loop(){
	if(!hmqClient.connected()){
		reconnect();
	}
	hmqClient.loop();

	unsigned long currentMillis = millis();

	if(digitalRead(SW_UI) && (currentMillis - battDispMillis > 10)){ // Debounce as well
		battDispMillis = currentMillis;

		readBattery();
	}

	if((currentMillis - battDispMillis >= BATT_DISP_DURATION) && battDispMillis > 0){ // Reset display to normal
		setNeoToSun();
		battDispMillis = 0;
	}

	if(currentMillis - petalsStartMillis > PETALS_TRAVEL_DURATION){ // Stop petals when open
		winchServo.easeTo(90); // Blocking. May cause issues?
	}
}

void homePetals(){
	Serial.println("Homing...");

	if(digitalRead(SW_LIMIT)){ //If already touching limit switch
		Serial.println("Homing stopped");
		return;
	}

	winchServo.easeTo(WINCH_RETRACT);

	unsigned long startMillis = millis();

	while(!digitalRead(SW_LIMIT)){ // Retract until limit sw hit
		
	}

	winchServo.easeTo(WINCH_STOP); // Stop pulling

	petalsStatus = PETALS_CLOSED; 

	Serial.println("Petals Homed");
}

void setPetals(bool isOpening){
	if(isOpening){
		winchServo.easeTo(WINCH_DEPLOY);
		petalsStatus = PETALS_OPENING;
	}
	else{
		winchServo.easeTo(WINCH_RETRACT);
		petalsStatus = PETALS_CLOSING;
	}

	petalsStartMillis = millis();
}

void updatePetals(){
	if(winchServo.getCurrentAngle() != WINCH_STOP){
		if(millis() - petalsStartMillis >= PETALS_TRAVEL_DURATION){
			winchServo.easeTo(WINCH_STOP);

			if(petalsStatus == PETALS_OPENING){
				petalsStatus = PETALS_OPEN;
			}
			else if(petalsStatus == PETALS_CLOSING){
				petalsStatus = PETALS_CLOSED;
			}
		}
	}
}

void updateRing(){
	if(petalsStatus == PETALS_CLOSING){
		setNeoToSun(constrain((int)neoStrip.getBrightness() + brightnessStep, 0, 255));
	}
	else if(petalsStatus == PETALS_OPENING){
		setNeoToSun(constrain((int)neoStrip.getBrightness() - brightnessStep, 0, 255));
	}
	
	neoStrip.show();
}

void readBattery(){
	int battAvg = (analogRead(BATT_ADC_A) + analogRead(BATT_ADC_B)) / 2; // 0-4095, 1.786-2.5

	Serial.print("Batt ADC: ");
	Serial.println(battAvg);

	int neoMapped = floor((battAvg - BATT_MIN) / (BATT_MAX - BATT_MIN) * (NEO_COUNT - 1) + 1);

	Serial.print("Neomapped: ");
	Serial.println(neoMapped);

	neoStrip.clear(); // Clear strip

	for(int i = 0; i < neoMapped; i++){
		//neoStrip.setPixelColor(i, 255, 0, 0);
		neoStrip.setPixelColor(i, neoStrip.gamma32(neoStrip.ColorHSV(map(i, 0, 15, 0, 65536/3)))); // Map 0-15 to HSV red-green range
	}

	neoStrip.show();
}

void setNeoToSun(){
	neoStrip.fill(neoStrip.gamma32(neoStrip.ColorHSV(COLOUR_SUN)), 0, NEO_COUNT);
	neoStrip.show();
}

void setNeoToSun(byte brightness){
	neoStrip.fill(neoStrip.gamma32(neoStrip.ColorHSV(COLOUR_SUN, 255, brightness)), 0, NEO_COUNT);
	neoStrip.show();
}

void setXZ(){ // TODO combine into single movement. ZXMove + (Xmove - ZXmove)?
	zServo.setEaseTo(headFinalZ);
	xServo.setEaseTo(headFinalZ + (headFinalX-headFinalZ)); //Hmm. Note sure about this.
	
	while(isOneServoMoving){
		hmqClient.loop(); // Maintain MQTT connection
		
		updateAllServos();
	}
}

void updateHead(){
	float brightLvl;

	bool opening;

	if(headFinalLux >= PETAL_THRESHOLD){
		brightLvl = 0.0f;
		opening = true;
		winchServo.easeTo(90 + WINCH_DEPLOY_RATE);
	}
	else{
		brightLvl = 255.0f;
		opening = false;
		winchServo.easeTo(90 + WINCH_RETRACT_RATE);
	}

	unsigned long startMillis = millis();

	float step = 255.0f / PETALS_TRAVEL_DURATION;

	while(millis() - startMillis < PETALS_TRAVEL_DURATION){
		hmqClient.loop();

		if(opening){
			brightLvl += step;
		}
		else{
			brightLvl -= step;
		}

		setNeoToSun(constrain((int)brightLvl, 0, 255));
	}

	winchServo.easeTo(90); //Stop moving
}

void setStatusRing(int statusColour){
	neoStrip.clear();

	neoStrip.fill(neoStrip.gamma32(neoStrip.ColorHSV(statusColour))); // Map 0-15 to HSV red-green range

	neoStrip.show();
}

void callback(char* topic, byte* message, unsigned int length) {
	Serial.print("Message arrived on topic: ");
	Serial.print(topic);
	Serial.print(". Message: ");
	String messageTemp;

	String xStr = "";
	String zStr = "";
	String luxStr = "";

	String *targStr;

	if(String(topic) == "tracker/data"){

		for (int i = 0; i < length; i++) {
			Serial.print((char)message[i]);
			messageTemp += (char)message[i];

			if((char)message[i] == 'X'){
				targStr = &xStr;
			}
			else if((char)message[i] == 'Z'){
				targStr = &zStr;
			}
			else if((char)message[i] == 'L'){
				targStr = &luxStr;
			}
			else if((int)message[i] >= 48 && (int)message[i] <= 57){ // Add char to pertinant str
				*targStr += (char)message[i];
			}
			else{
				Serial.print("MQTT Msg Unknown Char: ");
				Serial.print((char)message[i]);
			}
		}
		Serial.println();

		headFinalX = xStr.toInt();
		headFinalZ = zStr.toInt();
		headFinalLux = luxStr.toInt();

		setXZ();

		updateHead();

		hmqClient.publish("flower/ACK", "1");
	}
	else if(String(topic) == "tracker/dconn"){
		setStatusRing(COLOUR_DCONN);
	}
}

void setup_wifi(){
	setStatusRing(COLOUR_WIFI);

	netClient.setInsecure();

	delay(10);
	// We start by connecting to a WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(wifi_SSID);

	WiFi.begin(wifi_SSID, wifi_Pass);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi conneced");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());

	setStatusRing(COLOUR_NORMAL);
}

void reconnect() {
	setStatusRing(COLOUR_MQTT);

	// Loop until we're reconnected
	while (!hmqClient.connected()) {
		Serial.print("Attempting MQTT connection...");

		// Attempt to connect
		if (hmqClient.connect(
			"SF_Flower",
			hiveMQ_UName,
			hiveMQ_Pass,
			"flower/dconn",
			2,
			true,
			"0"
		)) {
			Serial.println("MQTT Connected");
			// Subscribe
			hmqClient.subscribe("tracker/#");
			hmqClient.publish("flower/conn", "1"); // connection message
		} else {
			Serial.print("failed, rc=");
			Serial.print(hmqClient.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}

	setStatusRing(COLOUR_NORMAL);
}