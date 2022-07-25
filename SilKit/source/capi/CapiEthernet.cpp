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

#include "silkit/capi/SilKit.h"
#include "silkit/SilKit.hpp"
#include "silkit/services/logging/ILogger.hpp"
#include "silkit/services/orchestration/all.hpp"
#include "silkit/services/orchestration/string_utils.hpp"
#include "silkit/services/ethernet/all.hpp"
#include "silkit/services/ethernet/string_utils.hpp"

#include <cstring>
#include "CapiImpl.hpp"

#define ETHERNET_MIN_FRAME_SIZE 60

SilKit_ReturnCode SilKit_EthernetController_Create(SilKit_EthernetController** outController, SilKit_Participant* participant,
                                            const char* name, const char* network)
{
    ASSERT_VALID_OUT_PARAMETER(outController);
    ASSERT_VALID_POINTER_PARAMETER(participant);
    ASSERT_VALID_POINTER_PARAMETER(name);
    ASSERT_VALID_POINTER_PARAMETER(network);
    CAPI_ENTER
    {
        auto cppParticipant = reinterpret_cast<SilKit::IParticipant*>(participant);
        auto ethernetController = cppParticipant->CreateEthernetController(name, network);
        *outController = reinterpret_cast<SilKit_EthernetController*>(ethernetController);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKit_EthernetController_Activate(SilKit_EthernetController* controller)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        cppController->Activate();
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKit_EthernetController_Deactivate(SilKit_EthernetController* controller)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        cppController->Deactivate();
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKit_EthernetController_AddFrameHandler(SilKit_EthernetController* controller, void* context,
                                                            SilKit_EthernetFrameHandler_t handler,
                                                            SilKit_Direction directionMask,
                                                            SilKit_HandlerId* outHandlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    ASSERT_VALID_OUT_PARAMETER(outHandlerId);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        auto cppHandlerId = cppController->AddFrameHandler(
            [handler, context, controller](auto* /*ctrl*/,
                                           const auto& cppFrameEvent) {
                auto& cppFrame = cppFrameEvent.frame;
                auto* dataPointer = !cppFrame.raw.empty() ? cppFrame.raw.data() : nullptr;

                SilKit_EthernetFrameEvent frameEvent{};
                SilKit_EthernetFrame frame{};
                frame.raw = {dataPointer, cppFrame.raw.size()};

                SilKit_Struct_Init(SilKit_EthernetFrameEvent, frameEvent);
                SilKit_Struct_Init(SilKit_EthernetFrame, frame);
                frameEvent.ethernetFrame = &frame;
                frameEvent.timestamp = cppFrameEvent.timestamp.count();

                handler(context, controller, &frameEvent);
            }, directionMask);
        *outHandlerId = static_cast<SilKit_HandlerId>(cppHandlerId);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKit_EthernetController_RemoveFrameHandler(SilKit_EthernetController* controller, SilKit_HandlerId handlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        cppController->RemoveFrameHandler(static_cast<SilKit::Util::HandlerId>(handlerId));
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKit_EthernetController_AddFrameTransmitHandler(SilKit_EthernetController* controller,
                                                                    void* context,
                                                                    SilKit_EthernetFrameTransmitHandler_t handler,
                                                                    SilKit_EthernetTransmitStatus transmitStatusMask,
                                                                    SilKit_HandlerId* outHandlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    ASSERT_VALID_OUT_PARAMETER(outHandlerId);
    CAPI_ENTER
    {
        using SilKit::Services::Ethernet::IEthernetController;
        using SilKit::Services::Ethernet::EthernetFrameTransmitEvent;
        using SilKit::Services::Ethernet::EthernetTransmitStatusMask;

        auto cppController = reinterpret_cast<IEthernetController*>(controller);
        auto cppHandlerId = cppController->AddFrameTransmitHandler(
            [context, handler](IEthernetController* controller, const EthernetFrameTransmitEvent& frameTransmitEvent) {
                SilKit_EthernetFrameTransmitEvent cEvent;
                SilKit_Struct_Init(SilKit_EthernetFrameTransmitEvent, cEvent);
                cEvent.status = (SilKit_EthernetTransmitStatus)frameTransmitEvent.status;
                cEvent.timestamp = frameTransmitEvent.timestamp.count();
                cEvent.userContext = frameTransmitEvent.userContext;
                handler(context, reinterpret_cast<SilKit_EthernetController*>(controller), &cEvent);
            }, transmitStatusMask);
        *outHandlerId = static_cast<SilKit_HandlerId>(cppHandlerId);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}
SilKit_ReturnCode SilKit_EthernetController_RemoveFrameTransmitHandler(SilKit_EthernetController* controller,
                                                                SilKit_HandlerId handlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        cppController->RemoveFrameTransmitHandler(static_cast<SilKit::Util::HandlerId>(handlerId));
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKit_EthernetController_AddStateChangeHandler(SilKit_EthernetController* controller, void* context,
                                                           SilKit_EthernetStateChangeHandler_t handler,
                                                           SilKit_HandlerId* outHandlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    ASSERT_VALID_OUT_PARAMETER(outHandlerId);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        auto cppHandlerId = cppController->AddStateChangeHandler(
            [handler, context, controller](SilKit::Services::Ethernet::IEthernetController*,
                                           const SilKit::Services::Ethernet::EthernetStateChangeEvent& stateChangeEvent) {
                SilKit_EthernetStateChangeEvent cStateChangeEvent;
                SilKit_Struct_Init(SilKit_EthernetStateChangeEvent, cStateChangeEvent);
                cStateChangeEvent.timestamp = stateChangeEvent.timestamp.count();
                cStateChangeEvent.state = (SilKit_EthernetState)stateChangeEvent.state;
                handler(context, controller, &cStateChangeEvent);
            });
        *outHandlerId = static_cast<SilKit_HandlerId>(cppHandlerId);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}
SilKit_ReturnCode SilKit_EthernetController_RemoveStateChangeHandler(SilKit_EthernetController* controller,
                                                              SilKit_HandlerId handlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        cppController->RemoveStateChangeHandler(static_cast<SilKit::Util::HandlerId>(handlerId));
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKit_EthernetController_AddBitrateChangeHandler(SilKit_EthernetController* controller, void* context,
                                                             SilKit_EthernetBitrateChangeHandler_t handler,
                                                             SilKit_HandlerId* outHandlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    ASSERT_VALID_HANDLER_PARAMETER(handler);
    ASSERT_VALID_OUT_PARAMETER(outHandlerId);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        auto cppHandlerId = cppController->AddBitrateChangeHandler(
            [handler, context, controller](SilKit::Services::Ethernet::IEthernetController*,
                                           const SilKit::Services::Ethernet::EthernetBitrateChangeEvent& bitrateChangeEvent) {
                SilKit_EthernetBitrateChangeEvent cBitrateChangeEvent;
                SilKit_Struct_Init(SilKit_EthernetBitrateChangeEvent, cBitrateChangeEvent);
                cBitrateChangeEvent.timestamp = bitrateChangeEvent.timestamp.count();
                cBitrateChangeEvent.bitrate = (SilKit_EthernetBitrate)bitrateChangeEvent.bitrate;

                handler(context, controller, &cBitrateChangeEvent);
            });
        *outHandlerId = static_cast<SilKit_HandlerId>(cppHandlerId);
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}
SilKit_ReturnCode SilKit_EthernetController_RemoveBitrateChangeHandler(SilKit_EthernetController* controller,
                                                                SilKit_HandlerId handlerId)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    CAPI_ENTER
    {
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);
        cppController->RemoveBitrateChangeHandler(static_cast<SilKit::Util::HandlerId>(handlerId));
        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}

SilKit_ReturnCode SilKit_EthernetController_SendFrame(SilKit_EthernetController* controller, SilKit_EthernetFrame* frame,
                                               void* userContext)
{
    ASSERT_VALID_POINTER_PARAMETER(controller);
    ASSERT_VALID_POINTER_PARAMETER(frame);
    CAPI_ENTER
    {
        if (frame->raw.size < ETHERNET_MIN_FRAME_SIZE)
        {
            SilKit_error_string = "An ethernet frame must be at least 60 bytes in size.";
            return SilKit_ReturnCode_BADPARAMETER;
        }
        using std::chrono::duration;
        auto cppController = reinterpret_cast<SilKit::Services::Ethernet::IEthernetController*>(controller);

        SilKit::Services::Ethernet::EthernetFrame ef;
        std::vector<uint8_t> rawFrame(frame->raw.data, frame->raw.data + frame->raw.size);
        ef.raw = rawFrame;
        cppController->SendFrame(ef, userContext);

        return SilKit_ReturnCode_SUCCESS;
    }
    CAPI_LEAVE
}
