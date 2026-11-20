// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace goggles {
struct Vec {
    float x, y, z;
};
struct Pose {
    Vec position;
    float x, y, z, w;
    std::uint8_t valid, tracked, padding[2];
};
struct Frame {
    Pose head;
    struct { Pose aim, grip; } hands[2];
    struct { Pose pose; float fov[4]; } eyes[2];
    float sticks[4], triggers[2], grips[2];
    std::uint8_t buttons[12], valid, padding[3];
};
static_assert(sizeof(Pose) == 32 && sizeof(Frame) == 0x130);
static_assert(offsetof(Frame, hands) == 0x20 && offsetof(Frame, buttons) == 0x120);

// TTWVR 1.1.7 maps the first face button to X on the left and A on the right.
constexpr int primary[2] = {0, 5};

inline Vec cross(Vec a, Vec b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
inline bool nearTemple(const Frame &f, int side) {
    const auto &h = f.head;
    const auto &hand = f.hands[side].grip;
    if (!h.valid || !hand.valid || !hand.tracked)
        return false;
    float norm = h.x*h.x + h.y*h.y + h.z*h.z + h.w*h.w;
    if (!(std::abs(norm - 1.f) < .05f))
        return false;
    Vec d{hand.position.x-h.position.x, hand.position.y-h.position.y, hand.position.z-h.position.z};
    Vec inverse{-h.x, -h.y, -h.z};
    Vec t = cross(inverse, d), u = cross(inverse, t);
    // Head-local metres. The grip origin sits a little outside, below and in front of a temple.
    float x = d.x + 2*(h.w*t.x+u.x) - (side ? .165f : -.165f);
    float y = d.y + 2*(h.w*t.y+u.y) + .04f;
    float z = d.z + 2*(h.w*t.z+u.z) + .065f;
    return x*x + y*y + z*z <= .13f*.13f;
}

class Gesture {
    unsigned down = 3; // Require a release after startup or interrupted tracking.
    unsigned captured = 0;
public:
    // Returns the newly pressed hands. Claim their button through release even if the hand moves.
    unsigned filter(Frame &f, bool eligible) {
        if (!f.valid) {
            down = 3;
            return 0;
        }
        unsigned pressed = 0;
        for (int h = 0; h < 2; ++h) {
            const unsigned bit = 1u << h;
            auto &button = f.buttons[primary[h]];
            if (!button) {
                captured &= ~bit;
                if (f.hands[h].grip.valid && f.hands[h].grip.tracked)
                    down &= ~bit;
                else
                    down |= bit;
                continue;
            }
            if (!(down & bit) && eligible && nearTemple(f,h)) {
                captured |= bit;
                pressed |= bit;
            }
            down |= bit;
            if (captured & bit)
                button = 0;
        }
        return pressed;
    }
    void interrupt() { down = 3; } // Never turn a held button into a fresh press.
};
}
