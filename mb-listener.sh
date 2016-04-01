#!/bin/bash

PATH_TO_FILE='/root/shine/input.config'
SOURCE_FILE='mb-listener.c'
EXE_FILE='mb-listener.elf'
#Remote data
remoteHost=$(awk '/^mqtt.remote.host/{print $NF}' $PATH_TO_FILE)
clientId=$(awk '/^generic.node-id/{print $NF}' $PATH_TO_FILE)
ledSubTopic=$(awk '/^mqtt.remote.ledSubTopic/{print $NF}' $PATH_TO_FILE)
remotePort=$(awk '/^mqtt.remote.port/{print $NF}' $PATH_TO_FILE)

#Local data
localHost=$(awk '/^mqtt.local.host/{print $NF}' $PATH_TO_FILE)
clientId=$(awk '/^generic.node-id/{print $NF}' $PATH_TO_FILE)
ledPubTopic=$(awk '/^mqtt.local.pubTopic/{print $NF}' $PATH_TO_FILE)
localPort=$(awk '/^mqtt.local.port /{print $NF}' $PATH_TO_FILE)


#val=$( sed -ne '/mqtt.remote.ledSubTopic/ s/.*\t *//p' $PATH_TO_FILE )

#Saving data in input_sub file
echo $remoteHost > input_sub.config
echo $clientId >> input_sub.config
echo $ledSubTopic >> input_sub.config
echo $remotePort >> input_sub.config

#Saving data n input_pub file
echo $localHost > input_pub.config
echo $clientId >> input_pub.config
echo $ledPubTopic"_2" >> input_pub.config
echo $localPort >> input_pub.config

if [ -e $EXE_FILE ]
then 
	./$EXE_FILE
elif [ -e $SOURCE_FILE ]
then
	gcc $SOURCE_FILE -o $EXE_FILE -l wiringPi -l paho-mqtt3c
	./$EXE_FILE
else
	echo "[ERROR] File not found"
fi


