/*
 * Start recording samples at a known time fter synchronizing the USRP with
 * system time.
 */

//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <complex>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>
#include "uhd_functions.hpp"

#define PROCEED 1

namespace po = boost::program_options;



void set_sample_rate(uhd::usrp::multi_usrp::sptr usrp, double rate, size_t channel)
{
    // set the sample rate
    if (rate <= 0.0) {
        std::cerr << "Please specify a valid sample rate" << std::endl;
        exit(~0);
    }
    std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6) << std::endl;
    usrp->set_rx_rate(rate, channel);
    std::cout << boost::format("Actual RX Rate: %f Msps...")
                     % (usrp->get_rx_rate(channel) / 1e6)
              << std::endl
              << std::endl;
}

void set_fc(uhd::usrp::multi_usrp::sptr usrp, size_t channel, double freq, double lo_offset, bool useIntN)
{

    // set the center frequency
        std::cout << boost::format("Setting RX Freq: %f MHz...") % (freq / 1e6)
                  << std::endl;
        std::cout << boost::format("Setting RX LO Offset: %f MHz...") % (lo_offset / 1e6)
                  << std::endl;
        uhd::tune_request_t tune_request(freq, lo_offset);
        if (useIntN)
        {
            tune_request.args = uhd::device_addr_t("mode_n=integer");
        }
        usrp->set_rx_freq(tune_request, channel);
        std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq(channel) / 1e6) << std::endl;
}
    
void set_gain(uhd::usrp::multi_usrp::sptr usrp, size_t channel, double gain)
{
    std::cout << boost::format("Setting RX Gain: %f dB...") % gain << std::endl;
    usrp->set_rx_gain(gain, channel);
    std::cout << boost::format("Actual RX Gain: %f dB...")
                        % usrp->get_rx_gain(channel)
                << std::endl;
}

void set_ifbw(uhd::usrp::multi_usrp::sptr usrp, size_t channel, double bw)
{    
    // set the IF filter bandwidth
    std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % (bw / 1e6)
                << std::endl;
    usrp->set_rx_bandwidth(bw, channel);
    std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % (usrp->get_rx_bandwidth(channel) / 1e6) << std::endl;
}

void check_lo_lock(uhd::usrp::multi_usrp::sptr usrp, size_t channel, double setup_time)
{
    check_locked_sensor(usrp->get_rx_sensor_names(channel), "lo_locked",
        [usrp, channel](const std::string& sensor_name) { return usrp->get_rx_sensor(sensor_name, channel); },
        setup_time);
}

void check_mimo_lock(uhd::usrp::multi_usrp::sptr usrp, double setup_time)
{
    check_locked_sensor(usrp->get_mboard_sensor_names(0), "mimo_locked",
        [usrp](const std::string& sensor_name) { return usrp->get_mboard_sensor(sensor_name); },
        setup_time);
}

void check_ref_lock(uhd::usrp::multi_usrp::sptr usrp, double setup_time)
{
    check_locked_sensor(usrp->get_mboard_sensor_names(0), "ref_locked",
        [usrp](const std::string& sensor_name) { return usrp->get_mboard_sensor(sensor_name); },
        setup_time);
}

static bool stop_signal_called = false;
void sig_int_handler(int)
{
    stop_signal_called = true;
    exit(0);
}

bool ntp_usrp_synced(uhd::usrp::multi_usrp::sptr usrp, double tThresh)
{
    // return true if ntp system tme and USRP system time are close enough
    double tusrp = usrp->get_time_now().get_real_secs();
    double tsys = timepoint2double<std::chrono::system_clock>(std::chrono::system_clock::now());
    return (abs(tusrp - tsys) <= tThresh);
}

// for a given time point, find the exact time point of the next second.
// provide an additional delay (integer seconds) to find the time point for later second events
inline std::chrono::system_clock::time_point get_next_sec (
    std::chrono::system_clock::time_point tp,
    int addn_dly=0 )
{
    using namespace std::chrono;
    // casting time point to granularity of seconds should result into the previous second
    auto prev_sec = time_point_cast<seconds> (tp);
    auto next_sec = prev_sec + seconds(1 + addn_dly);
    return next_sec;
}

