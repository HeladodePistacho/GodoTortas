extends Node2D

@export var acceleration = 2;
@export var friction: float = 1.0 
@export var maxSpeed = 10;
@export var dashSpeed = 0;

const speedThreshold = 1;
var dash: bool = false;
var currentSpeed;
var speedStrength;

# Called when the node enters the scene tree for the first time.
func _ready():
	currentSpeed = 0;
	speedStrength = 0;
	pass # Replace with function body.

func _unhandled_input(event):
	if(event.is_action("LLHorizontal") or event.is_action("LRHorizontal")):
		speedStrength = Input.get_axis("LLHorizontal", "LRHorizontal")

	pass

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
