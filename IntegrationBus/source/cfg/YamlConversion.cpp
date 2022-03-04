// Copyright (c) Vector Informatik GmbH. All rights reserved.
#include "YamlConversion.hpp"

#include <algorithm>
#include <sstream>
#include <chrono>
#include <type_traits>

using namespace ib::cfg::v1::datatypes;
using namespace ib::cfg;
using namespace ib;

// Local utilities
namespace {

auto macaddress_encode(const ib::util::Optional<std::array<uint8_t, 6>>& macAddress, YAML::Node& node, const std::string& fieldName)
{
    if (macAddress.has_value())
    {
        std::stringstream macOut;

        to_ostream(macOut, macAddress.value());
        node[fieldName] = macOut.str();
    }
}

auto macaddress_decode(const YAML::Node& node) ->ib::util::Optional < std::array<uint8_t, 6>>
{
    std::array<uint8_t, 6> macAddress;

    std::stringstream macIn(parse_as<std::string>(node));
    from_istream(macIn, macAddress);

    return macAddress;
}

//template<typename ConfigT>
template<typename ConfigT, typename std::enable_if<
    !(std::is_fundamental<ConfigT>::value || std::is_same<ConfigT, std::string>::value), bool>::type = true>
void optional_encode(const ib::util::Optional<ConfigT>& value, YAML::Node& node, const std::string& fieldName)
{
    if (value.has_value())
    {
        node[fieldName] = YAML::Converter::encode(value.value());
    }
}

template<typename ConfigT>
void optional_encode(const std::vector<ConfigT>& value, YAML::Node& node, const std::string& fieldName)
{
    if (value.size() > 0)
    {
        node[fieldName] = value;
    }
}

void optional_encode(const Replay& value, YAML::Node& node, const std::string& fieldName)
{
    if (value.useTraceSource.size() > 0)
    {
        node[fieldName] = value;
    }
}


template <typename ConfigT>
auto non_default_encode(const std::vector<ConfigT>& values, YAML::Node& node, const std::string& fieldName, const std::vector<ConfigT>& defaultValue)
{
    // Only encode vectors that have members that deviate from a default-value.
    // And also ensure we only encode values that are user-defined.
    if (!values.empty() && !(values == defaultValue))
    {
        std::vector<ConfigT> userValues;  //XXX must not encode uint8/uint16 as character!, need upgrade ConfigT to uint32_t here magically
        static const ConfigT defaultObj{};
        // only encode non-default values
        for (const auto& value : values)
        {
            if (!(value == defaultObj))
            {
                userValues.push_back(value);
            }
        }
        if (userValues.size() > 0)
        {
            node[fieldName] = values;
        }
    }
}

template <typename ConfigT>
auto non_default_encode(const ConfigT& value, YAML::Node& node, const std::string& fieldName, const ConfigT& defaultValue)
{
    if (!(value == defaultValue))
    {
        node[fieldName] = value;
    }
}

} //end anonymous

