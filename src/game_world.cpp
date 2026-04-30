#include "game_world.h"

#include <algorithm>
#include <limits>

GameWorld::GameWorld() {
    spawnDefaultTargets();
}

void GameWorld::spawnDefaultTargets() {
    m_Targets.clear();

    // Layout inspired by an aim-training range: a mix of close,
    // mid, and long-range targets at varied heights.
    struct Layout {
        glm::vec3 pos;
        float scale;
        float spin;
        float hp;
        int score;
    };

    const Layout layouts[] = {
        {{ 0.0f,  1.0f, -10.0f}, 1.0f, 0.6f, 100.0f, 100},
        {{ 5.0f,  1.5f, -15.0f}, 1.1f, 0.5f, 100.0f, 100},
        {{-5.0f,  1.5f, -15.0f}, 1.1f, 0.5f, 100.0f, 100},
        {{ 0.0f,  2.0f, -25.0f}, 1.4f, 0.4f, 100.0f, 150},
        {{10.0f,  1.0f, -20.0f}, 1.3f, 0.7f, 100.0f, 125},
        {{-10.0f, 1.0f, -20.0f}, 1.3f, 0.7f, 100.0f, 125},
        {{ 0.0f,  0.5f, -35.0f}, 1.7f, 0.3f, 100.0f, 200},
        {{ 3.0f,  3.0f, -12.0f}, 1.0f, 0.8f, 100.0f, 150},
    };

    m_Targets.reserve(sizeof(layouts) / sizeof(layouts[0]));
    for (const auto& l : layouts) {
        Target t;
        t.position    = l.pos;
        t.scale       = l.scale;
        t.spinSpeed   = l.spin;
        t.maxHp       = l.hp;
        t.hp          = l.hp;
        t.scoreValue  = l.score;
        m_Targets.push_back(t);
    }
}

void GameWorld::update(float deltaTime) {
    for (auto& t : m_Targets) {
        if (!t.alive) {
            t.respawnTimer += deltaTime;
            if (t.respawnTimer >= t.respawnDelay) {
                t.alive        = true;
                t.hp           = t.maxHp;
                t.respawnTimer = 0.0f;
            }
        }
    }
}

int GameWorld::raycastTargets(const glm::vec3& origin,
                              const glm::vec3& dir,
                              float maxDistance,
                              float& outDistance,
                              glm::vec3& outHitPoint) const {
    int   bestIndex = -1;
    float bestDist  = std::numeric_limits<float>::infinity();
    glm::vec3 bestHit{0.0f};

    for (size_t i = 0; i < m_Targets.size(); ++i) {
        float dist = 0.0f;
        glm::vec3 hp{0.0f};
        if (m_Targets[i].intersectRay(origin, dir, maxDistance, dist, hp)) {
            if (dist < bestDist) {
                bestDist  = dist;
                bestHit   = hp;
                bestIndex = static_cast<int>(i);
            }
        }
    }

    if (bestIndex >= 0) {
        outDistance = bestDist;
        outHitPoint = bestHit;
    }
    return bestIndex;
}

bool GameWorld::damageTarget(int index, float damage) {
    if (index < 0 || index >= static_cast<int>(m_Targets.size())) return false;

    Target& t = m_Targets[index];
    if (!t.alive) return false;

    t.hp -= damage;
    if (t.hp <= 0.0f) {
        t.alive        = false;
        t.hp           = 0.0f;
        t.respawnTimer = 0.0f;
        m_Score += t.scoreValue;
        ++m_Kills;
        return true;
    }
    return false;
}

void GameWorld::resetStats() {
    m_Score = 0;
    m_Hits  = 0;
    m_Shots = 0;
    m_Kills = 0;
}
