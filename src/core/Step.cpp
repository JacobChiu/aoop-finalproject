#include "core/Step.h"

#include <algorithm>

Step::Step(bool active, int velocity) : isActive(active) {
    setVelocity(velocity);
}

void Step::toggle() { isActive = !isActive; }

void Step::setActive(bool state) { isActive = state; }

bool Step::getIsActive() const { return isActive; }

int Step::getVelocity() const { return velocity; }

void Step::setVelocity(int v) { velocity = std::clamp(v, kMinVelocity, kMaxVelocity); }
