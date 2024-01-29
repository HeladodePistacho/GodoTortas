#ifndef INPUTBUFFER_H
#define INPUTBUFFER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <queue>

namespace godot {

struct InputElement
{
	public:
	InputElement() = default;
	InputElement(const InputEvent& event, const StringName& eventAction) : 
	action(eventAction),
	device(event.get_device()),
	pressed(event.is_action_pressed(eventAction)),
	released(event.is_action_released(eventAction)),
	value(event.get_action_strength(eventAction))
	{

	}

	int getDevice() const { return device; }
	const StringName& getAction() const { return action; }
	bool isAction(const StringName& act) const { return action == act; }
	bool isPressed() const { return pressed; }
	bool isReleased() const { return released; }
	double getValue() const { return value; }

	bool isProcessed() const { return processed; }

	private:
	int device = 0;
	StringName action = "";
	bool pressed = false;
	bool released = false;
	double value = 0.0f;

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

    void _ready() override;
	void _unhandled_input(const Ref<InputEvent>& event) override;
	void _process(double delta) override;

	//std::queue<InputElement> buffer;
	std::deque<InputElement> buffer;
};

}

#endif