// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include "ib/IntegrationBus.hpp"
#include "ib/mw/sync/all.hpp"
#include "ib/sim/all.hpp"

#include "MockParticipantConfiguration.hpp"

#include "GetTestPid.hpp"
#include "IntegrationTestInfrastructure.hpp"

using namespace std::chrono_literals;
using namespace ib::mw;
using namespace ib::mw::sync;
using namespace ib::cfg;
using namespace ib::sim::rpc;

const uint8_t rpcFuncIncrement = 100u;
const size_t defaultMsgSize = 3;
const uint32_t defaultNumCalls = 3;

class RpcITest : public testing::Test
{
protected:
    RpcITest() {}

    struct RpcClientInfo
    {
        RpcClientInfo(const std::string& newControllerName, const std::string& newFunctionName, const std::string& newMediaType,
                      const std::map<std::string, std::string>& newLabels, size_t newMessageSizeInBytes,
                      uint32_t newNumCalls, uint32_t newNumCallsToReturn)
        {
            expectIncreasingData = true;
            controllerName = newControllerName;
            rpcChannel = newFunctionName;
            dxf.mediaType = newMediaType;
            labels = newLabels;
            messageSizeInBytes = newMessageSizeInBytes;
            numCalls = newNumCalls;
            numCallsToReturn = newNumCallsToReturn;
        }
        RpcClientInfo(const std::string& newControllerName, const std::string& newFunctionName, const std::string& newMediaType,
                      const std::map<std::string, std::string>& newLabels, size_t newMessageSizeInBytes,
                      uint32_t newNumCalls, uint32_t newNumCallsToReturn,
                      const std::vector<std::vector<uint8_t>>& newExpectedReturnDataUnordered)
        {
            expectIncreasingData = false;
            controllerName = newControllerName;
            rpcChannel = newFunctionName;
            dxf.mediaType = newMediaType;
            labels = newLabels;
            messageSizeInBytes = newMessageSizeInBytes;
            numCalls = newNumCalls;
            numCallsToReturn = newNumCallsToReturn;
            expectedReturnDataUnordered = newExpectedReturnDataUnordered;
        }

        std::string controllerName;
        std::string rpcChannel;
        RpcExchangeFormat dxf;
        std::map<std::string, std::string> labels;
        size_t messageSizeInBytes;
        uint32_t numCalls;
        uint32_t numCallsToReturn;
        bool expectIncreasingData;
        std::vector<std::vector<uint8_t>> expectedReturnDataUnordered;
        uint32_t callCounter{0};
        uint32_t callReturnedSuccessCounter{0};
        bool allCalled{false};
        bool allCallsReturned{false};
        IRpcClient* rpcClient;

        void Call()
        {
            if (!allCalled)
            {
                auto argumentData = std::vector<uint8_t>(messageSizeInBytes, static_cast<uint8_t>(callCounter));
                auto callHandle = rpcClient->Call(argumentData);
                if (callHandle)
                {
                    callCounter++;
                    if (callCounter >= numCalls)
                    {
                        allCalled = true;
                    }
                }
            }
        }

        void OnCallReturned(const std::vector<uint8_t>& returnData)
        {
            if (expectIncreasingData)
            {
                auto expectedData = std::vector<uint8_t>(
                    messageSizeInBytes, static_cast<uint8_t>(callReturnedSuccessCounter + rpcFuncIncrement));
                EXPECT_EQ(returnData, expectedData);
            }
            else
            {
                auto foundDataIter =
                    std::find(expectedReturnDataUnordered.begin(), expectedReturnDataUnordered.end(), returnData);
                EXPECT_EQ(foundDataIter != expectedReturnDataUnordered.end(), true);
                if (foundDataIter != expectedReturnDataUnordered.end())
                {
                    expectedReturnDataUnordered.erase(foundDataIter);
                }
            }

            callReturnedSuccessCounter++;
            if (callReturnedSuccessCounter >= numCallsToReturn)
            {
                allCallsReturned = true;
            }
        }
    };

