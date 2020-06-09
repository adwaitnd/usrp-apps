#!/usr/bin/env sh
mqttserv="spatial.andrew.cmu.edu"
pubtop="usrp/command"

# fc=%lf,lo=%lf,sps=%lf,bw=%lf,g=%lf,t0=%lf,n=%zu,ant=%[^,]
tnow=$(date +%s)
fc="904.75e6"
lo="0"
sps="1e6"
ifbw="5e6"
gain="10"
trequest=$((tnow+5))
nsamp="1000000"
ant="TX/RX"
mosquitto_pub -h $mqttserv -t $pubtop -m "fc=$fc,lo=$lo,sps=$sps,bw=$ifbw,g=$gain,t0=$trequest,n=$nsamp,ant=$ant"
