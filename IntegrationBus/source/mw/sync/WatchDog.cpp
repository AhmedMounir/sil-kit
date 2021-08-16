// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "WatchDog.hpp"

#include <iostream>

using namespace std::chrono_literals;

namespace ib {
namespace mw {
namespace sync {

WatchDog::WatchDog(std::chrono::milliseconds warnTimeout, std::chrono::milliseconds errorTimeout)
    : _warnTimeout{warnTimeout}
    , _errorTimeout{errorTimeout}
    , _warnHandler{[](std::chrono::milliseconds) {}}
    , _errorHandler{[](std::chrono::milliseconds) {}}
{
    if (warnTimeout <= 0ms)
        throw std::runtime_error{"WatchDog requires warnTimeout > 0ms"};
    if (errorTimeout <= 0ms)
        throw std::runtime_error{"WatchDog requires errorTimeout > 0ms"};
        
    _watchThread = std::thread{&WatchDog::Run, this};
}

WatchDog::~WatchDog()
{
    _stopPromise.set_value();
    _watchThread.join();
}

void WatchDog::Start()
{
    _startTime.store(std::chrono::steady_clock::now().time_since_epoch());
}

void WatchDog::Reset()
{
    _startTime.store(std::chrono::steady_clock::duration::min());
}

void WatchDog::SetWarnHandler(std::function<void(std::chrono::milliseconds)> handler)
{
    _warnHandler = std::move(handler);
}

void WatchDog::SetErrorHandler(std::function<void(std::chrono::milliseconds)> handler)
{
    _errorHandler = std::move(handler);
}

void WatchDog::Run()
{

    enum class WatchDogState
    {
        Healthy,
        Warn,
        Error
    };
    
    WatchDogState state = WatchDogState::Healthy;
    auto stopFuture = _stopPromise.get_future();
    
    while (true)
    {
        auto futureStatus = stopFuture.wait_for(_resolution);

        if (futureStatus == std::future_status::ready)
        {
            // stop was signaled; stopping thread;
            return;
        }
        
        const auto startTime = _startTime.load();
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        const auto currentRunDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

        // We only communicate with the "main thread" via the atomic _startTime.
        // If _startTime is duration::min(), Start() has not yet been called.
        // Otherwise, _startTime is the duration since epoch when the Start() was called.
        if (startTime == std::chrono::steady_clock::duration::min())
        {
            // no job is currently running. Reset state and continue.
            state = WatchDogState::Healthy;
            continue;
        }


        if (currentRunDuration <= _warnTimeout)
        {
            state = WatchDogState::Healthy;
            continue;
        }
        if (currentRunDuration > _warnTimeout && currentRunDuration <= _errorTimeout)
        {
            if (state == WatchDogState::Healthy)
            {
                _warnHandler(currentRunDuration);
                state = WatchDogState::Warn;
            }
            continue;
        }

        if (currentRunDuration > _errorTimeout)
        {
            if (state != WatchDogState::Error)
            {
                _errorHandler(currentRunDuration);
                state = WatchDogState::Error;
            }
            continue;
        }
    }
}

    
} // namespace sync
} // namespace mw
} // namespace ib
