// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include "IIbServiceEndpoint.hpp"

namespace ib {
namespace mw {

template<typename T>
class IIbMessageReceiver
{
public:
    virtual void ReceiveIbMessage(const ib::mw::IIbServiceEndpoint* from, const T& msg) = 0;
};

} // namespace mw
} // namespace ib