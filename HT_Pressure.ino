#include <ESP8266WiFi.h>
#include <WEMOS_SHT3X.h>
#include <LOLIN_HP303B.h>
#include <PubSubClient.h>

//This sketch is to publish Temperature and Humidity from a SHT30 to an MQTT Server. This also publishes so Home Assistant's MQTT Auto Discover will work well.
//This sketch also publishes for pressure it reads from an HP303B. It publishes in a seperate topic (Though you could probably publish all 3 readings together)
//For this sketch to work correctly the PubSubClient.h must be modified. Change "#define MQTT_MAX_PACKET_SIZE 128" to "#define MQTT_MAX_PACKET_SIZE 256"

WiFiClient espClient;
PubSubClient client(espClient);
String MacAddress = WiFi.macAddress();
char EndMac[5];
char* roomName = "Bedroom"; //Dont make this longer than 25 chars
char* pressureName = "House";
const char* ssid = "Wifi SSID";
const char* password = "WIFI PASSWORD";
const char* mqttuser = "MQTT USER";
const char* mqttpassword = "MQTT PASS";
const int sleepTimeS = 300;
SHT3X sht30(0x45);
float curTemp;
float curHumidity;
char stateTopic[48]; //22 without room name
char pressureStateTopic[48];
char* mqttserver = "MQTT SERVER";
char MqttSend[50];
LOLIN_HP303B HP303BPressureSensor;


