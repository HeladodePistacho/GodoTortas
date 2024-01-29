#include "InputBuffer.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace InputBufferConsts
{
    constexpr int bufferSize = 10; //Inputs will be 10 frames here
}

void InputBuffer::_bind_methods() {
}

InputBuffer::InputBuffer() {
	// Initialize any variables here.
}

InputBuffer::~InputBuffer() {
	// Add your cleanup here.
}

void InputBuffer::_ready()
{
    set_process_priority(0);

    //Fill the buffer
    for(int i = 0; i < InputBufferConsts::bufferSize; ++i)
    {
        buffer.emplace_back(InputElement());
    }
}

void InputBuffer::_unhandled_input(const Ref<InputEvent> &event)
{    
    //Add InputElemnts to the buffer
    const auto& allActions = InputMap::get_singleton()->get_actions();    
    for(int i = 0; i < allActions.size(); ++i)
    {
        const StringName& currentAction = allActions[i];
        UtilityFunctions::print(currentAction);
        if(event->is_action(allActions[i]))
        {
            buffer.emplace_front(InputElement(*(event.ptr()), currentAction));
            
        }
    }

    //Send signal to    
}

void InputBuffer::_process(double delta)
{
   // UtilityFunctions::print("--------------------------------");
    for(InputElement& input : buffer)
    {
        //UtilityFunctions::print(input.getAction());
    }

}
