// async_subscribe.cpp
//
// This is a Paho MQTT C++ client, sample application.
//
// This application is an MQTT subscriber using the C++ asynchronous client
// interface, employing callbacks to receive messages and status updates.
//
// The sample demonstrates:
//  - Connecting to an MQTT server/broker.
//  - Subscribing to a topic
//  - Receiving messages through the callback API
//  - Receiving network disconnect updates and attempting manual reconnects.
//  - Using a "clean session" and manually re-subscribing to topics on
//    reconnect.
//

/*******************************************************************************
 * Copyright (c) 2013-2017 Frank Pagliughi <fpagliughi@mindspring.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Frank Pagliughi - initial implementation and documentation
 *******************************************************************************/

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

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const std::string CLIENT_ID("async_subcribe_cpp");
const std::string TOPIC("hello");

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

	std::string subTopic;
	ProtectedQ<std::string> *fromNetQ;

	// This deomonstrates manually reconnecting to the broker by calling
	// connect() again. This is a possibility for an application that keeps
	// a copy of it's original connect_options, or if the app wants to
	// reconnect with different options.
	// Another way this can be done manually, if using the same options, is
	// to just call the async_client::reconnect() method.
	void reconnect() {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

		std::cout << "[MQTTdebug] Subscribing to topic " << subTopic << std::endl;
		// // new but undocumented method found in the mqttpp_chat sample
		// mqtt::topic topic { cli_, subTopic, QOS };
		// auto subOpts = mqtt::subscribe_options(NO_LOCAL);
		// topic.subscribe(subOpts);
		
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
	CallbackHelper(mqtt::async_client& cli, mqtt::connect_options& connOpts, std::string subscribeTopic, ProtectedQ<std::string> *fromNetwork)
				: nretry_(0), cli_(cli), connOpts_(connOpts) {
					subTopic = subscribeTopic;
					fromNetQ = fromNetwork;
				}
};

/////////////////////////////////////////////////////////////////////////////

void mqtt_publisher(mqtt::async_client *client, std::string pubTopic, ProtectedQ<std::string> *toNetwork)
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
					std::string server,
					std::string userid,
					std::string pubtopic,
					std::string subtopic,
					ProtectedQ<std::string> *toNetwork,
					ProtectedQ<std::string> *fromNetwork )
{
	// create MQTT objects
	mqtt::connect_options connOpts;
	connOpts.set_keep_alive_interval(20);
	connOpts.set_clean_session(true);

	// create will message
	auto lwt = mqtt::make_message(pubtopic, "<<<"+userid+" was disconnected>>>", QOS, false);
	connOpts.set_will_message(lwt);

	mqtt::async_client client(server, userid);
	CallbackHelper cb(client, connOpts, subtopic, fromNetwork);
	client.set_callback(cb);

	// try connecting  to server and subscribing to topic
	try
	{
		std::cout << "[MQTTdebug] connecting to " << server << std::endl;
		client.connect(connOpts, nullptr, cb)->wait();
	}
	catch(const mqtt::exception& e)
	{
		std::cerr << "\n[MQTTerror] Unable to connect to MQTT server: '"
			<< server << "'" << std::endl;
		exit(1);
	}

	// setup publishing on topic
	mqtt_publisher(&client, pubtopic, toNetwork);

	// std::string lastmsg;
	// while(true)
	// {
	// 	lastmsg = fromNetwork->popItem();  // wait until protected queue has an item
	// 	std::cout << boost::format("received %s") % lastmsg << std::endl;
	// 	std::this_thread::sleep_for(std::chrono::seconds(1));
	// }
	
}

void dummyProcessor(ProtectedQ<std::string> *fromNetwork, ProtectedQ<std::string> *toNetwork)
{
	std::string rxMsg, txMsg;

	// parameters passed in messages
	double fc, sps, bw;
    long tstart_sec;
    size_t nSamples;
    int tstart_msec;
    char antc[32], formc[32];
    int nDecoded;
	while(true)
	{
		rxMsg = fromNetwork->popItem();
		std::cout << boost::format("[dummyProcessor] received request for '%s'") % rxMsg << std::endl;
		nDecoded = std::sscanf(rxMsg.c_str(), "fc=%lf,sps=%lf,bw=%lf,tstart=%ld.%d,n=%zu,ant=%[^,],form=%[^,]", &fc, &sps, &bw, &tstart_sec, &tstart_msec, &nSamples, antc, formc);
		
		if(nDecoded == 8)
		{
			std::cout << boost::format("decoded %d elements") % nDecoded << std::endl;
			txMsg = (boost::format("processed request at %ld.%03d") % tstart_sec % tstart_msec).str();

			toNetwork->addItem(txMsg);
			std::cout << boost::format("[dummyProcessor] sending response <%s>") % txMsg << std::endl;
		} else
		{
			std::cout << "[dummyProcessor] invalid request" << std::endl;
			toNetwork->addItem("invalid request");
		}
		
	}
}


int main(int argc, char* argv[])
{
	ProtectedQ<std::string> toNetwork, fromNetwork;

	std::thread processorThread(&dummyProcessor, &fromNetwork, &toNetwork);

	mqtt_pubsub_ops(SERVER_ADDRESS, "tester", "response", "command", &toNetwork, &fromNetwork);

	// mqtt::connect_options connOpts;
	// connOpts.set_keep_alive_interval(20);
	// connOpts.set_clean_session(true);

	// mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

	// CallbackHelper cb(client, connOpts, TOPIC);
	// client.set_callback(cb);

	// // Start the connection.
	// // When completed, the callback will subscribe to topic.

	// try {
	// 	std::cout << "Connecting to the MQTT server..." << std::flush;
	// 	client.connect(connOpts, nullptr, cb);
	// }
	// catch (const mqtt::exception&) {
	// 	std::cerr << "\nERROR: Unable to connect to MQTT server: '"
	// 		<< SERVER_ADDRESS << "'" << std::endl;
	// 	return 1;
	// }

	// // Just block till user tells us to quit.

	// while (std::tolower(std::cin.get()) != 'q')
	// 	;

	// // Disconnect

	// try {
	// 	std::cout << "\nDisconnecting from the MQTT server..." << std::flush;
	// 	client.disconnect()->wait();
	// 	std::cout << "OK" << std::endl;
	// }
	// catch (const mqtt::exception& exc) {
	// 	std::cerr << exc.what() << std::endl;
	// 	return 1;
	// }

 	return 0;
}