    struct RpcServerInfo
    {
        RpcServerInfo(const std::string& newControllerName, const std::string& newFunctionName, const std::string& newMediaType,
                      const std::map<std::string, std::string>& newLabels, size_t newMessageSizeInBytes,
                      uint32_t newNumCallsToReceive)
        {
            expectIncreasingData = true;
            controllerName = newControllerName;
            rpcChannel = newFunctionName;
            dxf.mediaType = newMediaType;
            labels = newLabels;
            messageSizeInBytes = newMessageSizeInBytes;
            numCallsToReceive = newNumCallsToReceive;
        }
        RpcServerInfo(const std::string& newControllerName, const std::string& newFunctionName, const std::string& newMediaType,
                      const std::map<std::string, std::string>& newLabels, size_t newMessageSizeInBytes,
                      uint32_t newNumCallsToReceive, const std::vector<std::vector<uint8_t>>& newExpectedDataUnordered)
        {
            expectIncreasingData = false;
            controllerName = newControllerName;
            rpcChannel = newFunctionName;
            dxf.mediaType = newMediaType;
            labels = newLabels;
            messageSizeInBytes = newMessageSizeInBytes;
            numCallsToReceive = newNumCallsToReceive;
            expectedDataUnordered = newExpectedDataUnordered;
        }

        std::string controllerName;
        std::string rpcChannel;
        RpcExchangeFormat dxf;
        std::map<std::string, std::string> labels;
        size_t messageSizeInBytes;
        uint32_t numCallsToReceive;
        bool expectIncreasingData;
        std::vector<std::vector<uint8_t>> expectedDataUnordered;
        uint32_t receiveCallCounter{0};
        bool allReceived{false};
        IRpcServer* rpcServer;

        void ReceiveCall(const std::vector<uint8_t>& argumentData)
        {
            if (!allReceived)
            {
                if (expectIncreasingData)
                {
                    auto expectedData =
                        std::vector<uint8_t>(messageSizeInBytes, static_cast<uint8_t>(receiveCallCounter));
                    EXPECT_EQ(argumentData, expectedData);
                }
                else
                {
                    auto foundDataIter =
                        std::find(expectedDataUnordered.begin(), expectedDataUnordered.end(), argumentData);
                    EXPECT_EQ(foundDataIter != expectedDataUnordered.end(), true);
                    if (foundDataIter != expectedDataUnordered.end())
                    {
                        expectedDataUnordered.erase(foundDataIter);
                    }
                }

                receiveCallCounter++;
                if (receiveCallCounter >= numCallsToReceive)
                {
                    allReceived = true;
                }
            }
        }
    };

    struct RpcParticipant
    {
        RpcParticipant(const std::string& newName, const std::vector<std::string>& newExpectedFunctionNames)
        {
            name = newName;
            expectedFunctionNames = newExpectedFunctionNames;
        }
        RpcParticipant(const std::string& newName, const std::vector<RpcServerInfo>& newRpcServers,
                       const std::vector<RpcClientInfo>& newRpcClients,
                       const std::vector<std::string>& newExpectedFunctionNames)
        {
            name = newName;
            rpcClients = newRpcClients;
            rpcServers = newRpcServers;
            expectedFunctionNames = newExpectedFunctionNames;
        }

        std::string name;
        std::vector<RpcClientInfo> rpcClients;
        std::vector<RpcServerInfo> rpcServers;
        std::unique_ptr<IParticipant> participant;
        std::vector<std::string> expectedFunctionNames;
        bool allCalled{false};
        std::promise<void> allCalledPromise;
        std::promise<void> allCallsReturnedPromise;
        bool allCallsReturned{false};
        std::promise<void> allDiscoveredPromise;
        bool allDiscovered{false};

        std::promise<void> allReceivedPromise;
        bool allReceived{false};

        std::chrono::milliseconds communicationTimeout{20000ms};

        void PrepareAllReceivedPromise()
        {
            if (std::all_of(rpcServers.begin(), rpcServers.end(), [](RpcServerInfo s) {
                    return s.numCallsToReceive == 0;
                }))
            {
                allReceived = true;
                allReceivedPromise.set_value();
            }
        }

        void CheckAllCalledPromise()
        {
            if (!allCalled && std::all_of(rpcClients.begin(), rpcClients.end(), [](RpcClientInfo c) {
                    return c.allCalled;
                }))
            {
                allCalled = true;
                allCalledPromise.set_value();
            }
        }
        void CheckAllCallsReceivedPromise()
        {
            if (!allReceived && std::all_of(rpcServers.begin(), rpcServers.end(), [](RpcServerInfo s) {
                    return s.allReceived;
                }))
            {
                allReceived = true;
                allReceivedPromise.set_value();
            }
        }
        void CheckAllCallsReturnedPromise()
        {
            if (!allCallsReturned && std::all_of(rpcClients.begin(), rpcClients.end(), [](RpcClientInfo clientInfo) {
                    return clientInfo.allCallsReturned;
                }))
            {
                allCallsReturned = true;
                allCallsReturnedPromise.set_value();
            }
        }