struct {
  char data[4];
}rtcData;


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    //clientId += String(random(0xffff), HEX);
    String MacAddress = WiFi.macAddress();
    clientId += MacAddress.charAt(12);
    clientId += MacAddress.charAt(13);
    clientId += MacAddress.charAt(15);
    clientId += MacAddress.charAt(16);
    Serial.print(" with ClientID: " + clientId + "....");
     // Attempt to connect
    if (client.connect((char*)clientId.c_str(),mqttuser,mqttpassword)) {
      Serial.println("connected");
      client.loop();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 100 milliseconds before retrying
      delay(100);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println();
  

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    //Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  strcpy(stateTopic, "ha/sensor/sensor");
  strcat(stateTopic, roomName);
  strcat(stateTopic, "/state");
  strcpy(pressureStateTopic, "ha/sensor/sensor");
  strcat(pressureStateTopic, pressureName);
  strcat(pressureStateTopic, "_press/state");
  EndMac[0] = MacAddress.charAt(12);
  EndMac[1] = MacAddress.charAt(13);
  EndMac[2] = MacAddress.charAt(15);
  EndMac[3] = MacAddress.charAt(16);
  EndMac[4] = '\0';
  Serial.print("EndMac is currently: ");
  Serial.println(EndMac);
  client.setServer(mqttserver,1883);
  HP303BPressureSensor.begin();

  if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
    if(rtcData.data[0] == EndMac[0] && rtcData.data[1] == EndMac[1] && rtcData.data[2] == EndMac[2] && rtcData.data[3] == EndMac[3]) {
      Serial.println("We've been booted before.");
    } else {
      //delay(10000);
      Serial.println("We've not been booted before.");
      rtcData.data[0] = EndMac[0];
      rtcData.data[1] = EndMac[1];
      rtcData.data[2] = EndMac[2];
      rtcData.data[3] = EndMac[3];
      if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
        Serial.println("Successfully wrote RTC Memory.");
        delay(10000);
        // Need to connect to MQTT here and publish configurations so Home Assistant will start watching this device.
        char tempConfigTopic[51] = "ha/sensor/sensor";
        strcat(tempConfigTopic, roomName);
        strcat(tempConfigTopic, "T/config");
        char humidityConfigTopic[51] = "ha/sensor/sensor";
        strcat(humidityConfigTopic, roomName);
        strcat(humidityConfigTopic, "H/config");
        char pressureConfigTopic[51] = "ha/sensor/sensor";
        strcat(pressureConfigTopic, pressureName);
        strcat(pressureConfigTopic, "P/config");
        if (!client.connected()) {
          reconnect();
        }
        char configHAMqtt[220];
        strcpy(configHAMqtt,"{\"dev_cla\": \"temperature\", \"name\": \"");
        strcat(configHAMqtt, roomName);
        strcat(configHAMqtt, "_temp\", \"stat_t\": \"");
        strcat(configHAMqtt, stateTopic);
        strcat(configHAMqtt, "\", \"unit_of_meas\": \"°F\", \"val_tpl\": \"{{ value_json.temperature}}\" }");
        Serial.print("Publishing to ");
        Serial.print(tempConfigTopic);
        Serial.print(" value: ");
        Serial.println(configHAMqtt);
        if(client.publish(tempConfigTopic, configHAMqtt, true)) {
          Serial.println("Successfully Published.");
          //We have to do this to force PubSub to send data
          client.disconnect();
          espClient.flush();
          while( client.state() != -1 ) {
            delay(10);
          } 
        } else {
          Serial.println("Publish Failed....");
        }
        if (!client.connected()) {
          reconnect();
        }
        strcpy(configHAMqtt,"{\"dev_cla\": \"humidity\", \"name\": \"");
        strcat(configHAMqtt,roomName);
        strcat(configHAMqtt, "_humidity\", \"stat_t\": \"");
        strcat(configHAMqtt,stateTopic);
        strcat(configHAMqtt, "\", \"unit_of_meas\": \"%\", \"val_tpl\": \"{{ value_json.humidity}}\" }");
        Serial.print("Publishing to ");
        Serial.print(humidityConfigTopic);
        Serial.print(" value: ");
        Serial.println(configHAMqtt);        
        if(client.publish(humidityConfigTopic, configHAMqtt, true)) {
          Serial.println("Successfully Published.");
          client.disconnect();
          espClient.flush();
          while( client.state() != -1 ) {
            delay(10);
          }
        } else {
          Serial.println("Publish Failed");
        }

        if (!client.connected()) {
          reconnect();
        }
        strcpy(configHAMqtt,"{\"dev_cla\": \"pressure\", \"name\": \"");
        strcat(configHAMqtt,pressureName);
        strcat(configHAMqtt, "_pressure\", \"stat_t\": \"");
        strcat(configHAMqtt,pressureStateTopic);
        strcat(configHAMqtt, "\", \"unit_of_meas\": \"hPa\", \"val_tpl\": \"{{ value_json.pressure}}\" }");
        Serial.print("Publishing to ");
        Serial.print(pressureConfigTopic);
        Serial.print(" value: ");
        Serial.println(configHAMqtt);        
        if(client.publish(pressureConfigTopic, configHAMqtt, true)) {
          Serial.println("Successfully Published.");
          client.disconnect();
          espClient.flush();
          while( client.state() != -1 ) {
            delay(10);
          }
        } else {
          Serial.println("Publish Failed");
        }
        
      }
    }
  } else {
    Serial.println("Error reading from RTC Memory.");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(sht30.get()==0){
    if (!client.connected()) {
      reconnect();
    }
    Serial.print("Temperature in Fahrenheit : ");
    curTemp = sht30.fTemp;
    Serial.println(curTemp);
    Serial.print("Relative Humidity : ");
    curHumidity = sht30.humidity;
    Serial.println(curHumidity);
    strcpy(MqttSend, "{\"temperature\":");
    char tempMqtt[7];
    dtostrf(curTemp, 6, 2, tempMqtt);
    strcat(MqttSend, tempMqtt);
    strcat(MqttSend, ",\"humidity\":");
    dtostrf(curHumidity, 6, 2, tempMqtt);
    strcat(MqttSend, tempMqtt);
    strcat(MqttSend, "}");
    client.publish(stateTopic, MqttSend);
    client.disconnect();
    espClient.flush();
    while( client.state() != -1 ) {
      delay(10);
    }
    
  }
  if (!client.connected()) {
      reconnect();
  }
  int32_t pressure;
  int16_t ret;
  ret = HP303BPressureSensor.measurePressureOnce(pressure, 4);
  if (ret != 0)
  {
    //Something went wrong.
    //Look at the library code for more information about return codes
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" Pascal");
    float hpa;
    hpa = pressure / 100.0;
    Serial.print("Pressure in hpa: ");
    Serial.println(hpa);
    char tempMqtt[8];
    dtostrf(hpa, 7, 2, tempMqtt);
    strcpy(MqttSend, "{\"pressure\": ");
    strcat(MqttSend, tempMqtt);
    strcat(MqttSend,"}");
    client.publish(pressureStateTopic, MqttSend);
    client.disconnect();
    espClient.flush();
    while( client.state() != -1 ) {
      delay(10);
    }
    
  }
  ESP.deepSleep(sleepTimeS*1000000, WAKE_RF_DEFAULT); // Sleep for 300 seconds
}
