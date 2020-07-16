#!/usr/bin/env bash

# provide MQTT server as 1st argument
# use the following format:
# $ run_timed_rx_file_mqtt "tcp://mqtt.example.com:1883"

./timed_rx_file_mqtt --usrpargs="addr=192.168.10.3" --mqttserv=$1 --id=$(hostname) --prefix="$HOME/Workspace/data/exp0_" --pubtop="usrp/response" --subtop="usrp/command" --ntpslack=0.2