        void WaitForAllCalled()
        {
            auto futureStatus = allCalledPromise.get_future().wait_for(communicationTimeout);
            EXPECT_EQ(futureStatus, std::future_status::ready) << "Test Failure: Awaiting clients to call timed out";
        }
        void WaitForAllCallsReturned()
        {
            auto futureStatus = allCallsReturnedPromise.get_future().wait_for(communicationTimeout);
            EXPECT_EQ(futureStatus, std::future_status::ready) << "Test Failure: Awaiting call return timed out";
        }
        void WaitForAllCallsReceived()
        {
            auto futureStatus = allReceivedPromise.get_future().wait_for(communicationTimeout);
            EXPECT_EQ(futureStatus, std::future_status::ready) << "Test Failure: Awaiting reception timed out";
        }
        void WaitForAllDiscovered()
        {
            auto futureStatus = allDiscoveredPromise.get_future().wait_for(communicationTimeout);
            EXPECT_EQ(futureStatus, std::future_status::ready) << "Test Failure: Awaiting server discovery timed out";
        }

        void OnRpcDiscovery(const std::vector<RpcDiscoveryResult>& discoveryResults)
        {
            for (const auto& entry : discoveryResults)
            {
                auto foundFunctionNameIter =
                    std::find(expectedFunctionNames.begin(), expectedFunctionNames.end(), entry.rpcChannel);
                if (foundFunctionNameIter != expectedFunctionNames.end())
                {
                    expectedFunctionNames.erase(foundFunctionNameIter);
                }
            }
            if (expectedFunctionNames.empty())
            {
                allDiscovered = true;
                allDiscoveredPromise.set_value();
            }
        }
    };

    void ParticipantThread(RpcParticipant& participant, uint32_t domainId, bool sync)
    {
        try
        {
            participant.participant = ib::CreateParticipant(ib::cfg::MockParticipantConfiguration(),
                                                                     participant.name, domainId, sync);

            // Create Clients
            for (auto& c : participant.rpcClients)
            {
                auto callReturnHandler = [&participant, &c](
                                             IRpcClient* /*client*/, IRpcCallHandle* /*callHandle*/,
                                             const CallStatus callStatus, const std::vector<uint8_t>& returnData) {
                    if (!c.allCallsReturned)
                    {
                        if (callStatus == CallStatus::Success)
                        {
                            c.OnCallReturned(returnData);
                        }
                    }
                    participant.CheckAllCallsReturnedPromise();
                };

                c.rpcClient =
                    participant.participant->CreateRpcClient(c.controllerName, c.rpcChannel, c.dxf, c.labels, callReturnHandler);
            }
            auto callTask = [&participant]() {
                for (auto& client : participant.rpcClients)
                {
                    client.Call();
                }
            };

            // Create Servers
            for (auto& s : participant.rpcServers)
            {
                participant.PrepareAllReceivedPromise();

                auto processCalls = [&participant, &s](IRpcServer* server, IRpcCallHandle* callHandle,
                                                             const std::vector<uint8_t>& argumentData) {
                    // Increment data
                    std::vector<uint8_t> returnData{argumentData};
                    for (auto& d : returnData)
                    {
                        d += rpcFuncIncrement;
                    }
                    server->SubmitResult(callHandle, returnData);

                    // Evaluate data and reception count
                    s.ReceiveCall(argumentData);
                    participant.CheckAllCallsReceivedPromise();
                };

                s.rpcServer = participant.participant->CreateRpcServer(s.controllerName, s.rpcChannel, s.dxf, s.labels, processCalls);
            }

            // Check RpcDiscovery after creating the local servers to discover them as well
            if (!participant.expectedFunctionNames.empty())
            {
                DiscoveryResultHandler discoveryResultsHandler =
                    [&participant](const std::vector<RpcDiscoveryResult>& discoveryResults) {
                        participant.OnRpcDiscovery(discoveryResults);
                    };

                while (!participant.allDiscovered)
                {
                    participant.participant->DiscoverRpcServers("", RpcExchangeFormat{""}, {}, discoveryResultsHandler);
                }
            }

            if (sync)
            {
                IParticipantController* participantController = participant.participant->GetParticipantController();
                participantController->SetPeriod(1s);
                participantController->SetSimulationTask([&participant, callTask](std::chrono::nanoseconds /*now*/) {
                    callTask();
                    participant.CheckAllCalledPromise();
                });
                auto finalStateFuture = participantController->RunAsync();
                finalStateFuture.get();
            }
            else
            {
                // Call by client
                if (!participant.rpcClients.empty())
                {
                    while (
                        std::none_of(participant.rpcClients.begin(), participant.rpcClients.end(), [](RpcClientInfo c) {
                            return c.allCalled;
                        }))
                    {
                        std::this_thread::sleep_for(500ms);
                        callTask();
                    }
                    participant.allCalledPromise.set_value();
                }
                // Calls received by server
                if (!participant.rpcServers.empty())
                {
                    participant.WaitForAllCallsReceived();
                }
                // Calls returned on client
                if (!participant.rpcClients.empty())
                {
                    participant.WaitForAllCallsReturned();
                }
            }

            for (const auto& c : participant.rpcClients)
            {
                EXPECT_EQ(c.callCounter, c.numCalls);
                EXPECT_EQ(c.callReturnedSuccessCounter, c.numCallsToReturn);
            }
            for (const auto& s : participant.rpcServers)
            {
                EXPECT_EQ(s.receiveCallCounter, s.numCallsToReceive);
            }
        }
        catch (const std::exception& error)
        {
            _testSystem.ShutdownOnException(error);
        }
    }

