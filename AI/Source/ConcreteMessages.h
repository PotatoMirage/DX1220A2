#ifndef CONCRETE_MESSAGE_H
#define CONCRETE_MESSAGE_H

#include "Message.h"
#include "GameObject.h"

struct MessageWRU : public Message
{
	enum SEARCH_TYPE
	{
		SEARCH_NONE = 0,
		NEAREST_SHARK,
		NEAREST_FISHFOOD,
		NEAREST_FULLFISH,
		HIGHEST_ENERGYFISH,
	};
	MessageWRU(GameObject *goValue, SEARCH_TYPE typeValue, float thresholdValue) : go(goValue), type(typeValue), threshold(thresholdValue) {}
	virtual ~MessageWRU() {}

	GameObject *go;
	SEARCH_TYPE type;
	float threshold;
};

struct MessageCheckActive : public Message
{
	MessageCheckActive() {}
};

struct MessageCheckFish : public Message
{
	MessageCheckFish() {}
};

struct MessageCheckFood : public Message
{
	MessageCheckFood() {}
};

struct MessageCheckShark : public Message
{
	MessageCheckShark() {}
};

//week 5
//this message asks the scene to spawn an object
struct MessageSpawn : public Message
{
	// owner of msg, what to spawn, # to spawn, # tiles(x & y) from owner
	// passing range array by reference to avoid array decay (to int*) - that way we can force users to only pass an array of size 2(no other sizes will be accepted)
	// alternatively, look into std::array(c++11 onwards)?
	MessageSpawn(GameObject* goVal, int typeVal, int countVal, int (&range)[2]) : go(goVal), type(typeVal), count(countVal)
	{
		distRange[0] = range[0];
		distRange[1] = range[1];
	}

	int distRange[2];
	int type;
	int count;
	GameObject* go;
};

struct MessageSpawnFood : public Message
{
	// owner of msg, what to spawn, # to spawn, # tiles(x & y) from owner
	// passing range array by reference to avoid array decay (to int*) - that way we can force users to only pass an array of size 2(no other sizes will be accepted)
	// alternatively, look into std::array(c++11 onwards)?
	MessageSpawnFood(GameObject* goVal, int typeVal, int countVal, int(&range)[2]) : go(goVal), type(typeVal), count(countVal)
	{
		distRange[0] = range[0];
		distRange[1] = range[1];
	}

	int distRange[2];
	int type;
	int count;
	GameObject* go;
};

struct MessageStop : public Message
{
	MessageStop() {}
};

//this message is meant to turn food into fish
struct MessageEvolve : public Message
{
	MessageEvolve(GameObject* goVal) : go(goVal) {}

	GameObject* go;
};

#endif
