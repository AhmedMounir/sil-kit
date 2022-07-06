// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "CanController.hpp"
#include "SimBehaviorTrivial.hpp" 
#include "silkit/core/logging/ILogger.hpp"


namespace SilKit {
namespace Services {
namespace Can {

SimBehaviorTrivial::SimBehaviorTrivial(Core::IParticipantInternal* participant, CanController* canController,
                    Core::Orchestration::ITimeProvider* timeProvider)
    : _participant{participant}
    , _parentController{canController}
    , _parentServiceEndpoint{dynamic_cast<Core::IServiceEndpoint*>(canController)}
    , _timeProvider{timeProvider}
{
    (void)_parentController;
}

template <typename MsgT>
void SimBehaviorTrivial::ReceiveSilKitMessage(const MsgT& msg)
{
    auto receivingController = dynamic_cast<Core::IMessageReceiver<MsgT>*>(_parentController);
    assert(receivingController);
    receivingController->ReceiveSilKitMessage(_parentServiceEndpoint, msg);
}

auto SimBehaviorTrivial::AllowReception(const Core::IServiceEndpoint* /*from*/) const -> bool 
{ 
    return true; 
}

void SimBehaviorTrivial::SendMsg(CanConfigureBaudrate&& /*baudRate*/)
{ 
}

void SimBehaviorTrivial::SendMsg(CanSetControllerMode&& mode)
{
    CanControllerStatus newStatus;
    newStatus.timestamp = _timeProvider->Now();
    newStatus.controllerState = mode.mode;

    ReceiveSilKitMessage(newStatus);
}

void SimBehaviorTrivial::SendMsg(CanFrameEvent&& canFrameEvent)
{
    if (_parentController->GetState() == CanControllerState::Started)
    {
        auto now = _timeProvider->Now();
        CanFrameEvent canFrameEventCpy = canFrameEvent;
        canFrameEventCpy.direction = TransmitDirection::TX;
        canFrameEventCpy.timestamp = now;

        _tracer.Trace(SilKit::Services::TransmitDirection::TX, now, canFrameEvent);

        // Self delivery as TX
        ReceiveSilKitMessage(canFrameEventCpy);

        // Send to others as RX
        canFrameEventCpy.direction = TransmitDirection::RX;
        _participant->SendMsg(_parentServiceEndpoint, canFrameEventCpy);

        // Self acknowledge
        CanFrameTransmitEvent ack{};
        ack.canId = canFrameEvent.frame.canId;
        ack.status = CanTransmitStatus::Transmitted;
        ack.transmitId = canFrameEvent.transmitId;
        ack.userContext = canFrameEvent.userContext;
        ack.timestamp = now;

        ReceiveSilKitMessage(ack);
    }
    else
    {
        _participant->GetLogger()->Warn("ICanController::SendFrame is called although can controller is not in state CanController::Started.");
    }
}

} // namespace Can
} // namespace Services
} // namespace SilKit