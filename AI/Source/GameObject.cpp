#include "GameObject.h"
#include "ConcreteMessages.h"

GameObject::GameObject(GAMEOBJECT_TYPE typeValue) 
	: type(typeValue),
	scale(1, 1, 1),
	active(false),
	mass(1.f),
	moveSpeed(1.f),
	energy(10.f),
	sm(NULL),
	nearest(NULL),
	nextState(nullptr),
	currentState(nullptr),
	currNode(0)
{
	static int count = 0;
	id = ++count;
	moveLeft = moveRight = moveUp = moveDown = true;
}

GameObject::~GameObject()
{
}

//week 4
bool GameObject::Handle(Message* message)
{
	//let's check if message is MessageCheckActive
	if (dynamic_cast<MessageCheckActive*>(message) != nullptr)
		return active;
	else if (dynamic_cast<MessageCheckFish*>(message) != nullptr)
		return active && type == GameObject::GO_FISH;
	else if (dynamic_cast<MessageCheckFood*>(message) != nullptr)
		return active && type == GameObject::GO_FISHFOOD;
	else if (dynamic_cast<MessageCheckShark*>(message) != nullptr)
		return active && type == GameObject::GO_SHARK;
	//week 5
	//set speed to 0 upon receiving stop message
	else if (dynamic_cast<MessageStop*>(message) != nullptr)
	{
		moveSpeed = 0;
		return true;
	}
	else if (dynamic_cast<MessageEvolve*>(message) != nullptr)
	{
		// Exercise Week 05
		type = GameObject::GO_FISH;
	}

	//note: pardon the inconsistency(when compared to SceneMovement's Handle)
	//we do NOT want to create a new message on the heap PER object for performance reasons
	return false;
}
