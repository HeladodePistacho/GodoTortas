#ifndef INPUTBUFFER_H
#define INPUTBUFFER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {

struct InputElement
{
	InputEvent input;
	bool processed = false;
};

class InputBuffer : public Node {
	GDCLASS(InputBuffer, Node)

private:
	double time_passed;

protected:
	static void _bind_methods();

public:
	InputBuffer();
	~InputBuffer();

	void _input(const Ref<InputEvent>& event) override;
	void _process(double delta) override;

	std::vector<InputElement> buffer;
};

}

#endif