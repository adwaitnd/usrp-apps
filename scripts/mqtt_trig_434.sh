#!/usr/bin/env sh
mqttserv=$1
pubtop="usrp/command"

# fc=%lf,lo=%lf,sps=%lf,bw=%lf,g=%lf,t0=%lf,n=%zu,ant=%[^,]
tnow=$(date +%s)
fc="433.75e6"
lo="0"
sps="1e6"
ifbw="5e6"
gain="10"
trequest=$((tnow+5))
nsamp="5000000"
ant="RX2"
mosquitto_pub -h $mqttserv -t $pubtop -m "fc=$fc,lo=$lo,sps=$sps,bw=$ifbw,g=$gain,t0=$trequest,n=$nsamp,ant=$ant"
