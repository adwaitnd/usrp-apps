#include <uhd/usrp/multi_usrp.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/format.hpp>

using dmilliseconds = std::chrono::duration<double, std::milli>;
using dseconds = std::chrono::duration<double>;
using fmilliseconds = std::chrono::duration<float, std::milli>;
using fseconds = std::chrono::duration<float>;

const std::string timespec_str(uhd::time_spec_t t);
const std::string time_str(std::chrono::system_clock::time_point t);
int main(int argc, char *argv[])
{
    using namespace std::chrono;
    auto t_now = system_clock::now();
    std::cout << "time now: " << time_str(t_now) << std::endl;
    double real_sec = duration_cast<dseconds>(t_now.time_since_epoch()).count();
    // std::cout << "sec: " << sec << std::endl;
    std::cout << "frac: " << real_sec << std::endl;

    // convert to uhd::time_spec_t
    uhd::time_spec_t tspec(real_sec);
    
    std::cout << "s.uuuuuu representation: " << timespec_str(tspec) << std::endl;
    std::cout << "full secs:" << tspec.get_full_secs() << std::endl;
    std::cout << "frac secs:" << tspec.get_frac_secs() << std::endl;
    std::cout << "real secs:" << tspec.get_real_secs() << std::endl;
    
    return ~0;
}

const std::string timespec_str(uhd::time_spec_t t)
{
    auto usrp_sec = t.get_full_secs();
    auto usrp_usec = int(t.get_frac_secs()*1000000);
    return str(boost::format("%d.%06d") % usrp_sec % usrp_usec);
}

const std::string time_str(std::chrono::system_clock::time_point t)
{
    using namespace std::chrono;
    auto sec = duration_cast<seconds>(t.time_since_epoch()).count();
    auto usec = duration_cast<microseconds>(t.time_since_epoch()).count() % 1000000;
    return (boost::format("%u.%06u") % sec % usec).str();
}