/*
 *
 */

#include <uhd/exception.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <chrono>
#include <complex>
#include <fstream>
#include <cstdio>
#include <thread>
#include <string>
#include "ops_helper.hpp"
#include "date.h"

template <typename Clock>
std::chrono::time_point<Clock, std::chrono::duration<double>> double2timepoint(double t)
{
    // converts double to duration with double internal type
    std::chrono::duration<double> d(t);
    // creates a time point for a particular clock from the previously computed duration
    return std::chrono::time_point<Clock, std::chrono::duration<double>>(d); 
}

template <typename Clock>
double timepoint2double(std::chrono::time_point<Clock> tp)
{
    // converts duration to a double value
    std::chrono::duration<double> ddouble = tp.time_since_epoch();
    return ddouble.count();
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

typedef std::function<uhd::sensor_value_t(const std::string&)> get_sensor_fn_t;
bool check_locked_sensor(std::vector<std::string> sensor_names,
    const char* sensor_name,
    get_sensor_fn_t get_sensor_fn,
    double setup_time)
{
    if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name)
        == sensor_names.end())
        return false;

    auto setup_timeout = std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(int64_t(setup_time * 1000));
    bool lock_detected = false;

    std::cout << boost::format("[UHDdebug] Waiting for \"%s\": ") % sensor_name;
    std::cout.flush();

    while (true) {
        if (lock_detected or (std::chrono::steady_clock::now() > setup_timeout)) {
            std::cout << " locked." << std::endl;
            break;
        }
        if (get_sensor_fn(sensor_name).to_bool()) {
            std::cout << "+";
            std::cout.flush();
            lock_detected = true;
        } else {
            if (std::chrono::steady_clock::now() > setup_timeout) {
                std::cout << std::endl;
                throw std::runtime_error(
                    str(boost::format(
                            "timed out waiting for consecutive locks on sensor \"%s\"")
                        % sensor_name));
            }
            std::cout << "_";
            std::cout.flush();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;
    return true;
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

uhd::usrp::multi_usrp::sptr create_device(
    const std::string args, 
    const std::string ref,
    const double setup_time,
    const std::string subdev = "")
{
    // create a usrp device
    std::cout << std::endl;
    std::cout << boost::format("[UHDdebug] Creating usrp dev: %s...") % args
              << std::endl;
    uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

    // Lock mboard clocks
    usrp->set_clock_source(ref);
    usrp->set_time_source(ref);

    // check Ref and MIMO Lock detection
    if (ref == "mimo")
        check_mimo_lock(usrp, setup_time);
    if (ref == "external")
        check_ref_lock(usrp, setup_time);

    // always select the subdevice first, the channel mapping affects the other settings
    if (subdev != "")
        usrp->set_rx_subdev_spec(subdev);

    std::cout << boost::format("[UHDdebug] Using Device: %s") % usrp->get_pp_string() << std::endl;

    return usrp;
}

template <typename samp_type>
bool timed_recv_to_file(uhd::usrp::multi_usrp::sptr usrp,
    const std::string& cpu_format,
    const std::string& wire_format,
    const size_t& channel,
    const std::string& file,
    size_t samps_per_buff,
    unsigned long long num_requested_samples,
    double t0,
    double timeout,
    bool bw_summary             = false,
    bool stats                  = false,
    bool null                   = false,
    bool enable_size_map        = false)
{
    unsigned long long num_total_samps = 0;
    // create a receive streamer
    uhd::stream_args_t stream_args(cpu_format, wire_format);
    std::vector<size_t> channel_nums;
    channel_nums.push_back(channel);
    stream_args.channels             = channel_nums;
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    // meta-data will be filled in by recv()
    uhd::rx_metadata_t md;
    std::vector<samp_type> buff(samps_per_buff);
    std::ofstream outfile;
    if (not null)
    {
        std::cout << "[UHDdebug] creating/opening file" << std::endl;
        outfile.open(file.c_str(), std::ofstream::binary);
    } else
    {
        std::cout << "[UHDdebug] skipping save file" << std::endl;
    }
    
        

    // setup streaming
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = size_t(num_requested_samples);
    stream_cmd.stream_now = false;
    stream_cmd.time_spec  = uhd::time_spec_t(t0);
    rx_stream->issue_stream_cmd(stream_cmd);

    typedef std::map<size_t, size_t> SizeMap;
    SizeMap mapSizes;
    const auto start_time = double2timepoint<std::chrono::system_clock> (t0);
    const auto stop_time = start_time + std::chrono::duration<double>(timeout);
    const double stop_time_double = t0 + timeout;

    // Track time and samps between updating the BW summary
    auto last_update                     = start_time;
    unsigned long long last_update_samps = 0;

    // calculate timeout of first recv call
    auto now = std::chrono::system_clock::now();
    auto tnow_double = timepoint2double<std::chrono::system_clock>(now);
    double recv_to = stop_time_double - tnow_double;


    std::cout << boost::format("[%s] requesting capture at %.06lf") % systime_str(now) % t0<< std::endl;

    // Run this loop until either time expired (if a duration was given), until
    // the requested number of samples were collected (if such a number was
    // given), or until Ctrl-C was pressed.
    while (num_requested_samples != num_total_samps)
    {
        now = std::chrono::system_clock::now();

        size_t num_rx_samps =
            rx_stream->recv(&buff.front(), buff.size(), md, recv_to, enable_size_map);

        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT)
        {
            std::cout << boost::format("Timeout while streaming") << std::endl;
            break;
        }
        if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW)
        {
            std::cerr<< boost::format("Could not sustain write rate of %fMB/s\n") % (usrp->get_rx_rate(channel) * sizeof(samp_type) / 1e6);
            break;
        }
        if(md.error_code == uhd::rx_metadata_t::ERROR_CODE_LATE_COMMAND)
        {
            std::cout << "Late command!" << std::endl;
            break;
        }
        if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            std::string error = str(boost::format("Receiver error: %s") % md.strerror());
            throw std::runtime_error(error);
        }

        if (enable_size_map) {
            SizeMap::iterator it = mapSizes.find(num_rx_samps);
            if (it == mapSizes.end())
                mapSizes[num_rx_samps] = 0;
            mapSizes[num_rx_samps] += 1;
        }

        num_total_samps += num_rx_samps;

        if (outfile.is_open()) {
            outfile.write((const char*)&buff.front(), num_rx_samps * sizeof(samp_type));
        }

        if (bw_summary) {
            last_update_samps += num_rx_samps;
            const auto time_since_last_update = now - last_update;
            if (time_since_last_update > std::chrono::seconds(1)) {
                const double time_since_last_update_s =
                    std::chrono::duration<double>(time_since_last_update).count();
                const double rate = double(last_update_samps) / time_since_last_update_s;
                std::cout << "\t" << (rate / 1e6) << " Msps" << std::endl;
                last_update_samps = 0;
                last_update       = now;
            }
        }
        now = std::chrono::system_clock::now();
        tnow_double = timepoint2double<std::chrono::system_clock>(now);
        // remaining time in transaction is timeoput for subsequent requests
        recv_to = stop_time_double - tnow_double;
    }
    const auto actual_stop_time = std::chrono::system_clock::now();

    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    rx_stream->issue_stream_cmd(stream_cmd);

    if (outfile.is_open()) {
        outfile.close();
        std::cout << "[UHDdebug] outfile closed" << std::endl;
    }

    if (stats) {
        std::cout << std::endl;
        const double actual_duration_seconds =
            std::chrono::duration<float>(actual_stop_time - start_time).count();

        std::cout << boost::format("Received %d samples in %f seconds") % num_total_samps
                         % actual_duration_seconds
                  << std::endl;
        const double rate = (double)num_total_samps / actual_duration_seconds;
        std::cout << (rate / 1e6) << " Msps" << std::endl;

        if (enable_size_map) {
            std::cout << std::endl;
            std::cout << "Packet size map (bytes: count)" << std::endl;
            for (SizeMap::iterator it = mapSizes.begin(); it != mapSizes.end(); it++)
                std::cout << it->first << ":\t" << it->second << std::endl;
        }
    }

    return (num_total_samps == num_requested_samples);
}


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

