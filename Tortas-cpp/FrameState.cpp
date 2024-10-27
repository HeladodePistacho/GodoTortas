#include "FrameState.h"
#include <godot_cpp/variant/utility_functions.hpp>

godot::FrameState::FrameState(const InputState &inputs, const GameState &gameState, int frame, FrameStatus status) : 
frameInputs(inputs), 
frameGameState(gameState), 
frameIndex(frame),
frameStatus(status)
{
      
}

godot::FrameState::~FrameState()
{

}
