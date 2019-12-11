#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
const char* ssid = "WIFI NAME";
const char* password = "WIFI PASS";
char* mqttserver = "MQTT SERVER";
//This is an early form of the Sketch. This only publishes temperature and humidity to seperate topics.


WiFiClient espClient;
PubSubClient client(espClient);
char* updateTempTopic = "/home/room2/temp";
char* updateHumidityTopic = "/home/room2/humidity";
char MqttSend[8];



SHT3X sht30(0x45);
const int sleepTimeS = 300;
float curTemp;
float curHumidity;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("I am the second one.");


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
  client.setServer(mqttserver,1883);
}


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
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.loop();
      if(sht30.get()==0){
        Serial.print("Temperature in Fahrenheit : ");
        curTemp = sht30.fTemp;
        Serial.println(curTemp);
        dtostrf(curTemp, 6, 2, MqttSend);
        client.publish(updateTempTopic, MqttSend);
        Serial.print("Relative Humidity : ");
        curHumidity = sht30.humidity;
        Serial.println(curHumidity);
        dtostrf(curHumidity, 6, 2, MqttSend);
        client.publish(updateHumidityTopic, MqttSend);
      } else {
        Serial.println("Error!");
      }

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 100 milliseconds before retrying
      delay(100);
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
    Serial.println("Deep Sleep for 300 seconds.");
    Serial.println();
    client.disconnect(); //We disconnect so it will finish sending data with the publish
    //delay(100);
    ESP.deepSleep(sleepTimeS*1000000, WAKE_RF_DEFAULT); // Sleep for 60 seconds
    //delay(60000);


}
