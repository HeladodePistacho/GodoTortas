#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

    struct InputElement
    {
        TypedArray<String> actions;
        TypedArray<float> values;

        void reset() 
        {
            actions.clear();
            values.clear();
        }
    };

    //This will hold the input state of each frame
    class InputState : public RefCounted
    {        
        GDCLASS(InputState, RefCounted)
        protected:
	        static void _bind_methods();

        public:
        InputElement localInputs;
        InputElement netInputs;

        InputState() = default;
        ~InputState();

        // Copy constructor
        InputState(const InputState& other);

        // Move constructor
        InputState(InputState&& other) noexcept;

        // Move assignment operator
        InputState& operator=(InputState&& other) noexcept;

        void reset();
    };
};

#endif