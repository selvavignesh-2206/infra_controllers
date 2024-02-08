#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "TRLIotCoreAdapter.hpp"

volatile sig_atomic_t stop = false;

void inthand(int signum)
{
    stop = 1;
}

void init_logging()
{
    boost::log::register_simple_formatter_factory<
        boost::log::trivial::severity_level,
        char>("Severity");
    std::string time_local = boost::posix_time::to_iso_string(
        boost::posix_time::second_clock::local_time());
    std::string file_name = time_local + "-TRLIotCoreAdapter.log";
    boost::log::add_file_log(
        boost::log::keywords::file_name = file_name,
        boost::log::keywords::format =
            "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%",
        boost::log::keywords::auto_flush = true);
    boost::log::add_console_log(
        std::cout,
        boost::log::keywords::format =
            "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");
    boost::log::core::get()->set_filter(
        boost::log::trivial::severity >= boost::log::trivial::info);
    boost::log::add_common_attributes();
}

int main(int argc, char **argv)
{
    signal(SIGINT, inthand);
    init_logging();
    std::string aws_config_file_path = "/home/abilash/ros2_workspaces/infra_controller_ht/infra-controllers/aws_iot_adapter/config/aws_iot_config.yaml";
    std::string lift_config_file_path = "/home/abilash/ros2_workspaces/infra_controller_ht/infra-controllers/aws_iot_adapter/config/lift_config.yaml";
    std::string doors_config_file_path = "/home/abilash/ros2_workspaces/infra_controller_ht/infra-controllers/aws_iot_adapter/config/doors_config.yaml";

    TRLIotCoreAdapter interface;
    if (interface.Initialize(
            aws_config_file_path,
            lift_config_file_path,
            doors_config_file_path)) {
        while (!stop) {
            if (interface.ConnectionCompleted()) {
                interface.PublishState();
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        }
        BOOST_LOG_TRIVIAL(info) << "Exiting..";
        exit(-1);
    } else {
        BOOST_LOG_TRIVIAL(fatal)
            << ("main TRLIotCoreAdapter initialize failed.");
        exit(-1);
    }
}