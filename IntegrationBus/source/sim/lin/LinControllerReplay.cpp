// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "LinControllerReplay.hpp"

#include <iostream>
#include <chrono>

#include "ib/mw/IComAdapter.hpp"
#include "ib/mw/sync/IParticipantController.hpp"
#include "ib/mw/logging/ILogger.hpp"
#include "ib/sim/lin/string_utils.hpp"

namespace {
inline bool IsReplayEnabled(const ib::cfg::Replay& config)
{
    // Replaying on a master node requires send and receive, implicitly.
    return ib::tracing::IsReplayEnabledFor(config, ib::cfg::Replay::Direction::Send)
        || ib::tracing::IsReplayEnabledFor(config, ib::cfg::Replay::Direction::Receive)
        ;
}
}
namespace ib {
namespace sim {
namespace lin {


LinControllerReplay::LinControllerReplay(mw::IComAdapter* comAdapter, cfg::LinController config,
            mw::sync::ITimeProvider* timeProvider)
    : _replayConfig{config.replay}
    , _controller{comAdapter, timeProvider}
    , _comAdapter{comAdapter}
    , _timeProvider{timeProvider}
{
}

void LinControllerReplay::Init(ControllerConfig config)
{
    // Replaying:
    // We explicitly rely on the Master/Slave controllers to properly 
    // initialize as part of the user's application code.

    _mode = config.controllerMode;
    _controller.Init(config);

    // Replaying is only supported on a master node.
    if (_mode == ControllerMode::Slave && IsReplayEnabled(_replayConfig))
    {
        _comAdapter->GetLogger()->Warn("Replaying on a slave controller is not supported! "
            "Please use tracing and replay on a master controller!");
        throw std::runtime_error("Replaying is not supported on Slave controllers!");
    }
}

auto LinControllerReplay::Status() const noexcept -> ControllerStatus
{
    return _controller.Status();
}

void LinControllerReplay::SendFrame(Frame, FrameResponseType)
{
    // SendFrame is an API only used by a master, we ensure that the API
    // is not called during a replay. That is, we don't support mixing
    // replay frames and user-supplied frames.
    _comAdapter->GetLogger()->Debug("Replaying: ignoring call to {}.", __FUNCTION__);
    return;
}

void LinControllerReplay::SendFrame(Frame , FrameResponseType , std::chrono::nanoseconds)
{
    // We don't allow mixing user API calls while replaying on a master.
    _comAdapter->GetLogger()->Debug("Replaying: ignoring call to {}.", __FUNCTION__);
    return;
}

void LinControllerReplay::SendFrameHeader(LinIdT)
{
    // We don't allow mixing user API calls while replaying.
    _comAdapter->GetLogger()->Debug("Replaying: ignoring call to {}.", __FUNCTION__);
    return;
}

void LinControllerReplay::SendFrameHeader(LinIdT, std::chrono::nanoseconds)
{
    // We don't allow mixing user API calls while replaying.
    _comAdapter->GetLogger()->Debug("Replaying: ignoring call to {}.", __FUNCTION__);
    return;
}

void LinControllerReplay::SetFrameResponse(Frame, FrameResponseMode)
{
    // We don't allow mixing user API calls while replaying.
    _comAdapter->GetLogger()->Debug("Replaying: ignoring call to {}.", __FUNCTION__);
    return;
}

void LinControllerReplay::SetFrameResponses(std::vector<FrameResponse>)
{
    // we don't allow mixing user API calls while replaying
    _comAdapter->GetLogger()->Debug("Replaying: ignoring call to {}.", __FUNCTION__);
    return;
}

void LinControllerReplay::GoToSleep()
{
    // We rely on the master being able to send sleep frames.
    _controller.GoToSleep();
}

void LinControllerReplay::GoToSleepInternal()
{
    // We rely on the master being able to send sleep frames.
    _controller.GoToSleepInternal();
}

void LinControllerReplay::Wakeup()
{
    // Wakeup Pulses are not part of the replay, so we rely on the application's
    // cooperation when waking from sleep, i.e. we do allow API calls here!
    _controller.Wakeup();
}

void LinControllerReplay::WakeupInternal()
{
    // Wakeup Pulses are not part of the replay, so we rely on the application's
    // cooperation when waking from sleep.
    _controller.WakeupInternal();
}

void LinControllerReplay::RegisterFrameStatusHandler(FrameStatusHandler handler)
{
    // Frame status callbacks might be triggered from a master node when doing a replay.
    // Thus, we handle them directly.
    _frameStatusHandler.emplace_back(std::move(handler));
}

void LinControllerReplay::RegisterGoToSleepHandler(GoToSleepHandler handler)
{
    // We call sleep handlers directly, since sleep frames might originate from a replay.
    _goToSleepHandler.emplace_back(std::move(handler));
}

void LinControllerReplay::RegisterWakeupHandler(WakeupHandler handler)
{
    // Wakeup Pulses are not part of the replay, so we rely on the application's
    // cooperation when waking from sleep.
    _controller.RegisterWakeupHandler(std::move(handler));
}

void LinControllerReplay::RegisterFrameResponseUpdateHandler(FrameResponseUpdateHandler handler)
{
    // FrameResponseUpdates are not part of the replay, we recreate them based on replay data.
    _controller.RegisterFrameResponseUpdateHandler(std::move(handler));
}

void LinControllerReplay::ReceiveIbMessage(ib::mw::EndpointAddress from, const Transmission& msg)
{
    // Transmissions are always issued by a master.
    _controller.ReceiveIbMessage(from, msg);
}

void LinControllerReplay::ReceiveIbMessage(ib::mw::EndpointAddress from, const WakeupPulse& msg)
{
    //Wakeup pulses are not part of a replay, but are valid during a replay.
    _controller.ReceiveIbMessage(from, msg);
}

void LinControllerReplay::ReceiveIbMessage(mw::EndpointAddress from, const ControllerConfig& msg)
{
    // ControllerConfigs are not part of a replay, but are valid during a replay.
    _controller.ReceiveIbMessage(from, msg);
}

void LinControllerReplay::ReceiveIbMessage(mw::EndpointAddress from, const FrameResponseUpdate& msg)
{
    // FrameResponseUpdates are generated from a master during a replay.
    _controller.ReceiveIbMessage(from, msg);
}

void LinControllerReplay::ReceiveIbMessage(mw::EndpointAddress from, const ControllerStatusUpdate& msg)
{
    // ControllerStatupsUpdates are not part of a replay, but are valid during a replay.
    _controller.ReceiveIbMessage(from, msg);
}

void LinControllerReplay::SetEndpointAddress(const ::ib::mw::EndpointAddress& endpointAddress)
{
    _controller.SetEndpointAddress(endpointAddress);
}

auto LinControllerReplay::EndpointAddress() const -> const ::ib::mw::EndpointAddress&
{
    return _controller.EndpointAddress();
}

// ITraceMessageSource
void LinControllerReplay::AddSink(extensions::ITraceMessageSink* sink)
{
    // NB: Tracing in _controller is never reached as a Master, because we send with its endpoint address in ReplayMessage
    _controller.AddSink(sink);
    // for active replaying we use our own tracer:
    _tracer.AddSink(EndpointAddress(), *sink);
}

// IReplayDataProvider

void LinControllerReplay::ReplayMessage(const extensions::IReplayMessage* replayMessage)
{
    switch (replayMessage->GetDirection())
    {
    case extensions::Direction::Send:
    case extensions::Direction::Receive:
        break;
    default:
        throw std::runtime_error("LinControllerReplay: replay message has undefined Direction");
        break;
    }

    // The Frame Response Updates ensures that all controllers have the same notion of the
    // response that is going to be generated by a slave.

    auto frame = dynamic_cast<const Frame&>(*replayMessage);
    auto mode = (replayMessage->GetDirection() == extensions::Direction::Receive)
        ? FrameResponseMode::Rx
        : FrameResponseMode::TxUnconditional;

    _tracer.Trace(replayMessage->GetDirection(), _timeProvider->Now(), frame);

    FrameResponse response;
    response.frame = frame;
    response.responseMode = mode;
    FrameResponseUpdate responseUpdate;
    responseUpdate.frameResponses.emplace_back(std::move(response));
    _comAdapter->SendIbMessage(replayMessage->EndpointAddress(), responseUpdate);

    if (_mode == ControllerMode::Master)
    {
        // When we are a master, also synthesize the frame header (IB type Transmission) based on the replay data.
        // NB: the actual transmission is always in RX-direction, only the callback handlers will see the actual
        //     direction.
        Transmission tm{};
        tm.timestamp = replayMessage->Timestamp();
        tm.frame = std::move(frame);
        tm.status = FrameStatus::LIN_RX_OK;
        _comAdapter->SendIbMessage(replayMessage->EndpointAddress(), tm);

        FrameStatus masterFrameStatus = tm.status;
        if (mode == FrameResponseMode::TxUnconditional)
        {
            masterFrameStatus = FrameStatus::LIN_TX_OK;
        }
        // dispatch local frame status handlers
        if (replayMessage->EndpointAddress() == _controller.EndpointAddress())
        {
            for (auto& handler : _frameStatusHandler)
            {
                handler(this, tm.frame, masterFrameStatus, tm.timestamp);
            }
        }
        // dispatch sleep handler
        if (tm.frame.id == GoToSleepFrame().id && tm.frame.data == GoToSleepFrame().data)
        {
            for (auto& handler : _goToSleepHandler)
            {
                handler(this);
            }
        }
    }
}

} // namespace lin
} // namespace sim
} // namespace ib
