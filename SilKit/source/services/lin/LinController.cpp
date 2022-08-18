/* Copyright (c) 2022 Vector Informatik GmbH

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "LinController.hpp"

#include <iostream>
#include <chrono>

#include "silkit/services/logging/ILogger.hpp"
#include "silkit/services/lin/string_utils.hpp"
#include "IServiceDiscovery.hpp"
#include "ServiceDatatypes.hpp"

namespace SilKit {
namespace Services {
namespace Lin {

LinController::LinController(Core::IParticipantInternal* participant, SilKit::Config::LinController config,
                               Services::Orchestration::ITimeProvider* timeProvider)
    : _participant{participant}
    , _config{config}
    , _logger{participant->GetLogger()}
    , _simulationBehavior{participant, this, timeProvider}
    , _timeProvider{timeProvider}
{
}

//------------------------
// Trivial or detailed
//------------------------

void LinController::RegisterServiceDiscovery()
{
    _participant->GetServiceDiscovery()->RegisterServiceDiscoveryHandler(
        [this](Core::Discovery::ServiceDiscoveryEvent::Type discoveryType,
                                  const Core::ServiceDescriptor& remoteServiceDescriptor) {
            // check if discovered service is a network simulator (if none is known)
            if (_simulationBehavior.IsTrivial())
            {
                // check if received descriptor has a matching simulated link
                if (discoveryType == Core::Discovery::ServiceDiscoveryEvent::Type::ServiceCreated
                    && IsRelevantNetwork(remoteServiceDescriptor))
                {
                    SetDetailedBehavior(remoteServiceDescriptor);
                }
            }
            else
            {
                if (discoveryType == Core::Discovery::ServiceDiscoveryEvent::Type::ServiceRemoved
                    && IsRelevantNetwork(remoteServiceDescriptor))
                
                {
                    SetTrivialBehavior();
                }
            }
        });
}

void LinController::SetDetailedBehavior(const Core::ServiceDescriptor& remoteServiceDescriptor)
{
    _simulationBehavior.SetDetailedBehavior(remoteServiceDescriptor);
}

void LinController::SetTrivialBehavior()
{
    _simulationBehavior.SetTrivialBehavior();
}

auto LinController::AllowReception(const IServiceEndpoint* from) const -> bool
{
    return _simulationBehavior.AllowReception(from);
}

auto LinController::IsRelevantNetwork(const Core::ServiceDescriptor& remoteServiceDescriptor) const -> bool
{
    return remoteServiceDescriptor.GetServiceType() == SilKit::Core::ServiceType::Link
           && remoteServiceDescriptor.GetNetworkName() == _serviceDescriptor.GetNetworkName();
}

template <typename MsgT>
void LinController::SendMsg(MsgT&& msg)
{
    _simulationBehavior.SendMsg(std::move(msg));
}

//------------------------
// Error handling
//------------------------

void LinController::ThrowIfUninitialized(const std::string& callingMethodName) const
{
    if (_controllerStatus == LinControllerStatus::Unknown)
    {
        std::string errorMsg = callingMethodName
                               + " must only be called when the controller is initialized! Check "
                                 "whether a call to LinController::Init is missing.";
        _logger->Error(errorMsg);
        throw SilKit::StateError{errorMsg};
    }
}

void LinController::ThrowIfNotMaster(const std::string& callingMethodName) const
{
    if (_controllerMode != LinControllerMode::Master)
    {
        std::string errorMsg = callingMethodName
                               + " must only be called in master mode!";
        _logger->Error(errorMsg);
        throw SilKitError{errorMsg};
    }
}

void LinController::ThrowIfNotConfiguredTxUnconditional(LinId linId)
{
    if (GetThisLinNode().responses[linId].responseMode != LinFrameResponseMode::TxUnconditional)
    {
        std::string errorMsg = fmt::format("This node must be configured with LinFrameResponseMode::TxUnconditional to "
                                           "update the TxBuffer for ID {}",
                                           static_cast<uint16_t>(linId));
        _logger->Error(errorMsg);
        throw SilKit::ConfigurationError{errorMsg};
    }
}

void LinController::WarnOnWrongDataLength(const LinFrame& receivedFrame, const LinFrame& configuredFrame) const
{
    std::string errorMsg =
        fmt::format("Mismatch between configured ({}) and received ({}) LinDataLength in LinFrame with ID {}",
                    configuredFrame.dataLength, receivedFrame.dataLength, static_cast<uint16_t>(receivedFrame.id));
    _logger->Warn(errorMsg);
}

void LinController::WarnOnWrongChecksum(const LinFrame& receivedFrame, const LinFrame& configuredFrame) const
{
    std::string errorMsg = fmt::format(
        "Mismatch between configured ({}) and received ({}) LinChecksumModel in LinFrame with ID {}",
        configuredFrame.checksumModel, receivedFrame.checksumModel, static_cast<uint16_t>(receivedFrame.id));
    _logger->Warn(errorMsg);
}

void LinController::WarnOnSendAttemptWithUndefinedChecksum(const LinFrame& frame) const
{
    std::string errorMsg =
        fmt::format("LinFrame with ID {} has an undefined checksum model, the transmission is rejected.",
                    static_cast<uint16_t>(frame.id));
    _logger->Warn(errorMsg);
}

void LinController::WarnOnOverwriteOfUnconfiguredChecksum(const LinFrame& frame) const
{
    std::string errorMsg = fmt::format("LinController received transmission but has configured "
                                       "LinChecksumModel::Undefined. Overwriting with {} for LinId {}.",
                                       frame.checksumModel, static_cast<uint16_t>(frame.id));
    _logger->Warn(errorMsg);
}

void LinController::WarnOnReceptionWithInvalidDataLength(LinDataLength invalidDataLength,
                                                         const std::string& fromParticipantName,
                                                         const std::string& fromServiceName) const
{
    std::string errorMsg =
        fmt::format("LinController received transmission with invalid payload length {} from {{{}, {}}}. This "
                    "tranmission is ignored.",
                    static_cast<uint16_t>(invalidDataLength), fromParticipantName, fromServiceName);
    _logger->Warn(errorMsg);
}

void LinController::WarnOnReceptionWithInvalidLinId(LinId invalidLinId, const std::string& fromParticipantName,
                                                    const std::string& fromServiceName) const
{
    std::string errorMsg = fmt::format(
        "LinController received transmission with invalid LIN ID {} from {{{}, {}}}. This transmission is ignored.",
        static_cast<uint16_t>(invalidLinId), fromParticipantName, fromServiceName);
    _logger->Warn(errorMsg);
}

void LinController::WarnOnReceptionWhileInactive() const
{
    std::string errorMsg = fmt::format("Inactive LinController received a transmission. This transmission is ignored.");
    _logger->Warn(errorMsg);
}

void LinController::WarnOnUnneededStatusChange(LinControllerStatus status) const
{
    std::string errorMsg =
        fmt::format("Invalid LinController status change: controller is already in {} mode.", to_string(status));
    _logger->Warn(errorMsg);
}

void LinController::ThrowOnErroneousInitialization() const
{
    std::string errorMsg{"A LinController can't be initialized with LinControllerMode::Inactive!"};
    _logger->Error(errorMsg);
    throw SilKit::StateError{errorMsg};
}

void LinController::ThrowOnDuplicateInitialization() const
{
    std::string errorMsg{"LinController::Init() must only be called once!"};
    _logger->Error(errorMsg);
    throw SilKit::StateError{errorMsg};
}

//------------------------
// Public API
//------------------------

void LinController::Init(LinControllerConfig config)
{
    if (config.controllerMode == LinControllerMode::Inactive)
    {
        ThrowOnErroneousInitialization();
    }

    if (_controllerStatus != LinControllerStatus::Unknown)
    {
        ThrowOnDuplicateInitialization();
    }

    auto& node = GetThisLinNode();
    node.controllerMode = config.controllerMode;
    node.controllerStatus = LinControllerStatus::Operational;
    node.UpdateResponses(config.frameResponses, _logger);

    _controllerMode = config.controllerMode;
    _controllerStatus = LinControllerStatus::Operational;
    SendMsg(config);
}

auto LinController::Status() const noexcept -> LinControllerStatus
{
    return _controllerStatus;
}

void LinController::SendFrame(LinFrame frame, LinFrameResponseType responseType)
{
    ThrowIfUninitialized(__FUNCTION__);
    ThrowIfNotMaster(__FUNCTION__);

    if (responseType == LinFrameResponseType::MasterResponse)
    {
        // Update local response reconfiguration
        if (GetThisLinNode().responses[frame.id].responseMode != LinFrameResponseMode::TxUnconditional)
        {
            std::vector<LinFrameResponse> responseUpdate;
            LinFrameResponse response{};
            response.frame = frame;
            response.responseMode = LinFrameResponseMode::TxUnconditional;
            responseUpdate.push_back(response);
            GetThisLinNode().UpdateResponses(responseUpdate, _logger);
            _simulationBehavior.UpdateTxBuffer(frame);
        }
    }
    else
    {
        // Only allow SendFrame of unconfigured LIN Ids for LinFrameResponseType::MasterResponse
        // that LinSlaveConfigurationHandler and GetSlaveConfiguration stays valid.
        if (!HasRespondingSlave(frame.id))
        {
            CallLinFrameStatusEventHandler(
                LinFrameStatusEvent{_timeProvider->Now(), frame, LinFrameStatus::LIN_RX_NO_RESPONSE});
            return;
        }
        
        if (responseType == LinFrameResponseType::SlaveResponse)
        {
            // As the master, we configure for RX in case of SlaveResponse
            std::vector<LinFrameResponse> responseUpdate;
            LinFrameResponse response{};
            response.frame = frame;
            response.responseMode = LinFrameResponseMode::Rx;
            responseUpdate.push_back(response);
            GetThisLinNode().UpdateResponses(responseUpdate, _logger);
        }
    }

    // Detailed: Send LinSendFrameRequest to BusSim
    // Trivial: SendFrameHeader
    SendMsg(LinSendFrameRequest{frame, responseType});
}

void LinController::SendFrameHeader(LinId linId)
{
    ThrowIfUninitialized(__FUNCTION__);
    ThrowIfNotMaster(__FUNCTION__);

    // Detailed: Send LinSendFrameHeaderRequest to BusSim
    // Trivial: Good case (numResponses == 1): Distribute LinSendFrameHeaderRequest, the receiving Tx-Node will generate the LinTransmission.
    //          Error case: Generate the LinTransmission and trigger a FrameStatusUpdate with 
    //                      LIN_RX_NO_RESPONSE (numResponses == 0) or LIN_RX_ERROR (numResponses > 1).
    SendMsg(LinSendFrameHeaderRequest{_timeProvider->Now(), linId});
}

void LinController::UpdateTxBuffer(LinFrame frame)
{
    ThrowIfUninitialized(__FUNCTION__);
    ThrowIfNotConfiguredTxUnconditional(frame.id);

    // Update the local payload
    GetThisLinNode().UpdateTxBuffer(frame.id, std::move(frame.data), _logger);

    // Detailed: Send LinFrameResponseUpdate with updated payload to BusSim
    // Trivial: Nop
    _simulationBehavior.UpdateTxBuffer(frame);
}

void LinController::GoToSleep()
{
    ThrowIfUninitialized(__FUNCTION__);
    ThrowIfNotMaster(__FUNCTION__);

    // Detailed: Send LinSendFrameRequest with GoToSleep-Frame and set LinControllerStatus::SleepPending. BusSim will trigger LinTransmission.
    // Trivial: Directly send LinTransmission with GoToSleep-Frame and call GoToSleepInternal() on this controller.
    _simulationBehavior.GoToSleep();

    _controllerStatus = LinControllerStatus::Sleep;
}

void LinController::GoToSleepInternal()
{
    ThrowIfUninitialized(__FUNCTION__);

    SetControllerStatusInternal(LinControllerStatus::Sleep);
}

void LinController::Wakeup()
{
    ThrowIfUninitialized(__FUNCTION__);

    // Detailed: Send LinWakeupPulse and call WakeupInternal()
    // Trivial: Send LinWakeupPulse and call WakeupInternal(), self-deliver LinWakeupPulse with TX
    _simulationBehavior.Wakeup();
}

void LinController::WakeupInternal()
{
    ThrowIfUninitialized(__FUNCTION__);

    SetControllerStatusInternal(LinControllerStatus::Operational);
}

LinSlaveConfiguration LinController::GetSlaveConfiguration()
{
    ThrowIfNotMaster(__FUNCTION__);

    return LinSlaveConfiguration{_linIdsRespondedBySlaves};
}

//------------------------
// Helpers
//------------------------

bool LinController::HasRespondingSlave(LinId id)
{
    auto it = std::find(_linIdsRespondedBySlaves.begin(), _linIdsRespondedBySlaves.end(), id);
    return it != _linIdsRespondedBySlaves.end();
}

void LinController::UpdateLinIdsRespondedBySlaves(const std::vector<LinFrameResponse>& responsesUpdate)
{
    for (auto&& response : responsesUpdate)
    {
        if (response.responseMode == LinFrameResponseMode::TxUnconditional)
        {
            if (!HasRespondingSlave(response.frame.id))
            {
                _linIdsRespondedBySlaves.push_back(response.frame.id);
            }
        }
    }
}

void LinController::SetControllerStatusInternal(LinControllerStatus status)
{
    if (_controllerStatus == status)
    {
        WarnOnUnneededStatusChange(status);
    }

    _controllerStatus = status;

    LinControllerStatusUpdate msg;
    msg.status = status;

    SendMsg(msg);
}

//------------------------
// Node bookkeeping
//------------------------

void LinController::LinNode::UpdateResponses(std::vector<LinFrameResponse> responsesToUpdate, Services::Logging::ILogger* logger)
{
    for (auto&& response : responsesToUpdate)
    {
        auto linId = response.frame.id;
        if (linId >= responses.size())
        {
            logger->Warn("Ignoring LinFrameResponse update for invalid ID={}", static_cast<uint16_t>(linId));
            continue;
        }
        responses[linId] = std::move(response);
   }
}

void LinController::LinNode::UpdateTxBuffer(LinId linId, std::array<uint8_t, 8> data,
                                            Services::Logging::ILogger* logger)
{
    if (linId >= responses.size())
    {
        logger->Warn("Ignoring LinFrameResponse update for invalid ID={}", static_cast<uint16_t>(linId));
        return;
    }
    responses[linId].frame.data = data;
}

auto LinController::GetThisLinNode() -> LinNode&
{
    return GetLinNode(_serviceDescriptor.to_endpointAddress());
}

auto LinController::GetLinNode(Core::EndpointAddress addr) -> LinNode&
{
    auto iter = std::lower_bound(_linNodes.begin(), _linNodes.end(), addr,
                                 [](const LinNode& lhs, const Core::EndpointAddress& address) {
                                     return lhs.address < address;
                                 });
    if (iter == _linNodes.end() || iter->address != addr)
    {
        LinNode node;
        node.address = addr;
        iter = _linNodes.insert(iter, node);
    }
    return *iter;
}

void LinController::CallLinFrameStatusEventHandler(const LinFrameStatusEvent& msg)
{
    // Trivial: Used to dispatch the LinFrameStatusEvent locally
    CallHandlers(msg);
}

auto LinController::GetResponse(LinId id) -> std::pair<int, LinFrame>
{
    LinFrame responseFrame;
    responseFrame.id = id;

    auto numResponses = 0;
    for (auto&& node : _linNodes)
    {
        if (node.controllerMode == LinControllerMode::Inactive)
            continue;
        if (node.controllerStatus != LinControllerStatus::Operational)
            continue;

        auto& response = node.responses[id];
        if (response.responseMode == LinFrameResponseMode::TxUnconditional)
        {
            responseFrame = response.frame;
            numResponses++;
        }
    }

    return {numResponses, responseFrame};
}


//------------------------
// ReceiveMsg
//------------------------

void LinController::ReceiveMsg(const IServiceEndpoint* from, const LinSendFrameHeaderRequest& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    // Detailed: Depends on how LinSendFrameHeaderRequest will work with BusSim, currently NOP
    // Trivial: Generate LinTransmission
    // In future: Also Trigger OnHeaderCallback
    _simulationBehavior.ReceiveFrameHeaderRequest(msg);
}

void LinController::ReceiveMsg(const IServiceEndpoint* from, const LinTransmission& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    if (_controllerMode == LinControllerMode::Inactive)
    {
        WarnOnReceptionWhileInactive();
        return;
    }
    
    auto& frame = msg.frame;

    if (frame.dataLength > _maxDataLength)
    {
        WarnOnReceptionWithInvalidDataLength(frame.dataLength, from->GetServiceDescriptor().GetParticipantName(),
                                             from->GetServiceDescriptor().GetServiceName());
        return;
    }

    if (frame.id >= _maxLinId)
    {
        WarnOnReceptionWithInvalidLinId(frame.id, from->GetServiceDescriptor().GetParticipantName(),
                                             from->GetServiceDescriptor().GetServiceName());
        return;
    }

    _tracer.Trace(SilKit::Services::TransmitDirection::RX, msg.timestamp, frame);

    bool isGoToSleepFrame = frame.id == GoToSleepFrame().id && frame.data == GoToSleepFrame().data;

    // If configured for RX, update undefined checksum
    auto& response = GetThisLinNode().responses[frame.id];
    if (!isGoToSleepFrame
        && response.responseMode == LinFrameResponseMode::Rx
        && response.frame.checksumModel == LinChecksumModel::Undefined)
    {
        WarnOnOverwriteOfUnconfiguredChecksum(frame);
        response.frame.checksumModel = msg.frame.checksumModel;
    }

    // Detailed: Just use msg.status
    // Trivial: Evaluate status using cached response
    const LinFrameStatus msgStatus = _simulationBehavior.CalcFrameStatus(msg, isGoToSleepFrame);

    // Only use LIN_RX_NO_RESPONSE on locally triggered LinFrameStatusEvent on erroneous SendFrame/SendFrameHeader,
    // not if received from remote. 
    if (msgStatus != LinFrameStatus::LIN_RX_NO_RESPONSE)
    {
        // Dispatch frame to handlers
        CallHandlers(LinFrameStatusEvent{msg.timestamp, frame, msgStatus});
    }

    // Dispatch GoToSleep frames to dedicated handlers
    if (isGoToSleepFrame)
    {
        // only call GoToSleepHandlers for slaves, i.e., not for the master that issued the GoToSleep command.
        if (_controllerMode == LinControllerMode::Slave)
        {
            CallHandlers(LinGoToSleepEvent{msg.timestamp});
        }
    }
}

void LinController::ReceiveMsg(const IServiceEndpoint* from, const LinWakeupPulse& msg)
{
    if (!AllowReception(from))
    {
        return;
    }

    CallHandlers(LinWakeupEvent{msg.timestamp, msg.direction});
}

void LinController::ReceiveMsg(const IServiceEndpoint* from, const LinControllerConfig& msg)
{
    // NOTE: Self-delivered messages are rejected
    if (from->GetServiceDescriptor() == _serviceDescriptor)
        return;

    auto& linNode = GetLinNode(from->GetServiceDescriptor().to_endpointAddress());
    linNode.controllerMode = msg.controllerMode;
    linNode.controllerStatus = LinControllerStatus::Operational;
    linNode.UpdateResponses(msg.frameResponses, _logger);

    if (msg.controllerMode == LinControllerMode::Slave)
    {
        UpdateLinIdsRespondedBySlaves(msg.frameResponses);
    }
    if (_controllerMode == LinControllerMode::Master)
    {
        auto& callbacks = std::get<CallbacksT<LinSlaveConfigurationEvent>>(_callbacks);

        auto receptionTime = _timeProvider->Now();
        if (callbacks.Size() == 0)
        {
            // No handlers yet, but received a LinSlaveConfiguration -> trigger upon handler addition
            _triggerLinSlaveConfigurationHandlers = true;
            _receptionTimeLinSlaveConfiguration = receptionTime;
        }
        CallHandlers(LinSlaveConfigurationEvent{receptionTime});
    }
}

void LinController::ReceiveMsg(const IServiceEndpoint* from, const LinControllerStatusUpdate& msg)
{
    auto& linNode = GetLinNode(from->GetServiceDescriptor().to_endpointAddress());
    linNode.controllerStatus = msg.status;
}

//------------------------
// Handlers
//------------------------

HandlerId LinController::AddFrameStatusHandler(FrameStatusHandler handler)
{
    return AddHandler(std::move(handler));
}

void LinController::RemoveFrameStatusHandler(HandlerId handlerId)
{
    if (!RemoveHandler<LinFrameStatusEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveFrameStatusHandler failed: Unknown HandlerId.");
    }
}

HandlerId LinController::AddGoToSleepHandler(GoToSleepHandler handler)
{
    return AddHandler(std::move(handler));
}

void LinController::RemoveGoToSleepHandler(HandlerId handlerId)
{
    if (!RemoveHandler<LinGoToSleepEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveGoToSleepHandler failed: Unknown HandlerId.");
    }
}

HandlerId LinController::AddWakeupHandler(WakeupHandler handler)
{
    return AddHandler(std::move(handler));
}

void LinController::RemoveWakeupHandler(HandlerId handlerId)
{
    if (!RemoveHandler<LinWakeupEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveWakeupHandler failed: Unknown HandlerId.");
    }
}

HandlerId LinController::AddLinSlaveConfigurationHandler(LinSlaveConfigurationHandler handler)
{
    auto handlerId = AddHandler(std::move(handler));

    // Trigger handler if a LinSlaveConfigurations was received before adding a handler
    // No need to cache the LinSlaveConfigs (just the reception time), 
    // as the user has to actively call GetSlaveConfiguration in the callback
    if (_triggerLinSlaveConfigurationHandlers)
    {
        _triggerLinSlaveConfigurationHandlers = false;
        CallHandlers(LinSlaveConfigurationEvent{_receptionTimeLinSlaveConfiguration});
    }
    return handlerId;
}

void LinController::RemoveLinSlaveConfigurationHandler(HandlerId handlerId)
{
    if (!RemoveHandler<LinSlaveConfigurationEvent>(handlerId))
    {
        _participant->GetLogger()->Warn("RemoveLinSlaveConfigurationHandler failed: Unknown HandlerId.");
    }
}

template <typename MsgT>
HandlerId LinController::AddHandler(CallbackT<MsgT> handler)
{
    auto& callbacks = std::get<CallbacksT<MsgT>>(_callbacks);
    return callbacks.Add(std::move(handler));
}

template <typename MsgT>
auto LinController::RemoveHandler(HandlerId handlerId) -> bool
{
    auto& callbacks = std::get<CallbacksT<MsgT>>(_callbacks);
    return callbacks.Remove(handlerId);
}

template <typename MsgT>
void LinController::CallHandlers(const MsgT& msg)
{
    auto& callbacks = std::get<CallbacksT<MsgT>>(_callbacks);
    callbacks.InvokeAll(this, msg);
}

} // namespace Lin
} // namespace Services
} // namespace SilKit
