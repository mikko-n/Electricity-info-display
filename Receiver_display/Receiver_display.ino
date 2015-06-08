/*
* Based on several tutorials & getting started example sketch for nRF24L01+ radios
*
* Firmware for Energy meter infodisplay unit, using
* Arduino Pro Mini, Nokia 5110 display & nRF24L01 radio
*/

#include <SPI.h>
#include "RF24.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);


/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 0;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(9,10);
/**********************************************************/

byte addresses[][6] = {"1Node","2Node"};

// Used to control whether this node is sending (0) or receiving (1) 
// with two radios and just one-direction messaging, this could be !radioNumber
bool role = 1;

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

// price per kwh, from energy company
float price_per_kwh = 0.11f;

void setup() {
  Serial.begin(57600);
  display.begin();
  
  // Info display splash screen
  // There's an adafruit logo in the library, this could perhaps be sliced off if available memory is an issue
  display.clearDisplay(); // clear the display immediately to prevent AF logo showing up
  display.display(); // push blank screen to display
  display.setContrast(60); // range from 0-127
  display.setTextSize(2); // 1 is small, 2 for large
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Hackathon 2015");
  display.display();
  delay(2000);
  
  Serial.println(F("*** Receiving from the other node"));
  
  initRadio();
}

// radio initialization
void initRadio() {
  radio.begin();

  // Set the PA Level below maximum to prevent power supply related issues. Soldering a small cap between
  // the modules GND/Vin could help if those appear (do an internet search for solutions).
  // Because the close proximity of the devices, no need to use RF24_PA_MAX, which is default.
  // Available options are: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  // So far no problems.
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

void loop() {
  readValues();
   
}

// do the radio stuff
void readValues() {
/****************** Ping Out Role ***************************/  
if (role == 1)  {
    
    radio.stopListening();                                    // First, stop listening so we can talk.
        
    Serial.println(F("Sending request..."));

    unsigned long time = millis();                             // Take the time, and send it.  This will block until complete
     if (!radio.write( &time, sizeof(unsigned long) )){
       //Serial.println(F("failed"));
     }
        
    radio.startListening();                                    // Now, continue listening
    
    unsigned long started_waiting_at = millis();               // Set up a timeout period, get the current microseconds
    boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not
    
    while ( ! radio.available() ){                             // While nothing is received
      if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
          timeout = true;
          break;
      }      
    }
        
    if ( timeout ){                                             // Describe the results
        //Serial.println(F("Failed, response timed out."));
    } else {
        unsigned long got_time;                                // Grab the response, compare, and send to debug print
     
        radio.read( &myData, sizeof(myData) );

        doInfoDisplay();         

        // What have we here?
        Serial.print(F("Sent :"));
        Serial.print(time);
        Serial.print(F(", Got response from:"));
        Serial.print(myData.sender);
        Serial.print(", to: ");
        Serial.print(myData.receiver);
        Serial.print(", requested at: ");
        Serial.print(myData._millis);  
        Serial.print(", current kWh = ");
        Serial.print((double)myData.currentWatts);
        Serial.print(", cumulative W = ");
		Serial.print(myData.cumulative_kW);
		Serial.print(" ");
        Serial.println(myData.cumulative_W); 
        Serial.print(F(". Round-trip delay "));
        Serial.print(millis()-got_time);
        Serial.println(F(" milliseconds"));
    }

    // Try again 1s later
    delay(1000);
  }

} // readValues

// method to print info to display
void doInfoDisplay() {
  String currentWatts = "nyt"; // nyt = now/atm in finnish
  String yhteensa = "yht"; // yht = abbr for "total" in finnish
  
  display.clearDisplay();
  display.setTextSize(1); // height 7px
  display.setCursor(2,0);
  display.println(currentWatts);
  display.setCursor(2,16);
  display.println(yhteensa);
  
  display.setTextSize(2); // height 14px
  display.setCursor(66,0);
  display.println("W");
  
  display.setTextSize(1);
  display.setCursor(66,16); // 14px + 2px margin
  display.println("W");
  display.setCursor(66,25);
  display.println("e");
  
  
  if (myData.currentWatts > 0) {
     display.setTextSize(2);
     float current = (float)myData.currentWatts; 
     display.setCursor(30,0);
     display.println((int)current);
  } else {
     display.setTextSize(2);
     display.setCursor(30,0);
     display.println('-');
  }
  
  if (myData.cumulative_W > 0) {
	 String cumulative;	 

	 // check the kW value for display purposes
	 if (myData.cumulative_kW > 0) {
		cumulative = myData.cumulative_kW + " " + myData.cumulative_W;
	 } else {
		cumulative = String(myData.cumulative_W);
	 }

     display.setTextSize(1);
     display.setCursor(30,16);	 	 
     display.println(cumulative);
     display.setCursor(30,25);
     float money = ((float)myData.cumulative_kW * 0.11f) + ((float)myData.cumulative_W * price_per_kwh / 1000.0f);
     display.println(money); 
  } else {
     display.setTextSize(1);
     display.setCursor(30,16);
     display.println('-');
     display.setCursor(30,25);
     display.println('-');
  }
  
  display.display();
}
