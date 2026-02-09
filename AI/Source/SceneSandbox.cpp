#include "SceneSandbox.h"
#include "GL\glew.h"
#include "Application.h"
#include <sstream>
#include "StatesSandbox.h"
#include "SceneData.h"
#include "PostOffice.h"
#include "ConcreteMessages.h"
#include <iomanip>
#include <queue>
#include <algorithm>

SceneSandbox::SceneSandbox()
	: m_goList{}, m_spatialGrid{}, m_speed{}, m_worldWidth{}, m_worldHeight{},
	m_noGrid{}, m_gridSize{}, m_gridOffset{},
	m_redWorkerCount{}, m_redResources{}, m_blueWorkerCount{}, m_blueResources{},
	m_redQueen{}, m_blueQueen{}, m_simulationTime{}, m_simulationEnded{}, m_winner{}, m_updateTimer{}, m_updateCycle{},
	m_terrainGrid{}, m_foodGrid{}, m_coloniesDetected(false),
	m_currPhase(PHASE_LOGIC), m_turnNumber(0), m_animationSpeed(5.0f), m_autoTurn(false), m_turnTimer(0.f), m_turnInterval(0.05f),
	m_renderFog(true), m_currentTeamFog(0)
{
}

SceneSandbox::~SceneSandbox()
{
}

void SceneSandbox::Init()
{
	SceneBase::Init();
	bLightEnabled = false;

	m_worldHeight = 100.f;
	m_worldWidth = m_worldHeight * (float)Application::GetWindowWidth() / Application::GetWindowHeight();

	m_speed = 1.f;
	m_animationSpeed = 500.f;

	Math::InitRNG();

	m_noGrid = 30;
	m_gridSize = m_worldHeight / m_noGrid;
	m_gridOffset = m_gridSize / 2;
	m_spatialGrid.resize(m_noGrid * m_noGrid);
	m_terrainGrid.assign(m_noGrid * m_noGrid, TERRAIN_FLOOR);
	m_foodGrid.assign(m_noGrid * m_noGrid, false);

	m_fogGrid.assign(m_noGrid * m_noGrid, true);
	m_renderFog = true;
	m_currentTeamFog = 0;
	GenerateMap();

	SceneData::GetInstance()->SetObjectCount(0);
	SceneData::GetInstance()->SetFishCount(0);
	SceneData::GetInstance()->SetNumGrid(m_noGrid);
	SceneData::GetInstance()->SetGridSize(m_gridSize);
	SceneData::GetInstance()->SetGridOffset(m_gridOffset);
	ResetGlobalSandboxVars();
	PostOffice::GetInstance()->Register("Scene", this);

	m_redWorkerCount = 0; m_redSoldierCount = 0; m_redHealerCount = 0; m_redScoutCount = 0; m_redTankCount = 0;
	m_blueWorkerCount = 0; m_blueSoldierCount = 0; m_blueHealerCount = 0; m_blueScoutCount = 0; m_blueTankCount = 0;
	m_redResources = 0; m_blueResources = 0;
	m_simulationTime = 0.f; m_simulationEnded = false; m_winner = 2;
	m_updateTimer = 0.f; m_updateCycle = 0;
	m_currPhase = PHASE_LOGIC;
	m_turnNumber = 1;

	m_currentEvent = EVENT_NONE;
	m_eventDuration = 0;

	m_redQueen = FetchGO(GameObject::GO_QUEEN); m_redQueen->teamID = 0;
	m_redQueen->pos.Set(m_gridSize * 3.f + m_gridOffset, m_gridSize * 3.f + m_gridOffset, 0);
	m_redQueen->homeBase = m_redQueen->pos; m_redQueen->scale.Set(m_gridSize * 1.5f, m_gridSize * 1.5f, 1.f); m_redQueen->maxHealth = 50.f; m_redQueen->health = 50.f; m_redQueen->moveSpeed = 0.f; m_redQueen->detectionRange = m_gridSize * 8.f; m_redQueen->sm = new StateMachine(); m_redQueen->sm->AddState(new StateQueenSpawning("Spawning", m_redQueen)); m_redQueen->sm->AddState(new StateQueenEmergency("Emergency", m_redQueen)); m_redQueen->sm->AddState(new StateQueenCooldown("Cooldown", m_redQueen)); m_redQueen->sm->SetNextState("Spawning");
	m_redQueen->target = m_redQueen->pos; m_redQueen->countDown = 0.f;
	m_redQueen->visibilityType = GameObject::VISIBILITY_2_TILE_OMNI;

	m_blueQueen = FetchGO(GameObject::GO_QUEEN); m_blueQueen->teamID = 1;
	m_blueQueen->pos.Set(m_gridSize * (m_noGrid - 4.f) + m_gridOffset, m_gridSize * (m_noGrid - 4.f) + m_gridOffset, 0);
	m_blueQueen->homeBase = m_blueQueen->pos; m_blueQueen->scale.Set(m_gridSize * 1.5f, m_gridSize * 1.5f, 1.f); m_blueQueen->maxHealth = 50.f; m_blueQueen->health = 50.f; m_blueQueen->moveSpeed = 0.f; m_blueQueen->detectionRange = m_gridSize * 8.f; m_blueQueen->sm = new StateMachine(); m_blueQueen->sm->AddState(new StateQueenSpawning("Spawning", m_blueQueen)); m_blueQueen->sm->AddState(new StateQueenEmergency("Emergency", m_blueQueen)); m_blueQueen->sm->AddState(new StateQueenCooldown("Cooldown", m_blueQueen)); m_blueQueen->sm->SetNextState("Spawning");
	m_blueQueen->target = m_blueQueen->pos; m_blueQueen->countDown = 0.f;
	m_blueQueen->visibilityType = GameObject::VISIBILITY_2_TILE_OMNI;

	for (int i = 0; i < 3; ++i) { SpawnUnit(MessageSpawnUnit::UNIT_SPEEDY_ANT_WORKER, m_redQueen->pos, 0); SpawnUnit(MessageSpawnUnit::UNIT_STRONG_ANT_WORKER, m_blueQueen->pos, 1); }
	for (int i = 0; i < 2; ++i) {
		SpawnUnit(MessageSpawnUnit::UNIT_SPEEDY_ANT_SOLDIER, m_redQueen->pos + Vector3(Math::RandFloatMinMax(-3, 3) * m_gridSize, Math::RandFloatMinMax(-3, 3) * m_gridSize, 0), 0);
		SpawnUnit(MessageSpawnUnit::UNIT_STRONG_ANT_SOLDIER, m_blueQueen->pos + Vector3(Math::RandFloatMinMax(-3, 3) * m_gridSize, Math::RandFloatMinMax(-3, 3) * m_gridSize, 0), 1);
	}
	SpawnUnit(MessageSpawnUnit::UNIT_SCOUT, m_redQueen->pos + Vector3(0, 2, 0), 0);
	SpawnUnit(MessageSpawnUnit::UNIT_SCOUT, m_blueQueen->pos + Vector3(0, -2, 0), 1);

	m_foodLocations.clear();
	std::vector<GameObject*> allFood;
	int foodCount = Math::RandIntMinMax(15, 25);
	for (int i = 0; i < foodCount; ++i)
	{
		GameObject* food = FetchGO(GameObject::GO_FOOD);
		int gridX, gridY;
		bool validPos = false;
		int attempts = 0;
		while (!validPos && attempts < 100) {
			attempts++;
			if (i < foodCount / 2) { int minC = (int)(m_noGrid * 0.3f); int maxC = (int)(m_noGrid * 0.7f); gridX = Math::RandIntMinMax(minC, maxC); gridY = Math::RandIntMinMax(minC, maxC); }
			else { gridX = Math::RandIntMinMax(2, m_noGrid - 3); gridY = Math::RandIntMinMax(2, m_noGrid - 3); }

			if (!IsWithinBoundary(gridX) || !IsWithinBoundary(gridY)) continue;
			if (m_terrainGrid[Get1DIndex(gridX, gridY)] == TERRAIN_WALL || m_terrainGrid[Get1DIndex(gridX, gridY)] == TERRAIN_WATER) continue;
			if (m_foodGrid[Get1DIndex(gridX, gridY)]) continue;
			if (gridX <= 5 && gridY <= 5) continue;
			if (gridX >= m_noGrid - 6 && gridY >= m_noGrid - 6) continue;
			validPos = true;
		}
		if (validPos) {
			float worldX = gridX * m_gridSize + m_gridOffset; float worldY = gridY * m_gridSize + m_gridOffset;
			food->pos.Set(worldX, worldY, 0); food->target = food->pos; food->scale.Set(m_gridSize * 0.8f, m_gridSize * 0.8f, 1.f);
			food->moveSpeed = 0.f; food->health = 1.f; food->resourceCount = 25; food->harvesterCount = 0; food->isMarked = false;
			m_foodGrid[Get1DIndex(gridX, gridY)] = true; m_foodLocations.push_back(food->pos); allFood.push_back(food);
		}
	}
}

GameObject* SceneSandbox::FetchGO(GameObject::GAMEOBJECT_TYPE type)
{
	for (size_t i = 0; i < m_goList.size(); ++i) {
		GameObject* go = m_goList[i];
		if (!go->active && go->type == type) { go->active = true; return go; }
	}
	for (unsigned i = 0; i < 10; ++i) {
		GameObject* go = new GameObject(type);
		m_goList.push_back(go);
	}
	return FetchGO(type);
}

