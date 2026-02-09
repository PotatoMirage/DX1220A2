#pragma once
#include "GameObject.h"
#include <vector>
#include <map>
#include "SceneBase.h"
#include "ObjectBase.h"
#include "ConcreteMessages.h"

class SceneSandbox : public SceneBase, public ObjectBase
{
public:
	enum TURN_PHASE
	{
		PHASE_LOGIC,
		PHASE_ANIMATION,
		PHASE_WAITING
	};

	// New Terrain Types
	enum TERRAIN_TYPE
	{
		TERRAIN_ROAD,     
		TERRAIN_FLOOR,    
		TERRAIN_FOREST,   
		TERRAIN_MUD,      
		TERRAIN_MOUNTAIN, 
		TERRAIN_WALL,     
		TERRAIN_WATER,
		NUM_TERRAIN
	};

	SceneSandbox();
	~SceneSandbox();

	virtual void Init();
	virtual void Update(double dt);
	virtual void Render();
	virtual void Exit();

	void RenderGO(GameObject* go);
	bool Handle(Message* message);

	GameObject* FetchGO(GameObject::GAMEOBJECT_TYPE type);
	void SpawnUnit(MessageSpawnUnit::UNIT_TYPE unitType, Vector3 position, int teamID);
	std::vector<MazePt> FindPathAStar(MazePt start, MazePt end, GameObject::GAMEOBJECT_TYPE unitType);
	std::vector<MazePt> FindPathDFS(MazePt start, MazePt end);   // For Exploration

protected:
	// Turn-Based Systems
	TURN_PHASE m_currPhase;
	int m_turnNumber;
	float m_animationSpeed;

	// UI Controls
	bool m_autoTurn;
	float m_turnTimer;
	float m_turnInterval;

	void ProcessTurnLogic();
	bool ProcessTurnAnimation(double dt);
	float GetTileCost(int x, int y, GameObject::GAMEOBJECT_TYPE unitType) const;
	float GetTerrainMovementCost(TERRAIN_TYPE terrain, GameObject::GAMEOBJECT_TYPE unitType) const;

	// Helper functions
	int IsWithinBoundary(int x) const;
	int Get1DIndex(int x, int y) const;
	void DetectNearbyEntities(GameObject* go);
	void FindNearestResource(GameObject* go);
	bool IsInTerritory(Vector3 pos, int teamID) const;
	void UpdateSpatialGrid();
	GameObject* GetNearestEnemy(Vector3 pos, int teamID, float maxRange);
	void FindNearestInjuredAlly(GameObject* go);

	// Map Generation
	void GenerateMap();
	void EnforceSymmetry();
	void EnsureConnectivity();
	void FillDeadZones();
	bool IsWalkable(TERRAIN_TYPE type) const;

	// Game state
	std::vector<GameObject*> m_goList;
	std::vector<std::vector<GameObject*>> m_spatialGrid;
	float m_speed;
	float m_worldWidth;
	float m_worldHeight;
	int m_noGrid;
	float m_gridSize;
	float m_gridOffset;

	// Terrain Data
	std::vector<TERRAIN_TYPE> m_terrainGrid;
	std::vector<bool> m_foodGrid;

	bool IsGridOccupied(int gridX, int gridY);
	MazePt GetNearestVacantNeighbor(MazePt target, MazePt start);
	void SpawnTrail(GameObject* startObj, GameObject* endFood, int teamID);

	// Red Colony (Team 0)
	int m_redWorkerCount;
	int m_redSoldierCount;
	int m_redHealerCount;
	int m_redScoutCount;
	int m_redTankCount;
	int m_redResources;
	GameObject* m_redQueen;

	// Blue Colony (Team 1)
	int m_blueWorkerCount;
	int m_blueSoldierCount;
	int m_blueHealerCount;
	int m_blueScoutCount;
	int m_blueTankCount;
	int m_blueResources;
	GameObject* m_blueQueen;

	// Food resources
	std::vector<Vector3> m_foodLocations;

	// Simulation state
	float m_simulationTime;
	bool m_simulationEnded;
	int m_winner; // 0 = Red, 1 = Blue, 2 = Draw

	// Performance optimization
	float m_updateTimer;
	int m_updateCycle;
	bool m_coloniesDetected;
};