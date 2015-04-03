#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <cc3000_PubSubClient.h>
#include <DHT.h>
#include<stdlib.h> 

#define aref_voltage 3.3

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!

// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);

#define WLAN_SSID       "XXX"
#define WLAN_PASS       "XXX"
#define DEVICE_ID       "arduino_uno"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11 

Adafruit_CC3000_Client client;
   
// We're going to set our broker IP and union it to something we can use
union ArrayToIp { byte array[4]; uint32_t ip; };

//ArrayToIp server = { 102,1,168,192 };
ArrayToIp server = { 168,11,170,107 };

char message_buff[50];
const int relayPin = 6;

void callback (char* topic, byte* payload, unsigned int length) {
   int i = 0;

  Serial.println("Message arrived:  topic: " + String(topic));
  Serial.println("Length: " + String(length,DEC));
  
  for(i=0; i<length; i++) { message_buff[i] = payload[i]; }
  message_buff[i] = '\0';
  
  String msgString = String(message_buff);
  Serial.println("Payload: " + msgString);    
  
  
}
cc3000_PubSubClient mqttclient(server.ip, 1883, callback, client, cc3000);

DHT dht(DHTPIN, DHTTYPE);

void setup(void)
{
  Serial.begin(115200);
  
  pinMode(relayPin, OUTPUT);
  
  Serial.println(F("Initialising the CC3000 ..."));
  if (!cc3000.begin()) {  Serial.println(F("Unable to initialise the CC3000! Check your wiring?")); while(1); }
 
  Serial.println(F("Deleting old connection profiles"));
  if (!cc3000.deleteProfiles()) {  Serial.println(F("Failed!"));  while(1);  }

  // Attempt to connect to an access point Max 32 chars 
  char *ssid = WLAN_SSID; 
  Serial.println(F("Attempting to connect to ")); Serial.println(ssid);
  
  // NOTE: Secure connections are not available in 'Tiny' mode!
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) { Serial.println(F("Failed!")); while(1); }
   
  Serial.println(F("Connected!"));
  
  // Wait for DHCP to complete
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) { delay(100);}
   
  dht.begin(); 
}

void loop(void) {
   
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  char tempBuffer[50];
  String temp = dtostrf(t, 5, 2, tempBuffer);
  
  char humidityBuffer[50];
  String humidity = dtostrf(h, 5, 2, humidityBuffer);
  
  Serial.println("Humidity: "+humidity+" Temperature: "+temp); 
  
 String humidityJson = "{\"value\": \""+humidity+"\"}";
 humidityJson.toCharArray(humidityBuffer, 50);
 
 String temperatureJson = "{\"value\": \""+temp+"\"}";
 temperatureJson.toCharArray(tempBuffer, 50);
    
  // are we still connected?
  if (!client.connected()) {
     Serial.println(F("trying to connect to the broker..."));
     client = cc3000.connectTCP(server.ip, 1883);
     
     if(client.connected()) {
       Serial.println(F("connected to the broker!"));
       if (mqttclient.connect(DEVICE_ID)) { 
         mqttclient.subscribe("/arduino_uno/callback");
       }
     } 
  } else {
    // yep, publish that test 
    mqttclient.publish("/arduino_uno/temperature", tempBuffer);
    mqttclient.publish("/arduino_uno/humidity", humidityBuffer);
  }

  mqttclient.loop();
  delay(10000);
}