// return true if the next second event will occur within a threshold from the provided time point
// or if the previous second was within a threshold.
bool check_time_bidir_prox(std::chrono::system_clock::time_point tp, std::chrono::milliseconds thr_ms)
{
    using namespace std::chrono;
    int msec = duration_cast<milliseconds> (tp.time_since_epoch()).count() % 1000;
    return ((msec + thr_ms.count() >= 1000) || (msec <= thr_ms.count())) ;
}

// return a string in s.uuuuuu format for the provided time spec
const std::string timespec_str(uhd::time_spec_t t)
{
    return str(boost::format("%.06f") % t.get_real_secs());
}

const std::string systime_str(std::chrono::system_clock::time_point t)
{
    using namespace std::chrono;
    auto sec = duration_cast<seconds>(t.time_since_epoch()).count();
    auto usec = duration_cast<microseconds>(t.time_since_epoch()).count() % 1000000;
    return (boost::format("%u.%06u") % sec % usec).str();
}

bool attempt_ntp_pps_sync(uhd::usrp::multi_usrp::sptr usrp)
{
    using namespace std::chrono;

    system_clock::time_point t_now, t_nextsec;
    milliseconds t_slack(20);

    // loop until we're not very close to the second event
    while(true)
    {
        // get time now (assume we are NTP synced)
        t_now = system_clock::now();
        t_nextsec = get_next_sec(t_now);

        std::cout << boost::format("[debug] t_now: %s | t_nextsec: %s") % systime_str(t_now) % systime_str(t_nextsec) << std::endl;
        
        // check if it is too close to the PPS second edge
        if(check_time_bidir_prox(t_now, t_slack) == false)
            break;

        // time point is too close
        // wait until the next second plus some slack
        std::cout << "waiting..." <<std::endl;
        std::this_thread::sleep_until(t_nextsec + t_slack + t_slack);
        
    }
    // time point is relatively far away. Schedule time for next PPS
    // since we use set_time_unknown_pps we need to schedule for 2 seconds away
    uhd::time_spec_t nextpps = uhd::time_spec_t(
        int64_t(duration_cast<seconds>(t_nextsec.time_since_epoch()).count() + 1), 
        0.0);
    usrp->set_time_unknown_pps(nextpps);
    std::cout << boost::format("[%s] setting time to %s") % systime_str(t_now) % timespec_str(nextpps) << std::endl;
}

