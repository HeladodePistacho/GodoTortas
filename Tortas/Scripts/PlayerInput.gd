extends Node2D

@export var rollbackManager : RollbackManager

@export var acceleration = 2;
@export var friction: float = 1.0 
@export var maxSpeed = 10;
@export var dashSpeed = 0;

const speedThreshold = 1;
var dash: bool = false;
var currentSpeed;
var speedStrength;
var test = 0;

# Called when the node enters the scene tree for the first time.
func _ready():
	currentSpeed = 0;
	speedStrength = 0;
	pass # Replace with function body.



# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if(abs(speedStrength) < 0.2):
		currentSpeed -= (currentSpeed * friction) * delta
	else:
		currentSpeed = clamp(currentSpeed + ((speedStrength * acceleration) * delta), -maxSpeed, maxSpeed);
	
	if(abs(currentSpeed) < speedThreshold):
		currentSpeed = 0;

	position.x += (currentSpeed * delta);
	pass


func _on_rollback_manager_on_handle_input(action, value):

	if(action == "LRHorizontal" or action == "LLHorizontal"):
		speedStrength = Input.get_axis("LLHorizontal", "LRHorizontal")
	
	#print(speedStrength)
	#print("Handle input signal ", action, " ! ", value)
	pass # Replace with function body.


func _on_rollback_manager_on_save_game_state():
	var myData = PackedByteArray([position.x, position.y])
	rollbackManager.addToGameState(name, var_to_bytes(position) + var_to_bytes(currentSpeed))
	
	#var tests = var_to_bytes(position)
	#print(tests)
	#print(bytes_to_var(tests))
	pass # Replace with function body.
