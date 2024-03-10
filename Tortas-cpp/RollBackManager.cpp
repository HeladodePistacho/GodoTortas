#include "RollBackManager.h"
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "InputBuffer.h"
#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

void RollbackManager::_bind_methods()
{
    //Properties
    /*ClassDB::bind_method(D_METHOD("getDelay"), &RollbackManager::getDelay);
	ClassDB::bind_method(D_METHOD("setDelay", "delay"), &RollbackManager::setDelay);
    ClassDB::bind_method(D_METHOD("getRollFrames"), &RollbackManager::getRollFrames);
	ClassDB::bind_method(D_METHOD("setRollFrames", "rollFrames"), &RollbackManager::setRollFrames);
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_processInputDelay", PROPERTY_HINT_RANGE, "0,120"), "setDelay", "getDelay");
    ClassDB::add_property("RollbackManager", PropertyInfo(Variant::INT, "_numRollbackFrames", PROPERTY_HINT_RANGE, "1,120"), "setRollFrames", "getRollFrames");

    //Methods
    ClassDB::bind_method(D_METHOD("addToGameState", "name", "data"), &RollbackManager::addToGameState);*/

    //Signals
    ADD_SIGNAL(MethodInfo("onSaveGameState"));
    ADD_SIGNAL(MethodInfo("onHandleInput", PropertyInfo(Variant::STRING, "action"), PropertyInfo(Variant::FLOAT, "value")));
    ADD_SIGNAL(MethodInfo("onFrameStart"));
    ADD_SIGNAL(MethodInfo("onFrameUpdate", PropertyInfo(Variant::FLOAT, "delta")));
    ADD_SIGNAL(MethodInfo("onFrameEnd"));

    ADD_SIGNAL(MethodInfo("onResetState", PropertyInfo(Variant::STRING, "element"), PropertyInfo(Variant::PACKED_BYTE_ARRAY, "gameState")));
}

RollbackManager::RollbackManager() : _currentInputState(memnew(InputState))
{

}

RollbackManager::~RollbackManager()
{

}


void godot::RollbackManager::_ready()
{
    if (Engine::get_singleton()->is_editor_hint())
        set_process_mode(Node::ProcessMode::PROCESS_MODE_DISABLED);
    else
        set_process_mode(Node::ProcessMode::PROCESS_MODE_INHERIT);

    CustomInput::init();

    for(int i = 0; i < 256; ++i)
    {
        Ref<InputState> newInputState;
        newInputState.instantiate();
        _inputs.push_back(newInputState);
    }

    //Init freame states
    //for(int i = 0; i < _numRollbackFrames; ++i)
    {
    //    _savedFrames.emplace(InputState(), GameState(), 0);
    }
}

void RollbackManager::_unhandled_input(const Ref<InputEvent> &event)
{
    for(const String& action : CustomInput::_customActions)
    {
        if(!event->is_action(action))
        {
            continue;
        }        
        _currentInputState->localInputs.actions.push_back(action);
        _currentInputState->localInputs.values.push_back(event->get_action_strength(action));
    }

    if(event->is_action_pressed("test"))
    {
        doreset = true;
    }

}

void godot::RollbackManager::_physics_process(double delta)
{
    //Process Inputs
    Ref<InputState> futureInputState = _inputs[(_frameNumber + _processInputDelay) % 256];
    futureInputState->reset();
    futureInputState->localInputs.actions.append_array(_currentInputState->localInputs.actions);
    futureInputState->localInputs.values.append_array(_currentInputState->localInputs.values);
    _currentInputState->reset();


    //Create Game State
    //_currentGameState.reset();
//    emit_signal("onSaveGameState");

    //Frame start
//    emit_signal("onFrameStart");

    //if(doreset)
    //{
    //    onResetGameState();
    //    doreset = false;
    //}

    //Handle frame Inputs
    onHandleInput(_inputs[_frameNumber]);

    //Frame Process
    emit_signal("onFrameUpdate", delta);

    //Frame End
    emit_signal("onFrameEnd", delta);

    //Store current frame state
    //_savedFrames.emplace(FrameState(_inputs[_frameNumber], _currentGameState, _frameNumber));

    //Remove oldest frame state
    //_savedFrames.pop();

    //Progress frame number
    _frameNumber >= 255 ? _frameNumber = 0 : ++_frameNumber;
}

void godot::RollbackManager::onHandleInput(const Ref<InputState> inputs)
{
    if(inputs->localInputs.actions.size() != inputs->localInputs.values.size())
    {
        return;
    }

    for(int i = 0; i < inputs->localInputs.actions.size(); ++i)
    {
        emit_signal("onHandleInput", inputs->localInputs.actions[i], inputs->localInputs.values[i]);
    }
}

/*GameState& godot::RollbackManager::getCurrentGameState()
{
    return GameState();
}

void godot::RollbackManager::addToGameState(const String &name, const PackedByteArray& data)
{
    //if(_currentGameState.elementsSaved.has(name))
    //{
    //    UtilityFunctions::print("You already have this element in the current game state");
    //    return;
    //}

    //Add element to saved elements
   // GameState::ElementBufferData elementData;
    //elementData.index = _currentGameState.stateBuffer.size();
    //elementData.size = data.size();
   // _currentGameState.elementsIds.push_back(name);

    
   // _currentGameState.elementsSaved.insert(name, std::move(elementData));
      

    //Add data to buffer
   //_currentGameState.stateBuffer.append_array(data);
   
  // UtilityFunctions::print("Current Game State Buffer Size: ", _currentGameState.stateBuffer.size());
}

void godot::RollbackManager::onResetGameState()
{
    const auto& oldGameState = _savedFrames.front().frameGameState;

    UtilityFunctions::print("buffer: ", oldGameState.stateBuffer);

    int acumulatedBufferSize = 0;
    auto& it = oldGameState.elementsSaved.begin();
    for(; it != oldGameState.elementsSaved.end(); ++it)
    {      
        acumulatedBufferSize += it->value.size;

        UtilityFunctions::print(oldGameState.stateBuffer.slice(it->value.index, acumulatedBufferSize));
        emit_signal("onResetState", it->key, oldGameState.stateBuffer.slice(it->value.index, acumulatedBufferSize));
    }


}*/