    void RunParticipants(std::vector<RpcParticipant>& rpcs, uint32_t domainId, bool sync)
    {
        try
        {
            for (auto& participant : rpcs)
            {
                _rpcThreads.emplace_back([this, &participant, domainId, sync] {
                    ParticipantThread(participant, domainId, sync);
                });
            }
        }
        catch (const std::exception& error)
        {
            _testSystem.ShutdownOnException(error);
        }
    }

    void JoinRpcThreads()
    {
        for (auto&& thread : _rpcThreads)
        {
            thread.join();
        }
    }

    void WaitForAllServersDiscovered(std::vector<RpcParticipant>& rpcs)
    {
        for (auto& r : rpcs)
        {
            if (!r.rpcClients.empty())
            {
                r.WaitForAllDiscovered();
            }
        }
    }

    void StopSimOnallCalledAndReceived(std::vector<RpcParticipant>& rpcs, bool sync)
    {
        if (sync)
        {
            for (auto& r : rpcs)
            {
                if (!r.rpcClients.empty())
                {
                    r.WaitForAllCalled();
                }
            }
            for (auto& r : rpcs)
            {
                if (!r.rpcServers.empty())
                {
                    r.WaitForAllCallsReceived();
                }
            }
            for (auto& r : rpcs)
            {
                if (!r.rpcClients.empty())
                {
                    r.WaitForAllCallsReturned();
                }
            }
            _testSystem.SystemMasterStop();
        }
    }

    void ShutdownSystem()
    {
        _rpcThreads.clear();
        _testSystem.ShutdownInfrastructure();
    }

    void RunSyncTest(std::vector<RpcParticipant>& rpcs)
    {
        const uint32_t domainId = static_cast<uint32_t>(GetTestPid());

        std::vector<std::string> requiredParticipantNames;
        for (const auto& p : rpcs)
        {
            requiredParticipantNames.push_back(p.name);
        }

        _testSystem.SetupRegistryAndSystemMaster(domainId, true, requiredParticipantNames);
        RunParticipants(rpcs, domainId, true);
        WaitForAllServersDiscovered(rpcs);
        StopSimOnallCalledAndReceived(rpcs, true);
        JoinRpcThreads();
        ShutdownSystem();
    }

    void RunAsyncTest(std::vector<RpcParticipant>& rpcs)
    {
        const uint32_t domainId = static_cast<uint32_t>(GetTestPid());

        _testSystem.SetupRegistryAndSystemMaster(domainId, false, {});
        RunParticipants(rpcs, domainId, false);
        WaitForAllServersDiscovered(rpcs);
        JoinRpcThreads();
        ShutdownSystem();
    }

protected:
    std::vector<std::thread> _rpcThreads;

private:
    TestInfrastructure _testSystem;

};