#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <chrono>

namespace Net {

using Tick = uint32_t;
using PlayerId = uint32_t;

struct Vec3 { float x,y,z; };

struct InputState {
    Tick tick;
    uint32_t seq; // sequence for ordered reliable acks
    float forward; // -1..1
    float right;   // -1..1
    bool jump;
    bool fire;
    float yaw, pitch; // view angles
};

struct EntityState {
    PlayerId id;
    Vec3 pos;
    Vec3 vel;
    float yaw, pitch;
};

struct Snapshot {
    Tick tick;
    std::vector<EntityState> entities;
};

} // namespace Net
