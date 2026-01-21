/**
 * Configuration
 * 
 * !! NO CREDENTIALS !!
 */

#define APP_DEBUG

#define HEAD_SPEED 60 // Speed of head servos
#define WINCH_SPEED 100 // Speed of winch servo

#define WINCH_RETRACT_RATE -10 //degrees
#define WINCH_DEPLOY_RATE 10 //degrees

#define HEAD_MAX_Z 120
#define HEAD_MAX_X 90

#define WINCH_DIAMETER 24 //mm. Diameter of winch drum

#define NEO_COUNT 16

// Battery level min and max
// Y = (X - A) / (B - A) * (D - C) + C
#define BATT_MAX 2216 // 3.0V
#define BATT_MIN 3103 // 4.2V

#define BATT_DISP_DURATION 10000 //milliseconds to show voltage readout

#define PETAL_THRESHOLD 1000 //lux. Below this the petals close

#define PETALS_TRAVEL_DURATION 3000 //ms time for petals to go change

// Colours
#define COLOUR_SUN 9830 //Might change to HSV hue