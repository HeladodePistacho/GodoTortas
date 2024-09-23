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
    ClassDB::bind_method(D_METHOD("getAxisSensitivity"), &RollbackManager::getAxisSensitivity);
	ClassDB::bind_method(D_METHOD("setAxisSensitivity", "axisSens"), &RollbackManager::setAxisSensitivity);   
    ClassDB::bind_method(D_METHOD("getCurrentFrame"), &RollbackManager::getCurrentFrame);
    ClassDB::bind_method(D_METHOD("getInputArrivedForFrame", "frame"), &RollbackManager::getInputArrivedForFrame);
    ClassDB::bind_method(D_METHOD("getLocalInputForFrame", "frame"), &RollbackManager::getLocalInputForFrame);
    ClassDB::bind_method(D_METHOD("getNetInputForFrame", "frame"), &RollbackManager::getNetInputForFrame);

    ClassDB::bind_method(D_METHOD("getIp"), &RollbackManager::getIp);
	ClassDB::bind_method(D_METHOD("setIp", "ipToConnect"), &RollbackManager::setIp);
    ClassDB::bind_method(D_METHOD("getPort"), &RollbackManager::getPort);
	ClassDB::bind_method(D_METHOD("setPort", "port"), &RollbackManager::setPort);
    ClassDB::bind_method(D_METHOD("getPortToListen"), &RollbackManager::getPortToListen);
	ClassDB::bind_method(D_METHOD("setPortToListen", "port"), &RollbackManager::setPortToListen);
    ClassDB::bind_method(D_METHOD("getPacketLossPercentage"), &RollbackManager::getPacketLossPercentage);
	ClassDB::bind_method(D_METHOD("setPacketLossPercentage", "packetloss"), &RollbackManager::setPacketLossPercentage);

    ClassDB::bind_method(D_METHOD("netInputThreadFunc"), &RollbackManager::netInputThreadFunc);
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_processInputDelay", PROPERTY_HINT_RANGE, "0,120"), "setDelay", "getDelay");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_numRollbackFrames", PROPERTY_HINT_RANGE, "1,120"), "setRollFrames", "getRollFrames");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::FLOAT, "_axisSensitivity", PROPERTY_HINT_RANGE, "0.0,1.0,0.05"), "setAxisSensitivity", "getAxisSensitivity");

    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::STRING, "_ipToConnect", PROPERTY_HINT_NONE, "Ip"), "setIp", "getIp");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_port", PROPERTY_HINT_RANGE, "1,15000"), "setPort", "getPort");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_portToListen", PROPERTY_HINT_RANGE, "1,15000"), "setPortToListen", "getPortToListen");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_packetLossPercentage", PROPERTY_HINT_RANGE, "0,100"), "setPacketLossPercentage", "getPacketLossPercentage");

    //Methods
    ClassDB::bind_method(D_METHOD("addToGameState", "name", "data"), &RollbackManager::addToGameState);

    //Signals
    ADD_SIGNAL(MethodInfo("onSaveGameState"));
    ADD_SIGNAL(MethodInfo("onHandleInput", PropertyInfo(Variant::STRING, "action"), PropertyInfo(Variant::FLOAT, "value")));
    ADD_SIGNAL(MethodInfo("onHandleNetInput", PropertyInfo(Variant::STRING, "action"), PropertyInfo(Variant::FLOAT, "value")));
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
        _inputRequestAvailablePerFrame[i] = false;
    }

    //Init frame states
    for(int i = 0; i < _numRollbackFrames; ++i)
    {
        _savedFrames.emplace(InputState(), GameState(), 0);
    }

    //We assume empty inputs for first frames of the game
    for(int i = 0; i < _processInputDelay; ++i)
    {
        _inputArrivedPerFrame[i] = true;
        _inputRequestAvailablePerFrame[i] = true;
    }

    //Start Net
    _socketUdp.instantiate();
    _inputArrayMutex.instantiate();
    _inputRequestMutex.instantiate();
    _inputReceivedMutex.instantiate();
    _netThread.instantiate();

    initializeUDPSocket();    
    _netThread->start(Callable(this, "netInputThreadFunc")); 

    _randomGenerator.instantiate(); 
    _randomGenerator->set_seed(1234567);  
}

void godot::RollbackManager::getCurrentInput()
{
    //Get current input state
    unsigned char inputBit = 1;
    const auto inputSingleton = Input::get_singleton();
    for(const String& action : CustomInput::_customActions)
    {                
        float value = inputSingleton->get_action_strength(action) >= _axisSensitivity ? 1 : 0;
        _currentInputState.localInputs.actions.insert(action, value);
        _currentInputState.localInputs.encodedValue += (inputBit) * value;
        inputBit *= 2;
    }
}