void SceneSandbox::SpawnUnit(MessageSpawnUnit::UNIT_TYPE unitType, Vector3 position, int teamID) {
	GameObject* unit = nullptr;
	float workerHP = 10.f; float workerSpeed = 5.f; float workerAtk = 0.5f;
	float soldierHP = 20.f; float soldierSpeed = 3.f; float soldierAtk = 3.0f;

	switch (unitType) {
	case MessageSpawnUnit::UNIT_SPEEDY_ANT_WORKER:
	case MessageSpawnUnit::UNIT_STRONG_ANT_WORKER:
		unit = FetchGO(GameObject::GO_WORKER); unit->teamID = teamID; unit->homeBase = (teamID == 0) ? m_redQueen->pos : m_blueQueen->pos;
		unit->maxHealth = workerHP; unit->health = workerHP; unit->attackPower = workerAtk; unit->moveSpeed = workerSpeed; unit->baseSpeed = workerSpeed; unit->detectionRange = m_gridSize * 6.f; unit->attackRange = m_gridSize * 0.8f;
		unit->visibilityType = GameObject::VISIBILITY_1_TILE;
		unit->sm = new StateMachine(); unit->sm->AddState(new StateWorkerIdle("Idle", unit)); unit->sm->AddState(new StateWorkerSearching("Searching", unit)); unit->sm->AddState(new StateWorkerGathering("Gathering", unit)); unit->sm->AddState(new StateWorkerFleeing("Fleeing", unit)); unit->sm->SetNextState("Idle"); break;
	case MessageSpawnUnit::UNIT_SPEEDY_ANT_SOLDIER:
	case MessageSpawnUnit::UNIT_STRONG_ANT_SOLDIER:
		unit = FetchGO(GameObject::GO_SOLDIER); unit->teamID = teamID; unit->homeBase = (teamID == 0) ? m_redQueen->pos : m_blueQueen->pos;
		unit->maxHealth = soldierHP; unit->health = soldierHP; unit->attackPower = soldierAtk; unit->moveSpeed = soldierSpeed; unit->baseSpeed = soldierSpeed; unit->detectionRange = m_gridSize * 8.f; unit->attackRange = m_gridSize * 1.3f;
		unit->visibilityType = GameObject::VISIBILITY_2_TILE_OMNI;
		unit->sm = new StateMachine(); unit->sm->AddState(new StateSoldierPatrolling("Patrolling", unit)); unit->sm->AddState(new StateSoldierAttacking("Attacking", unit)); unit->sm->AddState(new StateSoldierResting("Resting", unit)); unit->sm->AddState(new StateSoldierRetreating("Retreating", unit)); unit->sm->SetNextState("Patrolling"); break;
	case MessageSpawnUnit::UNIT_HEALER:
		unit = FetchGO(GameObject::GO_HEALER); unit->teamID = teamID; unit->homeBase = (teamID == 0) ? m_redQueen->pos : m_blueQueen->pos; unit->maxHealth = 8.f; unit->health = 8.f; unit->moveSpeed = 4.f; unit->baseSpeed = 4.f;
		unit->visibilityType = GameObject::VISIBILITY_2_TILE_OMNI;
		unit->sm = new StateMachine(); unit->sm->AddState(new StateHealerIdle("Idle", unit)); unit->sm->AddState(new StateHealerTraveling("Traveling", unit)); unit->sm->AddState(new StateHealerHealing("Healing", unit)); unit->sm->SetNextState("Idle"); break;
	case MessageSpawnUnit::UNIT_SCOUT:
		unit = FetchGO(GameObject::GO_SCOUT); unit->teamID = teamID; unit->homeBase = (teamID == 0) ? m_redQueen->pos : m_blueQueen->pos; unit->maxHealth = 5.f; unit->health = 5.f; unit->moveSpeed = 8.f; unit->baseSpeed = 8.f; unit->detectionRange = m_gridSize * 6.f;
		unit->visibilityType = GameObject::VISIBILITY_3_TILE_LINEAR;
		unit->sm = new StateMachine(); unit->sm->AddState(new StateScoutPatrolling("Patrolling", unit)); unit->sm->AddState(new StateScoutReturnToColony("ReturnToColony", unit)); unit->sm->AddState(new StateScoutHiding("Hiding", unit)); unit->sm->SetNextState("Patrolling"); break;
	case MessageSpawnUnit::UNIT_TANK:
		unit = FetchGO(GameObject::GO_TANK); unit->teamID = teamID; unit->homeBase = (teamID == 0) ? m_redQueen->pos : m_blueQueen->pos; unit->maxHealth = 40.f; unit->health = 40.f; unit->moveSpeed = 1.5f; unit->baseSpeed = 1.5f; unit->attackPower = 1.0f; unit->attackRange = m_gridSize * 0.5f;
		unit->visibilityType = GameObject::VISIBILITY_2_TILE_OMNI;
		unit->sm = new StateMachine(); unit->sm->AddState(new StateTankGuarding("Guarding", unit)); unit->sm->AddState(new StateTankBlocking("Blocking", unit)); unit->sm->AddState(new StateTankRecovering("Recovering", unit)); unit->sm->SetNextState("Guarding"); break;
	}

	if (unit) {
		int gx = (int)(position.x / m_gridSize);
		int gy = (int)(position.y / m_gridSize);

		if (!IsWithinBoundary(gx) || !IsWithinBoundary(gy) || !IsWalkable(m_terrainGrid[Get1DIndex(gx, gy)]))
		{
			bool found = false;
			std::queue<int> q;
			std::vector<bool> visited(m_noGrid * m_noGrid, false);

			if (IsWithinBoundary(gx) && IsWithinBoundary(gy)) {
				int startIdx = Get1DIndex(gx, gy);
				q.push(startIdx);
				visited[startIdx] = true;
			}

			int dx[] = { 0, 0, -1, 1, -1, -1, 1, 1 };
			int dy[] = { 1, -1, 0, 0, -1, 1, -1, 1 };

			while (!q.empty()) {
				int curr = q.front(); q.pop();
				int cx = curr % m_noGrid; int cy = curr / m_noGrid;

				if (IsWalkable(m_terrainGrid[curr])) {
					gx = cx; gy = cy;
					found = true;
					break;
				}

				for (int i = 0; i < 8; ++i) {
					int nx = cx + dx[i]; int ny = cy + dy[i];
					if (IsWithinBoundary(nx) && IsWithinBoundary(ny)) {
						int nIdx = Get1DIndex(nx, ny);
						if (!visited[nIdx]) {
							visited[nIdx] = true;
							q.push(nIdx);
						}
					}
				}
			}
		}

		unit->pos.Set(gx * m_gridSize + m_gridOffset, gy * m_gridSize + m_gridOffset, 0);
		unit->target = unit->pos;
		unit->countDown = 0.f;
		unit->scale.Set(m_gridSize, m_gridSize, 1.f);
		unit->targetFoodItem = nullptr;
	}
}

void SceneSandbox::SpawnTrail(GameObject* startObj, GameObject* endFood, int teamID)
{
	return;
}


bool SceneSandbox::IsGridOccupied(int gridX, int gridY) {
	if (!IsWithinBoundary(gridX) || !IsWithinBoundary(gridY)) return true;
	if (!IsWalkable(m_terrainGrid[Get1DIndex(gridX, gridY)])) return true;
	return false;
}

MazePt SceneSandbox::GetNearestVacantNeighbor(MazePt target, MazePt start) {
	int dx[] = { 0, 0, -1, 1 }; int dy[] = { 1, -1, 0, 0 };
	MazePt bestPt = start; float minDist = FLT_MAX; bool foundAny = false;
	for (int i = 0; i < 4; ++i) {
		int nx = target.x + dx[i]; int ny = target.y + dy[i];
		if (!IsGridOccupied(nx, ny)) {
			float dist = (float)((nx - start.x) * (nx - start.x) + (ny - start.y) * (ny - start.y));
			if (dist < minDist) { minDist = dist; bestPt.Set(nx, ny); foundAny = true; }
		}
	}
	if (!foundAny) return start;
	return bestPt;
}


