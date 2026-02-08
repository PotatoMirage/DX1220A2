#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <vector>
#include "Graph.h"
#include "Maze.h"
#include "Vector3.h"
#include "StateMachine.h"
#include "ObjectBase.h"
#include "NNode.h"

struct GameObject : public ObjectBase
{
	enum GAMEOBJECT_TYPE
	{
		GO_NONE = 0,
		GO_BALL,
		GO_CROSS,
		GO_CIRCLE,
		GO_FISH,
		GO_SHARK,
		GO_FISHFOOD,
		GO_BLACK,
		GO_WHITE,
		GO_NPC,
		GO_BIRD,
		GO_PIPE,
		GO_TOTAL, //must be last
	};
	enum STATE
	{
		STATE_NONE = 0,
		STATE_TOOFULL,
		STATE_FULL,
		STATE_HUNGRY,
		STATE_DEAD,
	};
	GAMEOBJECT_TYPE type;
	Vector3 pos;
	Vector3 vel;
	Vector3 scale;
	bool active;
	float mass;
	Vector3 target;
	int id;
	int steps;
	float energy;
	float moveSpeed;
	float countDown;
	STATE currState;
	GameObject *nearest;
	bool moveLeft;
	bool moveRight;
	bool moveUp;
	bool moveDown;
	StateMachine *sm;

	// For Week 05
	//each instance has to have its own currState and nextState pointer(can't be shared)
	State* currentState; //week 5: should probably be private. put that under TODO
	State* nextState; //week 5: should probably be private. put that under TODO

	// For Week 08
	std::vector<Maze::TILE_CONTENT> grid;
	std::vector<bool> visited;
	std::vector<MazePt> stack; //for dfs
	std::vector<MazePt> path;  //for storing path
	MazePt curr;

	//week 12
	int currNode; //stores an index
	std::vector<Node*> gStack;
	std::vector<Vector3> gPath;

	//week 16
	float score;
	bool alive;
	std::vector<NNode> hiddenNode; //we'll only use 1 hidden layer in this practical
	NNode outputNode;

	GameObject(GAMEOBJECT_TYPE typeValue = GO_NONE);
	~GameObject();

	bool Handle(Message* message);
};

#endif