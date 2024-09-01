#include "RollBackManager.h"
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "InputBuffer.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/input.hpp>

using namespace godot;

//namespace RollTest
//{
//    String addressToConect = "127.0.0.1";
//    int port = 8843;
//}

void RollbackManager::_bind_methods()
{
    //Properties
    ClassDB::bind_method(D_METHOD("getDelay"), &RollbackManager::getDelay);
	ClassDB::bind_method(D_METHOD("setDelay", "delay"), &RollbackManager::setDelay);
    ClassDB::bind_method(D_METHOD("getRollFrames"), &RollbackManager::getRollFrames);
	ClassDB::bind_method(D_METHOD("setRollFrames", "rollFrames"), &RollbackManager::setRollFrames);
    ClassDB::bind_method(D_METHOD("netInputThreadFunc"), &RollbackManager::netInputThreadFunc);
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_processInputDelay", PROPERTY_HINT_RANGE, "0,120"), "setDelay", "getDelay");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_numRollbackFrames", PROPERTY_HINT_RANGE, "1,120"), "setRollFrames", "getRollFrames");

    //Methods
    ClassDB::bind_method(D_METHOD("addToGameState", "name", "data"), &RollbackManager::addToGameState);

    //Signals
    ADD_SIGNAL(MethodInfo("onSaveGameState"));
    ADD_SIGNAL(MethodInfo("onHandleInput", PropertyInfo(Variant::STRING, "action"), PropertyInfo(Variant::FLOAT, "value")));
    ADD_SIGNAL(MethodInfo("onFrameStart"));
    ADD_SIGNAL(MethodInfo("onFrameUpdate", PropertyInfo(Variant::FLOAT, "delta")));
    ADD_SIGNAL(MethodInfo("onFrameEnd"));

    ADD_SIGNAL(MethodInfo("onResetState", PropertyInfo(Variant::STRING, "element"), PropertyInfo(Variant::PACKED_BYTE_ARRAY, "gameState")));
}

RollbackManager::RollbackManager()
{
    
}

RollbackManager::~RollbackManager()
{
   
}


void godot::RollbackManager::_ready()
{
    if (Engine::get_singleton()->is_editor_hint())
     {
        set_process_mode(Node::ProcessMode::PROCESS_MODE_DISABLED);
        return;
     }
    else
        set_process_mode(Node::ProcessMode::PROCESS_MODE_INHERIT);

    //Init InputStates
    CustomInput::init();
    for(int i = 0; i < 256; ++i)
    {
        _inputs.push_back(InputState{});
        _inputArrivedPerFrame[i] = false;
    }

    //Init frame states
    for(int i = 0; i < _numRollbackFrames; ++i)
    {
        _savedFrames.emplace(InputState(), GameState(), 0);
    }

    //Start Net
    _socketUdp.instantiate();
    _inputArrayMutex.instantiate();
    _inputRequestMutex.instantiate();
    _inputReceivedMutex.instantiate();
    _netThread.instantiate();

    initializeUDPSocket();    
    _netThread->start(Callable(this, "netInputThreadFunc"));    
}

void godot::RollbackManager::getCurrentInput()
{
    //Get current input state
    unsigned char inputBit = 1;
    const auto inputSingleton = Input::get_singleton();
    for(const String& action : CustomInput::_customActions)
    {
        float value = Math::floor(inputSingleton->get_action_strength(action));
        _currentInputState.localInputs.actions.insert(action, value);
        _currentInputState.localInputs.encodedValue += (inputBit) * value;
        inputBit *= 2;
    }
}

void godot::RollbackManager::_physics_process(double delta)
{
    //Process Inputs
    getCurrentInput();

    _inputArrayMutex->lock();
    InputState& futureInputState = _inputs[(_frameNumber + _processInputDelay) % 256];
    futureInputState.copy(_currentInputState);
    _currentInputState.reset();

    sendInputPacket(futureInputState);
    _inputArrayMutex->unlock();

    //Create Game State
    _currentGameState.reset();
    emit_signal("onSaveGameState");

    //Frame start
    emit_signal("onFrameStart");

    if(doreset)
    {
        onResetGameState();
        doreset = false;
    }

    //Handle frame Inputs
    onHandleInput(_inputs[_frameNumber]);

    //Frame Process
    emit_signal("onFrameUpdate", delta);

    //Frame End
    emit_signal("onFrameEnd", delta);

    //Store current frame state
    _savedFrames.emplace(FrameState(_inputs[_frameNumber], _currentGameState, _frameNumber));   

    //Remove oldest frame state
    _savedFrames.pop();

    //Progress frame number
    _frameNumber >= 255 ? _frameNumber = 0 : ++_frameNumber;
    _newInputsInCurrentFrame = false;
}

void godot::RollbackManager::_exit_tree()
{
    if(!_netThread.is_null())
        _netThread->wait_to_finish();
}

void godot::RollbackManager::onHandleInput(const InputState& inputs)
{        
    for(const auto& [action, value] : inputs.localInputs.actions)
    {
        emit_signal("onHandleInput", action, value);
    }  
}

