extends Control

@export var rollbackManager : RollbackManager

@export var defaultLabelSettings : LabelSettings
@export var greenLabelSettings : LabelSettings
@export var redLabelSettings : LabelSettings

@onready var InputStateData = $FrameInputStateData
@onready var InputArrivedData = $InputArrivedData

func addMiniGrid(node, frame, numLabels):
	for i in range(numLabels):				
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
	for i in range(256):
		addMiniGrid(InputStateData, i, 3)
		addMiniGrid(InputArrivedData, i, 2)
		#var newLabel : Label = Label.new()
		#newLabel.set_text(str(i))
		#newLabel.label_settings = defaultLabelSettings
		#grid.add_child(newLabel)

	
	pass # Replace with function body.

func updateCurrentFrame(dataNode, frame, numLabels):
	var frameLabel = dataNode.get_child(frame * numLabels)
	frameLabel.label_settings = greenLabelSettings
	
	var previousFrame = frame - 1
	if(previousFrame < 0):
		previousFrame = 255 
	
	dataNode.get_child(previousFrame * numLabels).label_settings = defaultLabelSettings
	pass

func updateStateData():
	
	for i in range(256):
		var localEndoded = rollbackManager.getLocalInputForFrame(i)
		var netEndoded = rollbackManager.getNetInputForFrame(i)
		InputStateData.get_child((i * 3) + 1).set_text(str(localEndoded))
		InputStateData.get_child((i * 3) + 2).set_text(str(netEndoded))
	pass
	
func updateArrivedData():
	for i in range(256):
		var arrived = int(rollbackManager.getInputArrivedForFrame(i))
		
		InputArrivedData.get_child((i * 2) + 1).set_text(str(arrived))

	pass

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):	
	var currentFrame = rollbackManager.getCurrentFrame()
	updateCurrentFrame(InputStateData, currentFrame, 3)
	updateCurrentFrame(InputArrivedData, currentFrame, 2)
	updateStateData()
	updateArrivedData()
	
	pass


func _on_frame_input_state_btn_button_up():
	InputStateData.visible = !InputStateData.visible
	pass # Replace with function body.
