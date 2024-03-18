#include "RollBackManager.h"
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "InputBuffer.h"
#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

//namespace RollTest
//{
//    String addressToConect = "127.0.0.1";
//    int port = 8843;
//}

void godot::RollbackManager::netInputThreadFunc()
{    
    while(true)
    {
        PackedByteArray netInData = _socketUdp->get_packet();

        if(netInData.is_empty())
            continue;

        UtilityFunctions::print("Ireceived a packet, frame: ", netInData[0], " Input: ", netInData[1]);
        //UtilityFunctions::print("Ireceived a packet, frame: ", netInData[0]);
    }
}

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
    }

    //Init frame states
    for(int i = 0; i < _numRollbackFrames; ++i)
    {
        _savedFrames.emplace(InputState(), GameState(), 0);
    }

    //Start Net
    _socketUdp.instantiate();
    _inputArrivedMutex.instantiate();
    _inputRequestMutex.instantiate();
    _inputReceivedMutex.instantiate();
    _netThread.instantiate();

    initializeUDPSocket();    
    _netThread->start(Callable(this, "netInputThreadFunc"));    
}

void RollbackManager::_unhandled_input(const Ref<InputEvent> &event)
{
    int inputBit = 1;
    for(const String& action : CustomInput::_customActions)
    {
        if(!event->is_action(action))
        {
            continue;
        }        
        _currentInputState.localInputs.actions.push_back(action);
        double value = event->get_action_strength(action);
        _currentInputState.localInputs.values.push_back(value);
        
        if(Math::floor(value) != 0.0)
        {
            _currentInputState.localInputs.encodedValue += inputBit;
        }
        
        inputBit *= 2;
    }

    if(event->is_action_pressed("test"))
    {
        doreset = true;
    }

}

void godot::RollbackManager::_physics_process(double delta)
{
    //Process Inputs
    InputState& futureInputState = _inputs[(_frameNumber + _processInputDelay) % 256];
    futureInputState.copy(_currentInputState);
    _currentInputState.reset();

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

    PackedByteArray netData{};
    netData.append(_frameNumber);
    netData.append(futureInputState.localInputs.encodedValue);

    Error packetError = _socketUdp->put_packet(netData);
    if(packetError != Error::OK)
    {
        UtilityFunctions::print("[Error] UDP.put_packet failed ErrorType: ", packetError);
    }

    //Progress frame number
    _frameNumber >= 255 ? _frameNumber = 0 : ++_frameNumber;
}

void godot::RollbackManager::_exit_tree()
{
    if(!_netThread.is_null())
        _netThread->wait_to_finish();
}

void godot::RollbackManager::onHandleInput(const InputState& inputs)
{
    if(inputs.localInputs.actions.size() != inputs.localInputs.values.size())
    {
        return;
    }

    for(int i = 0; i < inputs.localInputs.actions.size(); ++i)
    {
        emit_signal("onHandleInput", inputs.localInputs.actions[i], inputs.localInputs.values[i]);
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
