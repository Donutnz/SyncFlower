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

int headX = 90;
int headZ = 90;
int headLux = 0;

bool petalsClosed = false;

// Time batt disp started
unsigned long battDispMillis = 0;

// Time petals motor started
unsigned long petalsStartMillis = 0;

//Make petals hit the limit switch
void homePetals();

void openPetals();

void updateXZ();

void updateHead();

void setNeoToSun();

void setNeoToSun(byte brightness);

void readBattery();

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
	Serial.print("THing: ");
	Serial.println(digitalRead(SW_LIMIT));
	// Init Neopixel ring
	neoStrip.begin();
	neoStrip.show();

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
		winchServo.easeTo(0); // Blocking. May cause issues?
	}
}

void homePetals(){
	Serial.println("Homing...");

	if(digitalRead(SW_LIMIT)){ //If already touching limit switch
		Serial.println("home stooped");
		return;
	}

	//winchServo.easeTo(WINCH_RETRACT_RATE);

	unsigned long startMillis = millis();

	while(!digitalRead(SW_LIMIT)){
		//hmqClient.loop();
	}

	winchServo.easeTo(0); // Stop pulling

	Serial.println("Petals Homed");
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

void updateXZ(){
	zServo.setEaseTo(headZ);
	xServo.setEaseTo(headZ);
	
	while(isOneServoMoving){
		hmqClient.loop(); // Maintain MQTT connection
		
		updateAllServos();
	}
	
	xServo.setEaseTo(headX);

	while(isOneServoMoving){
		hmqClient.loop(); // Maintain MQTT connection

		updateAllServos();
	}
}

void updateHead(){
	float brightLvl;

	bool opening;

	if(headLux >= PETAL_THRESHOLD){
		brightLvl = 0.0f;
		opening = true;
		winchServo.easeTo(WINCH_DEPLOY_RATE);
	}
	else if(headLux < PETAL_THRESHOLD){
		brightLvl = 255.0f;
		opening = false;
		winchServo.easeTo(WINCH_RETRACT_RATE);
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

	winchServo.easeTo(0); //Stop moving
}

void callback(char* topic, byte* message, unsigned int length) {
	Serial.print("Message arrived on topic: ");
	Serial.print(topic);
	Serial.print(". Message: ");
	String messageTemp;

	for (int i = 0; i < length; i++) {
		Serial.print((char)message[i]);
		messageTemp += (char)message[i];
	}
	Serial.println();

	if(String(topic) == "tracker/X"){
		headX = atoi((char*)message);
	}
	else if(String(topic) == "tracker/Z"){
		headZ = atoi((char*)message);
	}
	else if(String(topic) == "tracker/lux"){
		headLux = atoi((char*)message);
	}
	else if(String(topic) == "tracker/EOF"){
		updateXZ();

		updateHead();

		hmqClient.publish("flower/ACK", "1");
	}
}

void setup_wifi(){
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
}

void reconnect() {
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
}