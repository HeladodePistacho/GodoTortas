extends Label
@export var rollbackManager : RollbackManager


var frame : int = 0;

# Called when the node enters the scene tree for the first time.
func _ready():
	
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	var rollbackDebug : String
	
	if(rollbackManager != null):
		rollbackDebug = "Recevier IP: " + rollbackManager.getIp() + " \n" + "Listening Port: " + str(rollbackManager.getPortToListen()) + " \n" + "Sending to Port: " + str(rollbackManager.getPort()) + " \n" + " Frame: " + str(frame) + '\n'
	else:
		rollbackDebug = "RollbackManager is Null"
	
	rollbackDebug += rollbackManager.getConnectionStatus()
	
	set_text(rollbackDebug)
	pass



func _on_rollback_manager_on_frame_update(delta):
	frame += 1
	pass # Replace with function body.
