#ifndef ROLLBACK_MANAGER_H
#define ROLLBACK_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/packet_peer_udp.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/array.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>

namespace godot
{
    //This will hold the input state of each frame
    struct InputState
    {
        public:
        TypedArray<String> actions;
        TypedArray<float> values;

        HashMap<String, float> localInputs;
        //std::unordered_map<std::string, float> localInputs;

        InputState() = default;

        // Copy constructor
        InputState(const InputState& other) : 
        localInputs(other.localInputs),
        actions(other.actions),
        values(other.values)
        {}

        // Move constructor
        InputState(InputState&& other) noexcept : 
        localInputs(std::move(other.localInputs)), 
        actions(std::move(other.actions)),
        values(std::move(other.values))
        {}

        InputState& operator=(const InputState& other) noexcept
        {
            if (this != &other) 
            {
                localInputs = other.localInputs;
                actions = other.actions;
                values = other.values;
            }
            return *this;
        }

        // Move assignment operator
        InputState& operator=(InputState&& other) noexcept
        {
            if (this != &other) 
            {
                localInputs = std::move(other.localInputs);
                actions = std::move(other.actions);
                values = std::move(other.values);
            }
            return *this;
        }

        void reset()
        {
            actions.clear();
            values.clear();
        }
    };

    
    struct GameState
    {
        struct ElementBufferData
        {
            int index = 0;
            int size = 0;
        };

        HashMap<String /*Name*/, ElementBufferData> elementsSaved;
        PackedByteArray stateBuffer;

        GameState() = default;

        // Copy Constructor
        GameState(const GameState& other) : elementsSaved(other.elementsSaved), stateBuffer(other.stateBuffer) {}

        // Move Constructor
        GameState(GameState&& other) noexcept : elementsSaved(std::move(other.elementsSaved)), stateBuffer(std::move(other.stateBuffer)) {}

        // Copy Assignment Operator
        GameState& operator=(const GameState& other) 
        {
            if (this != &other) {
                elementsSaved = other.elementsSaved;
                stateBuffer = other.stateBuffer;
            }
            return *this;
        }

        // Move Assignment Operator
        GameState& operator=(GameState&& other) noexcept 
        {
            if (this != &other) {
                elementsSaved = std::move(other.elementsSaved);
                stateBuffer = std::move(other.stateBuffer);
            }
            return *this;
        }

        void reset()
        {
            elementsSaved.clear();
            stateBuffer.clear();
        }
    };

    struct FrameState
    {
        InputState frameInputs{};
        GameState frameGameState{};
        int frameIndex = 0;

        FrameState(InputState&& inputs, GameState&& gameState, int frame) : 
        frameInputs(inputs),
        frameGameState(gameState),
        frameIndex(frame)
        {
            
        }
    };

    class RollbackManager : public Node 
    {
	    GDCLASS(RollbackManager, Node)

        int _processInputDelay = 5;
        int _numRollbackFrames = 10;

        //Between 0 and 255
        int _frameNumber = 0;

        //Inputs will cycle between 0-256
        std::vector<InputState> _inputs;

        //Queue with saved frames
        std::queue<FrameState> _savedFrames;

        InputState _currentInputState{};
        GameState _currentGameState{};

    protected:
	    static void _bind_methods();

    public:
	    RollbackManager();
	    ~RollbackManager();

        void _ready() override;
        void _unhandled_input(const Ref<InputEvent>& event) override;
        void _process(double delta) override;

        void onHandleInput(const InputState& inputs);

        GameState& getCurrentGameState();
        void addToGameState(const String &name, const PackedByteArray& data);

        bool test = true;

        void setDelay(const int delay)
        {
            _processInputDelay = delay;
        }

        int getDelay() const
        {
            return _processInputDelay;
        }

        void setRollFrames(const int rollFrames)
        {
            _numRollbackFrames = rollFrames;
        }

        int getRollFrames() const
        {
            return _numRollbackFrames;
        }
    };
}




#endif