void godot::RollbackManager::addToGameState(const String &name, const PackedByteArray& data)
{
    if(_currentGameState.elementsSaved.has(name))
    {
       UtilityFunctions::print("You already have this element in the current game state");
       return;
    }

    //Add element to saved elements
    GameState::ElementBufferData elementData;
    elementData.index = _currentGameState.stateBuffer.size();
    elementData.size = data.size();

    _currentGameState.elementsIds.push_back(name); 
    _currentGameState.elementsSaved.insert(name, std::move(elementData));

    //Add data to buffer
   _currentGameState.stateBuffer.append_array(data);
}

void godot::RollbackManager::onResetGameState()
{
    const auto& oldGameState = _savedFrames.front().frameGameState;

    int acumulatedBufferSize = 0;
    auto& it = oldGameState.elementsSaved.begin();
    for(; it != oldGameState.elementsSaved.end(); ++it)
    {      
        acumulatedBufferSize += it->value.size;
        emit_signal("onResetState", it->key, oldGameState.stateBuffer.slice(it->value.index, acumulatedBufferSize));
    }
}

Error godot::RollbackManager::initializeUDPSocket()
{    
    if(_socketUdp->is_bound())
        return Error::ERR_ALREADY_EXISTS;

    //Right now only local player, probably will have to NAT Punchtrough for multiplayer
    Error ret = _socketUdp->bind(8843, "127.0.0.1");
   if(ret != godot::Error::OK)
   {
        UtilityFunctions::print("[ERROR] UDP Socket failed to bind to: ErrorType: ", ret);
        return ret;
   }

   ret = _socketUdp->set_dest_address("127.0.0.1", 8843);
    if(ret != godot::Error::OK)
   {
        UtilityFunctions::print("[ERROR] UDP Socket failed to set destiny address to: ErrorType: ", ret);
        return ret;
   }

    return ret;
}

void godot::RollbackManager::netInputThreadFunc()
{    
    while(true)
    {
        PackedByteArray netInData = _socketUdp->get_packet();

        if(netInData.is_empty())
            continue;
       
        NET_PACKET_TYPE netInputType = (NET_PACKET_TYPE)netInData[0];
        switch(netInputType)
        {
            case NET_PACKET_TYPE::INPUT:
            {                
                processInputPacket(netInData);
                break;
            }
            case NET_PACKET_TYPE::INPUT_REQUESTED:
                break;
            case NET_PACKET_TYPE::HANDSHAKE:
                break;
            case NET_PACKET_TYPE::GAME_END:
                break;
        }
        

               
    
    }
}

void godot::RollbackManager::sendInputPacket(const InputState& inputToSend)
{
    PackedByteArray netData{};
    netData.append((unsigned char)NET_PACKET_TYPE::INPUT);

    int frameToSend = ((_frameNumber + _processInputDelay) % 256);

    //Add the current frame and previous frames to help with UDP missing packets
    for(int i = 0; i < _frameSendRange; --frameToSend, ++i)
    {
        if(frameToSend < 0)
        {
            frameToSend = 255;
        }        
        netData.append((unsigned char)frameToSend);
        netData.append(_inputs[frameToSend].localInputs.encodedValue);
    }
    
    //Send the packet multiple times to help with UDP unreliavility
    for(int i = 0; i < _packetSentAmount; ++i)
    {
        Error packetError = _socketUdp->put_packet(netData);
        if(packetError != Error::OK)
        {
            UtilityFunctions::print("[Error] UDP.put_packet failed ErrorType: ", packetError);
        }
    }    
}

void godot::RollbackManager::processInputPacket(const PackedByteArray &netData)
{
    //Packet Structure
    // NET_PACKET_TYPE::INPUT + Frame 0 + Frame 0 input + Frame 1 + Frame 1 input...            
    int packetIndex = 1;
    _inputArrayMutex->lock();
    while(packetIndex < netData.size())
    {
        int netEncodedInput = netData[packetIndex + 1];
        int netFrame = netData[packetIndex];
        if(_inputArrivedPerFrame[netFrame] == true)
        {
            //We already have input for this frame
            break;
        }

        unsigned char inputBit = 1; 
        InputState& frameInputState = _inputs[netFrame];                    
        for(const String& action : CustomInput::_customActions)
        {           
            float decodedValue = 0.0f;
            if(netEncodedInput & inputBit)
            {
                decodedValue = 1.0f;
            }

            frameInputState.netInputs.actions.insert(action, decodedValue);
            inputBit *= 2;                        
        }
        _inputArrivedPerFrame[netFrame] = true;  
        packetIndex += 2;                  
    }            
    _inputArrayMutex->unlock();


    _inputReceivedMutex->lock();
    _inputReceived = true;
    if(_connectionState == NET_STATE::WAITING)
    {
        _connectionState = NET_STATE::PLAYING;
    }
    _inputReceivedMutex->unlock();
}
