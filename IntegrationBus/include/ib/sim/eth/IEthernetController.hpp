// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <functional>

#include "EthernetDatatypes.hpp"

namespace ib {
namespace sim {
namespace eth {

/*! \brief Abstract Ethernet Controller API to be used by vECUs
 */
class IEthernetController
{
public:
    /*! \brief Generic Ethernet callback method
    */
    template<typename MsgT>
    using CallbackT = std::function<void(IEthernetController* controller, const MsgT& msg)>;

    /*! Callback type to indicate that an EthernetFrameEvent has been received.
    *  Cf. \ref AddFrameHandler(FrameHandler);
    */
    using FrameHandler = CallbackT<EthernetFrameEvent>;

    /*! Callback type to indicate that an EthernetFrameTransmitEvent has been received.
    *  Cf. \ref AddFrameTransmitHandler(FrameTransmitHandler);
    */
    using FrameTransmitHandler = CallbackT<EthernetFrameTransmitEvent>;

    /*! Callback type to indicate that the ::EthernetState has changed.
    *  Cf. \ref AddStateChangeHandler(StateChangeHandler);
    */
    using StateChangeHandler = CallbackT<EthernetStateChangeEvent>;

    /*! Callback type to indicate that the link bit rate has changed.
    *  Cf. \ref AddBitrateChangeHandler(BitrateChangeHandler);
    */
    using BitrateChangeHandler = CallbackT<EthernetBitrateChangeEvent>;

public:
    virtual ~IEthernetController() = default;

    /*! \brief Activates the Ethernet controller
     *
     * Upon activation of the controller, the controller attempts to
     * establish a link. Messages can only be sent once the link has
     * been successfully established,
     * cf. AddStateChangeHandler() and AddBitrateChangeHandler().
     *
     * NB: Only supported in VIBE simulation! In simple simulation,
     * messages can be sent without need to call Activate()
     */
    virtual void Activate() = 0;

    /*! \brief Deactivate the Ethernet controller
     *
     * Deactivate the controller and shut down the link. The
     * controller will no longer receive messages, and it cannot send
     * messages anymore.
     *
     * NB: Only supported in VIBE simulation! In simple simulation,
     * Deactivate() has no effects and messages can still be sent.
     */
    virtual void Deactivate() = 0;

    /*! \brief Register a callback for Ethernet message reception
     *
     * The handler is called when the controller receives a new
     * Ethernet message.
     */
    virtual void AddFrameHandler(FrameHandler handler) = 0;

    /*! \brief Register a callback for Ethernet transmit acknowledgments
     *
     * The handler is called when a previously sent message was
     * successfully transmitted or when the transmission has
     * failed. The original message is identified by the transmitId.
     *
     * NB: Full support in VIBE Ethernet simulation. In simple
     * simulation, all messages are immediately positively
     * acknowledged by a receiving controller.
     */
    virtual void AddFrameTransmitHandler(FrameTransmitHandler handler) = 0;

    /*! \brief Register a callback for changes of the controller state
     *
     * The handler is called when the state of the controller
     * changes. E.g., a call to Activate() causes the controller to
     * change from state ::Inactive to ::LinkDown. Later, when the link
     * has been established, the state changes again from ::LinkDown to
     * ::LinkUp. Similarly, the status changes back to ::Inactive upon a
     * call to Deactivate().
     *
     * NB: Only supported in VIBE Ethernet simulation.
     */
    virtual void AddStateChangeHandler(StateChangeHandler handler) = 0;

    /*! \brief Register a callback for changes of the link bit rate
     *
     * The handler is called when the bit rate of the connected link
     * changes. This is typically the case when a link was
     * successfully established, or the controller was deactivated.
     *
     * NB: Only supported in VIBE Ethernet simulation.
     */
    virtual void AddBitrateChangeHandler(BitrateChangeHandler handler) = 0;

    /*! \brief Send an Ethernet frame with the time provider's
    * current time.
    * NB: precise timestamps are always generated by the NetworkSimulator.
    */
    virtual auto SendFrame(EthernetFrame msg) -> EthernetTxId = 0;
};

} // namespace eth
} // namespace sim
} // namespace ib