void SceneSandbox::Update(double dt)
{
	SceneBase::Update(dt);
	m_worldHeight = 100.f; m_worldWidth = m_worldHeight * (float)Application::GetWindowWidth() / Application::GetWindowHeight();

	if (Application::IsKeyPressed(VK_END)) m_simulationEnded = true;

	static bool bMKeyState = false;
	if (Application::IsKeyPressed('M') && !bMKeyState) {
		bMKeyState = true;
		m_renderFog = !m_renderFog; // Toggle flag
	}
	else if (!Application::IsKeyPressed('M') && bMKeyState) {
		bMKeyState = false;
	}

	static bool bLKeyState = false;
	if (Application::IsKeyPressed('L') && !bLKeyState) {
		bLKeyState = true;
		m_currentTeamFog = (m_currentTeamFog + 1) % 2; // Toggle between 0 (Red) and 1 (Blue)
	}
	else if (!Application::IsKeyPressed('L') && bLKeyState) {
		bLKeyState = false;
	}

	static bool bTKeyState = false;
	if (Application::IsKeyPressed('T') && !bTKeyState) {
		bTKeyState = true;
		m_autoTurn = !m_autoTurn;
	}
	else if (!Application::IsKeyPressed('T') && bTKeyState) {
		bTKeyState = false;
	}
	if (Application::IsKeyPressed(VK_OEM_PLUS) || Application::IsKeyPressed(VK_ADD)) {
		m_turnInterval = max(0.01f, m_turnInterval - (float)dt * 0.5f);
	}
	if (Application::IsKeyPressed(VK_OEM_MINUS) || Application::IsKeyPressed(VK_SUBTRACT)) {
		m_turnInterval += (float)dt * 0.5f;
	}

	bool manualNextTurn = false;
	static bool bSpaceState = false;
	if (Application::IsKeyPressed(VK_SPACE) && !bSpaceState) {
		bSpaceState = true;
		if (!m_autoTurn && m_currPhase == PHASE_LOGIC) manualNextTurn = true;
	}
	else if (!Application::IsKeyPressed(VK_SPACE) && bSpaceState) {
		bSpaceState = false;
	}

	if (!m_simulationEnded)
	{
		UpdateSpatialGrid();
		UpdateFogOfWar(m_currentTeamFog);

		switch (m_currPhase)
		{
		case PHASE_LOGIC:
		{
			bool shouldAdvance = false;
			if (m_autoTurn) {
				m_turnTimer += (float)dt * m_speed;
				if (m_turnTimer >= m_turnInterval) { m_turnTimer = 0.f; shouldAdvance = true; }
			}
			else {
				if (manualNextTurn) shouldAdvance = true;
			}

			if (shouldAdvance) {
				ProcessTurnLogic();
				m_currPhase = PHASE_ANIMATION;
			}
		}
		break;

		case PHASE_ANIMATION:
			bool isStillMoving = ProcessTurnAnimation(dt);
			if (!isStillMoving) {
				m_currPhase = PHASE_LOGIC;
				m_turnNumber++;
				m_simulationTime += 1.0f;
			}
			break;
		}

		if (!m_redQueen->active) { m_simulationEnded = true; m_winner = 1; }
		if (!m_blueQueen->active) { m_simulationEnded = true; m_winner = 0; }
		if (m_simulationTime >= 240.f && !m_coloniesDetected) m_coloniesDetected = true;

		std::fill(m_foodGrid.begin(), m_foodGrid.end(), false);
		for (auto go : m_goList) { if (go->active && go->type == GameObject::GO_FOOD) { int gx = (int)(go->pos.x / m_gridSize); int gy = (int)(go->pos.y / m_gridSize); m_foodGrid[Get1DIndex(gx, gy)] = true; } }

		m_redWorkerCount = 0; m_redSoldierCount = 0; m_redHealerCount = 0; m_redScoutCount = 0; m_redTankCount = 0;
		m_blueWorkerCount = 0; m_blueSoldierCount = 0; m_blueHealerCount = 0; m_blueScoutCount = 0; m_blueTankCount = 0;
		for (auto go : m_goList) {
			if (!go->active) continue;
			if (go->teamID == 0) {
				if (go->type == GameObject::GO_WORKER) m_redWorkerCount++;
				else if (go->type == GameObject::GO_SOLDIER) m_redSoldierCount++;
				else if (go->type == GameObject::GO_HEALER) m_redHealerCount++;
				else if (go->type == GameObject::GO_SCOUT) m_redScoutCount++;
				else if (go->type == GameObject::GO_TANK) m_redTankCount++;
			}
			else if (go->teamID == 1) {
				if (go->type == GameObject::GO_WORKER) m_blueWorkerCount++;
				else if (go->type == GameObject::GO_SOLDIER) m_blueSoldierCount++;
				else if (go->type == GameObject::GO_HEALER) m_blueHealerCount++;
				else if (go->type == GameObject::GO_SCOUT) m_blueScoutCount++;
				else if (go->type == GameObject::GO_TANK) m_blueTankCount++;
			}
		}
	}
}

void SceneSandbox::ProcessTurnLogic()
{
	if (m_eventDuration > 0) {
		m_eventDuration--;
		if (m_eventDuration <= 0) {
			m_currentEvent = EVENT_NONE;
		}
	}

	if (m_turnNumber > 1000) {
		m_currentEvent = EVENT_SUDDEN_DEATH;
		TriggerSuddenDeathStrike();
	}
	else if (m_currentEvent == EVENT_NONE) {
		if (Math::RandIntMinMax(0, 100) < 5) {
			ProcessRandomEvents();
		}
	}

	for (size_t i = 0; i < m_goList.size(); ++i)
	{
		GameObject* go = m_goList[i];
		if (!go->active) continue;

		if (go->countDown > 0.0f) {
			go->countDown -= 1.0f;
			if (go->countDown > 0.0f) {
				continue;
			}
			go->countDown = 0.0f;
		}

		DetectNearbyEntities(go);

		if (go->type == GameObject::GO_WORKER && !go->isCarryingResource) FindNearestResource(go);
		if (go->type == GameObject::GO_HEALER) FindNearestInjuredAlly(go);
		if (go->type == GameObject::GO_SCOUT && go->targetFoodItem == nullptr) FindNearestResource(go);

		if (go->sm) go->sm->Update(1.0f);

		int gridX = static_cast<int>(go->pos.x / m_gridSize);
		int gridY = static_cast<int>(go->pos.y / m_gridSize);
		MazePt startPt(gridX, gridY);

		int targetGridX = static_cast<int>(go->target.x / m_gridSize);
		int targetGridY = static_cast<int>(go->target.y / m_gridSize);
		MazePt targetPt(targetGridX, targetGridY);

		if (go->targetFoodItem != nullptr && go->targetFoodItem->active) {
			bool shouldSnap = false;
			if (go->type == GameObject::GO_WORKER && !go->isCarryingResource) shouldSnap = true;
			if (go->type == GameObject::GO_SCOUT && (go->target - go->targetFoodItem->pos).LengthSquared() < (m_gridSize * 5) * (m_gridSize * 5)) shouldSnap = true;
			if (shouldSnap) {
				targetPt = GetNearestVacantNeighbor(MazePt((int)(go->targetFoodItem->pos.x / m_gridSize), (int)(go->targetFoodItem->pos.y / m_gridSize)), startPt);
			}
		}

		bool needRepath = false;
		if (go->path.empty())
		{
			if (gridX != targetPt.x || gridY != targetPt.y) needRepath = true;
		}
		else
		{
			MazePt pathEnd = go->path.back();
			if (pathEnd.x != targetPt.x || pathEnd.y != targetPt.y) needRepath = true;
		}

		if (needRepath)
		{
			bool useDFS = false;
			if (go->type == GameObject::GO_SCOUT) useDFS = true;
			if (go->type == GameObject::GO_WORKER && !go->isCarryingResource && go->targetFoodItem == nullptr) useDFS = true;

			if (useDFS) {
				go->path = FindPathDFS(startPt, targetPt);
			}
			else {
				go->path = FindPathAStar(startPt, targetPt, go->type);
			}

			if (!go->path.empty() && go->path[0].x == gridX && go->path[0].y == gridY)
				go->path.erase(go->path.begin());
		}

		if (!go->path.empty())
		{
			MazePt nextStep = go->path.front();
			go->path.erase(go->path.begin());

			go->target = Vector3(nextStep.x * m_gridSize + m_gridOffset, nextStep.y * m_gridSize + m_gridOffset, go->pos.z);

			if (go->type == GameObject::GO_WORKER && !go->isCarryingResource)
				go->pathHistory.push_back(nextStep);

			float cost = GetTileCost(nextStep.x, nextStep.y, go->type);

			go->countDown = (cost > 1.0f) ? (cost - 1.0f) : 0.0f;

			Vector3 dir = go->target - go->pos;
			if (dir.LengthSquared() > 0.001f) go->viewDir = dir.Normalized();
		}
		else
		{
			go->target = go->pos;
		}
	}
}

void SceneSandbox::ProcessRandomEvents()
{
	int roll = Math::RandIntMinMax(0, 100);

	if (roll < 40) {
		m_currentEvent = EVENT_HEAVY_RAIN;
		m_eventDuration = 10; // Lasts 10 turns
		std::cout << "EVENT: HEAVY RAIN! Movement is slower in mud/forest." << std::endl;
	}
	else if (roll < 70) {
		m_currentEvent = EVENT_EARTHQUAKE;
		m_eventDuration = 0; // Instantaneous
		TriggerEarthquake();
		std::cout << "EVENT: EARTHQUAKE! Terrain has shifted." << std::endl;
	}
	else {
		m_currentEvent = EVENT_PLAGUE;
		m_eventDuration = 0; // Instantaneous
		TriggerPlague();
		std::cout << "EVENT: PLAGUE! Units have taken damage." << std::endl;
	}
}

void SceneSandbox::TriggerEarthquake()
{
	// Randomly alter 5% of the map
	int totalCells = m_noGrid * m_noGrid;
	int changes = totalCells / 20;

	for (int i = 0; i < changes; ++i) {
		int idx = Math::RandIntMinMax(0, totalCells - 1);

		// Don't modify base perimeters (safety)
		int x = idx % m_noGrid;
		int y = idx / m_noGrid;
		if (x < 6 && y < 6) continue;
		if (x > m_noGrid - 7 && y > m_noGrid - 7) continue;

		TERRAIN_TYPE current = m_terrainGrid[idx];

		if (current == TERRAIN_WALL) {
			m_terrainGrid[idx] = TERRAIN_FLOOR; // Walls collapse
		}
		else if (current == TERRAIN_FLOOR) {
			if (Math::RandIntMinMax(0, 1) == 0) m_terrainGrid[idx] = TERRAIN_MUD; // Liquefaction
			else m_terrainGrid[idx] = TERRAIN_WALL; // Debris
		}
	}
	// Re-verify connectivity so units don't get trapped forever
	EnsureConnectivity();
}

