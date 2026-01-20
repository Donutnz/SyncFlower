/**
 * Configuration
 * 
 * !! NO CREDENTIALS !!
 */

#define APP_DEBUG

/**
 * Battery Level Read
 * 
 * Batt range: 4.2 - 3.0V
 */

#define BATT_R1 10000 //ohms
#define BATT_R2 3192 //ohms

// Servos scan field
#define SCAN_X_MAX 140 //degrees
#define SCAN_X_MIN 90 //degrees
#define SCAN_Z_MAX 180 //degrees
#define SCAN_Z_MIN 0 //degrees

// Divisior for scan field. Larger number = courser scan
#define SCAN_X_SAMPLES 4
#define SCAN_Z_SAMPLES 4

#define SERVO_SPEED 60 //100?

#define SCAN_DELAY 30000 //milliseconds. Time between scans

//Status LED States
#define STATUS_NORMAL 0
#define STATUS_STARTING 1
#define STATUS_DISCONNECT 2
#define STATUS_LOWBATT 3
#define STATUS_SCANNING 4
#define STATUS_ENDSCAN 5