/*
 * This file helps share functions between various cpp files for
 * MQTT and USRP operation
 */

#ifndef OPS_HELPER_HPP
#define OPS_HELPER_HPP

#include <string>
#include "protected_queue.hpp"

/*
 * cnvenient struct to keep all the MQTT connection parameters
 */
struct MqttParams
{
    std::string server;
    std::string userid;
    std::string pubtopic;
    std::string subtopic;
};

/*
 * MQTT self-complete function that will pass received messages
 * to a protected queue and wait for messages on 
 * 
 */
// void mqtt_pubsub_ops(
// 					std::string server,
// 					std::string userid,
// 					std::string pubtopic,
// 					std::string subtopic,
// 					ProtectedQ<std::string> *toNetwork,
// 					ProtectedQ<std::string> *fromNetwork );

void mqtt_pubsub_ops(
					struct MqttParams *params,
					ProtectedQ<std::string> *toNetwork,
					ProtectedQ<std::string> *fromNetwork );

struct UsrpParams
{
    std::string args;
    std::string file_prefix;
    std::string client_id;
    std::string clkref;
    double tslack;
    double ntpslack;
    size_t channel;
    std::string datafmt;
    std::string wirefmt;
    size_t spb;
    bool intn_flag;
    bool null;
    bool subdev_flag;
    std::string subdev;
};

void usrp_ops(
            struct UsrpParams *params,
            ProtectedQ<std::string> *toNetwork,
            ProtectedQ<std::string> *fromNetwork);

#endif // OPS_HELPER_HPP