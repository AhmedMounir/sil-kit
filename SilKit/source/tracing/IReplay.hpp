// Copyright (c) Vector Informatik GmbH. All rights reserved.
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <map>
#include <cstdint>
#include <iterator>

#include "silkit/core/logging/ILogger.hpp"
#include "silkit/services/datatypes.hpp"

#include "TraceMessage.hpp"
#include "ITraceMessageSink.hpp" //for 'enum class Direction'
#include "EndpointAddress.hpp"

namespace SilKit {

//forwards
class IReplayMessage;
class IReplayChannel;
class IReplayFile;

//for use in DLL
class IReplayDataProvider
{
public:
    virtual ~IReplayDataProvider() = default;
    //!< Pass the config (containing search path hints), the actual file to open
    //   and a logger to the extension.
    // TODO
    virtual auto OpenFile(/*const SilKit::Config::Config& config,*/
        const std::string& filePath,
        SilKit::Core::Logging::ILogger* logger) -> std::shared_ptr<IReplayFile> = 0;
};

class IReplayFile
{
public:
    enum class FileType
    {
        PcapFile,
        Mdf4File
    };

    virtual ~IReplayFile() = default;
    //! Get the filesystem path of the replay file
    virtual auto FilePath() const -> const std::string& = 0;
    //! Returns embedded SILKIT config or empty string for non-SILKIT replay files
    virtual auto SilKitConfig() const -> std::string = 0;

    //! Returns the file format type
    virtual FileType Type() const = 0;

    //! Use iterators to get access to the data channels of the file
    virtual std::vector<std::shared_ptr<IReplayChannel>>::iterator begin() = 0;
    virtual std::vector<std::shared_ptr<IReplayChannel>>::iterator end() = 0;
};

//! Interface shared among all ReplayMessage types.

//! Use a dynamic_cast to SILKIT message type to get actual data
class IReplayMessage
{
public:
    virtual ~IReplayMessage() = default;

    //! The timestamp associated with the replay message.
    virtual auto Timestamp() const -> std::chrono::nanoseconds = 0;
    //! The recorded direction of the replay message.
    virtual auto GetDirection() const -> SilKit::Services::TransmitDirection = 0;
    //! The endpoint address of the recording service.
    //! If unavailable from the underlying ReplayChannel, default value is returned.
    virtual auto EndpointAddress() const -> SilKit::Core::EndpointAddress = 0;
    //! Get the replay messages type, which is similar to the type used during tracing.
    virtual auto Type() const -> SilKit::TraceMessageType = 0;
};

class IReplayChannelReader
{
public:
    virtual ~IReplayChannelReader() = default;

    //! seek to the given message number, relative to current position
    virtual bool Seek(size_t messageNumber) = 0;
    virtual std::shared_ptr<IReplayMessage> Read() = 0;
};


class IReplayChannel
{
public:
    virtual ~IReplayChannel() = default;

    // Stream information
    //! Generic infos
    virtual auto Type() const -> SilKit::TraceMessageType = 0;
    virtual auto StartTime() const -> std::chrono::nanoseconds = 0;
    virtual auto EndTime() const -> std::chrono::nanoseconds = 0;
    virtual auto NumberOfMessages() const -> uint64_t = 0;
    //! a unique name suitable to identify this channel within its parent ReplayFile
    virtual auto Name() const -> const std::string& = 0;
    //!  File format specific meta data
    virtual auto GetMetaInfos() const -> const std::map<std::string, std::string>& = 0;

    //!  Get a reader instance that allows reading through the channel sequentially.
    virtual auto GetReader() -> std::shared_ptr<IReplayChannelReader> = 0;
};



} //end namespace SilKit