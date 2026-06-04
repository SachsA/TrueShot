#include "target.h"

#include <algorithm>
#include <cmath>
#include <limits>

// Slab-based ray vs AABB intersection.
// Returns the nearest positive intersection distance along the ray.
bool Target::intersectRay(const glm::vec3& origin, const glm::vec3& dir, float maxDistance,
                          float& outDistance, glm::vec3& outHitPoint) const {
    if (!alive) return false;

    const glm::vec3 minB = position - halfExtents * scale;
    const glm::vec3 maxB = position + halfExtents * scale;

    float tmin           = -std::numeric_limits<float>::infinity();
    float tmax           = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; ++i) {
        const float o = origin[i];
        const float d = dir[i];

        if (std::fabs(d) < 1e-6f) {
            // Ray parallel to the slab: must be inside on this axis.
            if (o < minB[i] || o > maxB[i]) return false;
            continue;
        }

        float t1 = (minB[i] - o) / d;
        float t2 = (maxB[i] - o) / d;
        if (t1 > t2) std::swap(t1, t2);

        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);

        if (tmin > tmax) return false;
    }

    // Pick the nearest non-negative hit.
    float t = (tmin >= 0.0f) ? tmin : tmax;
    if (t < 0.0f || t > maxDistance) return false;

    outDistance = t;
    outHitPoint = origin + dir * t;
    return true;
}
