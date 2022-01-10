// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <tuple>
#include <vector>
#include <map>

#include "ib/sim/can/ICanController.hpp"
#include "ib/extensions/ITraceMessageSource.hpp"
#include "ib/mw/fwd_decl.hpp"

#include "IIbToCanControllerProxy.hpp"
#include "IComAdapterInternal.hpp"

namespace ib {
namespace sim {
namespace can {

class CanControllerProxy
    : public ICanController
    , public IIbToCanControllerProxy
    , public extensions::ITraceMessageSource
    , public mw::IIbServiceEndpoint
{
public:
    // ----------------------------------------
    // Public Data Types

public:
    // ----------------------------------------
    // Constructors and Destructor
    CanControllerProxy() = delete;
    CanControllerProxy(const CanControllerProxy&) = default;
    CanControllerProxy(CanControllerProxy&&) = default;
    CanControllerProxy(mw::IComAdapterInternal* comAdapter);


public:
    // ----------------------------------------
    // Operator Implementations
    CanControllerProxy& operator=(CanControllerProxy& other) = default;
    CanControllerProxy& operator=(CanControllerProxy&& other) = default;

public:
    // ----------------------------------------
    // Public interface methods
    //
    // ICanController
    void SetBaudRate(uint32_t rate, uint32_t fdRate) override;

    void Reset() override;
    void Start() override;
    void Stop() override;
    void Sleep() override;

    auto SendMessage(const CanMessage& msg) -> CanTxId override;
    auto SendMessage(CanMessage&& msg) -> CanTxId override;

    void RegisterReceiveMessageHandler(ReceiveMessageHandler handler) override;
    void RegisterStateChangedHandler(StateChangedHandler handler) override;
    void RegisterErrorStateChangedHandler(ErrorStateChangedHandler handler) override;
    void RegisterTransmitStatusHandler(MessageStatusHandler handler) override;

    // IIbToCanController
    void ReceiveIbMessage(const IIbServiceEndpoint* from, const sim::can::CanMessage& msg) override;
    void ReceiveIbMessage(const IIbServiceEndpoint* from, const sim::can::CanControllerStatus& msg) override;
    void ReceiveIbMessage(const IIbServiceEndpoint* from, const sim::can::CanTransmitAcknowledge& msg) override;

    void SetEndpointAddress(const mw::EndpointAddress& endpointAddress) override;
    auto EndpointAddress() const -> const mw::EndpointAddress& override;

    //ITraceMessageSource
    inline void AddSink(extensions::ITraceMessageSink* sink) override;

    // IIbServiceEndpoint
    inline void SetServiceDescriptor(const mw::ServiceDescriptor& serviceDescriptor) override;
    inline auto GetServiceDescriptor() const -> const mw::ServiceDescriptor & override;

public:
    // ----------------------------------------
    // Public interface methods

private:
    // ----------------------------------------
    // private data types
    template<typename MsgT>
    using CallbackVector = std::vector<CallbackT<MsgT>>;

private:
    // ----------------------------------------
    // private methods
    void ChangeControllerMode(CanControllerState state);

    template<typename MsgT>
    void RegisterHandler(CallbackT<MsgT> handler);

    template<typename MsgT>
    void CallHandlers(const MsgT& msg);

    inline auto MakeTxId() -> CanTxId;

private:
    // ----------------------------------------
    // private members
    mw::IComAdapterInternal* _comAdapter;
    ::ib::mw::ServiceDescriptor _serviceDescriptor;

    CanTxId _canTxId = 0;
    CanControllerState _controllerState = CanControllerState::Uninit;
    CanErrorState _errorState = CanErrorState::NotAvailable;
    CanConfigureBaudrate _baudRate = { 0, 0 };

    std::tuple<
        CallbackVector<CanMessage>,
        CallbackVector<CanControllerState>,
        CallbackVector<CanErrorState>,
        CallbackVector<CanTransmitAcknowledge>
    > _callbacks;

    extensions::Tracer _tracer;
    std::map<CanTxId, CanMessage> _transmittedMessages;
};

// ================================================================================
//  Inline Implementations
// ================================================================================
auto CanControllerProxy::MakeTxId() -> CanTxId
{
    return ++_canTxId;
}

void CanControllerProxy::AddSink(extensions::ITraceMessageSink* sink)
{
    _tracer.AddSink(EndpointAddress(), *sink);
}

void CanControllerProxy::SetServiceDescriptor(const mw::ServiceDescriptor& serviceDescriptor)
{
    _serviceDescriptor = serviceDescriptor;
}
auto CanControllerProxy::GetServiceDescriptor() const -> const mw::ServiceDescriptor&
{
    return _serviceDescriptor;
}

} // namespace can
} // namespace sim
} // namespace ib
