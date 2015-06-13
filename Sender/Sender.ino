/*
* Based on several tutorials & getting Started example sketch for nRF24L01+ radios
*
* Firmware for Energy meter interface unit (blink calculator) using
* Arduino Pro Mini, led, LDR & nRF24L01 radio
*/

#include <SPI.h>
#include "RF24.h"

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 1;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(9,10);
/**********************************************************/

byte addresses[][6] = {"1Node","2Node"};

// Used to control whether this node is sending (0) or receiving (1)
bool role = 0;

int led = 3; // measurement detected -led
int LDR = A0; // LDR pin
boolean isLedOn = false;
int ldrValue = 255;

// LDR threshold value, good start value is around 200 (bigger=more sensitive)
// As LDR is an analog device, this depends on your energy meter led pulse duration.
// Shorter pulses require more generous threshold value to be detected.
int ldrThreshold = 200;

unsigned long lastMeasurement = 0;

const float w_per_pulse = 1;  // watts consumed per led pulse
const unsigned long ms_per_hour = 3600000UL; // milliseconds in hour
unsigned int chan1_count = 0; // how many blinks are counted since started
unsigned long report_time = 0; // time since last report
unsigned long chan1_first_pulse = 0;
unsigned long chan1_last_pulse = 0;

//volatile byte chan1_pulse = 0; // for ISR function

// data struct, changes here require an update to the receiver sketch, too
// note: sending 'String' type values does not work as intended, maybe a byte array should be used?
struct dataStruct{
	unsigned long _millis; // request timestamp
	String sender; // sending node id
	String receiver; // to who this packet is intended to
	unsigned int currentWatts;
	unsigned int cumulative_kW; // cumulative values divided to kW / W
	unsigned int cumulative_W;  // otherwise they will overflow quite quickly
	
} myData;


void setup() {
	Serial.begin(57600);
	Serial.println(F("*** Sending to the other node"));
	
	pinMode(led, OUTPUT);
	pinMode(LDR, INPUT);
	myData.sender = "Wattmeter";
	myData.receiver = "Server";
	myData.currentWatts = 0;
	myData.cumulative_kW = 0;
	myData.cumulative_W = 0;
	radio.begin();

	// Set the PA Level below maximum to prevent power supply related issues. Soldering a small cap between
	// the modules GND/Vin could help if those appear (do an internet search for solutions)
	// Because the close proximity of the devices, no need to use RF24_PA_MAX, which is default.
	// Available options are: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
	radio.setPALevel(RF24_PA_HIGH);
	
	// Open a writing and reading pipe on each radio, with opposite addresses
	if(radioNumber){
		radio.openWritingPipe(addresses[1]);
		radio.openReadingPipe(1,addresses[0]);
		}else{
		radio.openWritingPipe(addresses[0]);
		radio.openReadingPipe(1,addresses[1]);
	}
	
	// Start the radio listening for data
	radio.startListening();
}


// main loop, not much happens here
void loop() {
	
	doMeasurements();
	
}

// checks led, counts blinks, 1 blink = 1 watt
void doMeasurements() {
	
	unsigned long chan1_delta; // Time since last pulse
	static unsigned int chan1_watts; // current wattage
	
	// if 60 seconds since last detected pulse, current consumption = 0W
	if (millis() - chan1_last_pulse > 60000) {
		chan1_watts = 0;
	}
	
	// send reports in 5 second periods
	if (millis() - report_time > 5000) {
		
		// calculate time difference a.k.a time since last pulse
		chan1_delta = chan1_last_pulse - chan1_first_pulse;
		Serial.print("time (ms) pulse delta: ");
		Serial.println(chan1_delta);
		Serial.print("pulses counted since last report: ");
		Serial.println(chan1_count);
		
		
		if (chan1_delta != 0 && chan1_count != 0) {
						
			// calculate current W/h
			//             = (counted pulses - 1) * watts per pulse * ms in hour / time since last pulse
			chan1_watts = (chan1_count - 1) * w_per_pulse * ms_per_hour / chan1_delta;

			// current consumption calculated, reset counter
			chan1_count = 0;
			
			// put calculated value to data struct
			myData.currentWatts = chan1_watts;
		}
		
		// if current consumption and report time deviate from zero
		if (!(report_time == 0 && chan1_watts == 0)) {
			
			Serial.print("Current watts: ");
			Serial.println(chan1_watts);
			report_time = millis();
			
			reportValues();
			
		}
	}
	
	/*
	// should optimally come from ISR function
	if (chan1_pulse == 1) {
	chan1_count++;
	chan1_pulse = 0; // isLedOn

	chan1_last_pulse = millis();
	if (chan1_count == 1) { // was reset
	chan1_first_pulse = chan1_last_pulse;
	}
	}
	*/
	
	ldrValue = analogRead(LDR);

	if (ldrValue < ldrThreshold && !isLedOn) {
		Serial.print("LDR value = ");
		Serial.print(ldrValue);
		Serial.print(", led is ON again after ");
		Serial.print(millis()-lastMeasurement);
		Serial.println(" ms delay");
		isLedOn = true;
		digitalWrite(led, HIGH); // turn indicator led on
		
		myData.cumulative_W++;
		
		// check if kW counter should be updated
		if (myData.cumulative_W >= 1000) {
			myData.cumulative_kW++;
			myData.cumulative_W = 0;
		}
		
		lastMeasurement = millis();
	}
	else {
		if (isLedOn && ldrValue > ldrThreshold) {
			Serial.print("LDR value = ");
			Serial.print(ldrValue);
			Serial.println(", led turned OFF");
			isLedOn = false;
			digitalWrite(led, LOW); // turn indicator led on
			
			chan1_last_pulse = millis();
			chan1_count++;
			
			if (chan1_count == 1) { // was reset
				chan1_first_pulse = chan1_last_pulse;
			}
		}
	}
}

// do the radio stuff
void reportValues() {

	/****************** Pong Back Role (report readings on demand) ***************************/

	if ( role == 0 )
	{
		unsigned long got_time;
		//    static unsigned int chan1_watts;
		if( radio.available()){
			// Variable for the received timestamp
			while (radio.available()) {                                   // While there is data ready
				radio.read( &got_time, sizeof(unsigned long) );             // Get the payload
			}
			myData._millis = got_time;
			radio.stopListening();                                        // First, stop listening so we can talk
			radio.write( &myData, sizeof(myData) );                       // Send watt report
			radio.startListening();                                       // Now, resume listening so we catch the next packets.
			Serial.print(F("Sent response :"));
			
			Serial.print(myData._millis);
			Serial.print(", ");
			Serial.print(myData.currentWatts);
			Serial.print(", ");
			Serial.print(myData.cumulative_kW);
			Serial.print(", ");
			Serial.println(myData.cumulative_W);
		}
	}

} // reportValues()

// Method to read current supply voltage,
// could be useful in battery powered nodes
long readVcc() {
	long result;
	// Read 1.1V reference against AVcc
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Convert
	while (bit_is_set(ADCSRA,ADSC));
	result = ADCL;
	result |= ADCH<<8;
	result = 1126400L / result; // Back-calculate AVcc in mV
	return result;
}

