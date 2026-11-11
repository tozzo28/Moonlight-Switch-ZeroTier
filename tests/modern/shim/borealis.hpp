#pragma once
// Minimal platform-free input surface. Math and event implementations below
// are the production Borealis code; no HID or display behavior is emulated.
#include <ctime>
#include <cstdlib>
#include <map>
#include <borealis/core/logger.hpp>
#include <borealis/core/event.hpp>
#include <borealis/core/geometry.hpp>
namespace brls {
enum class TouchPhase { START, STAY, END, NONE };
enum class GestureState { INTERRUPTED, UNSURE, START, STAY, END, FAILED };
class View;
enum class Sound { NONE };
struct TouchState {int fingerId=0;TouchPhase phase=TouchPhase::NONE;Point position;View *view=nullptr;};
struct MouseState {};
struct TapGestureStatus {GestureState state;Point position;};
class GestureRecognizer {
public:
    virtual ~GestureRecognizer()=default;
    virtual GestureState recognitionLoop(TouchState,MouseState,View*,Sound*) {return state;}
protected:
    GestureState state=GestureState::FAILED;
};
}
