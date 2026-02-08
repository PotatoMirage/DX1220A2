#include <sstream>
#include <list>
#include "SceneGraph_Week12.h"
#include "GL\glew.h"
#include "Application.h"

//using this pointer to point to last create npc
static GameObject* npc = nullptr;

SceneGraph_Week12::SceneGraph_Week12()
	: m_graphKey{}, m_graph{}
{
}

SceneGraph_Week12::~SceneGraph_Week12()
{
}

void SceneGraph_Week12::Init()
{
	npc = nullptr;
	SceneBase::Init();

	//Calculating aspect ratio
	m_worldHeight = 100.f;
	m_worldWidth = m_worldHeight * (float)Application::GetWindowWidth() / Application::GetWindowHeight();

	//Physics code here
	m_speed = 1.f;

	Math::InitRNG();

	// Exercise Week 09
	//let's generate our graph
	m_graphKey = 123;
	//make 10 waypoints
	m_graph.Generate(m_graphKey, 10, Vector3{10, 10}, Vector3{ m_worldHeight - 10, m_worldHeight - 10 }, 20);
}

GameObject* SceneGraph_Week12::FetchGO(GameObject::GAMEOBJECT_TYPE type)
{
	for (std::vector<GameObject *>::iterator it = m_goList.begin(); it != m_goList.end(); ++it)
	{
		GameObject *go = (GameObject *)*it;
		if (!go->active && go->type == type)
		{
			go->active = true;
			++m_objectCount;
			return go;
		}
	}
	for (unsigned i = 0; i < 5; ++i)
	{
		GameObject *go = new GameObject(type);
		m_goList.push_back(go);
	}
	return FetchGO(type);
}

void SceneGraph_Week12::Update(double dt)
{
	if (dt > 0.03) dt = 0.03;
	SceneBase::Update(dt);

	//Calculating aspect ratio
	m_worldHeight = 100.f;
	m_worldWidth = m_worldHeight * (float)Application::GetWindowWidth() / Application::GetWindowHeight();

	if (Application::IsKeyPressed(VK_OEM_MINUS))
	{
		m_speed = Math::Max(0.f, m_speed - 0.1f);
	}
	if (Application::IsKeyPressed(VK_OEM_PLUS))
	{
		m_speed += 0.1f;
	}
	static bool bSpacePressed = false;
	if (Application::IsKeyPressed(' ') && !bSpacePressed)
	{
		bSpacePressed = true;

		// Exercise Week 12
		//a.	Using spacebar, spawn a GO_NPC and move it to the nearest node
		GameObject* go = FetchGO(GameObject::GAMEOBJECT_TYPE::GO_NPC);
		go->scale.Set(5.f, 5.f, 1.f);
		go->pos.Set(Math::RandFloatMinMax(0, m_worldHeight), Math::RandFloatMinMax(0, m_worldHeight));
		go->currNode = m_graph.NearestNode(go->pos);
		go->target = m_graph.m_nodes[go->currNode]->pos;
		//pushing this node into stack so that we may trigger DPS once we reached it
		go->gStack.push_back(m_graph.m_nodes[go->currNode]);

		npc = go;
	}
	else if (!Application::IsKeyPressed(' '))
	{
		bSpacePressed = false;
	}

	//Input Section
	static bool bLButtonState = false;
	if (!bLButtonState && Application::IsMousePressed(0))
	{
		bLButtonState = true;
		std::cout << "LBUTTON DOWN" << std::endl;
		double x, y;
		Application::GetCursorPos(&x, &y);
		int w = Application::GetWindowWidth();
		int h = Application::GetWindowHeight();
		float posX = static_cast<float>(x) / w * m_worldWidth;
		float posY = (h - static_cast<float>(y)) / h * m_worldHeight;
		if (posX < m_worldHeight && posY < m_worldHeight && npc!=nullptr && npc->gStack.empty())
		{
			// Exercise Week 12
			//d.	Click on anywhere on the map and make it move there using A* algorithm [See part 9]
			Node* startNode = m_graph.m_nodes[m_graph.NearestNode(npc->target)];
			Node* endNode = m_graph.m_nodes[m_graph.NearestNode(Vector3{ posX, posY })];
			AStar(npc, startNode, endNode);
		}
	}
	else if (bLButtonState && !Application::IsMousePressed(0))
	{
		bLButtonState = false;
		std::cout << "LBUTTON UP" << std::endl;
	}
	static bool bRButtonState = false;
	if (!bRButtonState && Application::IsMousePressed(1))
	{
		bRButtonState = true;
		std::cout << "RBUTTON DOWN" << std::endl;
	}
	else if (bRButtonState && !Application::IsMousePressed(1))
	{
		bRButtonState = false;
		std::cout << "RBUTTON UP" << std::endl;
	}
	static bool bSpaceState = false;
	if (!bSpaceState && Application::IsKeyPressed(VK_SPACE))
	{
		bSpaceState = true;
	}
	else if (bSpaceState && !Application::IsKeyPressed(VK_SPACE))
	{
		bSpaceState = false;
	}

	// Exercise Week 12
	//update all NPCs
	static const float NPC_VELOCITY = 10.f;
	for (auto go : m_goList)
	{
		if (go->active && go->type == GameObject::GO_NPC)
		{
			//checks if go is close enough to the target
			//warning: this method of checking if an entity has reached a position will fail
			//         if the movement speed is too high
			if ((go->pos - go->target).Length() < 1.f)
			{
				if (!go->gStack.empty())
				{
					// Exercise Week 12
					//c.	After spawn, make it do a DFS traversal of entire graph, visiting every node once [See part 8]
					DFSOnce(go);
					go->target = m_graph.m_nodes[go->currNode]->pos;
				}
				else if (!go->gPath.empty())
				{
					// Exercise Week 12
					//e.Make a game object go towards the first node in its gPath
					go->target = go->gPath[0];
					go->gPath.erase(go->gPath.begin());
				}
				else
				{
					// Exercise Week 12
					//b.	If go->gPath is empty, give it a random adjacent node and move it there [See part 7]
					GoRandomAdjacent(go);
				}
			}
			else
			{
				go->pos += (go->target - go->pos).Normalized()
					* NPC_VELOCITY * m_speed * static_cast<float>(dt);
			}
		}
	}
}


