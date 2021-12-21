#include "ib/cfg/ConfigBuilder.hpp"
#include "ib/capi/FlexRay.h"
#include "ib/capi/IntegrationBus.h"
#include "ib/sim/fr/all.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "MockComAdapter.hpp"

namespace
{
using namespace ib::mw;
using namespace ib::cfg;
using namespace ib::sim::fr;
using ::ib::mw::test::DummyComAdapter;
using ::testing::_;


class MockComAdapter : public DummyComAdapter
{
public:
    MOCK_METHOD2(SendIbMessage, void(EndpointAddress, const HostCommand&));
    MOCK_METHOD2(SendIbMessage, void(EndpointAddress, const ControllerConfig&));
    MOCK_METHOD2(SendIbMessage, void(EndpointAddress, const TxBufferConfigUpdate&));
    MOCK_METHOD2(SendIbMessage, void(EndpointAddress, const TxBufferUpdate&));
};

class MockFrController : public ib::sim::fr::IFrController
{
public:
  MOCK_METHOD1(Configure, void(const ControllerConfig& config));
  MOCK_METHOD2(ReconfigureTxBuffer, void(uint16_t txBufferIdx, const TxBufferConfig& config));
  MOCK_METHOD1(UpdateTxBuffer, void(const TxBufferUpdate& update));
  MOCK_METHOD0(Run, void());
  MOCK_METHOD0(DeferredHalt, void());
  MOCK_METHOD0(Freeze, void());
  MOCK_METHOD0(AllowColdstart, void());
  MOCK_METHOD0(AllSlots, void());
  MOCK_METHOD0(Wakeup, void());
  MOCK_METHOD1(RegisterMessageHandler, void(MessageHandler handler));
  MOCK_METHOD1(RegisterMessageAckHandler, void(MessageAckHandler handler));
  MOCK_METHOD1(RegisterWakeupHandler, void(WakeupHandler handler));
  MOCK_METHOD1(RegisterControllerStatusHandler, void(ControllerStatusHandler handler));
  MOCK_METHOD1(RegisterPocStatusHandler, void(PocStatusHandler handler));
  MOCK_METHOD1(RegisterSymbolHandler, void(SymbolHandler handler));
  MOCK_METHOD1(RegisterSymbolAckHandler, void(SymbolAckHandler handler));
  MOCK_METHOD1(RegisterCycleStartHandler, void(CycleStartHandler handler));
};

class CapiFlexRayTest : public testing::Test
{
public:
  CapiFlexRayTest()
    : configBuilder("TestBuilder")
    , controllerName("FR1")
    , simulationSetup{ configBuilder.SimulationSetup() }
  {
    auto participantName = "FRcontroller";
    auto linkName = "P0";

    simulationSetup.AddParticipant(participantName)
        .AddFlexray(controllerName)
        .WithClusterParameters(getClusterParameters())
        .WithNodeParameters(getNodeParameters())
        .WithLink(linkName);

    std::initializer_list<std::string> links{ linkName, "P1" };

    simulationSetup.AddParticipant("NetworkSimulator")
        .AddNetworkSimulator("BusSim")
        .WithLinks(links);
  }

  void SetUp() override
  {
    ibConfig = configBuilder.Build();
  }
  void TearDown() override
  {
  }
  static ib::sim::fr::ClusterParameters getClusterParameters();
  static ib::sim::fr::NodeParameters getNodeParameters();
  struct Callbacks
  {
    static void MessageHandler(void* context, ib_FlexRay_Controller* controller, const ib_FlexRay_Message* message)
    {
    }

    static void MessageAckHandler(void* context, ib_FlexRay_Controller* controller, const ib_FlexRay_MessageAck* acknowledge)
    {
    }

    static void WakeupHandler(void* context, ib_FlexRay_Controller* controller, const ib_FlexRay_Symbol* symbol)
    {
    }

    static void ControllerStatusHandler(void* context, ib_FlexRay_Controller* controller, const ib_FlexRay_ControllerStatus* status)
    {
    }

    static void PocStatusHandler(void* context, ib_FlexRay_Controller* controller, const ib_FlexRay_PocStatus* status)
    {
    }

    static void SymbolHandler(void* context, ib_FlexRay_Controller* controller, const ib_FlexRay_Symbol* symbol)
    {
    }

    static void SymbolAckHandler(void* context, ib_FlexRay_Controller* controller, const ib_FlexRay_SymbolAck* acknowledge)
    {
    }