// YAML type conversion helpers for ParticipantConfiguration data types
namespace YAML {

template<>
Node Converter::encode(const std::chrono::milliseconds& obj)
{
    Node node;
    node = obj.count();
    return node;
}
template<>
bool Converter::decode(const Node& node, std::chrono::milliseconds& obj)
{
    obj = std::chrono::milliseconds{parse_as<uint64_t>(node)};
    return true;
}

template<>
Node Converter::encode(const std::chrono::nanoseconds& obj)
{
    Node node;
    node = obj.count();
    return node;
}
template<>
bool Converter::decode(const Node& node, std::chrono::nanoseconds& obj)
{
    obj = std::chrono::nanoseconds{parse_as<uint64_t>(node)};
    return true;
}

template<>
Node Converter::encode(const Logging& obj)
{
    Node node;
    static const Logging defaultLogger{};

    non_default_encode(obj.logFromRemotes, node, "LogFromRemotes", defaultLogger.logFromRemotes);
    non_default_encode(obj.flushLevel, node, "FlushLevel", defaultLogger.flushLevel);
    // ParticipantConfiguration.schema.json: this is a required property:
    node["Sinks"] = obj.sinks;

    return node;
}
template<>
bool Converter::decode(const Node& node, Logging& obj)
{
    optional_decode(obj.logFromRemotes, node, "LogFromRemotes");
    optional_decode(obj.flushLevel, node, "FlushLevel");
    optional_decode(obj.sinks, node, "Sinks");
    return true;
}

template<>
Node Converter::encode(const Sink& obj)
{
    static const Sink defaultSink{};
    Node node;
    // ParticipantConfiguration.schema.json: Type is required:
    node["Type"] = obj.type;
    non_default_encode(obj.level, node, "Level", defaultSink.level);
    non_default_encode(obj.logName, node, "LogName", defaultSink.logName);
    return node;
}
template<>
bool Converter::decode(const Node& node, Sink& obj)
{
    optional_decode(obj.type, node, "Type");
    optional_decode(obj.level, node, "Level");

    if (obj.type == Sink::Type::File)
    {
        if (!node["LogName"])
        {
            throw ConversionError(node, "Sink of type Sink::Type::File requires a LogName");
        }
        obj.logName = parse_as<std::string>(node["LogName"]);
    }
    return true;
}

template<>
Node Converter::encode(const Sink::Type& obj)
{
    Node node;
    switch (obj)
    {
    case Sink::Type::Remote:
        node = "Remote";
        break;
    case Sink::Type::Stdout:
        node = "Stdout";
        break;
    case Sink::Type::File:
        node = "File";
        break;
    default:
        break;
    }
    return node;
}
template<>
bool Converter::decode(const Node& node, Sink::Type& obj)
{
    if (!node.IsScalar())
    {
        throw ConversionError(node, "Sink::Type should be a string of Remote|Stdout|File.");
    }
    auto&& str = parse_as<std::string>(node);
    if (str == "Remote" || str == "")
    {
        obj = Sink::Type::Remote;
    }
    else if (str == "Stdout")
    {
        obj = Sink::Type::Stdout;
    }
    else if (str == "File")
    {
        obj = Sink::Type::File;
    }
    else
    {
        throw ConversionError(node, "Unknown Sink::Type: " + str + ".");
    }
    return true;
}

template<>
Node Converter::encode(const mw::logging::Level& obj)
{
    Node node;
    switch (obj)
    {
    case mw::logging::Level::Critical:
        node = "Critical";
        break;
    case mw::logging::Level::Error:
        node = "Error";
        break;
    case mw::logging::Level::Warn:
        node = "Warn";
        break;
    case mw::logging::Level::Info:
        node = "Info";
        break;
    case mw::logging::Level::Debug:
        node = "Debug";
        break;
    case mw::logging::Level::Trace:
        node = "Trace";
        break;
    case mw::logging::Level::Off:
        node = "Off";
        break;
    }
    return node;
}
template<>
bool Converter::decode(const Node& node, mw::logging::Level& obj)
{
    if (!node.IsScalar())
    {
        throw ConversionError(node,
            "Level should be a string of Critical|Error|Warn|Info|Debug|Trace|Off.");
    }
    auto&& str = parse_as<std::string>(node);
    if (str == "Critical")
        obj = mw::logging::Level::Critical;
    else if (str == "Error")
        obj = mw::logging::Level::Error;
    else if (str == "Warn")
        obj = mw::logging::Level::Warn;
    else if (str == "Info")
        obj = mw::logging::Level::Info;
    else if (str == "Debug")
        obj = mw::logging::Level::Debug;
    else if (str == "Trace")
        obj = mw::logging::Level::Trace;
    else if (str == "Off")
        obj = mw::logging::Level::Off;
    else
    {
        throw ConversionError(node, "Unknown mw::logging::Level: " + str + ".");
    }
    return true;
}

template<>
Node Converter::encode(const MdfChannel& obj)
{
    Node node;
    optional_encode(obj.channelName, node, "ChannelName");
    optional_encode(obj.channelPath, node, "ChannelPath");
    optional_encode(obj.channelSource, node, "ChannelSource");
    optional_encode(obj.groupName, node, "GroupName");
    optional_encode(obj.groupPath, node, "GroupPath");
    optional_encode(obj.groupSource, node, "GroupSource");
    return node;
}
template<>
bool Converter::decode(const Node& node, MdfChannel& obj)
{
    if (!node.IsMap())
    {
        throw ConversionError(node, "MdfChannel should be a Map");
    }
    optional_decode(obj.channelName, node, "ChannelName");
    optional_decode(obj.channelPath, node, "ChannelPath");
    optional_decode(obj.channelSource, node, "ChannelSource");
    optional_decode(obj.groupName, node, "GroupName");
    optional_decode(obj.groupPath, node, "GroupPath");
    optional_decode(obj.groupSource, node, "GroupSource");
    return true;
}

template<>
Node Converter::encode(const Replay& obj)
{
    static const Replay defaultObj{};
    Node node;
    node["UseTraceSource"] = obj.useTraceSource;
    non_default_encode(obj.direction, node, "Direction", defaultObj.direction);
    non_default_encode(obj.mdfChannel, node, "MdfChannel", defaultObj.mdfChannel);
    return node;
}
template<>
bool Converter::decode(const Node& node, Replay& obj)
{
    obj.useTraceSource = parse_as<decltype(obj.useTraceSource)>(node["UseTraceSource"]);
    optional_decode(obj.direction, node, "Direction");
    optional_decode(obj.mdfChannel, node, "MdfChannel");
    return true;
}

template<>
Node Converter::encode(const Replay::Direction& obj)
{
    Node node;
    switch (obj)
    {
    case Replay::Direction::Send:
        node = "Send";
        break;
    case Replay::Direction::Receive:
        node = "Receive";
        break;
    case Replay::Direction::Both:
        node = "Both";
        break;
    case Replay::Direction::Undefined:
        node = "Undefined";
        break;
    }
    return node;
}
template<>
bool Converter::decode(const Node& node, Replay::Direction& obj)
{
    auto&& str = parse_as<std::string>(node);
    if (str == "Undefined" || str == "")
        obj = Replay::Direction::Undefined;
    else if (str == "Send")
        obj = Replay::Direction::Send;
    else if (str == "Receive")
        obj = Replay::Direction::Receive;
    else if (str == "Both")
        obj = Replay::Direction::Both;
    else
    {
        throw ConversionError(node, "Unknown Replay::Direction: " + str + ".");
    }
    return true;
}

template<>
Node Converter::encode(const CanController& obj)
{
    static const CanController defaultObj{};
    Node node;
    node["Name"] = obj.name;
    non_default_encode(obj.network, node, "Network", defaultObj.network);
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool Converter::decode(const Node& node, CanController& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    optional_decode(obj.network, node, "Network");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node Converter::encode(const LinController& obj)
{
    static const LinController defaultObj{};
    Node node;
    node["Name"] = obj.name;
    non_default_encode(obj.network, node, "Network", defaultObj.network);
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool Converter::decode(const Node& node, LinController& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    optional_decode(obj.network, node, "Network");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node Converter::encode(const EthernetController& obj)
{
    static const EthernetController defaultObj{};
    Node node;
    node["Name"] = obj.name;
    non_default_encode(obj.network, node, "Network", defaultObj.network);
    macaddress_encode(obj.macAddress, node, "MacAddress");
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");

    return node;
}
template<>
bool Converter::decode(const Node& node, EthernetController& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    optional_decode(obj.network, node, "Network");
    obj.macAddress = macaddress_decode(node["MacAddress"]);
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template<>
Node Converter::encode(const sim::fr::ClusterParameters& obj)
{
    Node node;
    // Parse parameters as an int value; uint8_t would be interpreted as a character
    node["gColdstartAttempts"] = static_cast<int>(obj.gColdstartAttempts);
    node["gCycleCountMax"] = static_cast<int>(obj.gCycleCountMax);
    node["gdActionPointOffset"] = static_cast<int>(obj.gdActionPointOffset);
    node["gdDynamicSlotIdlePhase"] = static_cast<int>(obj.gdDynamicSlotIdlePhase);
    node["gdMiniSlot"] = static_cast<int>(obj.gdMiniSlot);
    node["gdMiniSlotActionPointOffset"] = static_cast<int>(obj.gdMiniSlotActionPointOffset);
    node["gdStaticSlot"] = static_cast<int>(obj.gdStaticSlot);
    node["gdSymbolWindow"] = static_cast<int>(obj.gdSymbolWindow);
    node["gdSymbolWindowActionPointOffset"] = static_cast<int>(obj.gdSymbolWindowActionPointOffset);
    node["gdTSSTransmitter"] = static_cast<int>(obj.gdTSSTransmitter);
    node["gdWakeupTxActive"] = static_cast<int>(obj.gdWakeupTxActive);
    node["gdWakeupTxIdle"] = static_cast<int>(obj.gdWakeupTxIdle);
    node["gListenNoise"] = static_cast<int>(obj.gListenNoise);
    node["gMacroPerCycle"] = static_cast<int>(obj.gMacroPerCycle);
    node["gMaxWithoutClockCorrectionFatal"] = static_cast<int>(obj.gMaxWithoutClockCorrectionFatal);
    node["gMaxWithoutClockCorrectionPassive"] = static_cast<int>(obj.gMaxWithoutClockCorrectionPassive);
    node["gNumberOfMiniSlots"] = static_cast<int>(obj.gNumberOfMiniSlots);
    node["gNumberOfStaticSlots"] = static_cast<int>(obj.gNumberOfStaticSlots);
    node["gPayloadLengthStatic"] = static_cast<int>(obj.gPayloadLengthStatic);
    node["gSyncFrameIDCountMax"] = static_cast<int>(obj.gSyncFrameIDCountMax);
    return node;
}
template<>
bool Converter::decode(const Node& node, sim::fr::ClusterParameters& obj)
{
    // Parse parameters as an int value; uint8_t would be interpreted as a character
    auto parseInt = [&node](auto instance, auto name)
    {
        return static_cast<decltype(instance)>(parse_as<int>(node[name]));
    };
    obj.gColdstartAttempts = parseInt(obj.gColdstartAttempts, "gColdstartAttempts");
    obj.gCycleCountMax = parseInt(obj.gCycleCountMax, "gCycleCountMax");
    obj.gdActionPointOffset = parseInt(obj.gdActionPointOffset, "gdActionPointOffset");
    obj.gdDynamicSlotIdlePhase = parseInt(obj.gdDynamicSlotIdlePhase, "gdDynamicSlotIdlePhase");
    obj.gdMiniSlot = parseInt(obj.gdMiniSlot, "gdMiniSlot");
    obj.gdMiniSlotActionPointOffset = parseInt(obj.gdMiniSlotActionPointOffset, "gdMiniSlotActionPointOffset");
    obj.gdStaticSlot = parseInt(obj.gdStaticSlot, "gdStaticSlot");
    obj.gdSymbolWindow = parseInt(obj.gdSymbolWindow, "gdSymbolWindow");
    obj.gdSymbolWindowActionPointOffset = parseInt(obj.gdSymbolWindowActionPointOffset, "gdSymbolWindowActionPointOffset");
    obj.gdTSSTransmitter = parseInt(obj.gdTSSTransmitter, "gdTSSTransmitter");
    obj.gdWakeupTxActive = parseInt(obj.gdWakeupTxActive, "gdWakeupTxActive");
    obj.gdWakeupTxIdle = parseInt(obj.gdWakeupTxIdle, "gdWakeupTxIdle");
    obj.gListenNoise = parseInt(obj.gListenNoise, "gListenNoise");
    obj.gMacroPerCycle = parseInt(obj.gMacroPerCycle, "gMacroPerCycle");
    obj.gMaxWithoutClockCorrectionFatal = parseInt(obj.gMaxWithoutClockCorrectionFatal, "gMaxWithoutClockCorrectionFatal");
    obj.gMaxWithoutClockCorrectionPassive = parseInt(obj.gMaxWithoutClockCorrectionPassive, "gMaxWithoutClockCorrectionPassive");
    obj.gNumberOfMiniSlots = parseInt(obj.gNumberOfMiniSlots, "gNumberOfMiniSlots");
    obj.gNumberOfStaticSlots = parseInt(obj.gNumberOfStaticSlots, "gNumberOfStaticSlots");
    obj.gPayloadLengthStatic = parseInt(obj.gPayloadLengthStatic, "gPayloadLengthStatic");
    obj.gSyncFrameIDCountMax = parseInt(obj.gSyncFrameIDCountMax, "gSyncFrameIDCountMax");
    return true;
}
template<>
Node Converter::encode(const sim::fr::NodeParameters& obj)
{
    Node node;
    node["pAllowHaltDueToClock"] = static_cast<int>(obj.pAllowHaltDueToClock);
    node["pAllowPassiveToActive"] = static_cast<int>(obj.pAllowPassiveToActive);
    node["pClusterDriftDamping"] = static_cast<int>(obj.pClusterDriftDamping);
    node["pdAcceptedStartupRange"] = static_cast<int>(obj.pdAcceptedStartupRange);
    node["pdListenTimeout"] = static_cast<int>(obj.pdListenTimeout);
    node["pKeySlotId"] = static_cast<int>(obj.pKeySlotId);
    node["pKeySlotOnlyEnabled"] = static_cast<int>(obj.pKeySlotOnlyEnabled);
    node["pKeySlotUsedForStartup"] = static_cast<int>(obj.pKeySlotUsedForStartup);
    node["pKeySlotUsedForSync"] = static_cast<int>(obj.pKeySlotUsedForSync);
    node["pLatestTx"] = static_cast<int>(obj.pLatestTx);
    node["pMacroInitialOffsetA"] = static_cast<int>(obj.pMacroInitialOffsetA);
    node["pMacroInitialOffsetB"] = static_cast<int>(obj.pMacroInitialOffsetB);
    node["pMicroInitialOffsetA"] = static_cast<int>(obj.pMicroInitialOffsetA);
    node["pMicroInitialOffsetB"] = static_cast<int>(obj.pMicroInitialOffsetB);
    node["pMicroPerCycle"] = static_cast<int>(obj.pMicroPerCycle);
    node["pOffsetCorrectionOut"] = static_cast<int>(obj.pOffsetCorrectionOut);
    node["pOffsetCorrectionStart"] = static_cast<int>(obj.pOffsetCorrectionStart);
    node["pRateCorrectionOut"] = static_cast<int>(obj.pRateCorrectionOut);
    node["pWakeupPattern"] = static_cast<int>(obj.pWakeupPattern);
    node["pSamplesPerMicrotick"] = static_cast<int>(obj.pSamplesPerMicrotick);
    node["pWakeupChannel"] = obj.pWakeupChannel;
    node["pdMicrotick"] = obj.pdMicrotick;
    node["pChannels"] = obj.pChannels;
    return node;
}
template<>
bool Converter::decode(const Node& node, sim::fr::NodeParameters& obj)
{
    auto parseInt = [&node](auto instance, auto name)
    {
        return static_cast<decltype(instance)>(parse_as<int>(node[name]));
    };
    obj.pAllowHaltDueToClock = parseInt(obj.pAllowHaltDueToClock, "pAllowHaltDueToClock");
    obj.pAllowPassiveToActive = parseInt(obj.pAllowPassiveToActive, "pAllowPassiveToActive");
    obj.pClusterDriftDamping = parseInt(obj.pClusterDriftDamping, "pClusterDriftDamping");
    obj.pdAcceptedStartupRange = parseInt(obj.pdAcceptedStartupRange, "pdAcceptedStartupRange");
    obj.pdListenTimeout = parseInt(obj.pdListenTimeout, "pdListenTimeout");
    obj.pKeySlotId = parseInt(obj.pKeySlotId, "pKeySlotId");
    obj.pKeySlotOnlyEnabled = parseInt(obj.pKeySlotOnlyEnabled, "pKeySlotOnlyEnabled");
    obj.pKeySlotUsedForStartup = parseInt(obj.pKeySlotUsedForStartup, "pKeySlotUsedForStartup");
    obj.pKeySlotUsedForSync = parseInt(obj.pKeySlotUsedForSync, "pKeySlotUsedForSync");
    obj.pLatestTx = parseInt(obj.pLatestTx, "pLatestTx");
    obj.pMacroInitialOffsetA = parseInt(obj.pMacroInitialOffsetA, "pMacroInitialOffsetA");
    obj.pMacroInitialOffsetB = parseInt(obj.pMacroInitialOffsetB, "pMacroInitialOffsetB");
    obj.pMicroInitialOffsetA = parseInt(obj.pMicroInitialOffsetA, "pMicroInitialOffsetA");
    obj.pMicroInitialOffsetB = parseInt(obj.pMicroInitialOffsetB, "pMicroInitialOffsetB");
    obj.pMicroPerCycle = parseInt(obj.pMicroPerCycle, "pMicroPerCycle");
    obj.pOffsetCorrectionOut = parseInt(obj.pOffsetCorrectionOut, "pOffsetCorrectionOut");
    obj.pOffsetCorrectionStart = parseInt(obj.pOffsetCorrectionStart, "pOffsetCorrectionStart");
    obj.pRateCorrectionOut = parseInt(obj.pRateCorrectionOut, "pRateCorrectionOut");
    obj.pWakeupPattern = parseInt(obj.pWakeupPattern, "pWakeupPattern");
    obj.pSamplesPerMicrotick = parseInt(obj.pSamplesPerMicrotick, "pSamplesPerMicrotick");
    obj.pWakeupChannel = parse_as<sim::fr::Channel>(node["pWakeupChannel"]);
    obj.pdMicrotick = parse_as<sim::fr::ClockPeriod>(node["pdMicrotick"]);
    obj.pChannels = parse_as<sim::fr::Channel>(node["pChannels"]);
    return true;
}

template<>
Node Converter::encode(const sim::fr::TxBufferConfig& obj)
{
    Node node;
    node["channels"] = obj.channels;
    node["slotId"] = obj.slotId;
    node["offset"] = static_cast<int>(obj.offset);
    node["repetition"] = static_cast<int>(obj.repetition);
    node["PPindicator"] = obj.hasPayloadPreambleIndicator;
    node["headerCrc"] = obj.headerCrc;
    node["transmissionMode"] = obj.transmissionMode;
    return node;
}
template<>
bool Converter::decode(const Node& node, sim::fr::TxBufferConfig& obj)
{
    obj.channels = parse_as<sim::fr::Channel>(node["channels"]);
    obj.slotId = parse_as<uint16_t>(node["slotId"]);
    obj.offset = static_cast<uint8_t>(parse_as<int>(node["offset"]));
    obj.repetition = static_cast<uint8_t>(parse_as<int>(node["repetition"]));
    obj.hasPayloadPreambleIndicator = parse_as<bool>(node["PPindicator"]);
    obj.headerCrc = parse_as<uint16_t>(node["headerCrc"]);
    obj.transmissionMode = parse_as<sim::fr::TransmissionMode>(node["transmissionMode"]);
    return true;
}

template<>
Node Converter::encode(const sim::fr::Channel& obj)
{
    Node node;
    switch (obj)
    {
    case sim::fr::Channel::A:
        node = "A";
        break;
    case sim::fr::Channel::B:
        node = "B";
        break;
    case sim::fr::Channel::AB:
        node =  "AB";
        break;
    case sim::fr::Channel::None:
        node = "None";
        break;
    }
    return node;
}
template<>
bool Converter::decode(const Node& node, sim::fr::Channel& obj)
{
    auto&& str = parse_as<std::string>(node);
    if (str == "A")
        obj =  sim::fr::Channel::A;
    else if (str == "B")
        obj =  sim::fr::Channel::B;
    else if (str == "AB")
        obj =  sim::fr::Channel::AB;
    else if (str == "None" || str == "")
        obj = sim::fr::Channel::None;
    else
    {
        throw ConversionError(node, "Unknown sim::fr::Channel: " + str);
    }
    return true;
}

template<>
Node Converter::encode(const sim::fr::ClockPeriod& obj)
{
    Node node;
    switch (obj)
    {
    case sim::fr::ClockPeriod::T12_5NS:
        node = "12.5ns";
        break;
    case sim::fr::ClockPeriod::T25NS:
        node =  "25ns";
        break;
    case sim::fr::ClockPeriod::T50NS:
        node = "50ns";
        break;
    default:
        throw ConversionError(node, "Unknown sim::fr::ClockPeriod");
    }
    return node;
}
template<>
bool Converter::decode(const Node& node, sim::fr::ClockPeriod& obj)
{
    auto&& str = parse_as<std::string>(node);
    if (str == "12.5ns")
        obj = sim::fr::ClockPeriod::T12_5NS;
    else if (str == "25ns")
        obj = sim::fr::ClockPeriod::T25NS;
    else if (str == "50ns")
        obj = sim::fr::ClockPeriod::T50NS;
    else
    {
        throw ConversionError(node, "Unknown sim::fr::ClockPeriod: " + str);
    }
    return true;
}

template<>
Node Converter::encode(const sim::fr::TransmissionMode& obj)
{
    Node node;
    switch (obj)
    {
    case sim::fr::TransmissionMode::Continuous:
        node = "Continuous";
        break;
    case sim::fr::TransmissionMode::SingleShot:
        node = "SingleShot";
        break;
    default:
        throw ConversionError(node, "Unknown TransmissionMode");
    }
    return node;
}
template<>
bool Converter::decode(const Node& node, sim::fr::TransmissionMode& obj)
{
    auto&& str = parse_as<std::string>(node);
    if (str == "Continuous")
        obj = sim::fr::TransmissionMode::Continuous;
    else if (str == "SingleShot")
        obj = sim::fr::TransmissionMode::SingleShot;
    else
    {
        throw ConversionError(node, "Unknown sim::fr::TransmissionMode: " + str);
    }
    return true;
}

template<>
Node Converter::encode(const FlexRayController& obj)
{
    static const FlexRayController defaultObj{};
    Node node;
    node["Name"] = obj.name;
    non_default_encode(obj.network, node, "Network", defaultObj.network);
    if (obj.clusterParameters.has_value())
    {
        node["ClusterParameters"] = encode(obj.clusterParameters.value());
    }
    if (obj.nodeParameters.has_value())
    {
        node["NodeParameters"] = encode(obj.nodeParameters.value());
    }
    optional_encode(obj.clusterParameters, node, "ClusterParameters");
    optional_encode(obj.nodeParameters, node, "NodeParameters");
    optional_encode(obj.txBufferConfigurations, node, "TxBufferConfigurations");
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template<>
bool Converter::decode(const Node& node, FlexRayController& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    optional_decode(obj.network, node, "Network");
    optional_decode(obj.clusterParameters, node, "ClusterParameters");
    optional_decode(obj.nodeParameters, node, "NodeParameters");
    optional_decode(obj.txBufferConfigurations, node, "TxBufferConfigurations");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template <>
Node Converter::encode(const DataPublisher& obj)
{
    static const DataPublisher defaultObj{};
    Node node;
    node["Name"] = obj.name;
    non_default_encode(obj.network, node, "Network", defaultObj.network);
    //optional_encode(obj.history, node, "History");
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template <>
bool Converter::decode(const Node& node, DataPublisher& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    optional_decode(obj.network, node, "Network");
    //optional_decode(obj.history, node, "Replay");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template <>
Node Converter::encode(const DataSubscriber& obj)
{
    static const DataSubscriber defaultObj{};
    Node node;
    node["Name"] = obj.name;
    non_default_encode(obj.network, node, "Network", defaultObj.network);
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template <>
bool Converter::decode(const Node& node, DataSubscriber& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    optional_decode(obj.network, node, "Network");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template <>
Node Converter::encode(const RpcServer& obj)
{
    static const RpcServer defaultObj{};
    Node node;
    node["Name"] = obj.name;
    non_default_encode(obj.network, node, "Network", defaultObj.network);
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template <>
bool Converter::decode(const Node& node, RpcServer& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    optional_decode(obj.network, node, "Network");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template <>
Node Converter::encode(const RpcClient& obj)
{
    static const RpcClient defaultObj{};
    Node node;
    node["Name"] = obj.name;
    non_default_encode(obj.network, node, "Network", defaultObj.network);
    optional_encode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_encode(obj.replay, node, "Replay");
    return node;
}
template <>
bool Converter::decode(const Node& node, RpcClient& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    optional_decode(obj.network, node, "Network");
    optional_decode(obj.useTraceSinks, node, "UseTraceSinks");
    optional_decode(obj.replay, node, "Replay");
    return true;
}

template <>
Node Converter::encode(const HealthCheck& obj)
{
    Node node;
    optional_encode(obj.softResponseTimeout, node, "SoftResponseTimeout");
    optional_encode(obj.hardResponseTimeout, node, "HardResponseTimeout");
    return node;
}
template <>
bool Converter::decode(const Node& node, HealthCheck& obj)
{
    optional_decode(obj.softResponseTimeout, node, "SoftResponseTimeout");
    optional_decode(obj.hardResponseTimeout, node, "HardResponseTimeout");
    return true;
}

template<>
Node Converter::encode(const Tracing& obj)
{
    static const Tracing defaultObj{};
    Node node;
    optional_encode(obj.traceSinks, node, "TraceSinks");
    optional_encode(obj.traceSources, node, "TraceSources");
    return node;
}
template<>
bool Converter::decode(const Node& node, Tracing& obj)
{
    optional_decode(obj.traceSinks, node, "TraceSinks");
    optional_decode(obj.traceSources, node, "TraceSources");
    return true;
}

template<>
Node Converter::encode(const TraceSink& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["Type"] = obj.type;
    node["OutputPath"] = obj.outputPath;
    // Only serialize if disabled
    //if (!obj.enabled)
    //{
    //    node["Enabled"] = obj.enabled;
    //}

    return node;
}
template<>
bool Converter::decode(const Node& node, TraceSink& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    obj.type = parse_as<decltype(obj.type)>(node["Type"]);
    obj.outputPath = parse_as<decltype(obj.outputPath)>(node["OutputPath"]);
    //if (node["Enabled"])
    //{
    //    obj.enabled = parse_as<decltype(obj.enabled)>(node["Enabled"]);
    //}
    return true;
}

template<>
Node Converter::encode(const TraceSink::Type& obj)
{
    Node node;
    switch (obj)
    {
    case TraceSink::Type::Undefined:
        node = "Undefined";
        break;
    case TraceSink::Type::Mdf4File:
        node = "Mdf4File";
        break;
    case TraceSink::Type::PcapFile:
        node = "PcapFile";
        break;
    case TraceSink::Type::PcapPipe:
        node = "PcapPipe";
        break;
    default:
        throw ConfigurationError{ "Unknown TraceSink Type" };
    }
    return node;
}
template<>
bool Converter::decode(const Node& node, TraceSink::Type& obj)
{
    auto&& str = parse_as<std::string>(node);
    if (str == "Undefined" || str == "")
        obj = TraceSink::Type::Undefined;
    else if (str == "Mdf4File")
        obj = TraceSink::Type::Mdf4File;
    else if (str == "PcapFile")
        obj = TraceSink::Type::PcapFile;
    else if (str == "PcapPipe")
        obj = TraceSink::Type::PcapPipe;
    else
    {
        throw ConversionError(node, "Unknown TraceSink::Type: " + str + ".");
    }
    return true;
}

template<>
Node Converter::encode(const TraceSource& obj)
{
    Node node;
    node["Name"] = obj.name;
    node["Type"] = obj.type;
    node["InputPath"] = obj.inputPath;
    // Only serialize if disabled
    //if (!obj.enabled)
    //{
    //    node["Enabled"] = obj.enabled;
    //}
    return node;
}
template<>
bool Converter::decode(const Node& node, TraceSource& obj)
{
    obj.name = parse_as<std::string>(node["Name"]);
    obj.type = parse_as<decltype(obj.type)>(node["Type"]);
    obj.inputPath = parse_as<decltype(obj.inputPath)>(node["InputPath"]);
    //if (node["Enabled"])
    //{
    //    obj.enabled = parse_as<decltype(obj.enabled)>(node["Enabled"]);
    //}
    return true;
}

template<>
Node Converter::encode(const TraceSource::Type& obj)
{
    Node node;
    switch (obj)
    {
    case TraceSource::Type::Undefined:
        node = "Undefined";
        break;
    case TraceSource::Type::Mdf4File:
        node = "Mdf4File";
        break;
    case TraceSource::Type::PcapFile:
        node = "PcapFile";
        break;
    default:
        throw ConfigurationError{ "Unknown TraceSource Type" };
    }
    return node;
}
template<>
bool Converter::decode(const Node& node, TraceSource::Type& obj)
{
    auto&& str = parse_as<std::string>(node);
    if (str == "Undefined" || str == "")
        obj = TraceSource::Type::Undefined;
    else if (str == "Mdf4File")
        obj = TraceSource::Type::Mdf4File;
    else if (str == "PcapFile")
        obj = TraceSource::Type::PcapFile;
    else
    {
        throw ConversionError(node, "Unknown TraceSource::Type: " + str + ".");
    }
    return true;
}

template<>
Node Converter::encode(const Extensions& obj)
{
    static const Extensions defaultObj{};
    Node node;
    non_default_encode(obj.searchPathHints, node, "SearchPathHints", defaultObj.searchPathHints);
    return node;
}
template<>
bool Converter::decode(const Node& node, Extensions& obj)
{
    optional_decode(obj.searchPathHints, node, "SearchPathHints");
    return true;
}

template<>
Node Converter::encode(const Registry& obj)
{
    static const Registry defaultObj;
    Node node;
    non_default_encode(obj.hostname, node, "Hostname", defaultObj.hostname);
    non_default_encode(obj.port, node, "Port", defaultObj.port);
    non_default_encode(obj.logging, node, "Logging", defaultObj.logging);
    non_default_encode(obj.connectAttempts, node, "ConnectAttempts", defaultObj.connectAttempts);
    return node;
}
template<>
bool Converter::decode(const Node& node, Registry& obj)
{
    optional_decode(obj.hostname, node, "Hostname");
    optional_decode(obj.port, node, "Port");
    optional_decode(obj.logging, node, "Logging");
    optional_decode(obj.connectAttempts, node, "ConnectAttempts");

    if (obj.connectAttempts < 1)
    {
        obj.connectAttempts = 1;
    }
    return true;
}

template<>
Node Converter::encode(const Middleware& obj)
{
    Node node;
    static const Middleware defaultObj;
    non_default_encode(obj.registry, node, "Registry", defaultObj.registry);
    non_default_encode(obj.tcpNoDelay, node, "TcpNoDelay", defaultObj.tcpNoDelay);
    non_default_encode(obj.tcpQuickAck, node, "TcpQuickAck", defaultObj.tcpQuickAck);
    non_default_encode(obj.tcpReceiveBufferSize, node, "TcpReceiveBufferSize", defaultObj.tcpReceiveBufferSize);
    non_default_encode(obj.tcpSendBufferSize, node, "TcpSendBufferSize", defaultObj.tcpSendBufferSize);
    non_default_encode(obj.enableDomainSockets, node, "EnableDomainSockets", defaultObj.enableDomainSockets);
    return node;
}
template<>
bool Converter::decode(const Node& node, Middleware& obj)
{
    optional_decode(obj.registry, node, "Registry");
    optional_decode(obj.tcpNoDelay, node, "TcpNoDelay");
    optional_decode(obj.tcpQuickAck, node, "TcpQuickAck");
    optional_decode(obj.tcpReceiveBufferSize, node, "TcpReceiveBufferSize");
    optional_decode(obj.tcpSendBufferSize, node, "TcpSendBufferSize");
    optional_decode(obj.enableDomainSockets, node, "EnableDomainSockets");
    return true;
}

template<>
Node Converter::encode(const ParticipantConfiguration& obj)
{
    static const ParticipantConfiguration defaultObj{};
    Node node;
    node["SchemaVersion"] = obj.schemaVersion;
    node["Description"] = obj.description;
    node["ParticipantName"] = obj.participantName;

    optional_encode(obj.canControllers, node, "CanControllers");
    optional_encode(obj.linControllers, node, "LinControllers");
    optional_encode(obj.ethernetControllers, node, "EthernetControllers");
    optional_encode(obj.flexRayControllers, node, "FlexRayControllers");
    optional_encode(obj.dataPublishers, node, "DataPublishers");
    optional_encode(obj.dataSubscribers, node, "DataSubscribers");
    optional_encode(obj.rpcServers, node, "RpcServers");
    optional_encode(obj.rpcClients, node, "RpcClients");

    non_default_encode(obj.logging, node, "Logging", defaultObj.logging);
    non_default_encode(obj.healthCheck, node, "Extensions", defaultObj.healthCheck);
    non_default_encode(obj.tracing, node, "Extensions", defaultObj.tracing);
    non_default_encode(obj.extensions, node, "Extensions", defaultObj.extensions);
    non_default_encode(obj.middleware, node, "Middleware", defaultObj.middleware);
    return node;
}
template<>
bool Converter::decode(const Node& node, ParticipantConfiguration& obj)
{
    optional_decode(obj.schemaVersion, node, "SchemaVersion");
    optional_decode(obj.description, node, "Description");
    optional_decode(obj.participantName, node, "ParticipantName");

    optional_decode(obj.canControllers, node, "CanControllers");
    optional_decode(obj.linControllers, node, "LinControllers");
    optional_decode(obj.ethernetControllers, node, "EthernetControllers");
    optional_decode(obj.flexRayControllers, node, "FlexRayControllers");
    optional_decode(obj.dataPublishers, node, "DataPublishers");
    optional_decode(obj.dataSubscribers, node, "DataSubscribers");
    optional_decode(obj.rpcServers, node, "RpcServers");
    optional_decode(obj.rpcClients, node, "RpcClients");

    optional_decode(obj.logging, node, "Logging");
    optional_decode(obj.healthCheck, node, "HealthCheck");
    optional_decode(obj.tracing, node, "Tracing");
    optional_decode(obj.extensions, node, "Extensions");
    optional_decode(obj.middleware, node, "Middleware");
    return true;
}

} // namespace YAML
