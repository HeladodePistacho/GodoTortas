#ifndef ROLLBACK_MANAGER_H
#define ROLLBACK_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/packet_peer_udp.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>


#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/array.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <godot_cpp/classes/thread.hpp>

#include "InputState.h"
#include "GameState.h"
#include "FrameState.h"

namespace godot
{
    enum class NET_STATE
    {
        WAITING = 0,    //Waiting to connect to peer
        END,            //Conection finished
        CONNECTED       //Connected to another pier
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
        Ref<GameState> _currentGameState;

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

        //Input
        void onHandleInput(const Ref<InputState> inputs);

        //Game state 
        void addToGameState(const String &name, const PackedByteArray& data);
        void onResetGameState();
      
        //Properties
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