/* mqtt_pir
MIT license
written by Mariano Ravinale
*/

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <PubSubClient.h>
 
#define aref_voltage 3.3

#define ADAFRUIT_CC3000_IRQ   3  
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
 
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);
 
#define WLAN_SSID       "WiFi-SSID"
#define WLAN_PASS       "Password"
#define WLAN_SECURITY   WLAN_SEC_WPA2

char humidityBuffer[50];
char tempBuffer[50];
 
Adafruit_CC3000_Client client = Adafruit_CC3000_Client();
PubSubClient mqttclient("app.connectingthings.io", 1883, client);

int inputPin = 4;               // choose the input pin (for PIR sensor)
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;                    // variable for reading the pin status

void setup(void)
{
 Serial.begin(115200);
   
  Serial.println(F("Initialising the CC3000 ..."));
  if (!cc3000.begin()) {  Serial.println(F("Failed!")); while(1); }
 
  Serial.println(F("Deleting old connection profiles"));
  if (!cc3000.deleteProfiles()) {  Serial.println(F("Failed!"));  while(1);  }

  char *ssid = WLAN_SSID; 
  Serial.println(F("Attempting to connect to ")); Serial.println(ssid);
  
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) { Serial.println(F("Failed!")); while(1); }   
  Serial.println(F("Connected!"));
  
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) { delay(100);}
  displayConnectionDetails();
   
  pinMode(inputPin, INPUT);     // declare sensor as input 
}
 
void loop(void) {

  if (!mqttclient.connected()) {  reconnect(); }

  val = digitalRead(inputPin);  // read input value
  
  if (val == HIGH && pirState == LOW ) {     
    Serial.println("Motion detected!");     
    mqttclient.publish("/device/switch/key/xxxxxxx","{\"value\": \"1\",\"tag\": \"movement\"}");
    pirState = HIGH; 
    delay(10*1000); //10 seconds on hold   
  } else if (pirState == HIGH) {        
    Serial.println("Motion ended!"); 
    mqttclient.publish("/device/switch/key/xxxxxxx","{\"value\": \"0\",\"tag\": \"movement\"}");
    pirState = LOW;    
  }
 
  mqttclient.loop();
}

/**************************************************************************/
/*!
    @brief  Tries to reconnect the CC3000's to the mqtt broker
*/
/**************************************************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.println("Attempting MQTT connection...");   
    if (mqttclient.connect("arduinoClient")) {
      Serial.print("Connected!");       
    } else {
      Serial.print("Failed!");
      Serial.print(mqttclient.state());
      Serial.println("trying again in 5 seconds"); 
      delay(5000);
    }
  }
}
 
/**************************************************************************/
/*!
    @brief  Tries to read the IP address
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
    Serial.print(F("IP Address: ")); cc3000.printIPdotsRev(ipAddress); 
    Serial.println();
    return true;
  }
}
