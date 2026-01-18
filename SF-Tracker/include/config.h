/**
 * Configuration
 * 
 * !! NO CREDENTIALS !!
 */

/**
 * Battery Level Read
 * 
 * Batt range: 4.2 - 3.0V
 */

#define BATT_R1 10000 //ohms
#define BATT_R2 3192 //ohms

// Servos scan field
#define SCAN_X_MAX 180 //degrees
#define SCAN_X_MIN 0 //degrees
#define SCAN_Z_MAX 180 //degrees
#define SCAN_Z_MIN 0 //degrees

// Divisior for scan field. Larger number = courser scan
#define SCAN_X_SAMPLES 4
#define SCAN_Z_SAMPLES 4

#define SERVO_SPEED 60 //100?

#define SCAN_DELAY 30000 //milliseconds. Time between scans