void SceneSandbox::TriggerPlague()
{
	// Damage random clusters of units
	for (auto go : m_goList) {
		if (!go->active) continue;
		// 15% chance to be hit by plague
		if (Math::RandIntMinMax(0, 100) < 15) {
			go->health -= go->maxHealth * 0.3f; // 30% max HP damage
			if (go->health <= 0) {
				go->active = false; // Instant death
				PostOffice::GetInstance()->Send("Scene", new MessageUnitDied(go, go->teamID, go->type));
			}
		}
	}
}

void SceneSandbox::TriggerSuddenDeathStrike()
{
	// Meteor Shower Logic: Pick 3 random impact points per turn
	for (int i = 0; i < 3; ++i) {
		int tx = Math::RandIntMinMax(0, m_noGrid - 1);
		int ty = Math::RandIntMinMax(0, m_noGrid - 1);

		// Blast radius
		int rad = 2;
		for (int y = ty - rad; y <= ty + rad; ++y) {
			for (int x = tx - rad; x <= tx + rad; ++x) {
				if (IsWithinBoundary(x) && IsWithinBoundary(y)) {
					// Destroy Terrain
					int idx = Get1DIndex(x, y);
					m_terrainGrid[idx] = TERRAIN_MUD; // Crater effect

					// Kill Units
					for (auto go : m_spatialGrid[idx]) {
						if (go->active) {
							go->health = 0;
							go->active = false;
							PostOffice::GetInstance()->Send("Scene", new MessageUnitDied(go, go->teamID, go->type));
						}
					}
				}
			}
		}
	}
}

bool SceneSandbox::ProcessTurnAnimation(double dt)
{
	bool anyMoving = false;
	float step = m_animationSpeed * (float)dt * m_speed;

	// FIX: Use index-based loop here as well for safety
	for (size_t i = 0; i < m_goList.size(); ++i)
	{
		GameObject* go = m_goList[i];
		if (!go->active) continue;

		float distSq = (go->target - go->pos).LengthSquared();
		if (distSq > 0.01f)
		{
			anyMoving = true;
			Vector3 dir = go->target - go->pos;
			float dist = dir.Length();

			if (dist <= step) {
				go->pos = go->target;
			}
			else {
				go->pos += dir.Normalized() * step;
			}
		}
	}
	return anyMoving;
}

float SceneSandbox::GetTileCost(int x, int y, GameObject::GAMEOBJECT_TYPE unitType) const
{
	if (!IsWithinBoundary(x) || !IsWithinBoundary(y)) return FLT_MAX;

	int idx = Get1DIndex(x, y);
	float cost = GetTerrainMovementCost(m_terrainGrid[idx], unitType);

	if (cost >= FLT_MAX) return FLT_MAX;

	if (!m_spatialGrid[idx].empty())
	{
		cost += 5.0f;
	}

	return cost;
}

float SceneSandbox::GetTerrainMovementCost(TERRAIN_TYPE type, GameObject::GAMEOBJECT_TYPE unitType) const
{
	// BASE COSTS (Cost = Turns spent)
	// Road: 1 | Floor: 2 | Forest: 3 | Mud: 4 | Mountain: 5 | Water/Wall: INF

	float cost = FLT_MAX;

	switch (type) {
	case TERRAIN_ROAD:
		cost = 1.0f; // Fast for everyone
		break;
	case TERRAIN_FLOOR:
		cost = 2.0f; // Standard baseline
		break;
	case TERRAIN_FOREST:
		cost = 3.0f; // Difficult
		break;
	case TERRAIN_MUD:
		cost = 4.0f; // Very Slow
		break;
	case TERRAIN_WALL:
	case TERRAIN_WATER:
		return FLT_MAX; // Impassable
	default:
		cost = 2.0f;
	}
	if (m_currentEvent == EVENT_HEAVY_RAIN) {
		// Mud and Forest become extremely difficult to traverse
		if (type == TERRAIN_MUD || type == TERRAIN_FOREST) {
			cost *= 2.0f;
		}
		// Floor becomes slightly slippery
		if (type == TERRAIN_FLOOR) {
			cost += 1.0f;
		}
	}

	// --- UNIT SPECIALIZATIONS ---

	// SCOUT: Expert explorer, moves fast through rough terrain
	if (unitType == GameObject::GO_SCOUT) {
		if (type == TERRAIN_FOREST) cost = 1.0f;    // No penalty in forest
		if (type == TERRAIN_MUD) cost = 2.0f;       // Light feet
	}
	// TANK: Heavy treads, ignores mud but slow on mountains
	else if (unitType == GameObject::GO_TANK) {
		if (type == TERRAIN_MUD) cost = 2.0f;       // Treads grip mud
		if (type == TERRAIN_FOREST) cost = 4.0f;    // Too big for trees
	}
	// WORKER: Very dependent on Roads
	else if (unitType == GameObject::GO_WORKER || unitType == GameObject::GO_STRONG_ANT_WORKER) {
		if (type == TERRAIN_ROAD) cost = 1.0f;
		else if (type == TERRAIN_FLOOR) cost = 2.0f;
		else cost += 1.0f; // Workers struggle off-road
	}

	return cost;
}

void SceneSandbox::DetectNearbyEntities(GameObject* go)
{
	if (go->type == GameObject::GO_FOOD || go->type == GameObject::GO_QUEEN || go->type == GameObject::GO_STRONG_ANT_QUEEN) return;

	go->targetEnemy = nullptr;
	float nearestEnemyDistSq = FLT_MAX;

	int gridX = static_cast<int>(go->pos.x / m_gridSize);
	int gridY = static_cast<int>(go->pos.y / m_gridSize);

	int scanRange = 4;

	for (int dy = -scanRange; dy <= scanRange; ++dy) {
		for (int dx = -scanRange; dx <= scanRange; ++dx) {
			int checkX = gridX + dx; int checkY = gridY + dy;
			if (checkX < 0 || checkX >= m_noGrid || checkY < 0 || checkY >= m_noGrid) continue;
			int cellKey = checkY * m_noGrid + checkX;

			for (GameObject* other : m_spatialGrid[cellKey]) {
				if (!other->active || other == go) continue;
				if (other->teamID != go->teamID && other->teamID >= 0 && go->teamID >= 0) {
					if (other->type == GameObject::GO_FOOD) continue;

					if (CanSee(go, other))
					{
						float distSq = (go->pos - other->pos).LengthSquared();
						if (distSq < nearestEnemyDistSq) {
							nearestEnemyDistSq = distSq;
							go->targetEnemy = other;
						}
					}
				}
			}
		}
	}
}

void SceneSandbox::FindNearestResource(GameObject* go)
{
	go->targetResource.SetZero();
	go->targetFoodItem = nullptr;
	float nearestDistSq = FLT_MAX;

	// Only search for actual Food objects
	for (auto goItem : m_goList) {
		if (!goItem->active || goItem->type != GameObject::GO_FOOD) continue;
		if (goItem->harvesterCount >= 5 || goItem->resourceCount <= 0) continue;
		if (go->type == GameObject::GO_SCOUT && goItem->isMarked) continue;

		float distSq = (go->pos - goItem->pos).LengthSquared();
		if (distSq < nearestDistSq) {
			nearestDistSq = distSq;
			go->targetResource = goItem->pos;
			go->targetFoodItem = goItem;
		}
	}

	// REMOVED: Pheromone trail search logic
}

