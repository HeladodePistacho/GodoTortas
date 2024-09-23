#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

    struct InputElement
    {
        HashMap<String, float> actions;
        unsigned char encodedValue = 0;

        void reset() 
        {
            actions.clear();
            encodedValue = 0;
        }
    };

    //This will hold the input state of each frame
    class InputState
    {        
        public:
        InputElement localInputs;
        InputElement netInputs;

        InputState();
        ~InputState();

        // Copy constructor
        InputState(const InputState& other);

        // Move constructor
        InputState(InputState&& other) noexcept;

        // Move assignment operator
        InputState& operator=(InputState&& other) noexcept;

        void copyLocalInput(const InputState& other);
        void resetLocalInput();
        void resetNetInput();
        void print();
    };
};

#endif