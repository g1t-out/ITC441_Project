#!/usr/local/bin/python3

import paho.mqtt.client as mqtt
from datetime import datetime, timedelta
import json

hvacTempTopic = "ha/sensor/sensorHVAC/state"
furnaceStateTopic = "/home/furnace/state"
furnacePeriodUpdateTopic = "/home/furnace/state/p"
updatefrequency = timedelta(seconds=30)
mqttUser = "MQTT User"
mqttPass = "MQTT PASSWORD"
mqttServer = "MQTT SERVER IP ADDRESS"
curtime = datetime.now()
nextUpdate = datetime.now()

FurnaceOn = False
LastFurnaceTemp = 0
LastFurnaceOnTime = datetime.now() - timedelta(days=1)



def on_connect(client,userdata,flags,rc):
    #print("Connected with result code "+str(rc))
    client.subscribe(hvacTempTopic)


def on_message(client, userdata, msg):
    global FurnaceOn
    global LastFurnaceTemp
    global LastFurnaceOnTime
    #print(msg.topic + " : " + msg.payload.decode("utf-8"))
    decodedMessage = json.loads(msg.payload.decode("utf-8"))
    curTemp = decodedMessage['temperature']
    if FurnaceOn:
        ###We are looking for the furnace to shut off.
        if LastFurnaceTemp - curTemp > 3.5 or curTemp < 90:
            ###We saw a large drop in temperature --- I bet the furnace shut off
            FurnaceOn = False
            client.publish(furnaceStateTopic,"off")
            LastFurnaceOnTime = datetime.now()
    else:
        ###We are looking for the furnace to turn on.
        if (LastFurnaceOnTime + timedelta(minutes=5) < datetime.now() and curTemp > 90 and curTemp - LastFurnaceTemp > 1.9):
            FurnaceOn = True
            client.publish(furnaceStateTopic,"on")

    LastFurnaceTemp = curTemp


client = mqtt.Client()
client.on_connect=on_connect
client.on_message=on_message
client.username_pw_set(mqttUser,password=mqttPass)
client.connect(mqttServer, 1883, 60)
#client.loop_start()

while True:
    result = client.loop()
    ###We need logic down here for periodically publishing updates
    curtime = datetime.now()
    if curtime > nextUpdate:
        nextUpdate = curtime + updatefrequency
        try:
            if FurnaceOn:
                client.publish(furnacePeriodUpdateTopic, "on")
            else:
                client.publish(furnacePeriodUpdateTopic, "off")
        except:
            print("Had trouble pushing periodic update to Furnace")
    
            