void SceneSandbox::FindNearestInjuredAlly(GameObject* go)
{
	go->targetAlly = nullptr; float nearestDistSq = FLT_MAX; GameObject* nearestSoldier = nullptr; float nearestSoldierDist = FLT_MAX;
	for (GameObject* other : m_goList) {
		if (!other->active || other == go || other->teamID != go->teamID) continue;
		float distSq = (go->pos - other->pos).LengthSquared();
		if (other->health < other->maxHealth) { if (distSq < nearestDistSq) { nearestDistSq = distSq; go->targetAlly = other; } }
		if (other->type == GameObject::GO_SOLDIER || other->type == GameObject::GO_TANK) { if (distSq < nearestSoldierDist) { nearestSoldierDist = distSq; nearestSoldier = other; } }
	}
	if (go->targetAlly == nullptr && nearestSoldier != nullptr) go->targetAlly = nearestSoldier;
}
void SceneSandbox::GenerateMap()
{
	Math::InitRNG();

	// 1. Initialize Base (Walls and Floor)
	for (int i = 0; i < m_noGrid * m_noGrid; ++i) {
		int roll = Math::RandIntMinMax(0, 100);
		if (roll < 35) m_terrainGrid[i] = TERRAIN_WALL;
		else m_terrainGrid[i] = TERRAIN_FLOOR;
	}

	// 2. Cellular Automata (Smoothing)
	for (int iter = 0; iter < 4; ++iter) {
		std::vector<TERRAIN_TYPE> nextGrid = m_terrainGrid;
		for (int y = 0; y < m_noGrid; ++y) {
			for (int x = 0; x < m_noGrid; ++x) {
				int wallCount = 0;
				for (int dy = -1; dy <= 1; ++dy) {
					for (int dx = -1; dx <= 1; ++dx) {
						if (dx == 0 && dy == 0) continue;
						int nx = x + dx; int ny = y + dy;
						if (IsWithinBoundary(nx) && IsWithinBoundary(ny)) {
							if (m_terrainGrid[Get1DIndex(nx, ny)] == TERRAIN_WALL) wallCount++;
						}
						else { wallCount++; }
					}
				}
				if (wallCount > 4) nextGrid[Get1DIndex(x, y)] = TERRAIN_WALL;
				else if (wallCount < 4) nextGrid[Get1DIndex(x, y)] = TERRAIN_FLOOR;
			}
		}
		m_terrainGrid = nextGrid;
	}

	// 3. Clear Base Locations
	auto ClearArea = [&](int cx, int cy, int rad) {
		for (int y = cy - rad; y <= cy + rad; ++y) {
			for (int x = cx - rad; x <= cx + rad; ++x) {
				if (IsWithinBoundary(x) && IsWithinBoundary(y))
					m_terrainGrid[Get1DIndex(x, y)] = TERRAIN_FLOOR;
			}
		}
		};
	ClearArea(3, 3, 3); // Red Base
	ClearArea(m_noGrid - 4, m_noGrid - 4, 3); // Blue Base

	// 4. Ensure Connectivity (Crucial before adding obstacles)
	EnsureConnectivity();

	// 5. Add Biomes (Forest, Mud, Mountain, Water)
	float seedX = Math::RandFloatMinMax(0.f, 100.f);
	float seedY = Math::RandFloatMinMax(0.f, 100.f);

	for (int y = 0; y < m_noGrid; ++y) {
		for (int x = 0; x < m_noGrid; ++x) {
			int idx = Get1DIndex(x, y);
			if (m_terrainGrid[idx] == TERRAIN_FLOOR) {
				float nx = (x * 0.15f) + seedX;
				float ny = (y * 0.15f) + seedY;
				float noise = sin(nx) + cos(ny) + sin(nx * 0.5f + ny * 0.5f) * 1.5f;

				if (noise < -2.0f) m_terrainGrid[idx] = TERRAIN_WATER;
				else if (noise < -1.0f) m_terrainGrid[idx] = TERRAIN_MUD;
				else if (noise > 1.2f) m_terrainGrid[idx] = TERRAIN_FOREST;
			}
		}
	}

	// 6. Generate Roads (Connects random points to center)
	// Simple Random Walk from center
	int roadAgents = 4;
	for (int i = 0; i < roadAgents; ++i) {
		int cx = m_noGrid / 2;
		int cy = m_noGrid / 2;
		int life = m_noGrid * 1.5f;
		while (life > 0) {
			if (IsWithinBoundary(cx) && IsWithinBoundary(cy)) {
				if (m_terrainGrid[Get1DIndex(cx, cy)] != TERRAIN_WALL &&
					m_terrainGrid[Get1DIndex(cx, cy)] != TERRAIN_WATER) {
					m_terrainGrid[Get1DIndex(cx, cy)] = TERRAIN_ROAD;
				}
			}
			int dir = Math::RandIntMinMax(0, 3);
			if (dir == 0) cx++; else if (dir == 1) cx--; else if (dir == 2) cy++; else cy--;
			life--;
		}
	}

	// 7. Final Polish
	EnforceSymmetry();
	ClearArea(3, 3, 2);
	ClearArea(m_noGrid - 4, m_noGrid - 4, 2);
	EnsureConnectivity(); // Re-verify
}

void SceneSandbox::EnforceSymmetry()
{
	// Diagonal Rotational Symmetry (x,y) -> (max-1-x, max-1-y)
	// We only process the first half of the map + diagonal
	for (int y = 0; y < m_noGrid; ++y) {
		for (int x = 0; x < m_noGrid; ++x) {
			// Define the "source" half? Let's just iterate all and mirror top-left to bottom-right
			if (x + y < m_noGrid) { // Top-Left triangle
				int mirrorX = m_noGrid - 1 - x;
				int mirrorY = m_noGrid - 1 - y;
				m_terrainGrid[Get1DIndex(mirrorX, mirrorY)] = m_terrainGrid[Get1DIndex(x, y)];
			}
		}
	}
}

void SceneSandbox::EnsureConnectivity()
{
	MazePt start(3, 3);
	MazePt end(m_noGrid - 4, m_noGrid - 4);

	auto path = FindPathAStar(start, end, GameObject::GO_WORKER);

	if (path.empty()) {
		int x0 = start.x; int y0 = start.y;
		int x1 = end.x; int y1 = end.y;
		int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = dx + dy, e2;

		while (true) {
			m_terrainGrid[Get1DIndex(x0, y0)] = TERRAIN_FLOOR;
			m_terrainGrid[Get1DIndex(m_noGrid - 1 - x0, m_noGrid - 1 - y0)] = TERRAIN_FLOOR;

			if (x0 == x1 && y0 == y1) break;

			int prevX = x0;
			int prevY = y0;

			e2 = 2 * err;
			if (e2 >= dy) { err += dy; x0 += sx; }
			if (e2 <= dx) { err += dx; y0 += sy; }

			if (x0 != prevX && y0 != prevY) {
				m_terrainGrid[Get1DIndex(x0, prevY)] = TERRAIN_FLOOR;
				m_terrainGrid[Get1DIndex(m_noGrid - 1 - x0, m_noGrid - 1 - prevY)] = TERRAIN_FLOOR;
			}
		}
	}
}

void SceneSandbox::FillDeadZones()
{
	// Flood fill from Red Base. Any non-wall node NOT reached is a dead zone -> turn to wall.
	std::vector<bool> reachable(m_noGrid * m_noGrid, false);
	std::queue<MazePt> q;

	MazePt start(3, 3);
	if (IsWalkable(m_terrainGrid[Get1DIndex(start.x, start.y)])) {
		q.push(start);
		reachable[Get1DIndex(start.x, start.y)] = true;
	}

	int dx[] = { 0, 0, -1, 1 }; int dy[] = { 1, -1, 0, 0 };

	while (!q.empty()) {
		MazePt curr = q.front(); q.pop();
		for (int i = 0; i < 4; ++i) {
			int nx = curr.x + dx[i]; int ny = curr.y + dy[i];
			if (IsWithinBoundary(nx) && IsWithinBoundary(ny)) {
				int idx = Get1DIndex(nx, ny);
				if (!reachable[idx] && IsWalkable(m_terrainGrid[idx])) {
					reachable[idx] = true;
					q.push(MazePt(nx, ny));
				}
			}
		}
	}

	// Fill unreachable
	for (int i = 0; i < m_noGrid * m_noGrid; ++i) {
		if (!reachable[i] && IsWalkable(m_terrainGrid[i])) {
			m_terrainGrid[i] = TERRAIN_WALL;
		}
	}
}
std::vector<MazePt> SceneSandbox::FindPathAStar(MazePt start, MazePt end, GameObject::GAMEOBJECT_TYPE unitType)
{
	std::vector<MazePt> path;

	end = FindNearestWalkableTile(end);

	if (start.x == end.x && start.y == end.y) return path;
	if (!IsWalkable(m_terrainGrid[Get1DIndex(end.x, end.y)])) return path;

	struct Node {
		int index;
		float fCost;
		bool operator>(const Node& other) const { return fCost > other.fCost; }
	};

	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;
	std::vector<float> gScore(m_noGrid * m_noGrid, FLT_MAX);
	std::vector<int> parent(m_noGrid * m_noGrid, -1);

	int startIdx = Get1DIndex(start.x, start.y);
	int endIdx = Get1DIndex(end.x, end.y);

	gScore[startIdx] = 0.f;
	openList.push({ startIdx, 0.f });

	int dx[] = { 0, 0, -1, 1 };
	int dy[] = { 1, -1, 0, 0 };
	bool found = false;

	while (!openList.empty()) {
		Node current = openList.top();
		openList.pop();

		if (current.index == endIdx) {
			found = true;
			break;
		}

		if (current.fCost > gScore[current.index] + 200.0f) continue;

		int cx = current.index % m_noGrid;
		int cy = current.index / m_noGrid;

		for (int i = 0; i < 4; ++i) {
			int nx = cx + dx[i]; int ny = cy + dy[i];
			if (IsWithinBoundary(nx) && IsWithinBoundary(ny)) {
				int nIdx = Get1DIndex(nx, ny);

				float tileCost = GetTileCost(nx, ny, unitType);

				if (tileCost < 100.0f) {
					float newG = gScore[current.index] + tileCost;

					if (newG < gScore[nIdx]) {
						gScore[nIdx] = newG;
						parent[nIdx] = current.index;
						float dx = (float)(end.x - nx);
						float dy = (float)(end.y - ny);
						float h = sqrt(dx * dx + dy * dy);
						openList.push({ nIdx, newG + h });
					}
				}
			}
		}
	}

	if (found) {
		int currIdx = endIdx;
		while (currIdx != startIdx) {
			path.push_back(MazePt(currIdx % m_noGrid, currIdx / m_noGrid));
			currIdx = parent[currIdx];
		}
		std::reverse(path.begin(), path.end());
	}
	return path;
}

