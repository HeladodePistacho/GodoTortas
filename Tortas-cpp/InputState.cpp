#include "InputState.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

godot::InputState::InputState()
{

}

InputState::~InputState()
{

}

InputState::InputState(const InputState &other) : localInputs(other.localInputs)
{

}

InputState::InputState(InputState&& other) noexcept : localInputs(std::move(other.localInputs))
{

}

InputState& InputState::operator=(InputState&& other) noexcept
{
    if (this != &other) 
    {
        localInputs.actions = std::move(other.localInputs.actions);
        localInputs.values = std::move(other.localInputs.values);
    }
    return *this;
}

void godot::InputState::copy(const InputState &other)
{
    reset();
    localInputs.actions.append_array(other.localInputs.actions);
    localInputs.values.append_array(other.localInputs.values);
    localInputs.encodedValue  = other.localInputs.encodedValue;
}

void godot::InputState::reset()
{    
    localInputs.reset();
    netInputs.reset();
}
