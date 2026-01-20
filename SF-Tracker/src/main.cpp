#include <Arduino.h>
#include <ServoEasing.hpp>
#include <Adafruit_VEML7700.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#include "pins.h"
#include "config.h"
#include "creds.h"

/**
 * Tracker side gadget
 */

ServoEasing xServo;
ServoEasing zServo;

Adafruit_VEML7700 veml = Adafruit_VEML7700();

WiFiClientSecure netClient;
PubSubClient hmqClient(netClient);

//Char buffer for MQTT pub
char msgBuffer[12];

int sunPosX = 90;
int sunPosZ = 90;
float luxReading = 0;

int battLvl = 0;

bool flowerConnected = false;

unsigned long previousMillis = 0;

// Wifi setup stuff
void setup_wifi();

//max ADC 1.1V
float readBatteryVoltage();

/**
 * Set status LED
 */
void setStatusLED(int status);

//Collect data and send to flower
void updateFlower();

/*
Scanning for sun and find its angle coords
*/
void scanForSun();

void reconnect();

void callback(char* topic, byte* message, unsigned int length);

void setup(){

	Serial.begin(115200);

	#ifdef APP_DEBUG //Delay if debugging for serial port to warm up
	delay(5000);
	#endif
	
	Serial.println("Tracker Start...");

	// Init LEDs

	pinMode(ONBOARD_LED, OUTPUT);
	pinMode(STATUS_R, OUTPUT);
	pinMode(STATUS_G, OUTPUT);
	pinMode(STATUS_B, OUTPUT);

	setStatusLED(STATUS_STARTING);

	// Init Servos

	xServo.attach(SERVO_X, 90);
	zServo.attach(SERVO_Z, 90);
	
	xServo.setSpeed(SERVO_SPEED);
	zServo.setSpeed(SERVO_SPEED);

	xServo.setReverseOperation(true);
	
	setEasingTypeForAllServos(EASE_SINE_IN_OUT);

	// Wait for servos to get to their start positions
	synchronizeAllServosStartAndWaitForAllServosToStop();

	// Set servo operation to be non-blocking
	synchronizeAllServosAndStartInterrupt(false);

	// Init VEML
	Wire.begin(I2C_SDA, I2C_SCL); //Set I2C pin
	if(!veml.begin()){
		Serial.println("Sensor not found");
		while (1);
	}
	Serial.println("VEML Sensor Found");

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

	if (currentMillis - previousMillis >= SCAN_DELAY) {
		
		if(!flowerConnected){
			setStatusLED(STATUS_DISCONNECT);
		}
		//could have else() that stops the scan if the flower disconnects

		updateFlower();
		
		flowerConnected = false;

		previousMillis = millis(); //Scan takes a while. Make delay between end and start of scans

		//sprintf(msgBuffer, "%d", veml.readLux());
		//hmqClient.publish("tracker/lux", msgBuffer);
	}
}

void updateFlower(){
	scanForSun();

	sprintf(msgBuffer, "%d", sunPosX);
	hmqClient.publish("tracker/X", msgBuffer);

	sprintf(msgBuffer, "%d", sunPosZ);
	hmqClient.publish("tracker/Z", msgBuffer);

	sprintf(msgBuffer, "%d", luxReading); //20-5000 lux range
	hmqClient.publish("tracker/lux", msgBuffer);

	hmqClient.publish("tracker/EOT", "1"); // End of data

	Serial.println("Published update");
}

void scanForSun(){
	float lumMaxX = 0;
	float lumMaxZ = 0;

	int xGrad = (SCAN_X_MAX - SCAN_X_MIN) / SCAN_X_SAMPLES;
	int zGrad = (SCAN_Z_MAX - SCAN_Z_MIN) / SCAN_Z_SAMPLES;

	float inputLuxVal = 0;
	int nextSample = 0;

	setStatusLED(STATUS_SCANNING);

	for(int u = SCAN_X_MIN; u < SCAN_X_MAX; u+=xGrad){ // X Axis
		xServo.easeTo(u); // X servo into position

		nextSample = SCAN_Z_MIN;

		zServo.easeTo(SCAN_Z_MIN); // Go to start of scan
		zServo.setEaseTo(SCAN_Z_MAX); // Set end of scan

		while(zServo.isMoving()){
			hmqClient.loop(); // Keep MQTT connection alive

			if(zServo.getCurrentAngle() >= nextSample){
				//inputLuxVal = veml.readLux(VEML_LUX_NORMAL_NOWAIT); // Maybe readALS()?
				inputLuxVal = veml.readWhite(false); // Maybe readALS()?
				
				if(inputLuxVal > lumMaxZ){
					lumMaxZ = inputLuxVal; // Set the top Z lux for this sweep
					sunPosZ = zServo.getCurrentAngle(); // Save Z angle of top Lux
				}

				Serial.print("SCANNING: ");
				Serial.print("X: ");
				Serial.print(xServo.getCurrentAngle());
				Serial.print(" Z: ");
				Serial.print(zServo.getCurrentAngle());
				Serial.print(" Lux: ");
				Serial.println(inputLuxVal);

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

	setStatusLED(STATUS_ENDSCAN);

	//Home servos
	xServo.easeTo(sunPosX);
	zServo.easeTo(sunPosZ);

	luxReading = lumMaxX;
}

void setStatusLED(int status){
	digitalWrite(STATUS_R, LOW);
	digitalWrite(STATUS_G, LOW);
	digitalWrite(STATUS_B, LOW);

	switch(status){
		case STATUS_NORMAL:
			digitalWrite(STATUS_G, HIGH);
			break;
		case STATUS_DISCONNECT:
			digitalWrite(STATUS_R, HIGH);
			break;
		case STATUS_STARTING:
			digitalWrite(STATUS_B, HIGH);
			break;
		case STATUS_SCANNING:
			digitalWrite(STATUS_B, HIGH);
			digitalWrite(STATUS_R, HIGH);
			break;
		case STATUS_ENDSCAN:
			digitalWrite(STATUS_G, HIGH);
			digitalWrite(STATUS_R, HIGH);
			break;
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
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
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

	// Feel free to add more if statements to control more GPIOs with MQTT

	// If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
	// Changes the output state according to the message
	/*
	if (String(topic) == "esp32/output") {
		Serial.print("Changing output to ");
		if(messageTemp == "on"){
			Serial.println("on");
			//digitalWrite(ledPin, HIGH);
		}
		else if(messageTemp == "off"){
			Serial.println("off");
			//digitalWrite(ledPin, LOW);
		}
	}
	*/
	if(String(topic) == "flower/ack"){
		flowerConnected = true;
		setStatusLED(STATUS_NORMAL);

		Serial.print("Flower ACK: ");
		Serial.println(messageTemp);
	}
	else if(String(topic) =="flower/conn"){
		flowerConnected = true;
		setStatusLED(STATUS_NORMAL);
	}
	else if(String(topic) =="flower/dconn"){
		flowerConnected = false;
		setStatusLED(STATUS_DISCONNECT);
	}
}

void reconnect() {
	// Loop until we're reconnected
	while (!hmqClient.connected()) {
		Serial.print("Attempting MQTT connection...");

		// Attempt to connect
		if (hmqClient.connect(
				"SF_Tracker",
				hiveMQ_UName,
				hiveMQ_Pass,
				"tracker/discon",
				2,
				true,
				"0"
			)) {
			Serial.println("MQTT Connected");
			// Subscribe
			hmqClient.subscribe("flower/#");
			hmqClient.publish("tracker/connect", "1"); // connection message
		} else {
			Serial.print("failed, rc=");
			Serial.print(hmqClient.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}