#include <cstdio>
#include <iostream>
#include <string>
#include <boost/format.hpp>

int main(int argc, char* argv[])
{
    double fc, sps, bw;
    long tstart_sec;
    size_t nSamples;
    int tstart_msec;
    char antc[32], formc[32];
    int s;
    if(argc>1)
    {
        s = std::sscanf(argv[1], "fc=%lf,sps=%lf,bw=%lf,tstart=%ld.%d,n=%zu,ant=%[^,],form=%[^,]", &fc, &sps, &bw, &tstart_sec, &tstart_msec, &nSamples, antc, formc);    
    } else
    {
        std::string str="fc=123.456e3,sps=1e6,bw=2e6,tstart=888888.512,n=10000,ant=TX/RX,form=float";
        s = std::sscanf(str.c_str(), "fc=%lf,sps=%lf,bw=%lf,tstart=%ld.%d,n=%zu,ant=%[^,],form=%[^,]", &fc, &sps, &bw, &tstart_sec, &tstart_msec, &nSamples, antc, formc);
    }
    
    std::cout << s << " parameters parsed" << std::endl;
    std::string ant = std::string(antc);
    std::string form = std::string(formc);

    std::cout << "parsed outputs:" << std::endl;
    std::cout << "fc:" << fc << std::endl;
    std::cout << "sps:" << sps << std::endl;
    std::cout << "bw:" << bw << std::endl;
    std::cout << boost::format("tstart: %ld.%03d") % tstart_sec % tstart_msec << std::endl;
    std::cout << "n:" << nSamples << std::endl;
    std::cout << "ant: " << ant <<std::endl;
    std::cout << "form: " << form <<std::endl;
    return 0;
}