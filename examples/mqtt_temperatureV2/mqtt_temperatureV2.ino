#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <DHT.h>
#include <PubSubClient.h>
 
#define aref_voltage 3.3
 
// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
 
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
 
// Use hardware SPI for the remaining pins On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);
 
#define WLAN_SSID       "WiFi-Arnet-s7fr"
#define WLAN_PASS       "6z6n2sk9rc"
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11 

char humidityBuffer[50];
char tempBuffer[50];
 
Adafruit_CC3000_Client client = Adafruit_CC3000_Client();
PubSubClient mqttclient("app.connectingthings.io", 1883, callback, client);
 
void callback (char* topic, byte* payload, unsigned int length) {
  Serial.println(topic);
  Serial.write(payload, length);
  Serial.println("");
}

DHT dht(DHTPIN, DHTTYPE);

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n"));
 
  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);
  
  Serial.println(F("\nInitialising the CC3000 ..."));  if (!cc3000.begin()) { Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));  for(;;); }
 
  uint16_t firmware = checkFirmwareVersion();
  if (firmware < 0x113) {  Serial.println(F("Wrong firmware version!")); for(;;);  }
  
  displayMACAddress();
  
  Serial.println(F("\nDeleting old connection profiles"));
  if (!cc3000.deleteProfiles()) {  Serial.println(F("Failed!")); while(1); }

   /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);

  checkAndConnectToMQTT();
}
 
void loop(void) {

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  char tempBuffer[50];
  String temp = dtostrf(t, 5, 2, tempBuffer);
  
  char humidityBuffer[50];
  String humidity = dtostrf(h, 5, 2, humidityBuffer);
 
  String humidityJson = "{\"value\": \""+humidity+"\",\"tag\": \"humidity\"}";
  humidityJson.toCharArray(humidityBuffer, 50);
 
  String temperatureJson = "{\"value\": \""+temp+"\",\"tag\": \"temperature\"}";
  temperatureJson.toCharArray(tempBuffer, 50);
    
  // are we still connected?
  checkAndConnectToMQTT();
      
  Serial.println("Temperature: "+temp); 
  mqttclient.publish("/device/arduino_uno/key/1qaz2wsx",tempBuffer);
  delay(5000);
  Serial.println("Humidity: "+humidity); 
  mqttclient.publish("/device/arduino_uno/key/1qaz2wsx",humidityBuffer);
    
  mqttclient.loop();
  delay(5000);
}

/**************************************************************************/
/*!
    @brief  Tries to connect the CC3000's to the mqtt broker
*/
/**************************************************************************/
void checkAndConnectToMQTT(void)
{
  while (!client.connected()) {

    Serial.print("Attempting Inernet connection...");
    client = cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);

    if(client.connected()) {
      Serial.println(F("Connected!"));
    
      /* Wait for DHCP to complete */
      Serial.println(F("Request DHCP"));
      while (!cc3000.checkDHCP()) {  delay(100); }
   
      /* Display the IP address DNS, Gateway, etc. */  
      while (!displayConnectionDetails()) {  delay(1000); }
    
      // connect to the broker, and subscribe to a path
      if (mqttclient.connect("arduino_uno")) {
        Serial.println(F("MQTT Connected"));
        // mqttclient.subscribe("/device/arduino_uno/key/1qaz2wsx");      
      }
   } else {      
      Serial.print("failed, Internet connection");
      Serial.println(" trying again in 5 seconds");
      delay(5000);
   }
  }
}

 
/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;
  
#ifndef CC3000_TINY_DRIVER  
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}
 
/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];
  
  if(!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}
 
 
/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
