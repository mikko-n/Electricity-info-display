# Electricity-info-display
Simple RF Electricity info display with Arduinos, radios &amp; mandatory LED

![Display device] (/material/valmis3.jpg)

Creation from Nortal Hackathon 2015 event.

### Motivation
Without easily accessible and real-time measurement data the training process for living in our new house in the most paycheck-friendly way would be slow and based mostly on guesswork. To help ourselves to learn more quickly, I created a "training wheels" device which would tell us what is happening right now, energy-wise.

### Known bugs
- Cumulative wattage counter overflows (an int value)
- If display unit is not receiving anything, it keeps displaying the previous values instead of nothing (or err msg)

### Further development ideas
 list of "additional things to try" still includes some ideas:

-  Web / mobile interface for visualizing the spendings / consumed watts
-  REST API for the data stream(s)
-  Add real-time clocks / additional sensors / nodes (=mesh network)
-  Sleep mode for battery powered nodes
-  Add audio warnings for certain events: used kw/h limit or dramatic changes in electricity consumption, for example
-  Pull spot prices for electricity from internet and give suggestions when it's cheap to do laundry (and change your electricity plan to use the spot pricing)
-  Profile your appliances and find/weed out the most energy hungry ones
-  Profile your appliances and send a notification when your laundry / dishes / etc are done (electricity consumption drops)

### Inspiration
- Geo Solo II smart energy monitor (http://www.greenenergyoptions.co.uk/products-and-services/products/solo-2/)
- Emoncms.org Open source energy visualization (http://emoncms.org/site/home)
- EmonGLCD (http://openenergymonitor.org/emon/emonglcd)
- IoT + DIY = http://www.mysensors.org/
- Über Home automation with Arduino & RasPi (http://www.instructables.com/id/Uber-Home-Automation-w-Arduino-Pi/)
- http://en.wikipedia.org/wiki/Acme_Corporation
- https://learn.adafruit.com/nokia-5110-3310-monochrome-lcd/wiring
- http://serdisplib.sourceforge.net/ser/pcd8544.html
- https://maniacbug.wordpress.com/2012/03/30/rf24network/
