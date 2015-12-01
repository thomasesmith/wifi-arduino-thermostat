# WiFi-Connected Arduino Heater Thermostat
In addition to acting as a traditional heater thermostat (turning the heater on when the temperature falls below the set threshold temperature, and turning it off when it the temperature reaches or exceeds the set threshold temperature), it also sets up an mDNS HTTP server and creates endpoints to offer an HTTP API control interface.

### Hardware
This was written on a $15 SparkFun ESP8266 "Thing" Board, an Arduino-compatible development board built around the ESP8266 (https://www.sparkfun.com/products/13711). This particular code also requires a DHT-22 or DHT-11 temperature and humidity sensor, a relay, and optionally an LED.

The two wires leading to your heater should be inserted in to the "NO" (Normally Open) side of your relay, so that the circuit to your heater is open by default, and the circuit will need to be closed in order to turn the heater on. 

### Required Library
This Arduino sketch also requires the Adafruit DHT Sensor Library be installed in to your Arduino libraries. You can find it at https://github.com/adafruit/DHT-sensor-library.

### Endpoints
You can make your UI in whatever manner you like and have it `GET` these URIs – or you can just `curl` them from a command line. I personally made a web app that uses buttons to hit them, and then updates the UI with the feedback it gets.

In these examples, I use the mDNS name in the URI, but whatever IP is given to your Thing when it connected will work fine too. JSON is returned.

* `http://heater.local/status`   Returns all of the status information:

        {"status": "success", "temperature_fahrenheit": 70.88, "relative_humidity_percentage": 28.70,"heater_onoff": "off", "heater_currently_running": "no", "threshold_temp": 70.00 }
	* `temperature_fahrenheit` Returns the current temperature in degrees fahrenheit (from the DHT sensor).
	* `relative_humidity_percentage` Returns the current percentage of relative humidity (from the DHT sensor). 
	* `heater_onoff` Whether or not the system is currently on. If this is set to on, the heater itself will turn off and on around the 'threshold_temp.' If it is off, it will not.
	* `heater_currently_running` Whether or not the actual relay pin is raised and the heaters circuit is closed, meaning the heater itself is on. A value of 'yes' means the circuit is closed (heater is on), and 'no' means the circuit is open (heater is off).
 	* `threshold_temp` The temperature in fahrenheit the will trip the heater in to turning on. If 'heater_onoff' is set to on, and the temperature falls below that the threshold_temp, the heater circuit will close turning the heater on. It will not open it back up until the threshold temperature is reached again.
   

* `http://heater.local/on`       Turns heater system on. Responds with confirmation (this doesn't necessarily turn the actual heater on, it just allows the code to do so when the conditions are right):

        {"status": "success", "heater_onoff": "on"}

* `http://heater.local/off`      Turns the heater system off. Responds with confirmation:

        {"status": "success", "heater_onoff": "off"}

* `http://heater.local/up`       Adjusts threshold temperature up. Responds with new, current threshold value:

        {"status": "success", "threshold_temp": 71.00}

* `http://heater.local/down`     Adjusts threshold temperature down. Responds with new, current threshold value:

        {"status": "success", "threshold_temp": 69.00} 

## TODO
This could stand to be improved in the following ways: 
* The API isn't actually RESTful. There shouldn't be any verbs in the endpoints (i.e. 'up', 'down', 'off', 'on'), and I'm not yet taking advantage of the appropriate HTTP methods. So that should will eventually be adjusted.
* I'd like to add an easier way to switch between using fahrenheit or celsius units of temperature measurement. 

## Contact
You can tweet me at [@thomasesmith](http://twitter.com/thomasesmith) if you find any bugs or want me to address something. 
