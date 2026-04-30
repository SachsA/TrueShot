#pragma once

#include "target.h"

#include <glm/glm.hpp>
#include <vector>

// The world owns targets, the player score and round timer.
// It is rendering- and OpenGL-agnostic; the Renderer pulls
// data from it each frame.
class GameWorld {
public:
    GameWorld();

    // Per-frame update: advance respawn timers, etc.
    void update(float deltaTime);

    // Spawn the default practice-range layout.
    void spawnDefaultTargets();

    // Raycast against alive targets. Returns the index of the closest
    // hit target, or -1 if nothing was hit. Fills outDistance and
    // outHitPoint on success.
    int raycastTargets(const glm::vec3& origin,
                       const glm::vec3& dir,
                       float maxDistance,
                       float& outDistance,
                       glm::vec3& outHitPoint) const;

    // Apply damage to a target by index. Returns true if the target
    // was killed by this hit (so the caller can award score).
    bool damageTarget(int index, float damage);

    // Accessors
    std::vector<Target>&       targets()       { return m_Targets; }
    const std::vector<Target>& targets() const { return m_Targets; }

    int  score() const { return m_Score; }
    int  hits()  const { return m_Hits;  }
    int  shots() const { return m_Shots; }
    int  kills() const { return m_Kills; }

    void registerShot()     { ++m_Shots; }
    void registerHit()      { ++m_Hits;  }
    float accuracy() const  { return m_Shots > 0 ? float(m_Hits) / float(m_Shots) : 0.0f; }

    void resetStats();

private:
    std::vector<Target> m_Targets;

    int m_Score = 0;
    int m_Hits  = 0;
    int m_Shots = 0;
    int m_Kills = 0;
};
