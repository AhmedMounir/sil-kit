// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include "IMsgForEthController.hpp"
#include "IParticipantInternal.hpp"
#include "ITraceMessageSource.hpp"

#include "ISimBehavior.hpp"

namespace SilKit {
namespace Services {
namespace Ethernet {

class EthController;

class SimBehaviorDetailed : public ISimBehavior
{
public:
    SimBehaviorDetailed(Core::IParticipantInternal* participant, EthController* ethController,
                       const Core::ServiceDescriptor& serviceDescriptor);

    void SendMsg(EthernetFrameEvent&& msg) override;
    void SendMsg(EthernetSetMode&& msg) override;
    void OnReceiveAck(const EthernetFrameTransmitEvent& msg) override;
    
    auto AllowReception(const Core::IServiceEndpoint* from) const -> bool override;

    void SetSimulatedLink(const Core::ServiceDescriptor& simulatedLink);

private:
    template <typename MsgT>
    void SendMsgImpl(MsgT&& msg);

    Core::IParticipantInternal* _participant{nullptr};
    const Core::IServiceEndpoint* _parentServiceEndpoint{nullptr};
    const Core::ServiceDescriptor* _parentServiceDescriptor{nullptr};
    Core::ServiceDescriptor _simulatedLink;
    Tracer _tracer;
    std::map<EthernetTxId, EthernetFrame> _transmittedMessages;
};


} // namespace Ethernet
} // namespace Services
} // namespace SilKit