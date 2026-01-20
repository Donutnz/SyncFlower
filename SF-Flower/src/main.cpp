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
float headLux = 0;

//Make petals hit the limit switch
void homePetals();

void openPetals();

void angleHead(int x, int z);

void readBattery();

// Wifi setup stuff
void setup_wifi();

// MQTT Connect
void reconnect();

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

	zServo.attach(MOT_AX0, 90);
	xServo.attach(MOT_AX1, 90);

	winchServo.attach(MOT_AX1, 0);

	setEasingTypeForAllServos(EASE_QUADRATIC_IN_OUT);

	// Init Neopixel ring
	neoStrip.begin();
	neoStrip.show();

	// Init WiFi
	setup_wifi();

	// Init HiveMQ MQTT
	hmqClient.setServer(hiveMQ_Addr, HIVEMQ_PORT);
	hmqClient.setClient(netClient);
	hmqClient.setCallback(callback);

	// Wait for servos to get to their start positions
	synchronizeAllServosStartAndWaitForAllServosToStop();
	
	// Set servo operation to be non-blocking
	synchronizeAllServosAndStartInterrupt(false);
	
	Serial.println("Set up done, running...");
}

void loop(){
	if(!hmqClient.connected()){
		reconnect();
	}
	hmqClient.loop();

	if(digitalRead(SW_UI)){

	}
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

void readBattery(){
	int battAvg = (analogRead(BATT_ADC_A) + analogRead(BATT_ADC_B)) / 2; // 0-4096, 1.786-2.5

	Serial.print("Batt ADC: ");
	Serial.println(battAvg);

	for(int i = 0; i < NEO_COUNT; i++){

	}
}

int msgToInt(byte* msgBody, unsigned int msgLength){ //Delete. error.
	int retVal=0;
	for(int i=0; i< msgLength; i++){
		retVal += ((char)msgBody[i] - '0');
	}
	return retVal;
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
		//Move head
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
	Serial.println("WiFi connected");
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