void godot::RollbackManager::_physics_process(double delta)
{
    _inputReceivedMutex->lock();
    if(_inputReceived)
    {
        _inputArrayMutex->lock();
        if(_inputArrivedPerFrame[_frameNumber]) //If the input for the current frame has arrived we proceed
        {
            _inputArrayMutex->unlock();
            _inputReceivedMutex->unlock();
            updateGameState(delta);
        }
        else
        {
             _inputArrayMutex->unlock();
            _inputReceived = false;
            _inputReceivedMutex->unlock();
            sendRequestInputPacket(_frameNumber);
        }
    }
    else
    {
        _inputReceivedMutex->unlock();
        if(_connectionState == NET_STATE::PLAYING)
        {
            sendRequestInputPacket(_frameNumber);
            return;
        }

        if(_connectionState == NET_STATE::WAITING)
        {         
            sendHandshakePacket(false);
            return;
        }
    }
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

    for(const auto& [action, value] : inputs.netInputs.actions)
    {
        emit_signal("onHandleNetInput", action, value);
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
    Error ret = _socketUdp->bind(_portToListen);
    if(ret != godot::Error::OK)
    {
        UtilityFunctions::print("[ERROR] UDP Socket failed to bind to: ErrorType: ", ret);
        return ret;
    }

    ret = _socketUdp->set_dest_address(_ipToConnect, _port);
    if(ret != godot::Error::OK)
    {
        UtilityFunctions::print("[ERROR] UDP Socket failed to set destiny address to: ErrorType: ", ret);
        return ret;
    }

    return ret;
}

void godot::RollbackManager::printConnectionState()
{
    switch (_connectionState)
    {
    case NET_STATE::WAITING:
        UtilityFunctions::print("_connectionState == NET_STATE::WAITING");
        break;
    case NET_STATE::PLAYING:
        UtilityFunctions::print("_connectionState == NET_STATE::PLAYING");
        break;
    case NET_STATE::END:
        UtilityFunctions::print("_connectionState == NET_STATE::END");
        break;
    default:
        UtilityFunctions::print("_connectionState Not defined");
        break;
    }
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
                //UtilityFunctions::print("Received input packet");
                processInputPacket(netInData);
                break;
            }
            case NET_PACKET_TYPE::INPUT_REQUEST:
            {
                //UtilityFunctions::print("Received Request input packet");
                processRequestPacket(netInData);
                break;
            }
            case NET_PACKET_TYPE::HANDSHAKE:
            {
                //UtilityFunctions::print("Received Handshake packet");
                processHandshakePacket(netInData);
                break;
            }
            case NET_PACKET_TYPE::GAME_END:
            {
                break;
            }
        }
        

               
    
    }
}

