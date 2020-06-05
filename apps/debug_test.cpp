#include <iostream>
#include <string>
#include <boost/format.hpp>

int main(int argc, char *argv[])
{
    int n = 0;
    std::cout << boost::format("step %d") % ++n << std::endl;
    std::cout << boost::format("step %d") % ++n << std::endl;
    std::cout << boost::format("step %d") % ++n << std::endl;
    std::cout << boost::format("step %d") % ++n << std::endl;
    return ~0;
}