#include <iostream>
#include <chrono>
#include <string>
#include <boost/format.hpp>

template <class Clock>
class Stopwatch
{
    typename Clock::time_point tstart;
    typename Clock::time_point tlap;

    public:
    void start()
    {
        tstart = Clock::now();
        tlap = tstart;
    }

    typename Clock::duration lap()
    {
        typename Clock::time_point tnow = Clock::now();
        auto laptime = tnow - tlap;
        tlap = tnow;
        return laptime;
    }

    typename Clock::duration passed()
    {
        typename Clock::time_point tnow = Clock::now();
        return tnow - tstart;
    }

    const std::string str_laptime()
    {
        using namespace std::chrono;
        auto dt = lap();
        auto sec = duration_cast<seconds>(dt).count();
        auto usec = duration_cast<microseconds>(dt).count() % 1000000;
        return (boost::format("%u.%06u") % sec % usec).str();
    }

    void print_laptime(const std::string msg="")
    {
        std::cout << boost::format("[laptime %s] %s") % this->str_laptime() % msg << std::endl;
    }

    const std::string str_passedtime()
    {
        using namespace std::chrono;
        auto dt = passed();
        auto sec = duration_cast<seconds>(dt).count();
        auto usec = duration_cast<microseconds>(dt).count() % 1000000;
        return (boost::format("%u.%06u") % sec % usec).str();
    }

    void print_passedtime(const std::string msg="")
    {
        std::cout << boost::format("[passed time %s] %s") % this->str_passedtime() % msg << std::endl;
    }
};