void godot::RollbackManager::sendNetData(const PackedByteArray &netData)
{
    int randomValue = _randomGenerator->randi_range(0, 99);
    if(randomValue < _packetLossPercentage)
    {
        return;
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

void godot::RollbackManager::sendInputPacket(const InputState& inputToSend)
{
    //UtilityFunctions::print("Send inpur");
    PackedByteArray netData{};
    netData.append((unsigned char)NET_PACKET_TYPE::INPUT);

    //Add the current frame and previous frames to help with UDP missing packets
    int frameToSend = ((_frameNumber + _processInputDelay) % 256);
    for(int i = 0; i < _frameSendRange; --frameToSend, ++i)
    {
        if(frameToSend < 0)
        {
            frameToSend = 255;
        }        
        netData.append((unsigned char)frameToSend);
        netData.append(_inputs[frameToSend].localInputs.encodedValue);
    }
    
    sendNetData(netData);   
}

void godot::RollbackManager::sendInputPacket(int frameNeeded)
{   
    PackedByteArray netData{};
    netData.append((unsigned char)NET_PACKET_TYPE::INPUT);
    netData.append((unsigned char)frameNeeded);
    netData.append(_inputs[frameNeeded].localInputs.encodedValue);

    sendNetData(netData);
}

void godot::RollbackManager::sendRequestInputPacket(int frameNeeded)
{
    //UtilityFunctions::print("Send request");
    PackedByteArray netData{};
    netData.append((unsigned char)NET_PACKET_TYPE::INPUT_REQUEST);
    netData.append((unsigned char)frameNeeded);
    netData.append((unsigned char)((frameNeeded + _processInputDelay) % 256));

    //UtilityFunctions::print("Requesting for frame: ", frameNeeded);

    sendNetData(netData); 
}

void godot::RollbackManager::sendHandshakePacket(bool isReply)
{
    //UtilityFunctions::print("Send handshake");
    PackedByteArray netData{};
    netData.append((unsigned char)NET_PACKET_TYPE::HANDSHAKE);
    netData.append((unsigned char)isReply ? 1 : 0);

    sendNetData(netData);    
}

void godot::RollbackManager::processInputPacket(const PackedByteArray &netData)
{
    //Packet Structure
    // NET_PACKET_TYPE::INPUT + Frame 0 + Frame 0 input + Frame 1 + Frame 1 input...            
    int packetIndex = 1;
    bool newInput = false;
    _inputArrayMutex->lock();
    while(packetIndex < netData.size())
    {
        int netEncodedInput = netData[packetIndex + 1];
        int netFrame = netData[packetIndex];
        if(_inputArrivedPerFrame[netFrame] == true)
        {
            //We already have input for this frame 
            //UtilityFunctions::print("We already have input for this frame");    
            break;
        }

        unsigned char inputBit = 1; 
        InputState& frameInputState = _inputs[netFrame];  
        frameInputState.netInputs.encodedValue = netEncodedInput;                 
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
        newInput = true;
        packetIndex += 2;                  
    }            
    _inputArrayMutex->unlock();

    if(newInput)
    {
        _inputReceivedMutex->lock();
        _inputReceived = true;
        if(_connectionState == NET_STATE::WAITING)
        {
            _connectionState = NET_STATE::PLAYING;
        }
        _inputReceivedMutex->unlock();
    }    
}

void godot::RollbackManager::processRequestPacket(const PackedByteArray &netData)
{
    //Packet Structure
    // NET_PACKET_TYPE::REQUEST + Frame X + Frame Y
    // Requested frames from X to Y (Not inclusive)
    _inputRequestMutex->lock();
    _inputArrayMutex->lock();      
    for(int frame = netData[1]; frame != netData[2]; frame = (frame + 1) % 256)
    {          
        if(!_inputRequestAvailablePerFrame[frame])
        {
            //We break because if we don't have input for that frame
            //future frames should be empty (if not ¯\_(ツ)_/¯)
            UtilityFunctions::print("We break because if we don't have input for that frame: ", frame);
            break;
        }        
        sendInputPacket(frame);        
    }
    _inputArrayMutex->unlock();
    _inputRequestMutex->unlock();
}

void godot::RollbackManager::processHandshakePacket(const PackedByteArray &netData)
{
    _inputReceivedMutex->lock();
    if(_connectionState == NET_STATE::WAITING)
    {            
        _connectionState = NET_STATE::PLAYING;
        _inputReceived = true;
        _inputReceivedMutex->unlock();
    }
    else
    {
        _inputReceivedMutex->unlock();
        if(netData[1] == 0)
        {
            //Reply handshake packet
            sendHandshakePacket(true);            
        }
    }					
}

void godot::RollbackManager::updateGameState(float delta)
{
    //Process Inputs
    getCurrentInput();

    _inputArrayMutex->lock();
    InputState& futureInputState = _inputs[(_frameNumber + _processInputDelay) % 256];
    futureInputState.copy(_currentInputState);
    //futureInputState.print();
    _currentInputState.reset();

    sendInputPacket(futureInputState);

    //Reset input arrived  
    _inputArrivedPerFrame[(_frameNumber + (_processInputDelay * 2) + 1) % 256] = false;
    _inputArrayMutex->unlock();    
   
    _inputRequestMutex->lock();
    //Set current frame as available for request
    _inputRequestAvailablePerFrame[(_frameNumber + _processInputDelay) % 256] = true;

    int frameToReset = _frameNumber - _processInputDelay;
    if(frameToReset < 0)
    {
       frameToReset = 256 + frameToReset;
    }

    _inputRequestAvailablePerFrame[frameToReset] = false;
    _inputRequestMutex->unlock();

    //Create Game State
    _currentGameState.reset();
    emit_signal("onSaveGameState");

    //Frame start
    emit_signal("onFrameStart");

    const auto inputSingleton = Input::get_singleton();
    if(inputSingleton->get_action_strength("test"))
    {
        //onResetGameState();
    }
    /*if(doreset)
    {
        onResetGameState();
        doreset = false;
    }*/

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
}
