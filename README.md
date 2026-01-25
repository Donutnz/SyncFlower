# SyncFlower
A simple gadget for sharing sunshine with someone far away. 

## Description

A simple project that uses one gadget (Tracker) to track the position of the sun and transmit its angles (X/Z) and Lux. The other gadget (Flower) receives the data and reacts accordingly; turning, opening, and glowing.

## Set up

### Credentials
Copy creds.h file template from root into both SF-Tracker/include/ and SF-Flower/include/ directories. Fill out details. 

### Flashing 
Use PlatformIO (e.g. VSC Extension) for uploading on both devices. It is the easiest option.

### Configuration
- Flower: /SF-Flower/include/config.h
- Tracker: /SF-Tracker/include/config.h


## Requirements
PlatformIO

## Platform
ESP32-C3 SuperMini

## Operation

### Tracker RGB LED Codes
- Green: Normal
- Red: Flower gadget not connected (did not acknowledge data or sent a disconnect message)
- Blue: Device is starting up. System hasn't stabilised yet.
- Purple: Scanning for sun.
- Orange: Done scanning. Rotate to point at sun, transmitting data, and waiting for acknowledge message from flower.

### Flower

Press button to show battery level. It will revert after 10 seconds.
