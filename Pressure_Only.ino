#include <ESP8266WiFi.h>
#include <LOLIN_HP303B.h>
#include <PubSubClient.h>


//This sketch also publishes for pressure and temperature it reads from an HP303B. It supports Home Assistant's MQTT Auto Discover
//This sketch does not go into deep sleep. Instead it waits 14.5 seconds at the end of each loop
//For this sketch to work correctly the PubSubClient.h must be modified. Change "#define MQTT_MAX_PACKET_SIZE 128" to "#define MQTT_MAX_PACKET_SIZE 256"


WiFiClient espClient;
PubSubClient client(espClient);
String MacAddress = WiFi.macAddress();
char EndMac[5];
char* pressureName = "HVAC";
const char* ssid = "WIFI SSID";
const char* password = "WIFI PASSWORD";
const char* mqttuser = "MQTT USER";
const char* mqttpassword = "MQTT PASSWORD";
//const int sleepTimeS = 60;
float curTemp;
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
  strcpy(pressureStateTopic, "ha/sensor/sensor");
  strcat(pressureStateTopic, pressureName);
  strcat(pressureStateTopic, "/state");
  EndMac[0] = MacAddress.charAt(12);
  EndMac[1] = MacAddress.charAt(13);
  EndMac[2] = MacAddress.charAt(15);
  EndMac[3] = MacAddress.charAt(16);
  EndMac[4] = '\0';
  Serial.print("EndMac is currently: ");
  Serial.println(EndMac);
  client.setServer(mqttserver,1883);
  HP303BPressureSensor.begin();
  delay(10000);
    // Need to connect to MQTT here and publish configurations so Home Assistant will start watching this device.
    char tempConfigTopic[51] = "ha/sensor/sensor";
    strcat(tempConfigTopic, pressureName);
    strcat(tempConfigTopic, "T/config");
    char pressureConfigTopic[51] = "ha/sensor/sensor";
    strcat(pressureConfigTopic, pressureName);
    strcat(pressureConfigTopic, "P/config");
    if (!client.connected()) {
      reconnect();
    }
    char configHAMqtt[220];
    strcpy(configHAMqtt,"{\"dev_cla\": \"temperature\", \"name\": \"");
    strcat(configHAMqtt, pressureName);
    strcat(configHAMqtt, "_temp\", \"stat_t\": \"");
    strcat(configHAMqtt, pressureStateTopic);
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


void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
      reconnect();
  }
  int32_t pressure;
  int32_t temperature;
  float fahrenheit;
  int16_t ret;
  ret = HP303BPressureSensor.measurePressureOnce(pressure, 7);
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
    //I need to check temperature now.
    ret = HP303BPressureSensor.measureTempOnce(temperature,7);
    if (ret != 0)
    {
      //Something went wrong.
      //Look at the library code for more information about return codes
      Serial.print("FAIL! ret = ");
      Serial.println(ret);
    } else {
      fahrenheit = (temperature * 1.8 ) + 32;
      strcat(MqttSend, ",\"temperature\": ");
      dtostrf(fahrenheit, 7, 2, tempMqtt);
      Serial.print("Temperature: ");
      Serial.print(tempMqtt);
      Serial.println(" F");
      strcat(MqttSend, tempMqtt);
      strcat(MqttSend, "}");
      client.publish(pressureStateTopic, MqttSend);
    }
    
   }
    
  delay(14500);
  //ESP.deepSleep(sleepTimeS*1000000, WAKE_RF_DEFAULT); // Sleep for 300 seconds
}
