#!/usr/bin/env sh
./timed_rx_file_mqtt --usrpargs="addr=192.168.10.3" --id="griffin" --prefix="$HOME/Workspace/data/exp0_" --pubtop="usrp/response" --subtop="usrp/command"