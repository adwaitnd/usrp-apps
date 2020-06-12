/*
 *  MQTT thread function that listens for messages over a subscribed topic
 *  and publishes responses to another topic. It produces and consumes strings
 *  that are passed using two protected queues.
 * 
 *  All MQTT operations will sit here. Other programs may use this
 *  component but using the mqtt_pubsub_ops function.
 *  - Adwait D
 */

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <chrono>
#include <boost/format.hpp>
#include "mqtt/async_client.h"
#include "protected_queue.hpp"
#include "ops_helper.hpp"

const int	QOS = 1;
const int	N_RETRY_ATTEMPTS = 5;
const bool  NO_LOCAL = true;

/////////////////////////////////////////////////////////////////////////////

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */
class CallbackHelper : public virtual mqtt::callback,
					public virtual mqtt::iaction_listener

{
	// Counter for the number of connection retries
	int nretry_;
	// The MQTT client
	mqtt::async_client& cli_;
	// Options to use if we need to reconnect
	mqtt::connect_options& connOpts_;

	std::string pubTopic;
	std::string subTopic;
	ProtectedQ<std::string> *fromNetQ;

	// This deomonstrates manually reconnecting to the broker by calling
	// connect() again. This is a possibility for an application that keeps
	// a copy of it's original connect_options, or if the app wants to
	// reconnect with different options.
	// Another way this can be done manually, if using the same options, is
	// to just call the async_client::reconnect() method.
	void reconnect() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		try {
			cli_.connect(connOpts_, nullptr, *this);
		}
		catch (const mqtt::exception& exc) {
			std::cerr << "[MQTTError] " << exc.what() << std::endl;
			exit(1);
		}
	}

	// Re-connection failure
	void on_failure(const mqtt::token& tok) override {
		std::cout << "[MQTTdebug] Connection attempt failed" << std::endl;
		if (++nretry_ > N_RETRY_ATTEMPTS)
			exit(1);
		else
			reconnect();
	}

	// (Re)connection success
	// Either this or connected() can be used for callbacks.
	void on_success(const mqtt::token& tok) override {}

	// (Re)connection success
	void connected(const std::string& cause) override {
		std::cout << "\n[MQTTdebug] Connection success" << std::endl;
		std::string on_msg = (boost::format("<<<%s connected>>>") % cli_.get_client_id()).str();
		cli_.publish(pubTopic, on_msg.c_str());
		// // new but undocumented method found in the mqttpp_chat sample
		// mqtt::topic topic { cli_, subTopic, QOS };
		// auto subOpts = mqtt::subscribe_options(NO_LOCAL);
		// topic.subscribe(subOpts);
		
		std::cout << "[MQTTdebug] Subscribing to topic " << subTopic << std::endl;
		// method in the examples and documentation
		cli_.subscribe(subTopic, QOS);
	}

	// Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override {
		std::cout << "\n[MQTTdebug] Connection lost" << std::endl;
		if (!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;

		std::cout << "[MQTTdebug] Reconnecting..." << std::endl;
		nretry_ = 0;
		reconnect();
	}

	// Note: commented for testing
	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override
	{
		std::cout << "[MQTTdebug] Message arrived" << std::endl;
		fromNetQ->addItem(msg->to_string());
	}

	void delivery_complete(mqtt::delivery_token_ptr token) override
	{

	}

public:
	CallbackHelper(
		mqtt::async_client& cli,
		mqtt::connect_options& connOpts,
		std::string publishTopic,
		std::string subscribeTopic,
		ProtectedQ<std::string> *fromNetwork)
				: nretry_(0), cli_(cli), connOpts_(connOpts) {
					pubTopic = publishTopic;
					subTopic = subscribeTopic;
					fromNetQ = fromNetwork;
				}
};

/////////////////////////////////////////////////////////////////////////////

static void mqtt_publisher(mqtt::async_client *client, std::string pubTopic, ProtectedQ<std::string> *toNetwork)
{
	// when an element shows up on the relevant queue, publish it to a specific topic
	std::string msgStr;
	mqtt::topic top(*client, pubTopic, QOS);
	while(true)
	{
		msgStr = toNetwork->popItem();
		top.publish(msgStr);
	}
}

void mqtt_pubsub_ops(
					struct MqttParams *params,
					ProtectedQ<std::string> *toNetwork,
					ProtectedQ<std::string> *fromNetwork )
{
	// create MQTT objects
	mqtt::connect_options connOpts;
	connOpts.set_keep_alive_interval(30);
	connOpts.set_clean_session(true);
	std::cout << "[MQTTdebug] connection timeout: " << connOpts.get_connect_timeout().count() << std::endl;

	// create will message
	auto lwt = mqtt::make_message(params->pubtopic, "<<<"+params->userid+" disconnected>>>", QOS, false);
	connOpts.set_will_message(lwt);

	mqtt::async_client client(params->server, params->userid);
	CallbackHelper cb(client, connOpts,
						params->pubtopic, params->subtopic, fromNetwork);
	client.set_callback(cb);

	// try connecting  to server and subscribing to topic
	try
	{
		std::cout << boost::format("[MQTTdebug] connecting to %s as %s") % params->server % params->userid << std::endl;
		client.connect(connOpts, nullptr, cb)->wait();
	}
	catch(const mqtt::exception& e)
	{
		std::cerr << "\n[MQTTerror] Unable to connect to MQTT server: '"
			<< params->server << "'" << std::endl;
		exit(1);
	}

	// setup publishing on topic
	mqtt_publisher(&client, params->pubtopic, toNetwork);
	
}
