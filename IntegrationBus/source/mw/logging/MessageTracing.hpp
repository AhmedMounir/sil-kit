// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include "ib/mw/logging/ILogger.hpp"
#include "../internal/IIbServiceEndpoint.hpp"

namespace ib {
namespace mw {


template<class IbMessageT>
void TraceRx(logging::ILogger* logger, const IIbServiceEndpoint* addr, const IbMessageT& msg)
{
  logger->Trace("Recv from {}: {}", addr->GetServiceDescriptor(), msg);
}

template<class IbMessageT>
void TraceTx(logging::ILogger* logger, const IIbServiceEndpoint* addr, const IbMessageT& msg)
{
  logger->Trace("Send from {}: {}", addr->GetServiceDescriptor(), msg);
}

// Don't trace LogMessages - this could cause cycles!
inline void TraceRx(logging::ILogger* /*logger*/, IIbServiceEndpoint* /*addr*/, const logging::LogMsg& /*msg*/) {}
inline void TraceTx(logging::ILogger* /*logger*/, IIbServiceEndpoint* /*addr*/, const logging::LogMsg& /*msg*/) {}

} // namespace mw
} // namespace ib

