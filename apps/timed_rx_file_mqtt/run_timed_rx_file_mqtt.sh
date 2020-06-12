#!/usr/bin/env sh
./timed_rx_file_mqtt --usrpargs="addr=192.168.10.3" --mqttserv="tcp://spatial.andrew.cmu.edu:1883" --id=$(hostname) --prefix="$HOME/Workspace/data/exp0_" --pubtop="usrp/response" --subtop="usrp/command" --ntpslack=0.2
