# A collection of useful USRP utilities

### `apps` directory:

- **read_usrp_time:** reading the USRP time
- **reset_usrp_time:** resetting USRP time to 0.0
- **rx_timed_samples_to_file:** recording samples to a file staring at a known time
- **timed_rx_file_mqtt:** recording samples to files based on a trigger over mqtt

more complicated application have sample scripts named `run_<name>.sh` to use them.

### `scripts` directory:

- **mqtt_trig.sh:** simple mosquitto_pub based script to send a trigger for `timed_rx_file_mqtt`

## Prerequisites

To compile the (important MQTT) application included, we need non-SSL versions of [Paho MQTT C](https://github.com/adwaitnd/paho.mqtt.c.git), [Paho MQTT C++](https://github.com/adwaitnd/paho.mqtt.cpp.git) and [UHD](https://github.com/WiseLabCMU/uhd.git) installed in some known locations (I install everything to `~/install` but it can be changed by changing the `DESTDIR` in make install and `CMAKE_PREFIX_PATH` in the cmake command. Note that `CMAKE_PREFIX_PATH` would have to point to  `<install dir>/usr/local`). The other non-MQTT applications only need UHD installed. 

### compile and install paho.mqtt.c

Builds the paho C static library and install its to a local `~/install` folder.

Note: using a personal fork of the PAHO C and CPP libraries (we were facing some problems with compiling examples for the non-SSL versions earlier)

    git clone https://github.com/adwaitnd/paho.mqtt.c.git
    mkdir paho.mqtt.c/build
    cd paho.mqtt.c/build
    cmake -DPAHO_BUILD_STATIC=TRUE -DPAHO_BUILD_SHARED=FALSE -DPAHO_WITH_SSL=FALSE ..
    make
    make DESTDIR=~/install install

### compile and install paho.mqtt.cpp

Builds the paho C++ static library and install it to a local `~/install` folder in the
home directory. If this is to be changed, make changes to the DESTDIR option in
make install.

We're relying on a paho.mqtt.c static library being installed at
`~/install/usr/local/`. If this is different, make the appropriate changes to
CMAKE_PREFIX_PATH option for cmake.

Note: using a personal for of the PAHO C and CPP libraries (we were facing some problems with compiling examples earlier)

    git clone https://github.com/adwaitnd/paho.mqtt.cpp.git
    mkdir paho.mqtt.cpp/build
    cd paho.mqtt.cpp/build
    cmake -DCMAKE_PREFIX_PATH=~/install/usr/local -DPAHO_BUILD_STATIC=TRUE -DPAHO_BUILD_SHARED=FALSE -DPAHO_WITH_SSL=FALSE ..
    make
    make DESTDIR=~/install install

### compile and install UHD

Build a custom port of the UHD library and installs it to a local `~/install` folder.

    git clone https://github.com/WiseLabCMU/uhd.git
    mkdir uhd/host/build
    cd uhd/host/build
    cmake ..
    make uhd
    make DESTDIR=~/install install

## Compiling

    git clone https://github.com/adwaitnd/usrp-apps.git
    mkdir usrp-apps/build
    cd usrp-apps/build
    cmake -DCMAKE_PREFIX_PATH=~/install/usr/local ..
    make

## Use

### Timed capture using trigger on MQTT

Example use of `timed_rx_file_mqtt` (Also found in `run_timed_rx_file_mqtt.sh`):

    timed_rx_file_mqtt --usrpargs="addr=192.168.10.3" --mqttserv="tcp://mqtt.example.com:1883" --id=$(hostname) --prefix="$HOME/Workspace/data/exp0_" --pubtop="usrp/response" --subtop="usrp/command" --ntpslack=0.2

- usrpargs: where to find the USRP device
- mqttserv: where to find the MQTT server. Include protocol and port
- id: name of the computer. used for messaging the MQTT server correctly
- prefix: filename prefix. Can include complete path location
- pubtop: what topic the gateway will send notifications about the request
- subtop: what topic the gateway will use to listen for commands
- ntpslack: how much worst-case offset do we assume between NTP time on gateway host and GPS time on the GPSDO

### Timed capture using offset from local time

TODO: instructions for `rx_timed_sampled_to file`. Look at `run_rx_timed_sampled_to file.sh` for a quick and dirty reference

### Triggering a capture

Message format to send on the MQTT trigger topic to start a capture:

    "fc=$fc,lo=$lo,sps=$sps,bw=$ifbw,g=$gain,t0=$trequest,n=$nsamp,ant=$ant"

- fc: center frequency of capture in Hz (double)
- lo: LO offset in Hz (double). generally 0
- sps: samples per second or sample rate (double)
- bw: bandwidth of intermediate frequency filter in Hz (double)
- g: gain of frontend in dB (double)
- t0: exact capture starting time in UTC (double)
- n: number of samples to capture (int)
- ant: antenna to use (string)

Check `scripts/mqtt_trig_*.sh` for examples on triggering a capture using mosquitto_pub.`

## Other

### VSCode CMake Tools configurations

If you want to use the Cmake Tools extension for building and debugging,
add the following entry in your `.vscode/settings.json` file

    "cmake.configureSettings": {
            "CMAKE_PREFIX_PATH":"~/install/usr/local"
        }


## Relevant repositories

- MBED code for transmit-only SX1262 client that is used for experiments [https://github.com/adwaitnd/sx1262-tx] or [https://os.mbed.com/users/adwaitnd/code/SX126X_TXonly/]
- USRP synchronous data capture application [https://github.com/adwaitnd/usrp-apps]
- Tools to analyze USRP LoRa captures and to work with ray tracing simulation outputs [https://github.com/adwaitnd/lpwan-sdr-analysis]