#include "../src/Gesture.h"
#include <cassert>
#include <limits>
using namespace goggles;
int main() {
    Frame f{};
    f.valid = f.head.valid = f.head.tracked = 1;
    f.head.w = 1;
    f.head.position = {0,1.7f,0};
    for (int i=0;i<2;i++) {
        f.hands[i].grip = f.head;
        f.hands[i].grip.position = {i ? .16f : -.16f,1.65f,-.06f};
    }
    Gesture g;
    auto sample = [&](unsigned down,bool eligible=true) {
        f.buttons[0] = down&1;
        f.buttons[5] = (down>>1)&1;
        f.buttons[6] = 1;
        unsigned fired = g.filter(f,eligible);
        assert(f.buttons[6] == 1);
        return fired;
    };
    assert(sample(1)==0 && f.buttons[0]==1);
    sample(0);
    assert(sample(1)==1 && !f.buttons[0]);
    f.hands[0].grip.position.z=-.8f;
    assert(sample(1)==0 && !f.buttons[0]);
    sample(0);
    assert(sample(1)==0 && f.buttons[0]);
    f.hands[0].grip.position.z=-.06f;
    assert(sample(1)==0 && f.buttons[0]);
    sample(0);
    assert(sample(2)==2 && !f.buttons[5]);
    sample(0);
    assert(sample(3)==3 && !f.buttons[0] && !f.buttons[5]);
    sample(0);
    assert(sample(3,false)==0 && f.buttons[0] && f.buttons[5]);
    assert(sample(3,true)==0);
    sample(0);
    f.hands[1].grip.tracked=0;
    assert(sample(2)==0 && f.buttons[5]);
    f.hands[1].grip.tracked=1;
    assert(sample(2)==0);
    sample(0);
    g.interrupt();
    assert(sample(2)==0);
    sample(0);
    f.head.y=f.head.w=std::sqrt(.5f);
    f.hands[1].grip.position={-.065f,1.66f,-.165f};
    assert(nearTemple(f,1));
    assert(sample(2)==2);
    sample(0);
    f.hands[1].grip.position={-.065f,1.66f,.165f};
    assert(sample(2)==0);
    f.head.w=std::numeric_limits<float>::quiet_NaN();
    assert(!nearTemple(f,0));
}