std::vector<MazePt> SceneSandbox::FindPathDFS(MazePt start, MazePt end)
{
	std::vector<MazePt> path;

	end = FindNearestWalkableTile(end);

	if (start.x == end.x && start.y == end.y) return path;
	if (!IsWalkable(m_terrainGrid[Get1DIndex(end.x, end.y)])) return path;

	std::stack<int> s;
	std::vector<bool> visited(m_noGrid * m_noGrid, false);
	std::vector<int> parent(m_noGrid * m_noGrid, -1);

	int startIdx = Get1DIndex(start.x, start.y);
	int endIdx = Get1DIndex(end.x, end.y);

	s.push(startIdx);
	visited[startIdx] = true;

	bool found = false;

	int dx[] = { 0, 0, -1, 1 };
	int dy[] = { 1, -1, 0, 0 };

	// Randomize directions for more organic exploration
	int dirOrder[] = { 0, 1, 2, 3 };
	for (int i = 0; i < 4; ++i) {
		int r = Math::RandIntMinMax(0, 3);
		int temp = dirOrder[i];
		dirOrder[i] = dirOrder[r];
		dirOrder[r] = temp;
	}

	while (!s.empty()) {
		int currIdx = s.top();
		s.pop();

		if (currIdx == endIdx) {
			found = true;
			break;
		}

		int cx = currIdx % m_noGrid;
		int cy = currIdx / m_noGrid;

		for (int i = 0; i < 4; ++i) {
			int dir = dirOrder[i];
			int nx = cx + dx[dir];
			int ny = cy + dy[dir];

			if (IsWithinBoundary(nx) && IsWithinBoundary(ny)) {
				int nIdx = Get1DIndex(nx, ny);
				if (!visited[nIdx] && IsWalkable(m_terrainGrid[nIdx])) {
					visited[nIdx] = true;
					parent[nIdx] = currIdx;
					s.push(nIdx);
				}
			}
		}
	}

	if (found) {
		int currIdx = endIdx;
		while (currIdx != startIdx) {
			path.push_back(MazePt(currIdx % m_noGrid, currIdx / m_noGrid));
			currIdx = parent[currIdx];
		}
		std::reverse(path.begin(), path.end());
	}
	return path;
}
MazePt SceneSandbox::FindNearestWalkableTile(MazePt pt)
{
	if (IsWithinBoundary(pt.x) && IsWithinBoundary(pt.y) && IsWalkable(m_terrainGrid[Get1DIndex(pt.x, pt.y)]))
		return pt;

	std::queue<MazePt> q;
	q.push(pt);

	std::vector<bool> visited(m_noGrid * m_noGrid, false);
	if (IsWithinBoundary(pt.x) && IsWithinBoundary(pt.y))
		visited[Get1DIndex(pt.x, pt.y)] = true;

	int dx[] = { 0, 0, -1, 1, -1, 1, -1, 1 };
	int dy[] = { 1, -1, 0, 0, -1, -1, 1, 1 };

	int searchLimit = 0;

	while (!q.empty() && searchLimit < 100)
	{
		MazePt curr = q.front();
		q.pop();
		searchLimit++;

		for (int i = 0; i < 8; ++i)
		{
			int nx = curr.x + dx[i];
			int ny = curr.y + dy[i];

			if (IsWithinBoundary(nx) && IsWithinBoundary(ny))
			{
				int idx = Get1DIndex(nx, ny);
				if (!visited[idx])
				{
					if (IsWalkable(m_terrainGrid[idx]))
						return MazePt(nx, ny);

					visited[idx] = true;
					q.push(MazePt(nx, ny));
				}
			}
		}
	}
	return pt;
}
bool SceneSandbox::IsWalkable(TERRAIN_TYPE type) const {
	return type != TERRAIN_WALL && type != TERRAIN_WATER;
}

bool SceneSandbox::IsInTerritory(Vector3 pos, int teamID) const {
	float halfGrid = m_noGrid * 0.5f;
	if (teamID == 0) return pos.x < halfGrid * m_gridSize && pos.y < halfGrid * m_gridSize;
	else if (teamID == 1) return pos.x > halfGrid * m_gridSize && pos.y > halfGrid * m_gridSize;
	return false;
}
void SceneSandbox::UpdateSpatialGrid() {
	for (auto& cell : m_spatialGrid) cell.clear();
	for (auto go : m_goList) {
		if (!go->active) continue;
		int gridX = (int)(go->pos.x / m_gridSize); int gridY = (int)(go->pos.y / m_gridSize);
		if (IsWithinBoundary(gridX) && IsWithinBoundary(gridY)) {
			int cellKey = gridY * m_noGrid + gridX;
			m_spatialGrid[cellKey].push_back(go);
		}
	}
}

GameObject* SceneSandbox::GetNearestEnemy(Vector3 pos, int teamID, float maxRange) {
	GameObject* nearest = nullptr; float nearestDistSq = maxRange * maxRange;
	for (auto go : m_goList) {
		if (!go->active || go->teamID == teamID || go->type == GameObject::GO_FOOD) continue;
		float distSq = (pos - go->pos).LengthSquared();
		if (distSq < nearestDistSq) { nearestDistSq = distSq; nearest = go; }
	}
	return nearest;
}
int SceneSandbox::IsWithinBoundary(int x) const { return x >= 0 && x < m_noGrid; }
int SceneSandbox::Get1DIndex(int x, int y) const { return y * m_noGrid + x; }

bool SceneSandbox::Handle(Message* message) {
	MessageSpawnUnit* msgSpawn = dynamic_cast<MessageSpawnUnit*>(message);
	if (msgSpawn) {
		// IGNORE PHEROMONE MESSAGES
		if (msgSpawn->type == MessageSpawnUnit::UNIT_PHEROMONE) {
			return true;
		}

		int cost = 0; int currentCount = 0; int limit = 100;
		switch (msgSpawn->type) {
		case MessageSpawnUnit::UNIT_SPEEDY_ANT_WORKER: case MessageSpawnUnit::UNIT_STRONG_ANT_WORKER: cost = 3; limit = 10; currentCount = (msgSpawn->spawner->teamID == 0) ? m_redWorkerCount : m_blueWorkerCount; break;
		case MessageSpawnUnit::UNIT_SCOUT: cost = 4; limit = 2; currentCount = (msgSpawn->spawner->teamID == 0) ? m_redScoutCount : m_blueScoutCount; break;
		case MessageSpawnUnit::UNIT_SPEEDY_ANT_SOLDIER: case MessageSpawnUnit::UNIT_STRONG_ANT_SOLDIER: cost = 5; limit = 15; currentCount = (msgSpawn->spawner->teamID == 0) ? m_redSoldierCount : m_blueSoldierCount; break;
		case MessageSpawnUnit::UNIT_HEALER: cost = 8; limit = 5; currentCount = (msgSpawn->spawner->teamID == 0) ? m_redHealerCount : m_blueHealerCount; break;
		case MessageSpawnUnit::UNIT_TANK: cost = 10; limit = 5; currentCount = (msgSpawn->spawner->teamID == 0) ? m_redTankCount : m_blueTankCount; break;
		}

		if (currentCount >= limit) return true;

		if (msgSpawn->spawner->teamID == 0) {
			if (m_redResources >= cost) { m_redResources -= cost; SpawnUnit(msgSpawn->type, msgSpawn->position, 0); }
		}
		else {
			if (m_blueResources >= cost) { m_blueResources -= cost; SpawnUnit(msgSpawn->type, msgSpawn->position, 1); }
		}
		return true;
	}
	// ... (Keep existing handlers for ResourceDelivered, EnemySpotted, RequestHelp)
	MessageResourceDelivered* msgRes = dynamic_cast<MessageResourceDelivered*>(message); if (msgRes) { if (msgRes->teamID == 0) m_redResources += msgRes->resourceAmount; else m_blueResources += msgRes->resourceAmount; return true; }
	MessageEnemySpotted* msgEnemy = dynamic_cast<MessageEnemySpotted*>(message); if (msgEnemy) { m_coloniesDetected = true; for (size_t i = 0; i < m_goList.size(); ++i) { GameObject* go = m_goList[i]; if (!go->active || go->teamID != msgEnemy->teamID) continue; if ((go->type == GameObject::GO_SOLDIER || go->type == GameObject::GO_STRONG_ANT_SOLDIER) && (go->pos - msgEnemy->enemy->pos).LengthSquared() < m_gridSize * m_gridSize * 16.f) { go->targetEnemy = msgEnemy->enemy; } } return true; }
	MessageRequestHelp* msgHelp = dynamic_cast<MessageRequestHelp*>(message); if (msgHelp) { for (size_t i = 0; i < m_goList.size(); ++i) { GameObject* go = m_goList[i]; if (!go->active || go->teamID != msgHelp->teamID) continue; if ((go->type == GameObject::GO_SOLDIER || go->type == GameObject::GO_STRONG_ANT_SOLDIER) && (go->pos - msgHelp->position).LengthSquared() < m_gridSize * m_gridSize * 16.f) { go->target = msgHelp->position; } } return true; }
	return true;
}

bool SceneSandbox::IsSightBlocker(TERRAIN_TYPE type) const
{
	return (type == TERRAIN_WALL || type == TERRAIN_FOREST);
}

