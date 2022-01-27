// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <functional>
#include <ServiceDatatypes.hpp>

namespace ib {
namespace mw {
namespace service {

class IServiceDiscovery
{
public: //types
    using ServiceDiscoveryHandlerT = std::function<void(ServiceDiscoveryEvent::Type discoveryType, const ServiceDescriptor&)>;

    virtual ~IServiceDiscovery() = default;
    //!< Publish a locally created new ServiceDescriptor to all other participants
    virtual void NotifyServiceCreated(const ServiceDescriptor& serviceDescriptor) = 0;
    //!< Publish a participant-local service removal to all other participants
    virtual void NotifyServiceRemoved(const ServiceDescriptor& serviceDescriptor) = 0;

    //!< Register a handler for asynchronous service creation notifications
    virtual void RegisterServiceDiscoveryHandler(ServiceDiscoveryHandlerT handler) = 0;
};

} // namespace sync
} // namespace mw
} // namespace ib
