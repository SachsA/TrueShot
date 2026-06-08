#pragma once

#include <cstdint>

// Drives both client and server simulation at a fixed tick rate
// (128 Hz by default — see ADR-002). Decoupled from real time:
// callers feed it deltaTime, it returns a steady stream of ticks.
//
// Usage (per frame):
//   clock.advance(deltaTime);
//   while (clock.consumeTick()) {
//       simulateOneTick();
//   }
//   // optionally, for rendering interpolation:
//   float alpha = clock.interpolationAlpha();
//   renderInterpolated(alpha);
//
// On the server side, `advance(deltaTime)` is called from a thread that
// sleeps until the next tick boundary; on the client side it is called
// from the main render loop with real frame time. Same code, same
// semantics.
class TickClock {
public:
    // Construct with a target tick rate in Hz. Defaults to the project
    // wide Physics::TICK_RATE (128). Any rate ≥ 1 Hz is valid; the
    // server can override this for testing slower-paced gamemodes.
    explicit TickClock(double tickRateHz = 128.0);

    // Reset to tick 0 and discard any accumulated time.
    void reset();

    // Feed real time into the clock. Safe to call with deltaTime == 0.
    void advance(double deltaTime);

    // Pop one tick if enough time has accumulated. Returns true and
    // increments currentTick(); returns false when no more whole ticks
    // are pending.
    //
    // Typical usage:
    //   while (clock.consumeTick()) { stepSimulation(); }
    bool consumeTick();

    // Current tick counter — monotonically increasing across the whole
    // run. Wraps at uint32 max (~388 days at 128 Hz) which we do not
    // worry about for a match-length game.
    uint32_t currentTick() const { return m_Tick; }

    // 0..1 fraction of the next tick that has been advanced into but
    // not yet consumed. Useful to interpolate visual state between
    // simulation steps for smoother rendering.
    float interpolationAlpha() const;

    // Wall-time duration of a single tick (seconds). Constant after
    // construction.
    double tickInterval() const { return m_TickInterval; }

    // Convert a tick count to seconds, and vice versa.
    double ticksToSeconds(uint32_t ticks) const {
        return static_cast<double>(ticks) * m_TickInterval;
    }
    uint32_t secondsToTicks(double seconds) const {
        return static_cast<uint32_t>(seconds / m_TickInterval);
    }

private:
    double m_TickInterval = 1.0 / 128.0;
    double m_Accumulator  = 0.0;
    uint32_t m_Tick       = 0;
};
