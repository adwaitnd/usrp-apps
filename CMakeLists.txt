cmake_minimum_required(VERSION 3.5.1)
project(uhd-compile)

##### Boost libraries #####
set(UHD_BOOST_REQUIRED_COMPONENTS
    chrono
    date_time
    filesystem
    program_options
    regex
    system
    unit_test_framework
    serialization
    thread
)
find_package(Boost REQUIRED COMPONENTS ${UHD_BOOST_REQUIRED_COMPONENTS})
# Boost_INCLUDE_DIRS and Boost_LIBRARIES would be automatically populated when the Boost package is found

##### Paho MQTT libraries #####
find_path(paho_c_INCLUDE_DIRS NAMES mqtt) # find where the MQTT C includes are located (they follow a mqtt/xyz.h naming convention)
find_library(paho_c_async NAMES paho-mqtt3a) # search for asynchronous paho.mqtt.c lib
find_library(paho_c_sync NAMES paho-mqtt3c) # search for synchronous paho.mqtt.c lib
find_library(paho_c_async_ssl NAMES paho-mqtt3as) # search for asynchronous paho.mqtt.c lib with SSL
find_library(paho_c_sync_ssl NAMES paho-mqtt3cs) # search for asynchronous paho.mqtt.c lib with SSL

find_path(paho_cpp_INCLUDE_DIRS NAMES MQTTAsync.h) # find where the MQTT CPP heade files are located
find_library(paho_cpp NAMES paho-mqttpp3) # search for location of paho.mqtt.cpp

message(STATUS "[debug] MQTT C headers dir: ${paho_c_INCLUDE_DIRS}")
message(STATUS "[debug] MQTT C async library found: ${paho_c_async}")
message(STATUS "[debug] MQTT C sync library found: ${paho_c_sync}")
message(STATUS "[debug] MQTT C async library with SSL found: ${paho_c_async_ssl}")
message(STATUS "[debug] MQTT C sync library with SSL found: ${paho_c_sync_ssl}")
message(STATUS "[debug] MQTT CPP headers dir: ${paho_cpp_INCLUDE_DIRS}")
message(STATUS "[debug] MQTT CPP library used: ${paho_cpp}")

##### UHD libraries #####
find_library(uhd_lib NAMES uhd)
find_path(uhd_include NAMES uhd.h) # find where the UHD CPP header files are located

message(STATUS "[debug] uhd library: ${uhd_lib}")
message(STATUS "[debug] uhd headers dir: ${uhd_include}")

# read USRP and system time
add_executable(read_usrp_time apps/read_usrp_time.cpp)
target_include_directories(read_usrp_time PRIVATE ${uhd_include} ${Boost_INCLUDE_DIRS})
target_link_libraries(read_usrp_time ${uhd_lib} ${Boost_LIBRARIES})

# reset USRP time to zero
add_executable(reset_usrp_time apps/reset_usrp_time.cpp)
target_include_directories(reset_usrp_time PRIVATE ${uhd_include} ${Boost_INCLUDE_DIRS})
target_link_libraries(reset_usrp_time ${uhd_lib} ${Boost_LIBRARIES})

# using queues MQTT pubsub application with independent processing thread
add_executable(mqtt_combined_pubsub apps/mqtt_combined_pubsub.cpp)
target_include_directories(mqtt_combined_pubsub PRIVATE ${paho_c_INCLUDE_DIRS} ${paho_cpp_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} apps)
target_link_libraries(mqtt_combined_pubsub ${paho_cpp} ${paho_c_async} ${Boost_LIBRARIES} pthread)

# simple MQTT publishing example from paho examples
add_executable(topic_publish apps/topic_publish.cpp)
target_include_directories(topic_publish PRIVATE ${paho_c_INCLUDE_DIRS} ${paho_cpp_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} apps)
target_link_libraries(topic_publish ${paho_cpp} ${paho_c_async} ${Boost_LIBRARIES} pthread)

# record samples from GPS/PPS synced USRP at precise time in future
add_executable(rx_timed_samples_to_file apps/rx_timed_samples_to_file.cpp)
target_include_directories(rx_timed_samples_to_file PRIVATE ${uhd_include} ${Boost_INCLUDE_DIRS} apps)
target_link_libraries(rx_timed_samples_to_file ${uhd_lib} ${Boost_LIBRARIES})
add_custom_command(TARGET rx_timed_samples_to_file POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/apps/run_rx_timed_samples_to_file.sh
                    ${CMAKE_CURRENT_BINARY_DIR}/run_rx_timed_samples_to_file.sh)

# USRP record to file on receiving trigger/instructions over MQTT
add_executable(timed_rx_file_mqtt apps/timed_rx_file_mqtt/main.cpp apps/timed_rx_file_mqtt/mqtt_ops.cpp apps/timed_rx_file_mqtt/usrp_ops.cpp)
target_include_directories(timed_rx_file_mqtt PRIVATE ${uhd_include} ${paho_c_INCLUDE_DIRS} ${paho_cpp_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} apps/timed_rx_file_mqtt)
target_link_libraries(timed_rx_file_mqtt ${uhd_lib} ${paho_cpp} ${paho_c_async} ${Boost_LIBRARIES} pthread)
add_custom_command(TARGET timed_rx_file_mqtt POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/apps/timed_rx_file_mqtt/run_timed_rx_file_mqtt.sh
                    ${CMAKE_CURRENT_BINARY_DIR}/run_timed_rx_file_mqtt.sh)