void SceneGraph_Week12::RenderGO(GameObject *go)
{
	// Exercise Week 12
	//a.	Render GO_NPC with a colored circle or sprite
	switch (go->type)
	{
	case GameObject::GO_NPC:
		modelStack.PushMatrix();
		modelStack.Translate(go->pos.x, go->pos.y, 1.f);
		modelStack.Scale(go->scale.x, go->scale.y, 1.f);
		RenderMesh(meshList[GEO_AGENT], false);
		modelStack.PopMatrix();
		break;
	}
}

// Exercise Week 09
//just rendering the graph. nothing particularly interesting here.
void SceneGraph_Week12::RenderGraph()
{
	int index{};
	for (Node* node : m_graph.m_nodes)
	{
		// Exercise Week 09
		// a.Render each node as a circle or sprite
		modelStack.PushMatrix();
		modelStack.Translate(node->pos.x, node->pos.y, node->pos.z);
		modelStack.Scale(1.f, 1.f, 1.f);
		RenderMesh(meshList[GEO_BALL], false);

		// Exercise Week 09
		//b.Render the ID of the node at the node
		modelStack.Translate(0.f, -3.f, 0.f);
		modelStack.Scale(3.f, 3.f, 1.f);
		RenderText(meshList[GEO_TEXT], std::to_string(index), Color{0, 0, 0});
		modelStack.PopMatrix();

		++index;
	}

	// Exercise Week 09
	//c.	Render each edge as a coloured line
	for (const Edge& edge : m_graph.m_edges)
	{
		Node* node = edge.from;
		modelStack.PushMatrix();
		modelStack.Translate(node->pos.x, node->pos.y, node->pos.z);
		modelStack.Rotate(Math::RadianToDegree(atan2f(edge.to->pos.y - node->pos.y, edge.to->pos.x - node->pos.x)), 0.f, 0.f, 1.f);
		modelStack.PushMatrix();
		modelStack.Scale(edge.cost, 1.f, 1.f);
		RenderMesh(meshList[GEO_LINE], false);
		modelStack.PopMatrix();

		// Exercise Week 09
		//d.	Render the cost of the edge on the edge
		modelStack.Translate(edge.cost * 0.35f, -1.f, 0);
		modelStack.Scale(2.f, 2.f, 1.f);
		RenderText(meshList[GEO_TEXT], std::to_string(edge.cost), Color{ 0, 0, 0 });
		modelStack.PopMatrix();
	}
}

