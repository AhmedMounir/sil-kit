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

#include <chrono>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "silkit/participant/exception.hpp"

#include "NullConnectionParticipant.hpp"
#include "ConfigurationTestUtils.hpp"
#include "ParticipantExtentions.hpp"

namespace {

using namespace SilKit::Core;

class Test_Experimental : public testing::Test
{
protected:
    Test_Experimental() {}
};

TEST_F(Test_Experimental, create_system_controller_not_null)
{
    auto participant =
        CreateNullConnectionParticipantImpl(SilKit::Config::MakeEmptyParticipantConfiguration(), "TestParticipant");

    auto systemController = SilKit::Experimental::Participant::CreateSystemController(participant.get());
    EXPECT_NE(systemController, nullptr);
}

TEST_F(Test_Experimental, error_on_create_system_controller_twice)
{
    auto participant =
        CreateNullConnectionParticipantImpl(SilKit::Config::MakeEmptyParticipantConfiguration(), "TestParticipant");

    auto systemController = SilKit::Experimental::Participant::CreateSystemController(participant.get());
    SILKIT_UNUSED_ARG(systemController);
    EXPECT_THROW(SilKit::Experimental::Participant::CreateSystemController(participant.get()), SilKit::SilKitError);
}

} // anonymous namespace
