# Experiment instructions

- Start `timed_rx_file_mqtt` on each relevant base station and make sure they are listening on the mqtt channel `usrp/command`
- Keep an MQTT subscriber running on channel `usrp/response` to listen for acks and responses from various basestations
- Run `./mqtt_trig_905.sh <mqtt addr (w/o) port/protocol>` to start a 5 sec 904.75 MHz capture
- Run `./mqtt_trig_434.sh <mqtt addr (w/o) port/protocol>` to start a 5 sec 433.75 MHz capture