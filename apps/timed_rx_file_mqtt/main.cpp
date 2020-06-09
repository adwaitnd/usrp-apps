#include <iostream>
#include <string>
#include <thread>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include "protected_queue.hpp"
#include "ops_helper.hpp"

#define RUN_MQTT 1
#define RUN_USRP 1

namespace po = boost::program_options;

void testThread(struct UsrpParams* params,
                ProtectedQ<std::string> *toNetwork,
                ProtectedQ<std::string> *fromNetwork)
// void testThread(ProtectedQ<std::string> *toNetwork,
//                 ProtectedQ<std::string> *fromNetwork)
{
    
}

int main(int argc, char* argv[])
{
    std::string usrp_args, mqtt_serv, client_id, top_pub, top_sub, file_prefix, wirefmt, datafmt, subdev;
    size_t usrp_channel, samp_per_buf;
    double slack_time, ntpslack;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("usrpargs", po::value<std::string>(&usrp_args)->default_value(""), "multi uhd device address args")
        ("mqttserv", po::value<std::string>(&mqtt_serv)->default_value("tcp://localhost:1883"), "mqtt server to connect")
        ("id", po::value<std::string>(&client_id)->default_value("tester"), "own ID used on MQTT and filenames")
        ("pubtop", po::value<std::string>(&top_pub)->default_value("response"), "topic to send responses/updates to")
        ("subtop", po::value<std::string>(&top_sub)->default_value("command"), "topic to listen for triggers")
        ("prefix", po::value<std::string>(&file_prefix)->default_value(""), "prefix for save files. Could include location")
        ("subdev", po::value<std::string>(&subdev), "subdevice specification")
        ("channel", po::value<size_t>(&usrp_channel)->default_value(0), "which channel to use")
        ("slack", po::value<double>(&slack_time)->default_value(0.5), "additional slack for setup operations")
        ("ntpslack", po::value<double>(&ntpslack)->default_value(0.1), "slack allowed between NTP and GPS time")
        ("spb", po::value<size_t>(&samp_per_buf)->default_value(10000), "samples per buffer")
        ("wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8 or sc16)")
        ("datafmt", po::value<std::string>(&datafmt)->default_value("short"), "sample type: double, float, or short")
        ("int-n", "tune USRP with integer-N tuning")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
        std::cout << boost::format("timed RX with MQTT trigger %s") % desc << std::endl;
        std::cout << std::endl
                  << "This application saves USRP data at a known time as provided by a trigger over MQTT\n"
                  << std::endl;
        return ~0;
    }
    po::notify(vm);

    ProtectedQ<std::string> toNetwork;
    ProtectedQ<std::string> fromNetwork;

    // NOTE: USRP stuff runs in a different thread so has to be setup before
    // we perform blocking waits in the MQTT thread
    #if RUN_USRP==1

    struct UsrpParams usrp_global_params = {
        .args = usrp_args,
        .file_prefix = file_prefix,
        .client_id = client_id,
        .clkref = "external",
        .tslack = slack_time,
        .ntpslack = ntpslack,
        .channel = usrp_channel,
        .datafmt = datafmt,
        .wirefmt = wirefmt,
        .spb = samp_per_buf,
        .intn_flag = (vm.count("int-n") > 0),
        .null = false,
        .subdev_flag = (vm.count("subdev") > 0),
        .subdev = ((vm.count("subdev") > 0) ? subdev : "")
    };

    std::thread usrp_thread(&usrp_ops, &usrp_global_params, &toNetwork, &fromNetwork);
    // std::thread usrp_thread(&testThread, &toNetwork, &fromNetwork);

    #endif // RUN_USRP==1

    #if RUN_MQTT==1

    struct MqttParams mqtt_conn_params = {
        .server = mqtt_serv,
        .userid = client_id,
        .pubtopic = top_pub,
        .subtopic = top_sub
    };

    std::cout << "MQTT server: " << mqtt_serv << std::endl;
    std::cout << "Client/Dev ID: " << client_id << std::endl;
    std::cout << "publish topic: " << top_pub << std::endl;
    std::cout << "subscribe topic: " << top_sub << std::endl;

    mqtt_pubsub_ops(&mqtt_conn_params, &toNetwork, &fromNetwork);

    #endif // RUN_MQTT==1

    usrp_thread.join();

    return 0;
}