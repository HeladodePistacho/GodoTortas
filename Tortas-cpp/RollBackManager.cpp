#include "RollBackManager.h"
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "InputBuffer.h"

using namespace godot;

void RollbackManager::_bind_methods()
{
    //Methods
    ClassDB::bind_method(D_METHOD("addToGameState", "name", "data"), &RollbackManager::addToGameState);

    //Signals
    ADD_SIGNAL(MethodInfo("onSaveGameState"));
    ADD_SIGNAL(MethodInfo("onHandleInput", PropertyInfo(Variant::STRING, "action"), PropertyInfo(Variant::FLOAT, "value")));
    ADD_SIGNAL(MethodInfo("onFrameStart"));
    ADD_SIGNAL(MethodInfo("onFrameUpdate", PropertyInfo(Variant::FLOAT, "delta")));
    ADD_SIGNAL(MethodInfo("onFrameEnd"));
}

RollbackManager::RollbackManager()
{
}

RollbackManager::~RollbackManager()
{

}

void godot::RollbackManager::_ready()
{
    CustomInput::init();

    _inputs.reserve(256);
    for(int i = 0; i < 256; ++i)
    {
        _inputs.emplace_back(InputState{});
    }

    //Init freame states
    for(int i = 0; i < _numRollbackFrames; ++i)
    {
        _savedFrames.emplace(InputState(), GameState(), 0);
    }
}

void RollbackManager::_unhandled_input(const Ref<InputEvent> &event)
{
    InputState newState{};
    for(const String& action : CustomInput::_customActions)
    {
        if(!event->is_action(action))
        {
            continue;
        }

        newState.localInputs.insert(action, event->get_action_strength(action));                
    }

    _inputs[(_frameNumber + _processInputDelay) % 256] = std::move(newState);

    
}

void godot::RollbackManager::_process(double delta)
{
    if (Engine::get_singleton()->is_editor_hint())
        set_process_mode(Node::ProcessMode::PROCESS_MODE_DISABLED);

    //Create Game State
    _currentGameState.reset();
    emit_signal("onSaveGameState");

    //Frame start
    emit_signal("onFrameStart");

    //Handle frame Inputs
    onHandleInput(_inputs[_frameNumber % 256]);

    //Frame Process
    emit_signal("onFrameUpdate", delta);

    //Frame End
    emit_signal("onFrameEnd", delta);

    //Store current frame state

    //Remove oldest frame state

    //Progress frame number
    _frameNumber = (_frameNumber + 1) % 256;
}

void godot::RollbackManager::onHandleInput(const InputState &inputs)
{
    UtilityFunctions::print("-----------------------------");
    for(const auto& [action, value] : inputs.localInputs)
    {
        UtilityFunctions::print(action, " ", value);
    }

    for(const auto& [action, value] : inputs.localInputs)
    {
        emit_signal("onHandleInput", action, value);
    }
    
}

GameState& godot::RollbackManager::getCurrentGameState()
{
    return _currentGameState;
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

    //UtilityFunctions::print("Added element with name: ", name, " Index: ", elementData.index, " Size: ", elementData.size);
    _currentGameState.elementsSaved.insert(name, std::move(elementData));
      

    //Add data to buffer
   _currentGameState.stateBuffer.append_array(data);
   //UtilityFunctions::print("Current Game State Buffer Size: ", _currentGameState.stateBuffer.size());
}
