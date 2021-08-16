// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <iostream>

#include "ib/sim/fr/FrDatatypes.hpp"

namespace ib {
namespace sim {
namespace fr {

bool operator==(const Header& lhs, const Header& rhs);
bool operator==(const Frame& lhs, const Frame& rhs);
bool operator==(const FrMessage& lhs, const FrMessage& rhs);
bool operator==(const FrMessageAck& lhs, const FrMessageAck& rhs);
bool operator==(const FrSymbol& lhs, const FrSymbol& rhs);
bool operator==(const TxBufferConfigUpdate& lhs, const TxBufferConfigUpdate& rhs);
bool operator==(const TxBufferUpdate& lhs, const TxBufferUpdate& rhs);
bool operator==(const ControllerConfig& lhs, const ControllerConfig& rhs);
bool operator==(const HostCommand& lhs, const HostCommand& rhs);
bool operator==(const ControllerStatus& lhs, const ControllerStatus& rhs);
bool operator==(const PocStatus& lhs, const PocStatus& rhs);
bool operator==(const CycleStart& lhs, const CycleStart& rhs);


} // namespace fr
} // namespace sim
} // namespace ib