void SceneGraph_Week12::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Projection matrix : Orthographic Projection
	Mtx44 projection;
	projection.SetToOrtho(0, m_worldWidth, 0, m_worldHeight, -10, 10);
	projectionStack.LoadMatrix(projection);

	// Camera matrix
	viewStack.LoadIdentity();
	viewStack.LookAt(
		camera.position.x, camera.position.y, camera.position.z,
		camera.target.x, camera.target.y, camera.target.z,
		camera.up.x, camera.up.y, camera.up.z
		);
	// Model matrix : an identity matrix (model will be at the origin)
	modelStack.LoadIdentity();

	RenderMesh(meshList[GEO_AXES], false);

	modelStack.PushMatrix();
	modelStack.Translate(m_worldHeight * 0.5f, m_worldHeight * 0.5f, -1.f);
	modelStack.Scale(m_worldHeight, m_worldHeight, m_worldHeight);
	RenderMesh(meshList[GEO_WHITEQUAD], false);
	modelStack.PopMatrix();

	RenderGraph();

	//draw path
	if (npc!=nullptr && npc->gPath.size() > 0)
	{
		//draw target pos
		modelStack.PushMatrix();
		modelStack.Translate(npc->target.x, npc->target.y, 0.5f);
		modelStack.Scale(8.f, 8.f, 1.f);
		RenderMesh(meshList[GEO_CIRCLE], false);
		modelStack.PopMatrix();

		//draw rest of waypoints
		for (Vector3& pos : npc->gPath)
		{
			modelStack.PushMatrix();
			modelStack.Translate(pos.x, pos.y, 0.5f);
			modelStack.Scale(8.f, 8.f, 1.f);
			RenderMesh(meshList[GEO_CIRCLE], false);
			modelStack.PopMatrix();
		}
	}
	else if (npc != nullptr)
	{
		modelStack.PushMatrix();
		modelStack.Translate(npc->target.x, npc->target.y, 0.5f);
		modelStack.Scale(8.f, 8.f, 1.f);
		RenderMesh(meshList[GEO_CIRCLE], false);
		modelStack.PopMatrix();
	}

	for (GameObject* go : m_goList)
		RenderGO(go);

	//On screen text
	std::ostringstream ss;
	if (npc != nullptr)
	{
		if (!npc->gStack.empty())
			ss << "Wait: NPC doing DPS";
		else
			ss << "Click any node to perform A*";
		RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 50);
	}

	ss.str("");
	ss.precision(3);
	ss << "Speed:" << m_speed;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 6);

	ss.str("");
	ss.precision(5);
	ss << "FPS:" << fps;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 3);

	ss.str("");
	ss << "Graph " << m_graphKey;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 0);
}

void SceneGraph_Week12::Exit()
{
	SceneBase::Exit();
	//Cleanup GameObjects
	while (m_goList.size() > 0)
	{
		GameObject *go = m_goList.back();
		delete go;
		m_goList.pop_back();
	}
}

void SceneGraph_Week12::GoRandomAdjacent(GameObject* go)
{
	// Exercise Week 12
	// 7.	Implement these codes in SceneGraph_Week12::GoRandomAdjacent(). Can you explain what it does?
	//find node that npc is currently on
	go->currNode = m_graph.NearestNode(go->pos);
	Node* node = m_graph.m_nodes[go->currNode];

	//get any random outgoing edge
	Edge* edge = &node->edges[Math::RandIntMinMax(0, static_cast<int>(node->edges.size()) - 1)];
	//a target position to npc's path
	go->gPath.push_back(edge->to->pos);
}

void SceneGraph_Week12::DFSOnce(GameObject* go)
{
	// Exercise Week 12
	//8.	Compare this set of pseudo codes with the codes in SceneGraph_Week12::DFSOnce(). Can you explain what it does?
	//Push go->currNode onto go->gStack
	go->gStack.push_back(m_graph.m_nodes[go->currNode]);
	
	//setup container that tracks which nodes are visited
	if (go->visited.empty())
		go->visited.resize(m_graph.m_nodes.size(), false);
	
	//Set go->currNode as visited
	go->visited[go->currNode] = true;

	//Check neighbour nodes by iterating through each out-going edge of current node
	for (Edge& neighbour : m_graph.m_nodes[go->currNode]->edges)
	{
		//if neighbour is unvisited
		if (!go->visited[neighbour.to->id])
		{
			//easiest way to get the index to the neighbouring node is 
			//for each node to store its own index
			//add "int id" to Node if you haven't already done so
			go->currNode = neighbour.to->id;
			return;
		}
	}

	//nothing more to explore from this node, pop it off stack
	go->gStack.pop_back();

	//return to previous node
	if (!go->gStack.empty())
	{
		go->currNode = go->gStack.back()->id;
		go->gStack.pop_back();
		return;
	}
}

