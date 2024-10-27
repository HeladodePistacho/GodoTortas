#include "RollBackManager.h"
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "InputBuffer.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/input.hpp>

using namespace godot;

void RollbackManager::_bind_methods()
{
    //Methods for editor and scripts
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
    ClassDB::bind_method(D_METHOD("getConnectionStatus"), &RollbackManager::getConnectionStatus);

    ClassDB::bind_method(D_METHOD("getIp"), &RollbackManager::getIp);
	ClassDB::bind_method(D_METHOD("setIp", "ipToConnect"), &RollbackManager::setIp);
    ClassDB::bind_method(D_METHOD("getPort"), &RollbackManager::getPort);
	ClassDB::bind_method(D_METHOD("setPort", "port"), &RollbackManager::setPort);
    ClassDB::bind_method(D_METHOD("getPortToListen"), &RollbackManager::getPortToListen);
	ClassDB::bind_method(D_METHOD("setPortToListen", "port"), &RollbackManager::setPortToListen);
    ClassDB::bind_method(D_METHOD("getPacketLossPercentage"), &RollbackManager::getPacketLossPercentage);
	ClassDB::bind_method(D_METHOD("setPacketLossPercentage", "packetloss"), &RollbackManager::setPacketLossPercentage);

    ClassDB::bind_method(D_METHOD("netInputThreadFunc"), &RollbackManager::netInputThreadFunc);
    ClassDB::bind_method(D_METHOD("addToGameState", "name", "data"), &RollbackManager::addToGameState);

    //Properties
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_processInputDelay", PROPERTY_HINT_RANGE, "0,120"), "setDelay", "getDelay");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_numRollbackFrames", PROPERTY_HINT_RANGE, "1,120"), "setRollFrames", "getRollFrames");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::FLOAT, "_axisSensitivity", PROPERTY_HINT_RANGE, "0.0,1.0,0.05"), "setAxisSensitivity", "getAxisSensitivity");

    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::STRING, "_ipToConnect", PROPERTY_HINT_NONE, "Ip"), "setIp", "getIp");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_port", PROPERTY_HINT_RANGE, "1,15000"), "setPort", "getPort");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_portToListen", PROPERTY_HINT_RANGE, "1,15000"), "setPortToListen", "getPortToListen");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_packetLossPercentage", PROPERTY_HINT_RANGE, "0,100"), "setPacketLossPercentage", "getPacketLossPercentage");

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
    _prevFrameArrival.reserve(_numRollbackFrames);
    for(int i = 0; i < _numRollbackFrames; ++i)
    {
        _savedFrames.emplace_back(InputState(), loadCurrentGameState(), 0, FrameStatus::REAL);
        _prevFrameArrival.push_back(true);
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

void godot::RollbackManager::ProcessCurrentInput()
{
    //Get Input state
    getCurrentInput();
{
    //Set Inpput for frame [_frameNumber + _processInputDelay]
    LockGuard lock{_inputArrayMutex};
    InputState& futureInputState = _inputs[(_frameNumber + _processInputDelay) % 256];
    futureInputState.copyLocalInput(_currentInputState);
    _currentInputState.resetLocalInput();
    sendInputPacket(futureInputState);

    //Reset input arrived array
    _inputArrivedPerFrame[(_frameNumber + (_processInputDelay * 2) + _numRollbackFrames + 1) % 256] = false;  
}
{   
    LockGuard lock{_inputRequestMutex};
    //Set current frame input as available for request
    _inputRequestAvailablePerFrame[(_frameNumber + _processInputDelay) % 256] = true;

    //Set past frame input as not available for request
    //If doesnt work return to monke
    int frameToReset = getPreviousFrame(_processInputDelay + (_numRollbackFrames * 2));
    _inputRequestAvailablePerFrame[frameToReset] = false;
}
}

void godot::RollbackManager::_physics_process(double delta)
{
    if(isConnectionEndedTS())
    {
        sendEndGamePacket();
        if(!_netThread.is_null() && _netThread->is_alive())
        {    
            _netThread->wait_to_finish();
        }
    }

    if(getInputReceivedTS())
    {
        if(isPastFrameStateGuessedTS())
        {
            int pastFrameIndex;
            {
                LockGuard lock{_inputReceivedMutex};
                pastFrameIndex = _savedFrames.front().frameIndex;
            }

            if(getInputArrivedPerFrameTS(pastFrameIndex))
            {
                updateGameState(delta);
            }
            else
            {            
                sendRequestInputPacket(pastFrameIndex);

                LockGuard lock{_inputReceivedMutex};
                _inputReceived = false;
            }
        }
        else
        {
            updateGameState(delta);
        }
    }
    else
    {
        LockGuard lock{_inputReceivedMutex};
        if(_connectionState == NET_STATE::PLAYING)
        {
            sendRequestInputPacket(_savedFrames.front().frameIndex);
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
    if (Engine::get_singleton()->is_editor_hint())
    {        
        return;
    }
    UtilityFunctions::print("Cleanup");

    {
    LockGuard lock{_inputReceivedMutex};
    _connectionState = NET_STATE::END;
    }

    if(!_netThread.is_null() && _netThread->is_alive())
    {    
        _netThread->wait_to_finish();
    }

    if(!_socketUdp.is_null())
    {
        sendEndGamePacket();
        if(_socketUdp->is_bound())
        {
            _socketUdp->close();
        }
    }    
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

void godot::RollbackManager::onResetGameState(const FrameState& frameState)
{
    const auto& oldGameState = frameState.frameGameState;

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

bool godot::RollbackManager::getInputReceivedTS()
{
    LockGuard lock{_inputReceivedMutex};
    return _inputReceived;
}

bool godot::RollbackManager::getInputArrivedPerFrameTS(int frame)
{
    LockGuard lock{_inputArrayMutex};
    return _inputArrivedPerFrame[frame];
}

bool godot::RollbackManager::isConnectionEndedTS()
{
    LockGuard lock{_inputReceivedMutex};
    return _connectionState == NET_STATE::END;
}

bool godot::RollbackManager::isPastFrameStateGuessedTS()
{
    LockGuard lock{_inputReceivedMutex};
    return _savedFrames.front().frameStatus == FrameStatus::GUESSED;
}

InputState godot::RollbackManager::getInputStateForFrameTS(int frame)
{
    LockGuard lock{_inputArrayMutex};
    return _inputs[frame];
}

void godot::RollbackManager::netInputThreadFunc()
{    
    while(true)
    {
        if(isConnectionEndedTS())
        {
            return;
        }   

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
            case NET_PACKET_TYPE::INPUT_REQUEST:
            {
                processRequestPacket(netInData);
                break;
            }
            case NET_PACKET_TYPE::HANDSHAKE:
            {
                processHandshakePacket(netInData);
                break;
            }
            case NET_PACKET_TYPE::GAME_END:
            {
                processEndGamePacket();
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
    PackedByteArray netData{};
    netData.append((unsigned char)NET_PACKET_TYPE::INPUT_REQUEST);
    netData.append((unsigned char)frameNeeded);
    netData.append((unsigned char)((frameNeeded + _processInputDelay) % 256));

    sendNetData(netData); 
}

void godot::RollbackManager::sendHandshakePacket(bool isReply)
{
    PackedByteArray netData{};
    netData.append((unsigned char)NET_PACKET_TYPE::HANDSHAKE);
    netData.append((unsigned char)isReply ? 1 : 0);

    sendNetData(netData);    
}

void godot::RollbackManager::sendEndGamePacket()
{
    PackedByteArray netData{};
    netData.append((unsigned char)NET_PACKET_TYPE::GAME_END);
    sendNetData(netData);
}

void godot::RollbackManager::processInputPacket(const PackedByteArray &netData)
{
    //Packet Structure
    // NET_PACKET_TYPE::INPUT + Frame 0 + Frame 0 input + Frame 1 + Frame 1 input...            
    int packetIndex = 1;
    bool newInput = false;
    {
    LockGuard lock{_inputArrayMutex};
    while(packetIndex < netData.size())
    {
        int netEncodedInput = netData[packetIndex + 1];
        int netFrame = netData[packetIndex];
        if(_inputArrivedPerFrame[netFrame] == true)
        {
            //We already have input for this frame 
            break;
        }
        newInput = true;
         
        InputState& frameInputState = _inputs[netFrame]; 
        frameInputState.resetNetInput(); 
        frameInputState.netInputs.encodedValue = netEncodedInput; 

        unsigned char inputBit = 1;                
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
    }

    if(newInput)
    {
        LockGuard lock{_inputReceivedMutex};
        _inputReceived = true;
        if(_connectionState == NET_STATE::WAITING)
        {
            _connectionState = NET_STATE::PLAYING;
        }
    }    
}

void godot::RollbackManager::processRequestPacket(const PackedByteArray &netData)
{
    //Packet Structure
    // NET_PACKET_TYPE::REQUEST + Frame X + Frame Y
    // Requested frames from X to Y (Not inclusive)
    LockGuard inputRequestLock{_inputRequestMutex};
    LockGuard inputArrivedLock{_inputArrayMutex};      
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
} 

void godot::RollbackManager::processHandshakePacket(const PackedByteArray &netData)
{
    LockGuard lock{_inputReceivedMutex};
    if(_connectionState == NET_STATE::WAITING)
    {            
        _connectionState = NET_STATE::PLAYING;
        _inputReceived = true;
    }
    else
    {
        if(netData[1] == 0)
        {
            //Reply handshake packet
            sendHandshakePacket(true);            
        }
    }					
}

void godot::RollbackManager::processEndGamePacket()
{
    LockGuard lock{_inputReceivedMutex};
    _connectionState = NET_STATE::END;
}

void godot::RollbackManager::tryToRollback()
{
    LockGuard lock{_inputArrayMutex};

    //Look for frames that has been guessed
    LocalVector<bool> currentArrivalArray;
    currentArrivalArray.reserve(_numRollbackFrames + 1);
    LocalVector<InputElement> pastActualInputsArray;

    for(int i = 0; i < _numRollbackFrames; ++i)
    {
        int frameToCheck = getPreviousFrame(_numRollbackFrames - i);
        currentArrivalArray.push_back(_inputArrivedPerFrame[frameToCheck]);

        if(currentArrivalArray[i] != _prevFrameArrival[i])
        {
            pastActualInputsArray.push_back(_inputs[frameToCheck].netInputs);
        }
    }

    //Yo que se
    if(pastActualInputsArray.is_empty())
    { 
        return;
    }

    bool startedRollback = false;
    int stateIndex = 0;
    InputElement* newPastActualInput = nullptr;
    int pastActualInputsIndex = 0;

    for(FrameState& frameState : _savedFrames)
    {
        if(!_prevFrameArrival[stateIndex] && currentArrivalArray[stateIndex])
        {
            newPastActualInput = &pastActualInputsArray[pastActualInputsIndex];
            ++pastActualInputsIndex;

            if(frameState.frameInputs.netInputs.encodedValue != newPastActualInput->encodedValue)
            {
                frameState.frameInputs.netInputs = *newPastActualInput;
                frameState.frameStatus = FrameStatus::REAL;
                if(!startedRollback)
                {
                    //Reset game state for game elements
                    onResetGameState(frameState);
                    startedRollback = true;
                }

            }
        }

        if(startedRollback)
        {
            const GameState& currentGameState = loadCurrentGameState();
            onHandleInput(frameState.frameInputs);
            frameState.frameGameState = currentGameState;
        }
    }
}

int godot::RollbackManager::getPreviousFrame(int numFrameBehind)
{
    int frameBehind = _frameNumber - numFrameBehind;
    return frameBehind < 0 ? 256 + frameBehind : frameBehind;
}

const GameState& godot::RollbackManager::loadCurrentGameState()
{
    _currentGameState.reset();
    emit_signal("onSaveGameState");
    return _currentGameState;
}

FrameStatus godot::RollbackManager::getCurrentFrameStatus(InputState& frameInput)
{
    LockGuard lock{_inputArrayMutex};

    //if current frame input has not arrived we guess it -> in this case with the previous frame input
    FrameStatus frameStatus = FrameStatus::REAL;
    if(_inputArrivedPerFrame[_frameNumber])
    {
        int previousFrame = getPreviousFrame(1);
        frameInput.netInputs = _inputs[previousFrame].netInputs;
        _inputs[_frameNumber].netInputs = _inputs[previousFrame].netInputs;
        frameStatus = FrameStatus::GUESSED;
    }

    return frameStatus;
}

void godot::RollbackManager::updateGameState(float delta)
{
    //Process Inputs
    ProcessCurrentInput();
    InputState currentFrameInputState = getInputStateForFrameTS(_frameNumber);

    //This should probably go into threadsaafe so we don't have race condition 
    FrameStatus frameStatus = getCurrentFrameStatus(currentFrameInputState);   
    tryToRollback();

    //Create Game State
    loadCurrentGameState();

    //Frame start
    emit_signal("onFrameStart");

    //Handle frame Inputs
    onHandleInput(currentFrameInputState);

    //Frame Process
    emit_signal("onFrameUpdate", delta);

    //Frame End
    emit_signal("onFrameEnd", delta);

    //Store current frame state
    _savedFrames.emplace_back(FrameState(currentFrameInputState, _currentGameState, _frameNumber, frameStatus));   

    //Remove oldest frame state
    _savedFrames.pop_front();

    //Progress frame number
    _frameNumber >= 255 ? _frameNumber = 0 : ++_frameNumber;
}
