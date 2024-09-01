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
    }
    return *this;
}

void godot::InputState::copy(const InputState &other)
{
    reset();
    
    for(const auto& [action, value] : other.localInputs.actions)
    {
        localInputs.actions.insert(action, value);
    }

    localInputs.encodedValue  = other.localInputs.encodedValue;
}

void godot::InputState::reset()
{    
    localInputs.reset();
    netInputs.reset();
}

void godot::InputState::print()
{
    for(const auto& [action, value] : localInputs.actions)
    {
        UtilityFunctions::print("action: ", action, "value: ", value);
    }

    UtilityFunctions::print("Encoded value: ", localInputs.encodedValue);
}
