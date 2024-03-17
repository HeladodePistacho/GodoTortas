#include "FrameState.h"
#include <godot_cpp/variant/utility_functions.hpp>

godot::FrameState::FrameState()
{
    UtilityFunctions::print("FrameState()");
}

godot::FrameState::FrameState(const InputState &inputs, const GameState &gameState, int frame) : frameInputs(inputs), frameGameState(gameState), frameIndex(frame)
{
      
}

godot::FrameState::~FrameState()
{

}