bool SceneSandbox::HasLineOfSight(int x1, int y1, int x2, int y2) const
{
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int sx = (x1 < x2) ? 1 : -1;
	int sy = (y1 < y2) ? 1 : -1;
	int err = dx - dy;

	int cx = x1;
	int cy = y1;

	while (true)
	{
		if (cx == x2 && cy == y2) return true;

		if (cx != x1 || cy != y1)
		{
			if (!IsWithinBoundary(cx) || !IsWithinBoundary(cy)) return false;
			int idx = Get1DIndex(cx, cy);
			if (IsSightBlocker(m_terrainGrid[idx])) return false;
		}

		int e2 = 2 * err;
		if (e2 > -dy)
		{
			err -= dy;
			cx += sx;
		}
		if (e2 < dx)
		{
			err += dx;
			cy += sy;
		}
	}
	return true;
}

bool SceneSandbox::CanSee(GameObject* observer, GameObject* target)
{
	if (!observer || !target) return false;

	int x1 = (int)(observer->pos.x / m_gridSize);
	int y1 = (int)(observer->pos.y / m_gridSize);
	int x2 = (int)(target->pos.x / m_gridSize);
	int y2 = (int)(target->pos.y / m_gridSize);

	if (x1 == x2 && y1 == y2) return true;

	switch (observer->visibilityType)
	{
	case GameObject::VISIBILITY_1_TILE:
	{
		int dx = abs(x1 - x2);
		int dy = abs(y1 - y2);
		if (dx <= 1 && dy <= 1)
		{
			return HasLineOfSight(x1, y1, x2, y2);
		}
		return false;
	}
	case GameObject::VISIBILITY_2_TILE_OMNI:
	{
		int dx = abs(x1 - x2);
		int dy = abs(y1 - y2);
		if (dx <= 2 && dy <= 2)
		{
			return HasLineOfSight(x1, y1, x2, y2);
		}
		return false;
	}
	case GameObject::VISIBILITY_3_TILE_LINEAR:
	{
		int dx = abs(x1 - x2);
		int dy = abs(y1 - y2);

		bool isStraight = (dx == 0) || (dy == 0) || (dx == dy);

		if (isStraight && dx <= 3 && dy <= 3)
		{
			return HasLineOfSight(x1, y1, x2, y2);
		}
		return false;
	}
	}
	return false;
}

void SceneSandbox::UpdateFogOfWar(int teamID)
{
	std::fill(m_fogGrid.begin(), m_fogGrid.end(), true);

	for (GameObject* go : m_goList)
	{
		if (!go->active || go->teamID != teamID) continue;

		int gx = (int)(go->pos.x / m_gridSize);
		int gy = (int)(go->pos.y / m_gridSize);

		int range = 0;
		if (go->visibilityType == GameObject::VISIBILITY_3_TILE_LINEAR) range = 3;
		else if (go->visibilityType == GameObject::VISIBILITY_2_TILE_OMNI) range = 2;
		else range = 1;

		for (int y = gy - range; y <= gy + range; ++y)
		{
			for (int x = gx - range; x <= gx + range; ++x)
			{
				if (IsWithinBoundary(x) && IsWithinBoundary(y))
				{
					int idx = Get1DIndex(x, y);
					if (m_fogGrid[idx])
					{
						GameObject tempTarget;
						tempTarget.pos.Set(x * m_gridSize + m_gridOffset, y * m_gridSize + m_gridOffset, 0);
						if (CanSee(go, &tempTarget))
						{
							m_fogGrid[idx] = false;
						}
					}
				}
			}
		}
	}
}

void SceneSandbox::RenderGO(GameObject* go)
{
	// 1. Move to Object Position
	modelStack.PushMatrix();
	modelStack.Translate(go->pos.x, go->pos.y, 0.1f);

	// 2. Render THE UNIT (with Rotation)
	modelStack.PushMatrix();
	float angle = Math::RadianToDegree(atan2(go->viewDir.y, go->viewDir.x));
	modelStack.Rotate(angle - 90.0f, 0, 0, 1);
	modelStack.Scale(go->scale.x, go->scale.y, go->scale.z);

	switch (go->type)
	{
	case GameObject::GO_WORKER:
		if (go->teamID == 0) RenderMesh(meshList[GEO_WORKER_RED], false);
		else RenderMesh(meshList[GEO_WORKER_BLUE], false);
		break;
	case GameObject::GO_SOLDIER:
		if (go->teamID == 0) RenderMesh(meshList[GEO_SOLDIER_RED], false);
		else RenderMesh(meshList[GEO_SOLDIER_BLUE ], false);
		break;
	case GameObject::GO_QUEEN:
		if (go->teamID == 0) RenderMesh(meshList[GEO_QUEEN_RED], false);
		else RenderMesh(meshList[GEO_QUEEN_BLUE], false);
		break;
	case GameObject::GO_HEALER:
		if (go->teamID == 0)
		RenderMesh(meshList[GEO_HEALER_RED], false);
		else RenderMesh(meshList[GEO_HEALER_BLUE], false);
		break;
	case GameObject::GO_SCOUT:
		if (go->teamID == 0)
			RenderMesh(meshList[GEO_SCOUT_RED], false);
		else RenderMesh(meshList[GEO_SCOUT_BLUE], false);
		break;
	case GameObject::GO_TANK:
		modelStack.Scale(1.2f, 1.2f, 1.f);
		if (go->teamID == 0)
			RenderMesh(meshList[GEO_TANK_RED], false);
		else RenderMesh(meshList[GEO_TANK_BLUE], false);
		break;
	case GameObject::GO_FOOD:
		RenderMesh(meshList[GEO_FOOD], false);
		break;
	}
	modelStack.PopMatrix(); // End Unit Rotation
	//Health bar
	if (go->type != GameObject::GO_FOOD && go->health < go->maxHealth)
	{
		float healthPercent = go->health / go->maxHealth;
		modelStack.PushMatrix();
		modelStack.Translate(0, m_gridSize * 0.7f, 0.1f);
		modelStack.Scale(healthPercent * m_gridSize, m_gridSize * 0.15f, 1.f);

		if (healthPercent > 0.5f) RenderMesh(meshList[GEO_HPBAR_GREEN], false);
		else RenderMesh(meshList[GEO_HPBAR_RED], false);

		modelStack.PopMatrix();
	}
	// --- RENDER FOOD RESOURCE COUNT ---
	if (go->type == GameObject::GO_FOOD)
	{
		std::ostringstream ss;
		ss << go->resourceCount;

		// Render Text slightly above food
		modelStack.PushMatrix();
		modelStack.Translate(0.f, 0.f, 0.f);
		modelStack.Scale(m_gridSize, m_gridSize, 1.f); // Scale text to grid size
		RenderText(meshList[GEO_TEXT], ss.str(), Color(0, 0, 0));
		modelStack.PopMatrix();
	}

	modelStack.PopMatrix(); // End Object Position
}

