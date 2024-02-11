#ifndef ROLLBACK_MANAGER_H
#define ROLLBACK_MANAGER_H

#include <godot_cpp/classes/node.hpp>

namespace godot
{
class RollbackManager : public Node 
{
	GDCLASS(RollbackManager, Node)

protected:
	static void _bind_methods();

public:
	RollbackManager();
	~RollbackManager();
};
}




#endif