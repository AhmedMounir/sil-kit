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

#include "ParticipantConfiguration.hpp"

#include "silkit/capi/SilKit.h"
#include "silkit/SilKit.hpp"
#include "silkit/services/logging/ILogger.hpp"
#include "silkit/services/orchestration/all.hpp"
#include "silkit/services/orchestration/string_utils.hpp"
#include "silkit/participant/exception.hpp"

#include "IParticipantInternal.hpp"
#include "LifecycleService.hpp"
#include "CapiImpl.hpp"
#include "TypeConversion.hpp"

#include <memory>
#include <string>
#include <iostream>
#include <algorithm>
#include <map>
#include <mutex>
#include <cstring>

extern "C"
{

SilKit_ReturnCode SilKitCALL SilKit_SystemMonitor_Create(SilKit_SystemMonitor** outSystemMonitor,
                                                 SilKit_Participant* participant)
{
    ASSERT_VALID_OUT_PARAMETER(outSystemMonitor);
    ASSERT_VALID_POINTER_PARAMETER(participant);
    CAPI_ENTER
    {
        auto cppParticipant = reinterpret_cast<SilKit::IParticipant*>(participant);
        auto systemMonitor = cppParticipant->CreateSystemMonitor();
        *outSystemMonitor = reinterpret_cast<SilKit_SystemMonitor*>(systemMonitor);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

static auto from_c(const SilKit_LifecycleConfiguration* csc)
{
    SilKit::Services::Orchestration::LifecycleConfiguration cpp;
    cpp.operationMode = static_cast<decltype(cpp.operationMode)>(csc->operationMode);
    return cpp;
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_Create(SilKit_LifecycleService** outLifecycleService,
                                                 SilKit_Participant* participant,
                                                 const SilKit_LifecycleConfiguration* startConfiguration)
{
    ASSERT_VALID_OUT_PARAMETER(outLifecycleService);
    ASSERT_VALID_POINTER_PARAMETER(participant);
    ASSERT_VALID_POINTER_PARAMETER(startConfiguration);
    ASSERT_VALID_STRUCT_HEADER(startConfiguration);
    CAPI_ENTER
    {
        auto cppParticipant = reinterpret_cast<SilKit::IParticipant*>(participant);
        auto cppLifecycleService = cppParticipant->CreateLifecycleService(from_c(startConfiguration));
        *outLifecycleService = reinterpret_cast<SilKit_LifecycleService*>(
            static_cast<SilKit::Services::Orchestration::ILifecycleService*>(cppLifecycleService));
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_TimeSyncService_Create(SilKit_TimeSyncService** outTimeSyncService, SilKit_LifecycleService* lifecycleService)
{
    ASSERT_VALID_OUT_PARAMETER(outTimeSyncService);
    ASSERT_VALID_POINTER_PARAMETER(lifecycleService);
    CAPI_ENTER
    {
        auto cppLifecycleService =
            reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(lifecycleService);
        auto timeSyncService = dynamic_cast<SilKit::Services::Orchestration::LifecycleService*>(cppLifecycleService)->CreateTimeSyncService();
        *outTimeSyncService = reinterpret_cast<SilKit_TimeSyncService*>(timeSyncService);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_SetCommunicationReadyHandler(SilKit_LifecycleService* lifecycleService, void* context,
                                                          SilKit_LifecycleService_CommunicationReadyHandler_t handler)
{
    ASSERT_VALID_POINTER_PARAMETER(lifecycleService);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    CAPI_ENTER
    {
        auto* cppLifecycleService = reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(lifecycleService);
        
        cppLifecycleService->SetCommunicationReadyHandler([handler, context, lifecycleService]() {
            handler(context, lifecycleService);
        });

        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_SetCommunicationReadyHandlerAsync(SilKit_LifecycleService* lifecycleService, void* context,
                                                          SilKit_LifecycleService_CommunicationReadyHandler_t handler)
{
    ASSERT_VALID_POINTER_PARAMETER(lifecycleService);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    CAPI_ENTER
    {
        auto* cppLifecycleService = reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(lifecycleService);
        
        cppLifecycleService->SetCommunicationReadyHandlerAsync([handler, context, lifecycleService]() {
            handler(context, lifecycleService);
        });

        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_CompleteCommunicationReadyHandlerAsync(SilKit_LifecycleService* lifecycleService)
{
    ASSERT_VALID_POINTER_PARAMETER(lifecycleService);
    CAPI_ENTER
    {
        auto* cppLifecycleService = reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(lifecycleService);
        
        cppLifecycleService->CompleteCommunicationReadyHandlerAsync();

        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_SetStartingHandler(
    SilKit_LifecycleService* lifecycleService, void* context,
    SilKit_LifecycleService_StartingHandler_t handler)
{
    ASSERT_VALID_POINTER_PARAMETER(lifecycleService);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    CAPI_ENTER
    {
        auto* cppLifecycleService =
            reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(lifecycleService);

        cppLifecycleService->SetStartingHandler([handler, context, lifecycleService]() {
            handler(context, lifecycleService);
        });

        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_SetStopHandler(SilKit_LifecycleService* clifecycleService, void* context,
                                            SilKit_LifecycleService_StopHandler_t handler)
{
    ASSERT_VALID_POINTER_PARAMETER(clifecycleService);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    CAPI_ENTER
    {
        auto* cppLifecycleService =
            reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(clifecycleService);

        cppLifecycleService->SetStopHandler([handler, context, clifecycleService]() {
            handler(context, clifecycleService);
        });
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_SetShutdownHandler(SilKit_LifecycleService* clifecycleService, void* context,
                                                SilKit_LifecycleService_ShutdownHandler_t handler)
{
    ASSERT_VALID_POINTER_PARAMETER(clifecycleService);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    CAPI_ENTER
    {
        auto* cppLifecycleService =
            reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(clifecycleService);

        cppLifecycleService->SetShutdownHandler([handler, context, clifecycleService]() {
            handler(context, clifecycleService);
        });
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_SetAbortHandler(
    SilKit_LifecycleService* lifecycleService, void* context, SilKit_LifecycleService_AbortHandler_t handler)
{
    ASSERT_VALID_POINTER_PARAMETER(lifecycleService);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    CAPI_ENTER
    {
        auto* cppLifecycleService =
            reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(lifecycleService);

        cppLifecycleService->SetAbortHandler(
            [handler, context,
             lifecycleService](SilKit::Services::Orchestration::ParticipantState cppParticipantState) {
                handler(context, lifecycleService, static_cast<SilKit_ParticipantState>(cppParticipantState));
            });
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

// Lifecycle async execution
static std::map<SilKit_LifecycleService*, std::future<SilKit::Services::Orchestration::ParticipantState>> sRunAsyncFuturePerParticipant;

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_StartLifecycle(SilKit_LifecycleService* clifecycleService)
{
    ASSERT_VALID_POINTER_PARAMETER(clifecycleService);
    CAPI_ENTER
    {
        auto* cppLifecycleService =
            reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(
                clifecycleService);

        sRunAsyncFuturePerParticipant[clifecycleService] =
            cppLifecycleService->StartLifecycle();

        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_WaitForLifecycleToComplete(SilKit_LifecycleService* clifecycleService,
                                                        SilKit_ParticipantState* outParticipantState)
{
    ASSERT_VALID_POINTER_PARAMETER(clifecycleService);
    ASSERT_VALID_OUT_PARAMETER(outParticipantState);
    CAPI_ENTER
    {
        if (sRunAsyncFuturePerParticipant.find(clifecycleService) == sRunAsyncFuturePerParticipant.end())
        {
            SilKit_error_string = "Unknown participant to wait for completion of asynchronous run operation";
            return SilKit_ReturnCode_BADPARAMETER;
        }
        if (!sRunAsyncFuturePerParticipant[clifecycleService].valid())
        {
            SilKit_error_string = "Failed to access asynchronous run operation";
            return SilKit_ReturnCode_UNSPECIFIEDERROR;
        }
        auto finalState = sRunAsyncFuturePerParticipant[clifecycleService].get();
        *outParticipantState = static_cast<SilKit_ParticipantState>(finalState);
        sRunAsyncFuturePerParticipant.erase(clifecycleService);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_TimeSyncService_SetSimulationStepHandler(
    SilKit_TimeSyncService* ctimeSyncService, void* context, SilKit_TimeSyncService_SimulationStepHandler_t handler,
    SilKit_NanosecondsTime initialStepSize)
{
    ASSERT_VALID_POINTER_PARAMETER(ctimeSyncService);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    CAPI_ENTER
    {
        auto* timeSyncService = reinterpret_cast<SilKit::Services::Orchestration::ITimeSyncService*>(ctimeSyncService);
        timeSyncService->SetSimulationStepHandler(
            [handler, context, ctimeSyncService](std::chrono::nanoseconds now, std::chrono::nanoseconds duration) {
                handler(context, ctimeSyncService, static_cast<SilKit_NanosecondsTime>(now.count()),
                        static_cast<SilKit_NanosecondsTime>(duration.count()));
            },
            std::chrono::nanoseconds(initialStepSize));
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_TimeSyncService_SetSimulationStepHandlerAsync(
    SilKit_TimeSyncService* ctimeSyncService, void* context, SilKit_TimeSyncService_SimulationStepHandler_t handler,
    SilKit_NanosecondsTime initialStepSize)
{
    ASSERT_VALID_POINTER_PARAMETER(ctimeSyncService);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    CAPI_ENTER
    {
        auto* timeSyncService = reinterpret_cast<SilKit::Services::Orchestration::ITimeSyncService*>(ctimeSyncService);

        timeSyncService->SetSimulationStepHandlerAsync(
            [handler, context, ctimeSyncService](std::chrono::nanoseconds now, std::chrono::nanoseconds duration) {
                handler(context, ctimeSyncService, static_cast<SilKit_NanosecondsTime>(now.count()),
                        static_cast<SilKit_NanosecondsTime>(duration.count()));
            },
            std::chrono::nanoseconds(initialStepSize));
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_TimeSyncService_CompleteSimulationStep(SilKit_TimeSyncService* ctimeSyncService)
{
    ASSERT_VALID_POINTER_PARAMETER(ctimeSyncService);
    CAPI_ENTER
    {
        auto* timeSyncService = reinterpret_cast<SilKit::Services::Orchestration::ITimeSyncService*>(ctimeSyncService);
        timeSyncService->CompleteSimulationStep();
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_Pause(SilKit_LifecycleService* clifecycleService, const char* reason)
{
    ASSERT_VALID_POINTER_PARAMETER(clifecycleService);
    ASSERT_VALID_POINTER_PARAMETER(reason);
    CAPI_ENTER
    {
        auto* lifeCycleService =
            reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(clifecycleService);
        lifeCycleService->Pause(reason);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_LifecycleService_Continue(SilKit_LifecycleService* clifecycleService)
{
    ASSERT_VALID_POINTER_PARAMETER(clifecycleService);
    CAPI_ENTER
    {
        auto* lifeCycleService =
            reinterpret_cast<SilKit::Services::Orchestration::ILifecycleService*>(clifecycleService);
        lifeCycleService->Continue();
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

// SystemMonitor related functions
SilKit_ReturnCode SilKitCALL SilKit_SystemMonitor_GetParticipantStatus(SilKit_ParticipantStatus* outParticipantState,
                                                            SilKit_SystemMonitor* csystemMonitor,
                                                            const char* participantName)
{
    ASSERT_VALID_POINTER_PARAMETER(csystemMonitor);
    ASSERT_VALID_OUT_PARAMETER(outParticipantState);
    ASSERT_VALID_POINTER_PARAMETER(participantName);
    ASSERT_VALID_STRUCT_HEADER(outParticipantState);
    CAPI_ENTER
    {
        auto* systemMonitor = reinterpret_cast<SilKit::Services::Orchestration::ISystemMonitor*>(csystemMonitor);
        auto& participantStatus = systemMonitor->ParticipantStatus(participantName);
        SilKit_ParticipantStatus cstatus {};
        SilKit_Struct_Init(SilKit_ParticipantStatus, cstatus);

        cstatus.enterReason = participantStatus.enterReason.c_str();
        cstatus.enterTime = participantStatus.enterTime.time_since_epoch().count();
        cstatus.participantName = participantStatus.participantName.c_str();
        cstatus.participantState = (SilKit_ParticipantState)participantStatus.state;

        *outParticipantState = cstatus;
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_SystemMonitor_GetSystemState(SilKit_SystemState* outParticipantState, SilKit_SystemMonitor* csystemMonitor)
{
    ASSERT_VALID_POINTER_PARAMETER(csystemMonitor);
    ASSERT_VALID_OUT_PARAMETER(outParticipantState);
    CAPI_ENTER
    {
        auto* systemMonitor = reinterpret_cast<SilKit::Services::Orchestration::ISystemMonitor*>(csystemMonitor);
        auto systemState = systemMonitor->SystemState();
        *outParticipantState = (SilKit_SystemState)systemState;
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_SystemMonitor_AddSystemStateHandler(SilKit_SystemMonitor* csystemMonitor, void* context,
                                                   SilKit_SystemStateHandler_t handler, SilKit_HandlerId* outHandlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(csystemMonitor);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    ASSERT_VALID_OUT_PARAMETER(outHandlerId);
    CAPI_ENTER
    {
        auto* systemMonitor = reinterpret_cast<SilKit::Services::Orchestration::ISystemMonitor*>(csystemMonitor);

        auto cppHandlerId = systemMonitor->AddSystemStateHandler(
            [handler, context, csystemMonitor](SilKit::Services::Orchestration::SystemState systemState) {
                handler(context, csystemMonitor, (SilKit_SystemState)systemState);
            });
        *outHandlerId = static_cast<SilKit_HandlerId>(cppHandlerId);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_SystemMonitor_RemoveSystemStateHandler(SilKit_SystemMonitor* csystemMonitor, SilKit_HandlerId handlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(csystemMonitor);
    CAPI_ENTER
    {
        auto* systemMonitor = reinterpret_cast<SilKit::Services::Orchestration::ISystemMonitor*>(csystemMonitor);

        systemMonitor->RemoveSystemStateHandler(static_cast<SilKit::Util::HandlerId>(handlerId));

        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_SystemMonitor_AddParticipantStatusHandler(SilKit_SystemMonitor* csystemMonitor, void* context,
                                                         SilKit_ParticipantStatusHandler_t handler,
                                                         SilKit_HandlerId* outHandlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(csystemMonitor);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    ASSERT_VALID_OUT_PARAMETER(outHandlerId);
    CAPI_ENTER
    {
        auto* systemMonitor = reinterpret_cast<SilKit::Services::Orchestration::ISystemMonitor*>(csystemMonitor);

        auto cppHandlerId = systemMonitor->AddParticipantStatusHandler(
            [handler, context, csystemMonitor](SilKit::Services::Orchestration::ParticipantStatus cppStatus) {
                SilKit_ParticipantStatus cStatus;
                SilKit_Struct_Init(SilKit_ParticipantStatus, cStatus);

                cStatus.enterReason = cppStatus.enterReason.c_str();
                cStatus.enterTime =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(cppStatus.enterTime.time_since_epoch())
                        .count();
                cStatus.participantName = cppStatus.participantName.c_str();
                cStatus.participantState = (SilKit_ParticipantState)cppStatus.state;
                cStatus.refreshTime =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(cppStatus.refreshTime.time_since_epoch())
                        .count();
                handler(context, csystemMonitor, cppStatus.participantName.c_str(), &cStatus);
            });
        *outHandlerId = static_cast<SilKit_HandlerId>(cppHandlerId);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKitCALL SilKit_SystemMonitor_RemoveParticipantStatusHandler(SilKit_SystemMonitor* csystemMonitor,
                                                                      SilKit_HandlerId handlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(csystemMonitor);
    CAPI_ENTER
    {
        auto* systemMonitor = reinterpret_cast<SilKit::Services::Orchestration::ISystemMonitor*>(csystemMonitor);

        systemMonitor->RemoveParticipantStatusHandler(static_cast<SilKit::Util::HandlerId>(handlerId));

        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

} //extern "C"
