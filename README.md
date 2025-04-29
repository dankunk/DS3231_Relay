# Setting and reading the DS3231 RTC to UTC with Circuit Python

## 1. Flash the pico W board with the circuit python UF2 file 
- It can be found at the circuit python website. 

## 2. Add these neccessary libraries to the Pico W. 
- They can be found online at adafruit.com or the circuit python website.

#### A. adafruit_ntp
#### B. adafruit_ds3231
#### C. adafruit_register
	
- Most of the other needed libraries should come installed in your circuit python build that you flashed in the previous step. If not, they can be added to the lib folder as needed. Use google and an AI companion if you get stuck!
	
Note: The A, B, and C libraries can be found in the included lib/ directory in the PicoW_Set-ds3231/ github directory. Just copy over the files to the lib/ directory on your board.

## 3. Upload the code.py file to the root of the board.

- This can be found in the PicoW_Set-ds3231/ directory in this repo.

Note: The code.py file will automatically run everytime your board recieves power.

- So each time you plug in your board with the ds3231 attached it will be reset to UTC time after connecting to the wifi.

## 4. Add a secrets.py file to your boards root directory
#### In this file you should include your wifi name and password.
	SSID = "My_Wifi_Network_Name"
	PASSWORD = "password"

- Note: 5ghz connections won't connect. Use the 2.5ghz connection!

## 5. Wire the DS3231 into the Pi Pico. 

- Breadboards, jumper wires, directly soldering all work. Just get it connected. 

- Note: This script manually addresses pins 17 (clock) and 16 (data) but these can be updated as needed. 

		Power → DS3231 VCC → 3V3 (not 3V3_EN), GND → any Pico GND

		I²C → SDA → GP16 (or STEMMA SDA), SCL → GP17 (or STEMMA SCL)

		Plug in the backup battery on the DS3231 module (so it keeps time even when unconnected)

## 6. Now everything is ready to go! 
- After setting the ds3231, it can be unplugged and keep time if connected to a coin cell battery. 

- Now you can query the ds3231 with an arudino or any other compatible board! Check the Arduino-sketch folder for examples.

# Using the RTC and an Arduino to Trigger a IoT relay from DigitalLoggers (or a similar device):

## 1. Flash your arduino with the "check_ds3231_trigger-relay"
#### Note: It can be found in the Arduino sketches directory in this repo

## 2. Update the Schedule in the sketch

- Update this code with your schedule of interest!
```// ——— your schedule ———
// Note: all fired flags start false
Event events[] = {
  {  0,  0,  0,  8, LOW,  false },   // at 00:00:00 → pin8 LOW
  {  8,  0,  0,  8, HIGH, false },   // at 08:00:00 → pin8 HIGH
  // …add more if you like…
};
```

- This is what everything means in the curly brackets:
```{ hour, minute, second, GPIO pin that is being triggered, GPIO state, triggered already today?}```


## 3. Upload the sketch and plug in all the wires!
### It's that easy!

- GPIO pin 8 should be plugged into the (+) slot on the relay. 
The (-) slot on the relay should be plugged into a GND pin on the Arduino.


