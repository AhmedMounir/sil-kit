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

#include "EthController.hpp"

#include <chrono>
#include <functional>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "silkit/util/functional.hpp"

#include "MockParticipant.hpp"
#include "MockTraceSink.hpp"

#include "EthDatatypeUtils.hpp"
#include "ParticipantConfiguration.hpp"

namespace {

using namespace std::chrono_literals;

using testing::Return;
using testing::A;
using testing::An;
using testing::_;
using testing::InSequence;
using testing::NiceMock;

using namespace SilKit::Core;
using namespace SilKit::Services;
using namespace SilKit::Services::Ethernet;

using ::SilKit::Core::Tests::DummyParticipant;
using ::SilKit::Tests::MockTraceSink;

MATCHER_P(EthernetTransmitAckWithouthTransmitIdMatcher, truthAck, "") 
{
    *result_listener << "matches EthernetTransmitAcks without checking the transmit id";
    auto frame1 = truthAck;
    auto frame2 = arg;
    return frame1.sourceMac == frame2.sourceMac && frame1.status == frame2.status && frame1.timestamp == frame2.timestamp;
}

auto AnEthMessageWith(std::chrono::nanoseconds timestamp) -> testing::Matcher<const WireEthernetFrameEvent&>
{
    return testing::Field(&WireEthernetFrameEvent::timestamp, timestamp);
}

void SetSourceMac(std::vector<uint8_t>& raw, const EthernetMac& source)
{
    const size_t MinFrameSize = 64;
    const size_t SourceMacStart = sizeof(EthernetMac);
    if (raw.empty())
    {
        raw.resize(MinFrameSize);
    }

    std::copy(source.begin(), source.end(), raw.begin() + SourceMacStart);
}

class MockParticipant : public DummyParticipant
{
public:

    MOCK_METHOD2(SendMsg, void(const IServiceEndpoint*, const WireEthernetFrameEvent&));
    MOCK_METHOD2(SendMsg, void(const IServiceEndpoint*, const EthernetFrameTransmitEvent&));
    MOCK_METHOD2(SendMsg, void(const IServiceEndpoint*, const EthernetStatus&));
    MOCK_METHOD2(SendMsg, void(const IServiceEndpoint*, const EthernetSetMode&));
};

class EthernetControllerTrivialSimTest : public testing::Test
{
protected:
    struct Callbacks
    {
        MOCK_METHOD2(ReceiveMessage, void(Ethernet::IEthernetController*, const Ethernet::EthernetFrameEvent&));
        MOCK_METHOD2(MessageAck, void(Ethernet::IEthernetController*, Ethernet::EthernetFrameTransmitEvent));
        MOCK_METHOD2(StateChanged, void(Ethernet::IEthernetController*, Ethernet::EthernetState));
        MOCK_METHOD2(BitRateChanged, void(Ethernet::IEthernetController*, Ethernet::EthernetBitrate));
    };

protected:
    EthernetControllerTrivialSimTest()
        : controller(&participant, cfg, participant.GetTimeProvider())
        , controllerOther(&participant, cfg, participant.GetTimeProvider())
    {
        controller.SetServiceDescriptor(from_endpointAddress(controllerAddress));

        controller.AddFrameHandler(SilKit::Util::bind_method(&callbacks, &Callbacks::ReceiveMessage));
        controller.AddFrameTransmitHandler(SilKit::Util::bind_method(&callbacks, &Callbacks::MessageAck));

        controllerOther.SetServiceDescriptor(from_endpointAddress(otherAddress));
    }

protected:
    const EndpointAddress controllerAddress = {3, 8};
    const EndpointAddress otherAddress = {7, 2};

    MockTraceSink traceSink;
    MockParticipant participant;
    Callbacks callbacks;