int UHD_SAFE_MAIN(int argc, char* argv[])
{
    // variables to be set by po
    std::string args, file, type, ant, subdev, ref, wirefmt;
    size_t channel, total_num_samps, spb;
    double rate, freq, gain, bw, total_time, setup_time, lo_offset, t0;

    // setup the program options
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "multi uhd device address args")
        ("file", po::value<std::string>(&file)->default_value("usrp_samples.dat"), "name of the file to write binary samples to")
        ("type", po::value<std::string>(&type)->default_value("short"), "sample type: double, float, or short")
        ("nsamps", po::value<size_t>(&total_num_samps)->default_value(0), "total number of samples to receive")
        ("t0", po::value<double>(&t0)->default_value(0.0), "start time w.r.t system clock")
        ("spb", po::value<size_t>(&spb)->default_value(10000), "samples per buffer")
        ("rate", po::value<double>(&rate)->default_value(1e6), "rate of incoming samples")
        ("freq", po::value<double>(&freq)->default_value(0.0), "RF center frequency in Hz")
        ("lo-offset", po::value<double>(&lo_offset)->default_value(0.0),
            "Offset for frontend LO in Hz (optional)")
        ("gain", po::value<double>(&gain), "gain for the RF chain")
        ("ant", po::value<std::string>(&ant), "antenna selection")
        ("subdev", po::value<std::string>(&subdev), "subdevice specification")
        ("channel", po::value<size_t>(&channel)->default_value(0), "which channel to use")
        ("bw", po::value<double>(&bw), "analog frontend filter bandwidth in Hz")
        ("ref", po::value<std::string>(&ref)->default_value("internal"), "reference source (internal, external, mimo)")
        ("wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8, sc16 or s16)")
        ("setup", po::value<double>(&setup_time)->default_value(1.0), "seconds of setup time")
        ("progress", "periodically display short-term bandwidth")
        ("stats", "show average bandwidth on exit")
        ("sizemap", "track packet size and display breakdown on exit")
        ("null", "run without writing to file")
        ("continue", "don't abort on a bad packet")
        ("skip-lo", "skip checking LO lock status")
        ("int-n", "tune USRP with integer-N tuning")
    ;
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // print the help message
    if (vm.count("help")) {
        std::cout << boost::format("UHD RX samples to file %s") % desc << std::endl;
        std::cout << std::endl
                  << "This application streams data from a single channel of a USRP "
                     "device to a file.\n"
                  << std::endl;
        return ~0;
    }

    bool bw_summary             = vm.count("progress") > 0;
    bool stats                  = vm.count("stats") > 0;
    bool null                   = vm.count("null") > 0;
    bool enable_size_map        = vm.count("sizemap") > 0;
    bool continue_on_bad_packet = vm.count("continue") > 0;

    if (enable_size_map)
        std::cout << "Packet size tracking enabled - will only recv one packet at a time!"
                  << std::endl;

    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("Creating the usrp device with: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // Lock mboard clocks
    usrp->set_clock_source(ref);
    usrp->set_time_source(ref);

    // check Ref and MIMO Lock detection
    if (not vm.count("skip-lo")) {
        if (ref == "mimo")
            check_mimo_lock(usrp, setup_time);
        if (ref == "external")
            check_ref_lock(usrp, setup_time);
    }

    // always select the subdevice first, the channel mapping affects the other settings
    if (vm.count("subdev"))
        usrp->set_rx_subdev_spec(subdev);

    std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    // attempt to synchronize USRP with host NTP if their'e not close already
    while(ntp_usrp_synced(usrp, 0.1) == false)
    {
        std::cout << "usrp not synced with host NTP time" << std::endl;
        attempt_ntp_pps_sync(usrp);
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    }

    std::cout << "sync complete" <<std::endl;

    // // verify things were proerly set up afterwards
    // // all subsequent last_pps values should have exact zeros for fractional 
    // uhd::time_spec_t t_usrp;
    // uhd::time_spec_t t_usrp_lastpps;
    // while(true)
    // {
    //     auto t_now = std::chrono::system_clock::now();
    //     t_usrp_lastpps = usrp->get_time_last_pps();
    //     t_usrp = usrp->get_time_now();
    //     std::cout << boost::format("[%s] t_usrp: %s \t t_lastpps: %s") % systime_str(t_now) % timespec_str(t_usrp) % timespec_str(t_usrp_lastpps) << std::endl;
    //     auto t_nextsec = get_next_sec(t_now);
    //     std::this_thread::sleep_until(t_nextsec + std::chrono::milliseconds(25));
    // }

    set_sample_rate(usrp, rate, channel);
    
    bool useIntN = vm.count("int-n") > 0;
    set_fc(usrp, channel, freq, lo_offset, useIntN);

    if(vm.count("gain"))
        set_gain(usrp, channel, gain);

    if (vm.count("bw"))
        set_ifbw(usrp, channel, bw);

    // set the antenna
    if (vm.count("ant"))
        usrp->set_rx_antenna(ant, channel);

    // check Ref and LO Lock detect
    if (not vm.count("skip-lo")) {
        check_lo_lock(usrp, channel, setup_time);
    }

    if (total_num_samps == 0) {
        std::signal(SIGINT, &sig_int_handler);
        std::cout << "Press Ctrl + C to stop streaming..." << std::endl;
    }

    double timeout = 0.5 + double(total_num_samps)/rate;  // additional 0.5 sec slack provided as slack
    #define recv_to_file_args(format) \
        (usrp,                        \
         format,                   \
         wirefmt,                  \
         channel,                  \
         file,                     \
         spb,                      \
         total_num_samps,          \
         stop_signal_called,       \
         timeout,               \
         bw_summary,               \
         stats,                    \
         null,                     \
         enable_size_map,          \
         continue_on_bad_packet)
    
    // recv to file
    if (wirefmt == "s16") {
        if (type == "double")
            recv_to_file<double> recv_to_file_args("f64");
        else if (type == "float")
            recv_to_file<float> recv_to_file_args("f32");
        else if (type == "short")
            recv_to_file<short> recv_to_file_args("s16");
        else
            throw std::runtime_error("Unknown type " + type);
    } else {
        if (type == "double")
            recv_to_file<std::complex<double>> recv_to_file_args("fc64");
        else if (type == "float")
            recv_to_file<std::complex<float>> recv_to_file_args("fc32");
        else if (type == "short")
            recv_to_file<std::complex<short>> recv_to_file_args("sc16");
        else
            throw std::runtime_error("Unknown type " + type);
    }

    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;


    return EXIT_SUCCESS;
}
