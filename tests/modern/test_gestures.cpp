#include "click_gesture_recognizer.hpp"
#include "fingers_gesture_recognizer.hpp"
#include "two_finger_scroll_recognizer.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace brls;
static GestureState touch(GestureRecognizer &g,int id,TouchPhase phase,float x=100,float y=100) {
    TouchState t;t.fingerId=id;t.phase=phase;t.position=Point(x,y);
    return g.recognitionLoop(t,{},nullptr,nullptr);
}
int main() {
    int clicks=0;
    ClickGestureRecognizer left(1,[&](TapGestureStatus s){if(s.state==GestureState::END)++clicks;});
    touch(left,1,TouchPhase::START);touch(left,1,TouchPhase::END);
    assert(clicks==1);
    touch(left,1,TouchPhase::START);touch(left,1,TouchPhase::STAY,180,100);touch(left,1,TouchPhase::END,180,100);
    assert(clicks==1); // Dragging does not also tap-click.
    int rightClicks=0;
    ClickGestureRecognizer right(2,[&](TapGestureStatus s){if(s.state==GestureState::END)++rightClicks;});
    touch(right,1,TouchPhase::START);touch(right,2,TouchPhase::START,200,100);
    touch(right,1,TouchPhase::END);touch(right,2,TouchPhase::END,200,100);
    assert(rightClicks==1);
    int keyboards=0;
    FingersGestureRecognizer keyboard([]{return 3;},[&]{++keyboards;});
    touch(keyboard,1,TouchPhase::START);touch(keyboard,2,TouchPhase::START);
    assert(keyboards==0);touch(keyboard,3,TouchPhase::START);assert(keyboards==1);
    for(int i=1;i<=3;++i)touch(keyboard,i,TouchPhase::END);
    for(int i=1;i<=3;++i)touch(keyboard,i,TouchPhase::START);
    assert(keyboards==2);
    int disabled=0;FingersGestureRecognizer off([]{return -1;},[&]{++disabled;});
    for(int i=1;i<=3;++i)touch(off,i,TouchPhase::START);
    assert(disabled==0);
    int starts=0,moves=0;Point last;
    TwoFingerScrollGestureRecognizer scroll([&](TwoFingerScrollState s){if(s.state==GestureState::START)++starts;if(s.state==GestureState::STAY){++moves;last=s.delta;}});
    touch(scroll,1,TouchPhase::START);touch(scroll,2,TouchPhase::START,200,100);
    assert(starts==1);touch(scroll,2,TouchPhase::STAY,200,140);assert(moves==1&&std::abs(last.y)>0);
    assert(touch(scroll,1,TouchPhase::END)==GestureState::END);
    touch(scroll,2,TouchPhase::END);scroll.forceReset();
    touch(scroll,1,TouchPhase::START);touch(scroll,1,TouchPhase::STAY,100,150);
    assert(moves==1);
    puts("original gestures: tap, drag exclusion, two-finger right click/scroll and three-finger keyboard OK");
}
