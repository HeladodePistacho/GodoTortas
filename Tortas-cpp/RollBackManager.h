#ifndef ROLLBACK_MANAGER_H
#define ROLLBACK_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/packet_peer_udp.hpp>
#include <godot_cpp/classes/udp_server.hpp>

#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>


#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/array.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>

#include "InputState.h"
#include "GameState.h"
#include "FrameState.h"

namespace godot
{
    enum class NET_STATE : unsigned char
    {
        WAITING = 0,    //Waiting to connect to peer
        END,            //Conection finished
        PLAYING       //Connected to another peer
    };

    enum class NET_PACKET_TYPE : unsigned char
    {
        INPUT = 0,
        INPUT_REQUEST,
        HANDSHAKE,
        GAME_END
    };

    class RollbackManager : public Node 
    {
	    GDCLASS(RollbackManager, Node)

        int _processInputDelay = 5;
        int _numRollbackFrames = 10;
        float _axisSensitivity = 0.0f;

        //Between 0 and 255
        int _frameNumber = 0;

        //Inputs will cycle between 0-256
        void getCurrentInput();
        LocalVector<InputState> _inputs;

        //Queue with saved frames
        std::queue<FrameState> _savedFrames;

        InputState _currentInputState;

        GameState _currentGameState;

        bool doreset = false;
 
        // Net
        //Maybe move all of this to its own class
        NET_STATE _connectionState = NET_STATE::WAITING;
        String _stateDetails;
        Ref<PacketPeerUDP> _socketUdp;
        Ref<Thread> _netThread;

        bool _inputArrivedPerFrame[256]; //Tracks if an input has arrived for frame X
        Ref<Mutex> _inputArrayMutex;

        bool _inputRequestAvailablePerFrame[256]; //Tracks if local inputs for a given frame are ready to be sent by request
        Ref<Mutex> _inputRequestMutex;
        
        bool _inputReceived = false; //boolean to communicate between threads if new inputs have been received
        Ref<Mutex> _inputReceivedMutex; //encloses input_received and also the game variable that tracks current game status
        
        //frame range of past inputs to send every frame
        int _frameSendRange = 5;

        //amount of input packets to send per frame
        int _packetSentAmount = 3;

        String _ipToConnect;
        int _port = 0;
        int _portToListen = 0;
        int _packetLossPercentage = 0;
        Ref<RandomNumberGenerator> _randomGenerator;

        void netInputThreadFunc();
        void sendNetData(const PackedByteArray& netData);
        void sendInputPacket(const InputState& inputToSend);
        void sendInputPacket(int frameNeededStart);
        void sendRequestInputPacket(int frameNeeded);
        void sendHandshakePacket(bool isReply);
        void processInputPacket(const PackedByteArray& netData);
        void processRequestPacket(const PackedByteArray& netData);
        void processHandshakePacket(const PackedByteArray& netData);

        void updateGameState(float delta);

    protected:
	    static void _bind_methods();

    public:
	    RollbackManager();
	    ~RollbackManager();

        void _ready() override;
        void _physics_process(double delta) override;
        void _exit_tree() override;

        //Input
        void onHandleInput(const InputState& inputs);

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

        void setRollFrames(int rollFrames)
        {
            _numRollbackFrames = rollFrames;
        }
        int getRollFrames() const
        {
            return _numRollbackFrames;
        }

        void setIp(const String& ipToConnect)
        {
            _ipToConnect = ipToConnect;
        }
        const String& getIp() const
        {
            return _ipToConnect;
        }

        void setPort(int port)
        {
            _port = port;
        }
        int getPort() const        
        {
            return _port;
        }

        void setPortToListen(int port)
        {
            _portToListen = port;
        }
        int getPortToListen() const        
        {
            return _portToListen;
        }

        void setAxisSensitivity(float axisSens)
        {
            _axisSensitivity = axisSens;
        }
        float getAxisSensitivity() const 
        {
            return _axisSensitivity;
        }       

        void setPacketLossPercentage(int packetloss)
        {
            _packetLossPercentage = packetloss;
        }
        int getPacketLossPercentage() const 
        {
            return _packetLossPercentage;
        }   

        int getCurrentFrame() const 
        {
            return _frameNumber;
        }    
        bool getInputArrivedForFrame(int frame) const 
        {
            return _inputArrivedPerFrame[frame];
        }
        int getLocalInputForFrame(int frame) const 
        {
            return _inputs[frame].localInputs.encodedValue;
        }
        int getNetInputForFrame(int frame) const 
        {
            return _inputs[frame].netInputs.encodedValue;
        }

        //Net
        Error initializeUDPSocket();

        //Test
        void printConnectionState();
    };
}




#endif