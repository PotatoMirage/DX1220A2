#ifndef SCENE_GRAPH_WEEK09_H
#define SCENE_GRAPH_WEEK09_H

#include "GameObject.h"
#include <vector>
#include "SceneBase.h"
#include "Maze.h"
#include <queue>
#include "Graph.h"

class SceneGraph_Week09 : public SceneBase
{
public:
	SceneGraph_Week09();
	~SceneGraph_Week09();

	virtual void Init();
	virtual void Update(double dt);
	virtual void Render();
	virtual void Exit();

	void RenderGO(GameObject *go);
	GameObject* FetchGO(GameObject::GAMEOBJECT_TYPE type);

	void RenderGraph();

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