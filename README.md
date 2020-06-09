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

## compiling applications in this repository

    git clone https://github.com/adwaitnd/usrp-apps.git
    mkdir usrp-apps/build
    cd usrp-apps/build
    cmake -DCMAKE_PREFIX_PATH=~/install/usr/local ..
    make

## VSCode CMake Tools configurations

If you want to use the Cmake Tools extension for building and debugging,
add the following entry in your `.vscode/settings.json` file

    "cmake.configureSettings": {
            "CMAKE_PREFIX_PATH":"~/install/usr/local"
        }
