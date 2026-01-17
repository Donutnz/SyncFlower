/**
 * Configuration
 * 
 * !! NO CREDENTIALS !!
 */

/**
 * Battery Level Read
 * 
 * ADC Max: 1.1V
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
#define SCAN_X_GRADS_DIV 4
#define SCAN_Z_GRADS_DIV 4

#define SERVO_SPEED 60 //100?