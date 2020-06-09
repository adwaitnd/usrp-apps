#!/usr/bin/env sh
tnow=$(date +%s)
tstart=$((tnow+10))
./rx_timed_samples_to_file --args="addr=192.168.10.3" --file="/home/adwait/Workspace/data/sampledata2.dat" --type=short --rate=1e6 --freq=904.75e6 --ant TX/RX --gain=10 --ref="external" --nsamps=100000 --t0=$tstart --stats