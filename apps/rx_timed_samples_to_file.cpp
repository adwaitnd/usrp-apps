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
        ("ref", po::value<std::string>(&ref), "reference source (internal, external, mimo)")
        ("wirefmt", po::value<std::string>(&wirefmt)->default_value("sc16"), "wire format (sc8 or sc16)")
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

    // bool bw_summary             = vm.count("progress") > 0;
    // bool stats                  = vm.count("stats") > 0;
    // bool null                   = vm.count("null") > 0;
    // bool enable_size_map        = vm.count("sizemap") > 0;
    // bool continue_on_bad_packet = vm.count("continue") > 0;

    // if (enable_size_map)
    //     std::cout << "Packet size tracking enabled - will only recv one packet at a time!"
    //               << std::endl;

    // // create a usrp device
    // std::cout << std::endl;
    // std::cout << boost::format("Creating the usrp device with: %s...") % args
    //           << std::endl;
    // uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // // Lock mboard clocks
    // usrp->set_clock_source(ref);
    // usrp->set_time_source(ref);

    // // check Ref and MIMO Lock detection
    // if (not vm.count("skip-lo")) {
    //     if (ref == "mimo")
    //         check_mimo_lock(usrp, setup_time);
    //     if (ref == "external")
    //         check_ref_lock(usrp, setup_time);
    // }

    // // always select the subdevice first, the channel mapping affects the other settings
    // if (vm.count("subdev"))
    //     usrp->set_rx_subdev_spec(subdev);

    // std::cout << boost::format("Using Device: %s") % usrp->get_pp_string() << std::endl;

    uhd::usrp::multi_usrp::sptr usrp = create_device(
                args,       // device creating arguments
                ref,        // clock reference
                setup_time, // 
                (vm.count("subdev") > 0) ? subdev : "");

    sync_usrp_ntp(usrp);
    
    bool ret = process_rx_request(
                    usrp,               // USRP pointer
                    channel,            // USRP channel to use
                    ant,                // antenna to use
                    file,               // name of file to save data
                    freq,               // center freq
                    lo_offset,          // LO offset
                    rate,               // sampling rate
                    (vm.count("gain") > 0), // change analog gain?
                    gain,               // analog gain
                    (vm.count("bw") > 0),   // change IF bw?
                    bw,                 // IF bandwidth
                    t0,                 // start time
                    total_num_samps,    // total samples
                    0.5,                // timeout slack
                    type,               // data type for storage
                    wirefmt,            // data type over ethernet
                    spb,                // samples per buffer
                    setup_time,         // waiting time for LO lock
                    (vm.count("int-n") > 0),    // force int-N multiplier
                    (vm.count("progress") > 0), // show instantaneous data bw
                    (vm.count("stats") > 0),    // summarize results
                    (vm.count("null") > 0),     // dont write to file
                    (vm.count("sizemap") > 0)); // show packet sizes

    
    // finished
    std::cout << std::endl << "Done!" << std::endl << std::endl;


    return EXIT_SUCCESS;
}