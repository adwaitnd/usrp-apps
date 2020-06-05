# compile and install paho.mqtt.c

Builds the paho C static library and install its to a local `~/install` folder.

Note: using a personal fork of the PAHO C and CPP libraries (we were facing some problems with compiling examples for the non-SSL versions earlier)

    git clone https://github.com/adwaitnd/paho.mqtt.c.git
    mkdir paho.mqtt.c/build
    cd paho.mqtt.c/build
    cmake -DPAHO_BUILD_STATIC=TRUE -DPAHO_BUILD_SHARED=FALSE -DPAHO_WITH_SSL=FALSE ..
    make
    make DESTDIR=~/install

# compile and install paho.mqtt.cpp

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
    make DESTDIR=~/install

# compile and install UHD


# VSCode CMake Tools configurations

If you want to use the Cmake Tools extension for building and debugging,
add the following entry in your `.vscode/settings.json` file

    "cmake.configureSettings": {
            "CMAKE_PREFIX_PATH":"~/install/usr/local"
        }
