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
    double t0;

    // setup program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("nowarn", "disable USRP warnings")
        ("addr", po::value<std::string>(&addr)->default_value("192.168.10.3"), "IP address of the USRP")
        ("time", po::value<double>(&t0)->default_value(0), "reset time")
    ;

    // register program options
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // if help flag is called, print the description
    if(vm.count("help"))
    {
        std::cout << desc << std::endl;
        exit(0);
    }

    if(vm.count("nowarn"))
    {
        // set USRP logging level to errors only
        uhd::log::set_log_level(uhd::log::error);
    }

    // create USRP device
    std::string usrp_args = (boost::format("addr=%s") % addr).str();
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(usrp_args);
    // std::cout << boost::format("Using device: %s") % usrp->get_pp_string() << std::endl;

    // read and parse USRP time
    uhd::time_spec_t usrp_time = usrp->get_time_now();
    auto usrp_sec = usrp_time.get_full_secs();
    auto usrp_usec = int(usrp_time.get_frac_secs()*1000000);
    std::cout << boost::format("usrp time before: %d.%06d\n") % usrp_sec % usrp_usec;

    std::cout << "set time to " << t0 <<std::endl;
    uhd::time_spec_t tspec0(t0);
    usrp->set_time_now(tspec0);

    // read and parse USRP time
    usrp_time = usrp->get_time_now();
    usrp_sec = usrp_time.get_full_secs();
    usrp_usec = int(usrp_time.get_frac_secs()*1000000);
    std::cout << boost::format("usrp time after: %d.%06d\n") % usrp_sec % usrp_usec;

    return 0;
}