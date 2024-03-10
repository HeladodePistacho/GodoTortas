extends Node2D

@export var rollbackManager : RollbackManager

@export var walkSpeedStrenght: float = 1.0;
@export var acceleration = 2;
@export var friction: float = 1.0 
@export var maxSpeed = 10;
@export var dashSpeed = 0;

var dirInputCurrentFrame: bool = false
const speedThreshold = 1;
var dash: bool = false;
var currentSpeed: float;
var speedStrength;
var test = 0;

var positionSize : int = var_to_bytes(position).size()
var speedSize : int = var_to_bytes(currentSpeed).size()
var tessssdsdt = 0
# Called when the node enters the scene tree for the first time.
func _ready():
	currentSpeed = 0;
	speedStrength = 0;
	pass # Replace with function body.

var frame : int = 0
func _on_rollback_manager_on_frame_update(delta):
	
	if(abs(speedStrength) < 0.2):
		currentSpeed -= (currentSpeed * friction) * delta
	else:
		currentSpeed = clamp(currentSpeed + ((speedStrength * acceleration)), -maxSpeed, maxSpeed);
	
	if(abs(currentSpeed) < speedThreshold):
		currentSpeed = 0;

	position.x += (currentSpeed * delta);
	pass # Replace with function body.


func _on_rollback_manager_on_handle_input(action, value):	
	#This code asumes this 2 actions are exclusive one is positive when the other one is 0
	if(action == "LLHorizontal"):
		speedStrength = -walkSpeedStrenght if(value != 0.0) else 0.0
			
	if(action == "LRHorizontal"):
		if(value != 0):
			speedStrength = walkSpeedStrenght
	
	pass

func _on_rollback_manager_on_save_game_state():

	var gameState : PackedByteArray = PackedByteArray()
	gameState += var_to_bytes(position)
	gameState += var_to_bytes(floorf(currentSpeed))
	
	print(name, gameState)
	rollbackManager.addToGameState(name, gameState)
	print("?")
	pass # Replace with function body.

func _on_rollback_manager_on_reset_state(element, gameState : PackedByteArray):
	
	#In order to remove elements that didn't exist in the previous frames
	#I need to know if I'm the last element, if last is not me free()	
	if(name == element):
		position = bytes_to_var(gameState.slice(0, positionSize))
		currentSpeed = bytes_to_var(gameState.slice(positionSize, positionSize + speedSize))
	#else:
		#queue_free()
	
	pass # Replace with function body.