    static void CycleStartHandler(void* context, ib_FlexRay_Controller* controller, const ib_FlexRay_CycleStart* cycleStart)
    {
    }
  };

protected:
    std::string controllerName;
    ConfigBuilder configBuilder;
    MockComAdapter comAdapter;
    MockFrController mockController;
    SimulationSetupBuilder& simulationSetup;
    ib::cfg::Config ibConfig;
};
TEST_F(CapiFlexRayTest, make_flexray_controller)
{

  ib_ReturnCode returnCode;
  ib_FlexRay_Controller* frController = nullptr;
  returnCode = ib_FlexRay_Controller_Create(&frController, (ib_SimulationParticipant*)&comAdapter, controllerName.c_str());
  // needs NullConnectionComAdapter, which won't link with C-API. So just expect a general failure here.
  EXPECT_EQ(returnCode, ib_ReturnCode_UNSPECIFIEDERROR);
  EXPECT_EQ(frController, nullptr);
  // When using the NullConnectionComAdapter, enable this:
  //EXPECT_NE(frController, nullptr);
}

TEST_F(CapiFlexRayTest, fr_controller_function_mapping)
{

  ib_ReturnCode               returnCode;
  ib_FlexRay_ControllerConfig cfg;
  memset(&cfg, 0, sizeof(cfg));

  EXPECT_CALL(mockController, Configure(_)).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_Configure((ib_FlexRay_Controller*)&mockController, &cfg);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, RegisterMessageHandler(testing::_)).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_RegisterMessageHandler((ib_FlexRay_Controller*)&mockController, NULL, &Callbacks::MessageHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, RegisterMessageAckHandler(testing::_)).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_RegisterMessageAckHandler((ib_FlexRay_Controller*)&mockController, NULL, &Callbacks::MessageAckHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, RegisterWakeupHandler(testing::_)).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_RegisterWakeupHandler((ib_FlexRay_Controller*)&mockController, NULL, &Callbacks::WakeupHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, RegisterPocStatusHandler(testing::_)).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_RegisterPocStatusHandler((ib_FlexRay_Controller*)&mockController, NULL, &Callbacks::PocStatusHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, RegisterSymbolHandler(testing::_)).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_RegisterSymbolHandler((ib_FlexRay_Controller*)&mockController, NULL, &Callbacks::SymbolHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, RegisterSymbolAckHandler(testing::_)).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_RegisterSymbolAckHandler((ib_FlexRay_Controller*)&mockController, NULL, &Callbacks::SymbolAckHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, RegisterCycleStartHandler(testing::_)).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_RegisterCycleStartHandler((ib_FlexRay_Controller*)&mockController, NULL, &Callbacks::CycleStartHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, Run()).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_ExecuteCmd((ib_FlexRay_Controller*)&mockController, ib_FlexRay_ChiCommand_RUN);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, DeferredHalt()).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_ExecuteCmd((ib_FlexRay_Controller*)&mockController, ib_FlexRay_ChiCommand_DEFERRED_HALT);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, Freeze()).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_ExecuteCmd((ib_FlexRay_Controller*)&mockController, ib_FlexRay_ChiCommand_FREEZE);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, AllowColdstart()).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_ExecuteCmd((ib_FlexRay_Controller*)&mockController, ib_FlexRay_ChiCommand_ALLOW_COLDSTART);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, AllSlots()).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_ExecuteCmd((ib_FlexRay_Controller*)&mockController, ib_FlexRay_ChiCommand_ALL_SLOTS);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

  EXPECT_CALL(mockController, Wakeup()).Times(testing::Exactly(1));
  returnCode = ib_FlexRay_Controller_ExecuteCmd((ib_FlexRay_Controller*)&mockController, ib_FlexRay_ChiCommand_WAKEUP);
  EXPECT_EQ(returnCode, ib_ReturnCode_SUCCESS);

}