    SilKit::Config::EthernetController cfg;
    EthController controller;
    EthController controllerOther;
};

/*! \brief SendFrame must invoke the TimeProvider and triggers SendMsg on the participant
*/
TEST_F(EthernetControllerTrivialSimTest, send_eth_frame)
{
    ON_CALL(participant.mockTimeProvider, Now())
        .WillByDefault(testing::Return(42ns));

    const auto now = 42ns;
    EXPECT_CALL(participant, SendMsg(&controller, AnEthMessageWith(now))).Times(1);

    EthernetFrameTransmitEvent ack{};
    ack.sourceMac = EthernetMac{0, 0, 0, 0, 0, 0};
    ack.status = EthernetTransmitStatus::Transmitted;
    ack.timestamp = 42ns;
    EXPECT_CALL(callbacks, MessageAck(&controller, EthernetTransmitAckWithouthTransmitIdMatcher(ack))).Times(1);

    // once for activate and once for sending the frame
    EXPECT_CALL(participant.mockTimeProvider, Now()).Times(2);

    std::vector<uint8_t> rawFrame;
    SetSourceMac(rawFrame, EthernetMac{ 0, 0, 0, 0, 0, 0 });

    EthernetFrame frame{rawFrame};
    controller.Activate();
    controller.SendFrame(frame);
}

/*! \brief SendFrame without Activate must trigger a nack
*/
TEST_F(EthernetControllerTrivialSimTest, nack_on_inactive_controller)
{
    ON_CALL(participant.mockTimeProvider, Now()).WillByDefault(testing::Return(42ns));

    const auto now = 42ns;
    EXPECT_CALL(participant, SendMsg(&controller, AnEthMessageWith(now))).Times(0);

    EthernetFrameTransmitEvent nack{};
    nack.sourceMac = EthernetMac{0, 0, 0, 0, 0, 0};
    nack.status = EthernetTransmitStatus::ControllerInactive;
    nack.timestamp = 42ns;
    EXPECT_CALL(callbacks, MessageAck(&controller, EthernetTransmitAckWithouthTransmitIdMatcher(nack))).Times(1);

    // once for the nack
    EXPECT_CALL(participant.mockTimeProvider, Now()).Times(1);


    std::vector<uint8_t> rawFrame;
    SetSourceMac(rawFrame, EthernetMac{ 0, 0, 0, 0, 0, 0 });

    EthernetFrame frame{rawFrame};
    controller.SendFrame(frame);
}


/*! \brief The controller must change it's state when a Call to Activate/Deactivate is triggered
*/
TEST_F(EthernetControllerTrivialSimTest, linkup_controller_inactive_on_activate_deactivate)
{
    controller.Activate();
    ASSERT_TRUE(controller.GetState() == EthernetState::LinkUp);
    controller.Deactivate();
    ASSERT_TRUE(controller.GetState() == EthernetState::Inactive);
}

/*! \brief Passing an EthernetFrameEvent to an EthControllers must trigger the registered callback
 */
TEST_F(EthernetControllerTrivialSimTest, trigger_callback_on_receive_message)
{
    std::vector<uint8_t> rawFrame;
    SetSourceMac(rawFrame, EthernetMac{ 0, 0, 0, 0, 0, 0 });

    WireEthernetFrameEvent msg{};
    msg.frame = WireEthernetFrame{rawFrame};
    msg.direction = TransmitDirection::RX;

    EXPECT_CALL(callbacks, ReceiveMessage(&controller, ToEthernetFrameEvent(msg)))
        .Times(1);

    controller.Activate();
    controller.ReceiveMsg(&controllerOther, msg);
}

/*! \brief Passing an Ack to an EthControllers must trigger the registered callback, if
 *         it sent a message with corresponding transmit ID and source MAC
 */
TEST_F(EthernetControllerTrivialSimTest, trigger_callback_on_receive_ack)
{
    std::vector<uint8_t> rawFrame;
    SetSourceMac(rawFrame, EthernetMac{ 1, 2, 3, 4, 5, 6 });

    EthernetFrameEvent msg{};
    msg.frame.raw = rawFrame;

    EXPECT_CALL(participant, SendMsg(&controller, AnEthMessageWith(0ns))).Times(1);
    EthernetFrameTransmitEvent ack{ EthernetMac{ 1, 2, 3, 4, 5, 6 }, 0ms, EthernetTransmitStatus::Transmitted, reinterpret_cast<void *>(0) };
    EXPECT_CALL(callbacks, MessageAck(&controller, EthernetTransmitAckWithouthTransmitIdMatcher(ack)))
        .Times(1);

    // once for activate and once for sending the frame
    EXPECT_CALL(participant.mockTimeProvider, Now()).Times(2);
    controller.Activate();
    controller.SendFrame(EthernetFrame{rawFrame});
}

/*! \brief Multiple handlers added and removed
 */
TEST_F(EthernetControllerTrivialSimTest, add_remove_handler)
{
    EthController testController{&participant, cfg, participant.GetTimeProvider()};

    const int numHandlers = 10;
    std::vector<SilKit::Services::HandlerId> handlerIds;
    for (int i = 0; i < numHandlers; i++)
    {
        handlerIds.push_back(testController.AddFrameHandler(SilKit::Util::bind_method(&callbacks, &Callbacks::ReceiveMessage)));
    }

    std::vector<uint8_t> rawFrame;
    SetSourceMac(rawFrame, EthernetMac{0, 0, 0, 0, 0, 0});

    WireEthernetFrameEvent msg{};
    msg.frame.raw = rawFrame;
    msg.direction = TransmitDirection::RX;

    EXPECT_CALL(callbacks, ReceiveMessage(&testController, ToEthernetFrameEvent(msg))).Times(numHandlers);
    testController.ReceiveMsg(&controllerOther, msg);

    for (auto&& handlerId : handlerIds)
    {
        testController.RemoveFrameHandler(handlerId);
    }

    EXPECT_CALL(callbacks, ReceiveMessage(&testController, ToEthernetFrameEvent(msg))).Times(0);
    testController.ReceiveMsg(&controllerOther, msg);
}

/*! \brief Removed handler in handler
 */
TEST_F(EthernetControllerTrivialSimTest, remove_handler_in_handler)
{
    EthController testController{&participant, cfg, participant.GetTimeProvider()};

    auto handlerIdToRemove =
        testController.AddFrameHandler(SilKit::Util::bind_method(&callbacks, &Callbacks::ReceiveMessage));

    auto testHandler = [handlerIdToRemove](Ethernet::IEthernetController* ctrl, const Ethernet::EthernetFrameEvent&) {
        ctrl->RemoveFrameHandler(handlerIdToRemove);
    };
    testController.AddFrameHandler(testHandler);

    std::vector<uint8_t> rawFrame;
    SetSourceMac(rawFrame, EthernetMac{0, 0, 0, 0, 0, 0});

    WireEthernetFrameEvent msg{};
    msg.frame.raw = rawFrame;
    msg.direction = TransmitDirection::RX;

    EXPECT_CALL(callbacks, ReceiveMessage(&testController, ToEthernetFrameEvent(msg))).Times(1);
    // Calls testHandler and Callbacks::ReceiveMessage, the latter is removed in testHandler 
    testController.ReceiveMsg(&controllerOther, msg);
    EXPECT_CALL(callbacks, ReceiveMessage(&testController, ToEthernetFrameEvent(msg))).Times(0);
    // Call testHandler again, handlerIdToRemove is invalid now but should only result in a warning
    testController.ReceiveMsg(&controllerOther, msg);
}


TEST_F(EthernetControllerTrivialSimTest, DISABLED_ethcontroller_uses_tracing)
{
#if (0)


    const auto now = 1337ns;
    ON_CALL(participant.mockTimeProvider, Now())
        .WillByDefault(testing::Return(now));

    SilKit::Config::EthernetController config{};
    auto ethController = EthController(&participant, config, participant.GetTimeProvider());
    ethController.SetServiceDescriptor(from_endpointAddress(controllerAddress));
    ethController.AddSink(&traceSink);


    EthernetFrame frame{};
    SetDestinationMac(frame, EthernetMac{ 1, 2, 3, 4, 5, 6 });
    SetSourceMac(frame, EthernetMac{ 9, 8, 7, 6, 5, 4 });

    //Send direction
    EXPECT_CALL(participant.mockTimeProvider, Now())
        .Times(1);
    EXPECT_CALL(traceSink,
        Trace(SilKit::Services::TransmitDirection::TX, controllerAddress, now, frame))
        .Times(1);
    ethController.SendFrame(frame);

    // Receive direction
    EXPECT_CALL(traceSink,
        Trace(SilKit::Services::TransmitDirection::RX, controllerAddress, now, frame))
        .Times(1);

    EthernetFrameEvent ethMsg{};
    ethMsg.frame = frame;
    ethMsg.timestamp = now;
    ethController.ReceiveMsg(&controllerOther, ethMsg);
#endif // 0
}

} // anonymous namespace
