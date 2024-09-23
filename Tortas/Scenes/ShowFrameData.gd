extends Control

@export var rollbackManager : RollbackManager

@export var defaultLabelSettings : LabelSettings
@export var greenLabelSettings : LabelSettings
@export var redLabelSettings : LabelSettings

var grid

func addMiniGrid(node, frame):
	for i in range(3):				
		var newLabel : Label = Label.new()
		newLabel.label_settings = defaultLabelSettings
		
		#First label is frame num
		if(i == 0):
			newLabel.set_text(str(frame))
		else: 
			newLabel.set_text(str(0))
		node.add_child(newLabel)
	pass

# Called when the node enters the scene tree for the first time.
func _ready():
	grid = get_node("Grid")

	for i in range(256):
		addMiniGrid(grid, i)
		#var newLabel : Label = Label.new()
		#newLabel.set_text(str(i))
		#newLabel.label_settings = defaultLabelSettings
		#grid.add_child(newLabel)

	
	pass # Replace with function body.

func updateCurrentFrame(frame):
	var frameLabel = grid.get_child(frame * 3)
	frameLabel.label_settings = greenLabelSettings
	
	var previousFrame = frame - 1
	if(previousFrame < 0):
		previousFrame = 255 
	
	grid.get_child(previousFrame * 3).label_settings = defaultLabelSettings
	pass

func updateInputs():
	
	for i in range(256):
		var localEndoded = rollbackManager.getLocalInputForFrame(i)
		var netEndoded = rollbackManager.getNetInputForFrame(i)
		grid.get_child((i * 3) + 1).set_text(str(localEndoded))
		grid.get_child((i * 3) + 2).set_text(str(netEndoded))
	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):	
	var currentFrame = rollbackManager.getCurrentFrame()
	updateCurrentFrame(currentFrame)
	updateInputs();
	
	
	pass