TEST_F(CapiFlexRayTest, fr_controller_nullpointer_params)
{

  auto                   cMockComAdapter = (ib_SimulationParticipant*)&comAdapter;
  auto                   cMockController = (ib_Lin_Controller*)&mockController;
  ib_ReturnCode               returnCode;
  ib_FlexRay_ControllerConfig cfg;
  memset(&cfg, 0, sizeof(cfg));
  ib_FlexRay_Controller* cController = (ib_FlexRay_Controller*)&mockController;
  ib_FlexRay_Controller* cControllerReturn = nullptr;
  returnCode = ib_FlexRay_Controller_Create(nullptr, nullptr, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_Create(nullptr, nullptr, "bad");
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_Create(&cControllerReturn, nullptr, "bad");
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_Create(nullptr, cMockComAdapter, "bad");
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_Create(&cControllerReturn, cMockComAdapter, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_ControllerConfig_Create(nullptr, nullptr, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  ib_FlexRay_ControllerConfig* configPtr = NULL;
  returnCode = ib_FlexRay_ControllerConfig_Create(nullptr, nullptr, "bad");
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_ControllerConfig_Create(&configPtr, nullptr, "bad");
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_Configure(cController, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_Configure(nullptr, &cfg);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_Configure(nullptr, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_ExecuteCmd(nullptr, ib_FlexRay_ChiCommand_RUN);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_RegisterMessageHandler(nullptr, NULL, &Callbacks::MessageHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_RegisterMessageHandler(cController, NULL, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_RegisterMessageAckHandler(nullptr, NULL, &Callbacks::MessageAckHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_RegisterMessageAckHandler(cController, NULL, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_RegisterWakeupHandler(nullptr, NULL, &Callbacks::WakeupHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_RegisterWakeupHandler(cController, NULL, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_RegisterPocStatusHandler(nullptr, NULL, &Callbacks::PocStatusHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_RegisterPocStatusHandler(cController, NULL, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_RegisterSymbolHandler(nullptr, NULL, &Callbacks::SymbolHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_RegisterSymbolHandler(cController, NULL, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_RegisterSymbolAckHandler(nullptr, NULL, &Callbacks::SymbolAckHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_RegisterSymbolAckHandler(cController, NULL, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);

  returnCode = ib_FlexRay_Controller_RegisterCycleStartHandler(nullptr, NULL, &Callbacks::CycleStartHandler);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
  returnCode = ib_FlexRay_Controller_RegisterCycleStartHandler(cController, NULL, nullptr);
  EXPECT_EQ(returnCode, ib_ReturnCode_BADPARAMETER);
}

ib::sim::fr::ClusterParameters CapiFlexRayTest::getClusterParameters()
{
    ib::sim::fr::ClusterParameters clusterParams;
    clusterParams.gColdstartAttempts = 8;
    clusterParams.gCycleCountMax = 63;
    clusterParams.gdActionPointOffset = 2;
    clusterParams.gdDynamicSlotIdlePhase = 1;
    clusterParams.gdMiniSlot = 5;
    clusterParams.gdMiniSlotActionPointOffset = 2;
    clusterParams.gdStaticSlot = 31;
    clusterParams.gdSymbolWindow = 1;
    clusterParams.gdSymbolWindowActionPointOffset = 1;
    clusterParams.gdTSSTransmitter = 9;
    clusterParams.gdWakeupTxActive = 60;
    clusterParams.gdWakeupTxIdle = 180;
    clusterParams.gListenNoise = 2;
    clusterParams.gMacroPerCycle = 3636;
    clusterParams.gMaxWithoutClockCorrectionFatal = 2;
    clusterParams.gMaxWithoutClockCorrectionPassive = 2;
    clusterParams.gNumberOfMiniSlots = 291;
    clusterParams.gNumberOfStaticSlots = 70;
    clusterParams.gPayloadLengthStatic = 16;
    clusterParams.gSyncFrameIDCountMax = 15;
    return clusterParams;
}

ib::sim::fr::NodeParameters CapiFlexRayTest::getNodeParameters()
{
    ib::sim::fr::NodeParameters nodeParams;
    nodeParams.pAllowHaltDueToClock = 1;
    nodeParams.pAllowPassiveToActive = 0;
    nodeParams.pChannels = ib::sim::fr::Channel::AB;
    nodeParams.pClusterDriftDamping = 2;
    nodeParams.pdAcceptedStartupRange = 212;
    nodeParams.pdListenTimeout = 400162;
    nodeParams.pKeySlotId = 0;
    nodeParams.pKeySlotOnlyEnabled = 0;
    nodeParams.pKeySlotUsedForStartup = 0;
    nodeParams.pKeySlotUsedForSync = 0;
    nodeParams.pLatestTx = 249;
    nodeParams.pMacroInitialOffsetA = 3;
    nodeParams.pMacroInitialOffsetB = 3;
    nodeParams.pMicroInitialOffsetA = 6;
    nodeParams.pMicroInitialOffsetB = 6;
    nodeParams.pMicroPerCycle = 200000;
    nodeParams.pOffsetCorrectionOut = 127;
    nodeParams.pOffsetCorrectionStart = 3632;
    nodeParams.pRateCorrectionOut = 81;
    nodeParams.pWakeupChannel = ib::sim::fr::Channel::A;
    nodeParams.pWakeupPattern = 33;
    nodeParams.pdMicrotick = ib::sim::fr::ClockPeriod::T25NS;
    nodeParams.pSamplesPerMicrotick = 2;
    return nodeParams;
}

} // namespace