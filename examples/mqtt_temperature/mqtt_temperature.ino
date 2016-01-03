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

#define WLAN_SSID       "XXX-XXX-XXX"
#define WLAN_PASS       "xxxxxxxxx"
#define DEVICE_ID       "arduino_uno" 

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11 

Adafruit_CC3000_Client client;
   
// We're going to set our broker IP and union it to something we can use
union ArrayToIp { byte array[4]; uint32_t ip; };

ArrayToIp server = { 5,1,168,192 };
//ArrayToIp server = { 227,182,241,192 };

char message_buff[100];
char tempBuffer[10];


void callback (char* topic, byte* payload, unsigned int length) { }
cc3000_PubSubClient mqttclient(server.ip, 1883, callback, client, cc3000);

DHT dht(DHTPIN, DHTTYPE);

void setup(void)
{
  Serial.begin(115200);
   
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
   
 float t = dht.readTemperature(); 
 String temp = dtostrf(t, 5, 2, tempBuffer);
  
String messageJson = "{\"sensors\":[{\"value\":\""+temp+"\",\"tag\":\"temperature\"}]}"; 
 messageJson.toCharArray(message_buff, 100);

 
 Serial.println("message to send: "+messageJson); 
  // are we still connected?
  if (!client.connected()) {
     Serial.println(F("trying to connect to the broker..."));
     client = cc3000.connectTCP(server.ip, 1883);
     
     if(client.connected()) {
       Serial.println(F("connected to the broker!"));
       if (mqttclient.connect(DEVICE_ID)) { 
         Serial.println(F("connected to the mqtt broker!"));
       }
     } 
  } else {
    // yep, publish that test  
    mqttclient.publish("/device/arduino_uno/key/xxxxxx", message_buff); 
  }

  mqttclient.loop();
  delay(5000);
}


