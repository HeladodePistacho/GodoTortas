#include "InputState.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

godot::InputState::InputState()
{

}

InputState::~InputState()
{

}

InputState::InputState(const InputState &other) : localInputs(other.localInputs), netInputs(other.netInputs)
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

void godot::InputState::copyLocalInput(const InputState &other)
{
    resetLocalInput();
    
    for(const auto& [action, value] : other.localInputs.actions)
    {
        localInputs.actions.insert(action, value);
    }

    localInputs.encodedValue  = other.localInputs.encodedValue;
}

void godot::InputState::resetLocalInput()
{    
    localInputs.reset();
    localInputs.encodedValue = 0;
}

void godot::InputState::resetNetInput()
{
    netInputs.reset();
    netInputs.encodedValue = 0;
}

void godot::InputState::print()
{
    for(const auto& [action, value] : localInputs.actions)
    {
        UtilityFunctions::print("action: ", action, "value: ", value);
    }

    UtilityFunctions::print("Encoded value: ", localInputs.encodedValue);

    for(const auto& [action, value] : netInputs.actions)
    {
        UtilityFunctions::print("Net action: ", action, "Net value: ", value);
    }

    UtilityFunctions::print("Net Encoded value: ", netInputs.encodedValue);
}
