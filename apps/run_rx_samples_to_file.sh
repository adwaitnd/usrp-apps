#!/usr/bin/env sh
./rx_timed_samples_to_file --args addr=192.168.10.3 --file ~/Desktop/sampledata.dat --type=float --rate 1e6 --freq 904.75e6 --ant TX/RX --gain 10 --ref external --duration 10
