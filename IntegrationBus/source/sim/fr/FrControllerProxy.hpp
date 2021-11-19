// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include "ib/sim/fr/IFrController.hpp"
#include "ib/extensions/ITraceMessageSource.hpp"
#include "ib/mw/fwd_decl.hpp"

#include <tuple>
#include <vector>

#include "IIbToFrControllerProxy.hpp"
#include "IComAdapterInternal.hpp"
#include "IServiceId.hpp"

namespace ib {
namespace sim {
namespace fr {

/*! \brief FlexRay Controller implementation for Network Simulator usage
 *
 * Acts as a proxy to the controllers implemented and simulated by the Network Simulator. For operation
 * without a Network Simulator cf. FrController.
 */
class FrControllerProxy
    : public IFrController
    , public IIbToFrControllerProxy
    , public extensions::ITraceMessageSource
    , public mw::IServiceId
{
public:
    // ----------------------------------------
    // Public Data Types

public:
    // ----------------------------------------
    // Constructors and Destructor
    FrControllerProxy() = delete;
    FrControllerProxy(const FrControllerProxy&) = default;
    FrControllerProxy(FrControllerProxy&&) = default;
    FrControllerProxy(mw::IComAdapterInternal* comAdapter);

public:
    // ----------------------------------------
    // Operator Implementations
    FrControllerProxy& operator=(FrControllerProxy& other) = default;
    FrControllerProxy& operator=(FrControllerProxy&& other) = default;

public:
    // ----------------------------------------
    // Public interface methods
    //
    // IEthController
    void Configure(const ControllerConfig& config) override;

    void ReconfigureTxBuffer(uint16_t txBufferIdx, const TxBufferConfig& config) override;

    /*! \brief Update the content of a previously configured TX buffer.
     *
     * The FlexRay message will be sent immediately and only once.
     * I.e., the configuration according to cycle, repetition, and transmission mode is
     * ignored. In particular, even with TransmissionMode::Continuous, the message will be
     * sent only once.
     *
     *  \see IFrController::Configure(const ControllerConfig&)
     */
    void UpdateTxBuffer(const TxBufferUpdate& update) override;

    void Run() override;
    void DeferredHalt() override;
    void Freeze() override;
    void AllowColdstart() override;
    void AllSlots() override;
    void Wakeup() override;

    void RegisterMessageHandler(MessageHandler handler) override;
    void RegisterMessageAckHandler(MessageAckHandler handler) override;
    void RegisterWakeupHandler(WakeupHandler handler) override;

    [[deprecated("superseded by RegisterPocStatusHandler")]]
    void RegisterControllerStatusHandler(ControllerStatusHandler handler) override;

    void RegisterPocStatusHandler(PocStatusHandler handler) override;
    void RegisterSymbolHandler(SymbolHandler handler) override;
    void RegisterSymbolAckHandler(SymbolAckHandler handler) override;
    void RegisterCycleStartHandler(CycleStartHandler handler) override;

    // IIbToFrController
    void ReceiveIbMessage(const IServiceId* from, const FrMessage& msg) override;
    void ReceiveIbMessage(const IServiceId* from, const FrMessageAck& msg) override;
    void ReceiveIbMessage(const IServiceId* from, const FrSymbol& msg) override;
    void ReceiveIbMessage(const IServiceId* from, const FrSymbolAck& msg) override;
    void ReceiveIbMessage(const IServiceId* from, const CycleStart& msg) override;
    void ReceiveIbMessage(const IServiceId* from, const PocStatus& msg) override;

    void SetEndpointAddress(const mw::EndpointAddress& endpointAddress) override;
    auto EndpointAddress() const -> const mw::EndpointAddress& override;

    // ITraceMessageSource
    inline void AddSink(extensions::ITraceMessageSink* sink) override;

    // IServiceId
    inline void SetServiceId(const mw::ServiceId& serviceId) override;
    inline auto GetServiceId() const -> const mw::ServiceId & override;
private:
    // ----------------------------------------
    // private data types
    template<typename MsgT>
    using CallbackVector = std::vector<CallbackT<MsgT>>;

private:
    // ----------------------------------------
    // private methods
    template<typename MsgT>
    void RegisterHandler(CallbackT<MsgT> handler);

    template<typename MsgT>
    void CallHandlers(const MsgT& msg);

    template<typename MsgT>
    inline void SendIbMessage(MsgT&& msg);

private:
    // ----------------------------------------
    // private members
    mw::IComAdapterInternal* _comAdapter = nullptr;
    ::ib::mw::ServiceId _serviceId;

    std::vector<TxBufferConfig> _bufferConfigs;

    std::tuple<
        CallbackVector<FrMessage>,
        CallbackVector<FrMessageAck>,
        CallbackVector<FrSymbol>,
        CallbackVector<FrSymbolAck>,
        CallbackVector<CycleStart>,
        CallbackVector<ControllerStatus>,
        CallbackVector<PocStatus>
    > _callbacks;

    extensions::Tracer _tracer;

    CallbackVector<FrSymbol> _wakeupHandlers;
};


// ==================================================================
//  Inline Implementations
// ==================================================================
void FrControllerProxy::AddSink(extensions::ITraceMessageSink* sink)
{
    _tracer.AddSink(EndpointAddress(), *sink);
}

void FrControllerProxy::SetServiceId(const mw::ServiceId& serviceId)
{
    _serviceId = serviceId;
}

auto FrControllerProxy::GetServiceId() const -> const mw::ServiceId&
{
    return _serviceId;
}
} // namespace fr
} // SimModels
} // namespace ib
