#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "DHT.h"

/* 
WiFi-connected Heater Thermostat, by Thomas Smith (@thomasesmith)

Requires the Adafruit DHT Sensor Library be installed in to your Arduino libraries: https://github.com/adafruit/DHT-sensor-library.

Please read README.md for more information

The code will connect to your Thing to your WiFi AP and then responds with JSON to the following requests

http://heater.local/status   Returns status information
http://heater.local/on       Turns heater system on, responds with confirmation 
http://heater.local/off      Turns heater system off, responds with confirmation 
http://heater.local/up       Adjusts threshold temperature up, responds with changed threshold value
http://heater.local/down     Adjusts threshold temperature down, responds with changed threshold value
*/

#define DHT_PIN 12      // Pin that your DHT sensor will use
#define DHTTYPE DHT22   // Change 'DHT22' to 'DHT11' if you're using a DHT11 sensor
#define LED_PIN 12      // Pin of your status LED (use 5 if you want to use the Things on-board LED)
#define RELAY_PIN 16    // Pin of your your relay

// WiFi AP Connection Parameters
const char WiFiSSID[] = ""; // Set this to your APs SSID name
const char WiFiPSK[] =  ""; // Set this to your APs WiFi Key 



////////////////////////////////////////////

float TEMP_THRESHOLD = 70.0;  // Power-up default of threshold temp.
float CURRENT_TEMP = 0;       // These two will get set on power-up, and then continually
float CURRENT_RH = 0; 

unsigned long MILLIS_OF_LAST_POLL = 0;
unsigned long MILLIS_OF_LAST_HEATER_TURN_ON = 0;

int HEATER_POWER = 0; // Stores power on/off 

WiFiServer server(80);
DHT dht(DHT_PIN, DHTTYPE);

void setup() 
{
    initHardware();
    connectWiFi();
    server.begin();
    setupMDNS();
    dht.begin();

    getTemp(); // To set the CURRENT_TEMP var on boot
    getRH(); // To set the CURRENT_RH var on boot
}

