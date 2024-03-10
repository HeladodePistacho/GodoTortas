#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

namespace godot {

    class GameState : public RefCounted
    {
        GDCLASS(GameState, RefCounted)
        protected:
	        static void _bind_methods();
        public:

        struct ElementBufferData
        {
            int index = 0;
            int size = 0;
        };

        HashMap<String /*Name*/, ElementBufferData> elementsSaved;
        TypedArray<String> elementsIds;
        PackedByteArray stateBuffer;

        GameState() = default;

        // Copy Constructor
        GameState(const GameState& other) : elementsSaved(other.elementsSaved), stateBuffer(other.stateBuffer), elementsIds(other.elementsIds) {}

        // Move Constructor
        GameState(GameState&& other) noexcept : elementsSaved(std::move(other.elementsSaved)), stateBuffer(std::move(other.stateBuffer)), elementsIds(std::move(other.elementsIds)) {}

        // Copy Assignment Operator
        //GameState& operator=(const GameState& other) 
        //{
        //    if (this != &other) {
        //        elementsSaved = other.elementsSaved;
        //        stateBuffer = other.stateBuffer;
        //        elementsIds = other.elementsIds;
        //    }
        //    return *this;
        //}

        // Move Assignment Operator
        GameState& operator=(GameState&& other) noexcept 
        {
            if (this != &other) {
                elementsSaved = std::move(other.elementsSaved);
                stateBuffer = std::move(other.stateBuffer);
                elementsIds = std::move(other.elementsIds);
            }
            return *this;
        }

        void reset()
        {
            elementsSaved.clear();
            elementsIds.clear();
            stateBuffer.clear();
        }
    };
};

#endif