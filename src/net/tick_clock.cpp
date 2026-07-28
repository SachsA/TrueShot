#include "net/tick_clock.h"

#include <algorithm>

TickClock::TickClock(double tickRateHz) {
    // Clamp to a sane minimum so we never divide by zero or negative.
    if (tickRateHz < 1.0) tickRateHz = 1.0;
    m_TickInterval = 1.0 / tickRateHz;
    reset();
}

void TickClock::reset() {
    m_Accumulator = 0.0;
    m_Tick        = 0;
}

void TickClock::advance(double deltaTime) {
    if (deltaTime <= 0.0) return;
    m_Accumulator += deltaTime;

    // Cap the accumulator so a long stall (debugger pause, tab swap,
    // breakpoint) does not produce a flood of catch-up ticks.
    // 0.25 s ≈ 32 ticks at 128 Hz — past that, drop time on the floor.
    constexpr double kMaxAccumulator = 0.25;
    m_Accumulator                    = std::min(m_Accumulator, kMaxAccumulator);
}

bool TickClock::consumeTick() {
    if (m_Accumulator < m_TickInterval) return false;
    m_Accumulator -= m_TickInterval;
    ++m_Tick;
    return true;
}

float TickClock::interpolationAlpha() const {
    if (m_TickInterval <= 0.0) return 0.0f;
    const double a = m_Accumulator / m_TickInterval;
    if (a <= 0.0) return 0.0f;
    if (a >= 1.0) return 1.0f;
    return static_cast<float>(a);
}