void loop() 
{  
    unsigned long currentMillis = millis();

    // Poll the temp from the DHT every 5 seconds or so
    if ((currentMillis - MILLIS_OF_LAST_POLL) > 5000) {
        getTemp(); // Sets the CURRENT_TEMP var 
        getRH(); // Sets the CURRENT_RH var 
        MILLIS_OF_LAST_POLL = currentMillis;
    }

    // Check if a client has connected
    WiFiClient client = server.available();
  
    if (!client) {

        if (CURRENT_TEMP < (TEMP_THRESHOLD) && HEATER_POWER == 1) {
            digitalWrite(RELAY_PIN, HIGH); // Close the thermostat circuits (on)
            MILLIS_OF_LAST_HEATER_TURN_ON = currentMillis; // Log millis of last power-on
        } else {
            if ((currentMillis - MILLIS_OF_LAST_HEATER_TURN_ON) > 90000) {
                // If the heater was turned on, we should let it run for at least 90 seconds
                // before letting the threshold turn it off, eegardless of whether or not the
                // threshold temperature has been reached or exceeded.
                // Without this, the relay ticks on and off frantically near the threshold amount. 

                digitalWrite(RELAY_PIN, LOW); // Open the thermostat circuit (off)
            }
        }

        return; // This loop ends here... 
    }

    // ...unless there was an http request...

    // Read the first line of the request
    String req = client.readStringUntil('\r');
    Serial.println(req);
    client.flush();
               
    String httpResponseCode = "";
    String jsonResponse = "";
    int heaterOnOff = digitalRead(RELAY_PIN); // Get the current status of the relay
  
    if (req.indexOf("/status") != -1) {
        httpResponseCode = "200 OK";

        jsonResponse =  "{\"status\": \"success\", ";
        jsonResponse += "\"temperature_fahrenheit\": " + String(CURRENT_TEMP) + ", \"relative_humidity_percentage\": " + String(CURRENT_RH) + ",";
        jsonResponse += "\"heater_onoff\": \"";
        
        if (HEATER_POWER == 1) { 
            jsonResponse +=  "on";  
        } else {
            jsonResponse +=  "off";  
        }

        jsonResponse += "\", ";
        jsonResponse += "\"heater_currently_running\": \"";

        if (heaterOnOff == 1) { 
            jsonResponse +=  "yes";  
        } else {
            jsonResponse +=  "no";  
        }

        jsonResponse += "\", ";
        jsonResponse += "\"threshold_temp\": " + String(TEMP_THRESHOLD) + " ";
        jsonResponse += "}";   
    
    } else if (req.indexOf("/on") != -1) {

        HEATER_POWER = 1;
        httpResponseCode = "200 OK";
        jsonResponse = "{\"status\": \"success\", ";
        jsonResponse += "\"heater_onoff\": \"on\" }";

    } else if (req.indexOf("/off") != -1) {
        HEATER_POWER = 0;
        httpResponseCode = "200 OK";
        jsonResponse = "{\"status\": \"success\", ";
        jsonResponse += "\"heater_onoff\": \"off\" }";

    } else if (req.indexOf("/down") != -1) {
        TEMP_THRESHOLD--;
        httpResponseCode = "200 OK";
        jsonResponse = "{\"status\": \"success\", \"threshold_temp\": " + String(TEMP_THRESHOLD) + "}";

    } else if (req.indexOf("/up") != -1) {
        TEMP_THRESHOLD++;
        httpResponseCode = "200 OK";
        jsonResponse = "{\"status\": \"success\", \"threshold_temp\": " + String(TEMP_THRESHOLD) + "}";

    } else {
        httpResponseCode = "501 Not Implemented";
        jsonResponse = "{\"status\": \"error\"}";

    } 

    client.flush();

    // Prepare the response and send it
    String s = "HTTP/1.1 ";
    s += httpResponseCode;
    s += "\r\n";
    s += "Content-Type: text/plain\r\n\r\n";
    s += jsonResponse;

    // Send the response to the client
    Serial.println("- Responding...");
    Serial.println(s);
    client.print(s);
    delay(1);

    Serial.println("- Finished responding.");
}

void connectWiFi()
{
    byte ledStatus = LOW;
    delay(1000); // Wait 1 second after power-up to begin trying to connect to WiFi

    Serial.println();
    Serial.println("- Connecting to: " + String(WiFiSSID));

    // Set WiFi mode to station (as opposed to AP or AP_STA)
    WiFi.mode(WIFI_STA);

    // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
    // to the stated [ssid], using the [passkey] as a WPA, WPA2,
    // or WEP passphrase.
    WiFi.begin(WiFiSSID, WiFiPSK);

    // Use the WiFi.status() function to check if the ESP8266
    // is connected to a WiFi network.
    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, ledStatus);
        ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

        delay(100); // Necessary delay to let the ESP2866 do some stuff
    }

    Serial.println("- WiFi connected!");  
    Serial.println("- IP address: ");
    Serial.println(WiFi.localIP());
}

void setupMDNS()
{
    // MDNS.begin(<domain>) will set up an mDNS that you can point to
    // "<domain>.local"

    if (!MDNS.begin("heater"))  {   // will respond to http://heater.local/
        Serial.println("- Error setting up MDNS responder!");

        while(1) { 
            delay(1000);
        }
    }

    Serial.println("- mDNS responder started.");
}

void initHardware()
{
    Serial.begin(9600);

    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(RELAY_PIN, LOW);
}


String getTemp()
{
    float f = dht.readTemperature(true);
    String r = "";

    if (isnan(f)) {
        r += "0";
        return r;
    }

    r += f;

    if (CURRENT_TEMP != f) {
        CURRENT_TEMP = f;
    }

    return r;
}

String getRH()
{
    float h = dht.readHumidity();
    String r = "";

    if (isnan(h)) {
        r += "0";
        return r;
    }

    r += h;

    if (CURRENT_RH != h) {
        CURRENT_RH = h;
    }

    return r;
}