bool process_rx_request(
    uhd::usrp::multi_usrp::sptr usrp,
    const size_t channel,
    const std::string ant,
    const std::string file,
    double freq,
    double lo_offset,
    double rate,
    bool set_gain_flag,
    double gain,
    bool set_bw_flag,
    double bw,
    double t0,
    unsigned long long num_requested_samples,
    double to_slack,
    const std::string cpu_format,
    const std::string wire_format,
    size_t samps_per_buff,
    double setup_time,
    bool use_intn_flag               = false,
    bool bw_summary_flag             = false,
    bool stats_flag                  = false,
    bool null_flag                   = false,
    bool enable_size_map_flag        = false)
{
    set_sample_rate(usrp, rate, channel);    
    set_fc(usrp, channel, freq, lo_offset, use_intn_flag);
    if(set_gain_flag)
        set_gain(usrp, channel, gain);
    if(set_bw_flag)
        set_ifbw(usrp, channel, bw);
    usrp->set_rx_antenna(ant, channel);

    // check Ref and LO Lock detect
    check_lo_lock(usrp, channel, setup_time);
    double timeout = to_slack + double(num_requested_samples)/rate;  // timeout per call
    #define timed_recv_to_file_args(format) \
        (usrp,                  \
         format,                \
         wire_format,           \
         channel,               \
         file,                  \
         samps_per_buff,        \
         num_requested_samples, \
         t0,                    \
         timeout,               \
         bw_summary_flag,            \
         stats_flag,                 \
         null_flag,                  \
         enable_size_map_flag)
    
    // recv to file
    bool ret;
    if (cpu_format == "double")
        ret = timed_recv_to_file<std::complex<double>> timed_recv_to_file_args("fc64");
    else if (cpu_format == "float")
        ret = timed_recv_to_file<std::complex<float>> timed_recv_to_file_args("fc32");
    else if (cpu_format == "short")
        ret = timed_recv_to_file<std::complex<short>> timed_recv_to_file_args("sc16");
    else
        throw std::runtime_error("Unknown type " + cpu_format);

    if (ret == false)
    {  
        std::cout << "[UHDdebug] USRP rx error. Removing file " << file << std::endl;
        std::remove(file.c_str());
    }

    return ret;
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

void sync_usrp_ntp(uhd::usrp::multi_usrp::sptr usrp, double sync_slack)
{
// attempt to synchronize USRP with host NTP if their'e not close already
    while(ntp_usrp_synced(usrp, sync_slack) == false)
    {
        std::cout << "usrp not synced with host NTP time" << std::endl;
        attempt_ntp_pps_sync(usrp);
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    }

    std::cout << boost::format("[%s] synced usrp time: %.6lf") % systime_str(std::chrono::system_clock::now()) % timespec_str(usrp->get_time_now()) << std::endl;
}

void usrp_ops(
                struct UsrpParams* params,
                ProtectedQ<std::string> *toNetwork,
                ProtectedQ<std::string> *fromNetwork)
{
    size_t n_samples;
    double fc, lo_off, sps, ifbw, gain, tstart;
    char antc[32];
    int n_decoded;

    std::cout << "[UHDdebug] USRP thread created" << std::endl;

    uhd::usrp::multi_usrp::sptr usrp = create_device(
        params->args,
        params->clkref,
        params->tslack,
        params->subdev_flag ? params->subdev : ""
    );

    while(true)
    {
        // double check that're we are within the time sync bound for
        // every processed operation
        sync_usrp_ntp(usrp, params->ntpslack);

        // wait for new request
        std::string rxmsg = fromNetwork->popItem();
        std::cout << "[UHDdebug] request recvd" << std::endl;

        // parse request
        n_decoded = std::sscanf(rxmsg.c_str(),
        "fc=%lf,lo=%lf,sps=%lf,bw=%lf,g=%lf,t0=%lf,n=%zu,ant=%[^,]",
            &fc,
            &lo_off,
            &sps,
            &ifbw,
            &gain,
            &tstart,
            &n_samples,
            antc);
        
        // check if request is valid, if not, start loop from the top
        // validity is based on format
        if(n_decoded != 8)
        {
            std::string txmsg = str(boost::format("<%s invalid msg>") % params->client_id);
            toNetwork->addItem(txmsg);
            std::cout << txmsg << std::endl;
            continue;
        }

        // TODO: parse all request parameters
        const auto trequest = double2timepoint<std::chrono::system_clock>(tstart);
        const auto datestr = date::format("%F-%T", std::chrono::time_point_cast<std::chrono::milliseconds>(trequest));
        std::string rx_filename = (boost::format("%s%.3lfM_%s.dat")
                        % params->file_prefix
                        % (fc/1e6)
                        % datestr
                        ).str();

        // Check if the request was too late
        double tnow_double = timepoint2double<std::chrono::system_clock>(std::chrono::system_clock::now());
        // in the worst case, NTP time lags GPS PPS and setup takes max slack time
        // error on: tnow + ntp_error + slack_time > tstart
        if((tnow_double + params->ntpslack + params->tslack) > tstart)
        {
            std::string txmsg = (boost::format("<%s host late command @%s>") % params->client_id % datestr).str();
            toNetwork->addItem(txmsg);
            std::cout << txmsg << std::endl;
            continue;
        }

        std::cout << "saving to file: " << rx_filename;

        bool ret = process_rx_request(
                    usrp,
                    params->channel,
                    std::string(antc),
                    rx_filename,
                    fc,
                    lo_off,
                    sps,
                    true,
                    gain,
                    true,
                    ifbw,
                    tstart,
                    n_samples,
                    0.5,
                    params->datafmt,
                    params->wirefmt,
                    params->spb,
                    params->tslack,
                    params->intn_flag,
                    true,
                    true,
                    false,
                    false);

        if(ret)
        {
            std::string txmsg = (boost::format("<%s req saved %s>") % params->client_id % rx_filename).str();
            toNetwork->addItem(txmsg);
            std::cout << txmsg << std::endl;
        } else
        {
            std::string txmsg = (boost::format("<%s req failed @ %s>") % params->client_id % datestr).str();
            toNetwork->addItem(txmsg);
            std::cout << txmsg << std::endl;
        }
        

    }

}