//Heuristics hasn't been mentioned in the lab sheet, but it is a key part of A*.
//without heuristics, it'd be Dijkstra's algo, not A*
//the heuristic function returns the estimated distance from current node to destination node
//this is what makes A* FASTER than Dijkstra's
static float GetHeuristic(Node* curr, Node* dest)
{
	return (curr->pos - dest->pos).Length();
}

// Exercise Week 12
//9.	Compare this set of pseudo codes with the codes in SceneGraph_Week12::AStar(). Can you explain what it does?
bool SceneGraph_Week12::AStar(GameObject* go, Node* startNode, Node* endNode)
{
	go->gPath.clear(); //reset container for storing sequence of waypoints to destination

	//gScore stores the total walking distance from startNode to the current node in the search
	std::vector<float> gScore(m_graph.m_nodes.size(), FLT_MAX);
	//fScore is defined as "actual distance from startNode to curr + "estimated distance from curr to endNode"
	//i.e. fScore = gScore + Heuristic. every node has a score.
	std::vector<float> fScore(m_graph.m_nodes.size(), FLT_MAX);
	//prev just stores where the agent came from before reaching this node in the search
	std::vector<int> prev(m_graph.m_nodes.size(), -1);

	//stores nodes for exploring/searching
	//NOTE: ideally, this should be implemented using a priorityqueue
	// the reason is because in A*, the next node we want to explore is always the one with the smallest
	// estimated cost, NOT necessarily the one that was pushed into the queue first
	// (cost in this context is defined as the total distance it'll take to go from start pt to destination)
	//For the ease of implementation, I'll just use an std::list at the (possible) cost of performance.
	std::list<Node*> nodeQueue;
	nodeQueue.push_back(startNode);
	gScore[startNode->id] = 0; //the distance from start node to start node is ZERO
	fScore[startNode->id] = GetHeuristic(startNode, endNode); //get a rough estimate of the distance it'll take to walk from startnode to endnode

	//visit each node in the queue until we find a path to destination
	while (!nodeQueue.empty())
	{
		//get node with the lowest fscore from nodeQueue
		//lowest fscore meant the node has a higher chance of being in the shortest path to destination
		auto it =
		std::min_element(nodeQueue.begin(), nodeQueue.end(), [&, fScore](Node* lhs, Node* rhs) {
			return fScore[lhs->id] < fScore[rhs->id];
			});

		Node* curr = *it;

		//check if path is found
		if (curr == endNode)
		{
			//constructing our path backwards
			while (curr != startNode)
			{
				//inserting at the front of a vector isn't efficient. 
				//but our path is short enough that we don't really need to care
				go->gPath.insert(go->gPath.begin(), m_graph.m_nodes[curr->id]->pos);
				curr = m_graph.m_nodes[prev[curr->id]];
			}
			go->gPath.insert(go->gPath.begin(), m_graph.m_nodes[startNode->id]->pos);
			return true;
		}

		//remove curr from 'queue'
		nodeQueue.erase(it);

		//go through every neighbouring nodes and add them to queue if they provide a better path(by comparing gscore)
		for (Edge& edge : curr->edges)
		{
			Node* neighbour = edge.to;
			//remember, the gscore is the walking distance from startnode to this neighbour
			//we always priortize the lowest scoring edges to be part of the shortest path
			float tempG = gScore[curr->id] + edge.cost; //+edge.cost because that's the distance from curr to neighbour
			if (tempG < gScore[neighbour->id])
			{
				prev[neighbour->id] = curr->id; //record the path!
				gScore[neighbour->id] = tempG;
				fScore[neighbour->id] = tempG + GetHeuristic(neighbour, endNode);
				//add neighbour into 'queue' if it's not already added
				//we will eventually visit this node to explore its neighbours
				if (std::find(nodeQueue.begin(), nodeQueue.end(), neighbour) == nodeQueue.end())
					nodeQueue.push_back(neighbour);
			}
		}
	}

	return false;
}
