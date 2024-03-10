#ifndef ROLLBACK_MANAGER_H
#define ROLLBACK_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/packet_peer_udp.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/array.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <godot_cpp/classes/thread.hpp>
#include "InputState.h"

namespace godot
{
    enum class NET_STATE
    {
        WAITING = 0,    //Waiting to connect to peer
        END,            //Conection finished
        CONNECTED       //Connected to another pier
    };

  

    
    struct GameState
    {
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
        GameState& operator=(const GameState& other) 
        {
            if (this != &other) {
                elementsSaved = other.elementsSaved;
                stateBuffer = other.stateBuffer;
                elementsIds = other.elementsIds;
            }
            return *this;
        }

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

    struct FrameState
    {
        InputState frameInputs{};
        GameState frameGameState{};
        int frameIndex = 0;

        FrameState(const InputState& inputs, const GameState& gameState, int frame) : 
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
        Vector<Ref<InputState>> _inputs;

        //Queue with saved frames
        std::queue<FrameState> _savedFrames;

        Ref<InputState> _currentInputState;
        //GameState _currentGameState{};

        bool doreset = false;
/*
        // Net
        NET_STATE connectionState = NET_STATE::WAITING;
        String stateDetails;
        PacketPeerUDP udpPeer;
        Thread netThread;*/

    protected:
	    static void _bind_methods();

    public:
	    RollbackManager();
	    ~RollbackManager();

        void _ready() override;
        void _unhandled_input(const Ref<InputEvent>& event) override;
        void _physics_process(double delta) override;

        void onHandleInput(const Ref<InputState> inputs);

        /*GameState& getCurrentGameState();
        void addToGameState(const String &name, const PackedByteArray& data);
        void onResetGameState();
      
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
        }*/
    };
}




#endif