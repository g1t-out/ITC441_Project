#!/usr/local/bin/python3
#The above location is python3 in the Home Assistant Container in Hassio

import paho.mqtt.client as mqtt
from datetime import datetime, timedelta
import json

mqttUser = "MQTT USER"
mqttPass = "MQTT PASSWORD"
mqttServer = "MQTT SERVER IP ADDRESS"
notificationTopic = "/home/notify/deadsensor"
haTopic = "ha/#"
notifytime = timedelta(hours=2)

###This dictionary has the config topic as the key and the state topic as the value. I do this so if a null is published to a config, I can remove the state value.
configTopics = {}
###This dictionary has the state topic as the key and the last time we heard from it as the value.
stateTopics = {}
####This dictionary has the state topic as hte key and if we've notified since we last heard from the topic.
notifyStateTopics = {}

def on_connect(client,userdata,flags,rc):
    #print("Connected with result code "+str(rc))
    client.subscribe(haTopic)


def on_message(client, userdata, msg):
    global configTopics
    global stateTopics
    global notifyStateTopics
    #print(msg.topic + " : " + msg.payload.decode("utf-8"))
    topicReceived = msg.topic
    if topicReceived.endswith("/config"):
        ###This is a config Topic
        #print("Found a config")
        if msg.payload.decode("utf-8") == "":
            ###We need to delete this topic.
            if topicReceived in configTopics:
                stateTopicToDelete = configTopics[topicReceived]
                del notifyStateTopics[stateTopicToDelete]
                del stateTopics[stateTopicToDelete]
                del configTopics[topicReceived]
                #print(f'Deleted Config Topic: {topicReceived}')
                #print(f'Deleted State Topic: {stateTopicToDelete}')
        else:
            #print("Converting JSON")
            decodedMessage = json.loads(msg.payload.decode("utf-8"))
            stateTopic = None
            if "stat_t" in decodedMessage:
                stateTopic = decodedMessage['stat_t']
               #print("Found stat_t value")
            elif "state_topic" in decodedMessage:
                stateTopic = decodedMessage['state_topic']
                #print("Found state_topic value")
            if stateTopic != None:
                configTopics[topicReceived] = stateTopic
                stateTopics[stateTopic] = datetime.now()
                notifyStateTopics[stateTopic] = False
                #print(f'Updated Config Topic: {topicReceived}')
                #print(f'State Topic: {stateTopic}')
    else:
        ###Assuming this is data on a state topic
        if topicReceived in stateTopics:
            stateTopics[topicReceived] = datetime.now()
            notifyStateTopics[topicReceived] = False
            #print(f'Received Data on a state topic: {topicReceived}')

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
    for Topic in stateTopics:
        if stateTopics[Topic] + notifytime < curtime:
            ### We haven't heard from this sensor in a while.
            if notifyStateTopics[Topic] == False:
                ### We need to notify someone here.
                #print(f'We have not seen {Topic} sensor in a while.')
                client.publish(notificationTopic, Topic + " last heard from at: " + stateTopics[Topic].strftime("%m/%d/%Y %H:%M:%S"))
                notifyStateTopics[Topic] = True
    
            