#!/usr/bin/env sh
mqttserv=$1
pubtop="usrp/command"

# fc=%lf,lo=%lf,sps=%lf,bw=%lf,g=%lf,t0=%lf,n=%zu,ant=%[^,]
tnow=$(date +%s)
fc="904.75e6"
lo="0"
sps="1e6"
ifbw="5e6"
gain="10"
trequest=$((tnow+5))
nsamp="5000000"
ant="TX/RX"

fc2="433.75e6"
lo2="0"
sps2="1e6"
ifbw2="5e6"
gain2="10"
trequest2=$((tnow+15))
nsamp2="5000000"
ant2="RX2"

mosquitto_pub -h $mqttserv -t $pubtop -m "fc=$fc,lo=$lo,sps=$sps,bw=$ifbw,g=$gain,t0=$trequest,n=$nsamp,ant=$ant"

mosquitto_pub -h $mqttserv -t $pubtop -m "fc=$fc2,lo=$lo2,sps=$sps2,bw=$ifbw2,g=$gain2,t0=$trequest2,n=$nsamp2,ant=$ant2"
