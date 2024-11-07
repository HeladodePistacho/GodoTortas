#ifndef FRAME_STATE_H
#define FRAME_STATE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <InputState.h>
#include <GameState.h>

namespace godot {

    enum class FrameStatus
    {
        REAL = 0,
        GUESSED
    };

    class FrameState
    {
        public:
        InputState frameInputs{};
        GameState frameGameState{};
        int frameIndex = 0;
        FrameStatus frameStatus =  FrameStatus::REAL;

        FrameState(const InputState& inputs, const GameState& gameState, int frame, FrameStatus status);
        ~FrameState();
    };
};

#endif