void SceneSandbox::Render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Projection matrix
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
	modelStack.LoadIdentity();

	// Render background
	modelStack.PushMatrix();
	modelStack.Translate(m_worldHeight * 0.5f, m_worldHeight * 0.5f, -1.f);
	modelStack.Scale(m_worldHeight, m_worldHeight, m_worldHeight);
	RenderMesh(meshList[GEO_GRASS], false);
	modelStack.PopMatrix();

	//Render terrain
	for (int row = 0; row < m_noGrid; ++row)
	{
		for (int col = 0; col < m_noGrid; ++col)
		{
			TERRAIN_TYPE type = m_terrainGrid[Get1DIndex(col, row)];

			if (type == TERRAIN_FLOOR) continue; // Skip floor (show background)

			modelStack.PushMatrix();
			modelStack.Translate(col * m_gridSize + m_gridOffset, row * m_gridSize + m_gridOffset, -0.9f);
			modelStack.Scale(m_gridSize, m_gridSize, 1.f);

			if (type == TERRAIN_WALL) {
				modelStack.Translate(0, 0, 1.0f); // Pop up
				RenderMesh(meshList[GEO_WALL], true);
			}
			else if (type == TERRAIN_MUD) {
				RenderMesh(meshList[GEO_MUD], true);
			}
			else if (type == TERRAIN_WATER) {
				RenderMesh(meshList[GEO_WATER], true);
			}
			else if (type == TERRAIN_FOREST) {
				RenderMesh(meshList[GEO_FOREST], true);
			}
			else if (type == TERRAIN_ROAD) {
				RenderMesh(meshList[GEO_ROAD], true);
			}

			modelStack.PopMatrix();
		}
	}

	if (m_renderFog)
	{
		for (int row = 0; row < m_noGrid; ++row)
		{
			for (int col = 0; col < m_noGrid; ++col)
			{
				if (m_fogGrid[Get1DIndex(col, row)])
				{
					modelStack.PushMatrix();
					modelStack.Translate(col * m_gridSize + m_gridOffset, row * m_gridSize + m_gridOffset, 2.0f);
					modelStack.Scale(m_gridSize, m_gridSize, 1.f);
					meshList[GEO_WHITEQUAD]->material.kAmbient.Set(0.1f, 0.1f, 0.1f);
					RenderMesh(meshList[GEO_WHITEQUAD], true);
					meshList[GEO_WHITEQUAD]->material.kAmbient.Set(1.f, 1.f, 1.f);
					modelStack.PopMatrix();
				}
			}
		}
	}

	// Render territory markers
	float territorySize = m_gridSize * 8.f;

	// Speedy Ant Territory (Bottom-Left: 0 to 8)
	// Center = 4.0 * gridSize
	meshList[GEO_WHITEQUAD]->material.kAmbient.Set(0.8f, 0.2f, 0.2f); // RED
	modelStack.PushMatrix();
	modelStack.Translate(m_gridSize * 4.0f, m_gridSize * 4.0f, -0.8f);
	modelStack.Scale(territorySize, territorySize, 1.f);
	RenderMesh(meshList[GEO_TERRITORYRED], true);
	modelStack.PopMatrix();

	// Strong Ant Territory (Top-Right: 22 to 30)
	// Center = 26.0 * gridSize
	meshList[GEO_WHITEQUAD]->material.kAmbient.Set(0.2f, 0.2f, 0.8f); // BLUE
	modelStack.PushMatrix();
	modelStack.Translate(m_gridSize * 26.0f, m_gridSize * 26.0f, -0.8f);
	modelStack.Scale(territorySize, territorySize, 1.f);
	RenderMesh(meshList[GEO_TERRITORYBLUE], true);
	modelStack.PopMatrix();

	// Reset to White for other objects using this mesh
	meshList[GEO_WHITEQUAD]->material.kAmbient.Set(1.f, 1.f, 1.f);

	// --- STACKING LOGIC ---
	// Map: CellIndex -> GameObjectType -> Count
	std::map<int, std::map<int, int>> cellCounts;

	// Pass 1: Count objects per cell
	for (auto go : m_goList)
	{
		if (!go->active) continue;
		int gx = (int)(go->pos.x / m_gridSize);
		int gy = (int)(go->pos.y / m_gridSize);
		// Safety clamp
		if (gx < 0) gx = 0; if (gx >= m_noGrid) gx = m_noGrid - 1;
		if (gy < 0) gy = 0; if (gy >= m_noGrid) gy = m_noGrid - 1;

		int idx = gy * m_noGrid + gx;
		cellCounts[idx][go->type]++;
	}

	// Pass 2: Render unique objects with counts
	for (std::vector<GameObject*>::iterator it = m_goList.begin(); it != m_goList.end(); ++it)
	{
		GameObject* go = (GameObject*)*it;
		if (go->active)
		{
			int gx = (int)(go->pos.x / m_gridSize);
			int gy = (int)(go->pos.y / m_gridSize);
			if (gx < 0) gx = 0; if (gx >= m_noGrid) gx = m_noGrid - 1;
			if (gy < 0) gy = 0; if (gy >= m_noGrid) gy = m_noGrid - 1;
			int idx = gy * m_noGrid + gx;

			// Check the count for this specific type in this cell
			int count = cellCounts[idx][go->type];

			// If count > 0, it means we haven't rendered this type for this cell yet
			if (count > 0)
			{
				RenderGO(go);

				// If there is more than 1, draw the count text
				if (count > 1)
				{
					std::ostringstream ss;
					ss << count; // e.g. "3"

					modelStack.PushMatrix();
					// Position text slightly offset from the unit center (top-right)
					modelStack.Translate(go->pos.x + m_gridSize * 0.5f, go->pos.y + m_gridSize * 0.2f, 0.2f);
					// Scale text appropriate to grid size
					modelStack.Scale(m_gridSize*2.f, m_gridSize*2.f, 1.f);
					RenderText(meshList[GEO_TEXT], ss.str(), Color(1, 1, 1)); // White text
					modelStack.PopMatrix();
				}

				// Set count to 0 so we don't render this type for this cell again this frame
				cellCounts[idx][go->type] = 0;
			}
			// If count was 0, we skip RenderGO (this unit is "hidden" inside the stack)
		}
	}

	// Render all game objects
	for (std::vector<GameObject*>::iterator it = m_goList.begin(); it != m_goList.end(); ++it)
	{
		GameObject* go = (GameObject*)*it;
		if (go->active)
		{
			RenderGO(go);
		}
	}

	// On screen text
	std::ostringstream ss;
	ss.precision(3);

	// Stats Column
	float colX = 50.f;

	// Simulation Stats
	ss.str(""); ss.precision(5);
	ss << "FPS:" << fps;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 68.f, 54);

	float uiX = 50.f; // Adjusted to be on the right side
	float uiY = 56.f;
	float spacing = 2.5f;

	ss.str(""); ss << "=== Turn-based ===";
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 1, 1), 3.0f, uiX, uiY); uiY -= spacing;

	if (m_currentEvent != EVENT_NONE) {
		std::string eventName = "NORMAL";
		Color eventColor(1, 1, 1);
		if (m_currentEvent == EVENT_HEAVY_RAIN) { eventName = "HEAVY RAIN"; eventColor.Set(0.5f, 0.5f, 1.0f); }
		else if (m_currentEvent == EVENT_SUDDEN_DEATH) { eventName = "SUDDEN DEATH"; eventColor.Set(1.0f, 0.0f, 0.0f); }

		ss.str(""); ss << "EVENT: " << eventName;
		if (m_eventDuration > 0) ss << " (" << m_eventDuration << ")";
		RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), eventColor, 3.0f, uiX, 8); uiY -= spacing;
	}

	// Mode Display
	ss.str(""); ss << "Mode: " << (m_autoTurn ? "AUTO" : "MANUAL");
	Color modeColor = m_autoTurn ? Color(0, 1, 0) : Color(1, 1, 0); // Green for Auto, Yellow for Manual
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), modeColor, 3.0f, uiX, uiY); uiY -= spacing;

	// Turn Info
	ss.str(""); ss << "Turn: " << m_turnNumber;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 1, 1), 3.0f, uiX, uiY); uiY -= spacing;

	// Instructions
	if (!m_autoTurn) {
		ss.str(""); ss << "Action: Press [SPACE]";
		RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.5f, 0), 2.5f, uiX, uiY); uiY -= spacing;
	}
	else {
		ss.str(""); ss << "Status: Running...";
		RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.5f, 1, 0.5f), 2.5f, uiX, uiY); uiY -= spacing;
	}

	uiY -= spacing;
	ss.str(""); ss << "[T] Toggle Mode";
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.8f, 0.8f, 0.8f), 2.0f, uiX+12.f, uiY); uiY -= spacing;

	// --- NEW: INSTRUCTION FOR FOG TOGGLE ---
	ss.str(""); ss << "[M] Toggle Map View";
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.8f, 0.8f, 0.8f), 2.0f, uiX + 12.f, uiY); uiY -= spacing;
	// --------------------------
	ss.str(""); ss << "[L] Switch Fog Team";
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.8f, 0.8f, 0.8f), 2.0f, uiX + 12.f, uiY); uiY -= spacing;

	uiY -= spacing;

	// --- RED ANT COLONY (Team 0) ---
	ss.str(""); ss << "=== RED ANT COLONY ===";
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.3f, 0.3f), 2.5f, colX, 45);

	ss.str(""); ss << "Workers: " << m_redWorkerCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.3f, 0.3f), 2.0f, colX, 42);
	ss.str(""); ss << "Soldiers: " << m_redSoldierCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.3f, 0.3f), 2.0f, colX, 40);
	ss.str(""); ss << "Healers: " << m_redHealerCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.3f, 0.3f), 2.0f, colX, 38);
	ss.str(""); ss << "Scouts:  " << m_redScoutCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.3f, 0.3f), 2.0f, colX, 36);
	ss.str(""); ss << "Tanks:   " << m_redTankCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.3f, 0.3f), 2.0f, colX, 34);
	ss.str(""); ss << "Food:    " << m_redResources;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.3f, 0.3f), 2.0f, colX, 32);
	ss.str(""); ss << "Queen HP:" << (m_redQueen->active ? (int)m_redQueen->health : 0);
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 0.3f, 0.3f), 2.0f, colX, 30);

	// --- BLUE ANT COLONY (Team 1) ---
	ss.str(""); ss << "=== BLUE ANT COLONY ===";
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.3f, 0.3f, 1), 2.5f, colX, 25);

	ss.str(""); ss << "Workers: " << m_blueWorkerCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.3f, 0.3f, 1), 2.0f, colX, 22);
	ss.str(""); ss << "Soldiers: " << m_blueSoldierCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.3f, 0.3f, 1), 2.0f, colX, 20);
	ss.str(""); ss << "Healers: " << m_blueHealerCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.3f, 0.3f, 1), 2.0f, colX, 18);
	ss.str(""); ss << "Scouts:  " << m_blueScoutCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.3f, 0.3f, 1), 2.0f, colX, 16);
	ss.str(""); ss << "Tanks:   " << m_blueTankCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.3f, 0.3f, 1), 2.0f, colX, 14);
	ss.str(""); ss << "Food:    " << m_blueResources;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.3f, 0.3f, 1), 2.0f, colX, 12);
	ss.str(""); ss << "Queen HP:" << (m_blueQueen->active ? (int)m_blueQueen->health : 0);
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0.3f, 0.3f, 1), 2.0f, colX, 10);

	if (m_simulationEnded)
	{
		ss.str(""); ss << "WINNER: " << (m_winner == 0 ? "RED COLONY" : m_winner == 1 ? "BLUE COLONY" : "DRAW");
		RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(1, 1, 1), 3.f, 20, 30);
	}
}
void SceneSandbox::Exit()
{
	SceneBase::Exit();
	while (m_goList.size() > 0) { GameObject* go = m_goList.back(); if (go->sm) delete go->sm; delete go; m_goList.pop_back(); }
	m_spatialGrid.clear(); m_foodLocations.clear(); m_terrainGrid.clear();
}