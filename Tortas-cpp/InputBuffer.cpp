#include "InputBuffer.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace InputBufferConsts
{
    constexpr int bufferSize = 10; //Inputs will be 10 frames here
}

void InputBuffer::_bind_methods() {
}

InputBuffer::InputBuffer() {
	// Initialize any variables here.
    buffer.reserve(InputBufferConsts::bufferSize);
}

InputBuffer::~InputBuffer() {
	// Add your cleanup here.
}

void InputBuffer::_input(const Ref<InputEvent> &event)
{
    
}

void InputBuffer::_process(double delta)
{
    //this->_input
}
