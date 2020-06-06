/*
 *  Get the current time and clock source of a target USRP
 */

#include <iostream>
#include <chrono>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/log.hpp>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    std::string addr;

    // setup program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("nowarn", "disable USRP warnings")
        ("addr", po::value<std::string>(&addr)->default_value("192.168.10.3"), "IP address of the USRP")
    ;

    // register program options
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // if help flag is called, print the description
    if(vm.count("help"))
    {
        std::cout << desc << std::endl;
    }

    if(vm.count("nowarn"))
    {
        // set USRP logging level to errors only
        uhd::log::set_log_level(uhd::log::error);
    }

    // create USRP device
    std::string usrp_args = (boost::format("addr=%s") % addr).str();
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(usrp_args);
    std::cout << boost::format("Using device: %s") % usrp->get_pp_string() << std::endl;

    // show current clock source
    std::string clk_source = usrp->get_clock_source(0);
    std::cout << boost::format("current clock source: %s\n") % clk_source;

    // read and parse USRP time
    uhd::time_spec_t usrp_time = usrp->get_time_now();
    auto usrp_sec = usrp_time.get_full_secs();
    auto usrp_usec = int(usrp_time.get_frac_secs()*1000000);
    std::cout << boost::format("usrp time: %d.%06d\n") % usrp_sec % usrp_usec;

    // read and parse host time
    auto host_time = std::chrono::system_clock::now().time_since_epoch();
    auto host_msec = std::chrono::duration_cast<std::chrono::milliseconds>(host_time).count() % 1000;
    auto host_sec = std::chrono::duration_cast<std::chrono::seconds>(host_time).count();
    std::cout << boost::format("current host time: %d.%03d\n") % host_sec % host_msec;

    return 0;
}