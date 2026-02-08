#ifndef SCENE_GRAPH_WEEK12_H
#define SCENE_GRAPH_WEEK12_H

#include "GameObject.h"
#include <vector>
#include "SceneBase.h"
#include "Maze.h"
#include <queue>
#include "Graph.h"

class SceneGraph_Week12 : public SceneBase
{
public:
	SceneGraph_Week12();
	~SceneGraph_Week12();

	virtual void Init();
	virtual void Update(double dt);
	virtual void Render();
	virtual void Exit();

	void RenderGO(GameObject *go);
	GameObject* FetchGO(GameObject::GAMEOBJECT_TYPE type);

	void RenderGraph();

	// Exercise Week 12
	//4.	Declare these methods in SceneGraph_Week12.h
	void GoRandomAdjacent(GameObject* go);
	void DFSOnce(GameObject* go);
	bool AStar(GameObject* go, Node* startNode, Node* endNode);

protected:
	Graph m_graph;
	unsigned m_graphKey;

	std::vector<GameObject *> m_goList;
	float m_speed;
	float m_worldWidth;
	float m_worldHeight;
	int m_objectCount;
};

#endif