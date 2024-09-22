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
        netInputs.actions = std::move(other.netInputs.actions);
        localInputs.encodedValue  = other.localInputs.encodedValue;
        netInputs.encodedValue  = other.netInputs.encodedValue;
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

    for(const auto& [action, value] : other.netInputs.actions)
    {
        netInputs.actions.insert(action, value);
    }

    localInputs.encodedValue  = other.localInputs.encodedValue;
    netInputs.encodedValue  = other.netInputs.encodedValue;
}

void godot::InputState::reset()
{    
    localInputs.reset();
    netInputs.reset();
    localInputs.encodedValue = 0;
    netInputs.encodedValue = 0;
}

void godot::InputState::print()
{
    for(const auto& [action, value] : localInputs.actions)
    {
        UtilityFunctions::print("action: ", action, "value: ", value);
    }

    UtilityFunctions::print("Encoded value: ", localInputs.encodedValue);
}
