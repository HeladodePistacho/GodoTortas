#ifndef FRAME_STATE_H
#define FRAME_STATE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <InputState.h>
#include <GameState.h>

namespace godot {

    class FrameState
    {
        public:
        InputState frameInputs{};
        GameState frameGameState{};
        int frameIndex = 0;

        FrameState();
        FrameState(const InputState& inputs, const GameState& gameState, int frame);
        ~FrameState();
    };
};

#endif