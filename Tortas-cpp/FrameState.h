#ifndef FRAME_STATE_H
#define FRAME_STATE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <InputState.h>
#include <GameState.h>

namespace godot {

    class FrameState : public RefCounted
    {
        GDCLASS(FrameState, RefCounted)

        protected:
	        static void _bind_methods();

        public:
        InputState frameInputs{};
        GameState frameGameState{};
        int frameIndex = 0;

        FrameState() = default;
        FrameState(const InputState& inputs, const GameState& gameState, int frame) : 
        frameInputs(inputs),
        frameGameState(gameState),
        frameIndex(frame)
        {
            
        }
    };
};

#endif