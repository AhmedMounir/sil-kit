// Copyright (c) Vector Informatik GmbH. All rights reserved.

#define _CRT_SECURE_NO_WARNINGS 1

#include <algorithm>
#include <ctime>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <thread>

#include "silkit/version.hpp"
#include "silkit/SilKit.hpp"
#include "silkit/core/sync/all.hpp"
#include "silkit/core/sync/string_utils.hpp"
#include "silkit/core/logging/ILogger.hpp"

#include "CommandlineParser.hpp"

using namespace SilKit::Core;
using namespace std::chrono_literals;

std::ostream& operator<<(std::ostream& out, std::chrono::nanoseconds timestamp)
{
    auto seconds = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1>>>(timestamp);
    out << seconds.count() << "s";
    return out;
}

int main(int argc, char** argv)
{
    SilKit::Util::CommandlineParser commandlineParser;
    commandlineParser.Add<SilKit::Util::CommandlineParser::Flag>("version", "v", "[--version]",
        "-v, --version: Get version info.");
    commandlineParser.Add<SilKit::Util::CommandlineParser::Flag>("help", "h", "[--help]",
        "-h, --help: Get this help.");

    commandlineParser.Add<SilKit::Util::CommandlineParser::Option>(
        "connect-uri", "u", "silkit://localhost:8500", "[--connect-uri <silkitUri>]",
        "-u, --connect-uri <silkitUri>: The registry URI to connect to. Defaults to 'silkit://localhost:8500'.");
    commandlineParser.Add<SilKit::Util::CommandlineParser::Option>("name", "n", "SystemMonitor", "[--name <participantName>]",
        "-n, --name <participantName>: The participant name used to take part in the simulation. Defaults to 'SystemMonitor'.");
    commandlineParser.Add<SilKit::Util::CommandlineParser::Option>("configuration", "c", "", "[--configuration <configuration>]",
        "-c, --configuration <configuration>: Path and filename of the Participant configuration YAML or JSON file. Note that the format was changed in v3.6.11.");

    std::cout << "Vector SilKit -- System Monitor" << std::endl
        << std::endl;

    try
    {
        commandlineParser.ParseArguments(argc, argv);
    }
    catch (std::runtime_error & e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        commandlineParser.PrintUsageInfo(std::cerr, argv[0]);

        return -1;
    }

    if (commandlineParser.Get<SilKit::Util::CommandlineParser::Flag>("help").Value())
    {
        commandlineParser.PrintUsageInfo(std::cout, argv[0]);

        return 0;
    }

    if (commandlineParser.Get<SilKit::Util::CommandlineParser::Flag>("version").Value())
    {
        std::string hash{ SilKit::Version::GitHash() };
        auto shortHash = hash.substr(0, 7);
        std::cout
            << "Version Info:" << std::endl
            << " - Vector SilKit: " << SilKit::Version::String() << ", #" << shortHash << std::endl;

        return 0;
    }

    auto connectUri{ commandlineParser.Get<SilKit::Util::CommandlineParser::Option>("connect-uri").Value() };
    auto participantName{ commandlineParser.Get<SilKit::Util::CommandlineParser::Option>("name").Value() };
    auto configurationFilename{ commandlineParser.Get<SilKit::Util::CommandlineParser::Option>("configuration").Value() };

    std::shared_ptr<SilKit::Config::IParticipantConfiguration> configuration;
    try
    {
        configuration = !configurationFilename.empty() ?
            SilKit::Config::ParticipantConfigurationFromFile(configurationFilename) :
            SilKit::Config::ParticipantConfigurationFromString("");
    }
    catch (const SilKit::ConfigurationError & error)
    {
        std::cerr << "Error: Failed to load configuration '" << configurationFilename << "', " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();

        return -2;
    }

    try
    {
        std::cout << "Creating participant '" << participantName << "' with registry " << connectUri << std::endl;

        auto participant = SilKit::CreateParticipant(std::move(configuration), participantName, connectUri);

        auto* logger = participant->GetLogger();
        auto* systemMonitor = participant->GetSystemMonitor();

        systemMonitor->AddParticipantStatusHandler([logger](const Orchestration::ParticipantStatus& status) {
            std::stringstream buffer;
            buffer << "New ParticipantState: " << status.participantName << " is " << status.state
                   << ",\tReason: " << status.enterReason;
            logger->Info(buffer.str());
        });

        systemMonitor->AddSystemStateHandler([logger](Orchestration::SystemState state) {
            std::stringstream buffer;
            buffer << "New SystemState: " << state;
            logger->Info(buffer.str());
        });

        std::cout << "Press enter to terminate the SystemMonitor..." << std::endl;
        std::cin.ignore();
    }
    catch (const std::exception & error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();

        return -3;
    }

    return 0;
}