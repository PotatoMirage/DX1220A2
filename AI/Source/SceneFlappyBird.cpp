#include "SceneFlappyBird.h"
#include "GL\glew.h"
#include "Application.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

//#define _ORIGINAL 0

//CAN WE TRAIN A BOT TO PLAY FLAPPY BIRD?
//Some things to explore on your own:
//1) modify the inputs to include distance to the next-next pipe so that bot can be trained to react to them
//2) add an additional hidden layer and see if that yields better results


static std::vector<GameObject*> birds; //array of bots. they will start off super dumb. note that this container is file-scoped

constexpr int INPUTSIZE = 4;  //4 input neurons
constexpr int HIDDENSIZE = 4; //4 neurons in the hidden layer. just an arbitrary number of neurons. experiment with this number!
                              //we also only have 1 hidden layer. this game is a simple enough use case that 1 layer is likely enough(?)

static float operator*(const std::vector<float>& lhs, const std::vector<float>& rhs)
{
	float total = 0;
	size_t size = lhs.size();
	for (size_t i = 0; i < size; ++i)
	{
		total += lhs[i] * rhs[i];
	}

	return total;
}

bool AABBvsCIRCLE(AABB aabb, Vector3 pos, float radius)
{
	float rSQ = radius * radius;

	float DeltaX = pos.x - Math::Max(aabb.position.x - aabb.width / 2, Math::Min(pos.x, aabb.position.x + aabb.width / 2));
	float DeltaY = pos.y - Math::Max(aabb.position.y - aabb.height / 2, Math::Min(pos.y, aabb.position.y + aabb.height / 2));
	float result = (DeltaX * DeltaX + DeltaY * DeltaY);
	return result < rSQ;
}

SceneFlappyBird::SceneFlappyBird()
	: m_speed(0.0f)
	, m_worldWidth(0.0f)
	, m_worldHeight(0.0f)
	, m_objectCount(0)
	, m_liveObjectCount(0)
	, m_noGrid(0)
	, m_gridSize(0.0f)
	, m_gridOffset(0.0f)
	, m_jumpForce(0.0f)
	, m_characterRadius(0.0f)
	, m_pipeScaleX(0.0f)
	, m_pipeScaleY(0.0f)
	, m_pipeGapMin(0.0f)
	, m_pipeGapMax(0.0f)
	, m_gravity(0.0f)
	, m_gameSpeed(0.0f)
	, m_spawnTimer(0.0f)
	, m_spawnTime(0.0f)
	, m_spawnReduceTime(0.0f)
	, m_spawnReducerTimer(0.0f)
	, m_highScore(0)
	, m_lastScore(0)
	, m_score(0)
	, generation(0)
	, iRandSeed(0)
	, bResetiRandSeed(false)
{
}

SceneFlappyBird::~SceneFlappyBird()
{
}

void SceneFlappyBird::Init()
{
	SceneBase::Init();

	//Calculating aspect ratio
	m_worldHeight = 100.f;
	m_worldWidth = m_worldHeight * (float)Application::GetWindowWidth() / Application::GetWindowHeight();

	//Physics code here
	m_speed = 1.f;

	iRandSeed = (unsigned int)time(0);

	Math::InitRNG(iRandSeed);

	m_objectCount = 0;
	m_liveObjectCount = 0;

	m_characterRadius = 5.0f;
	m_gravity = -57.6f;
	m_gameSpeed = 25.0f;

	m_spawnTime = 2.25f;
	m_spawnTimer = 0;
	m_spawnReduceTime = 4.0f;
	m_spawnReducerTimer = 0.0;

	m_pipeScaleX = 10;
	m_pipeScaleY = 55;

	//m_pipeGapMin = 37.5f;
	//m_pipeGapMax = 55.0f;
	m_pipeGapMin = 45.0f;
	m_pipeGapMax = 60.0f;

	m_jumpForce = 30;
	m_score = 0;
	m_highScore = 0;
	m_lastScore = 0;
	generation = 1;

	birds.clear();
	//make iNumBirds bots
	for (int i = 0; i < iNumBirds; ++i)
	{
		GameObject* go = FetchGO(GameObject::GO_BIRD);
		go->pos.x = m_worldWidth * 0.15f;
		go->pos.y = m_worldHeight * 0.5f;
		go->vel.SetZero();
		go->score = 0;
		go->alive = true;

		//setup nodes & weights
		RandomizeWeight(go);
		birds.push_back(go);
	}
}

GameObject* SceneFlappyBird::FetchGO(GameObject::GAMEOBJECT_TYPE type)
{
	for (auto go : m_goList)
	{
		//GameObject* go = (GameObject*)*it;
		if (!go->active && type == go->type)
		{
			go->active = true;
			++m_objectCount;
			return go;
		}
	}
	for (unsigned i = 0; i < 1; ++i)
	{
		GameObject* go = new GameObject(type);
		go->id = m_goList.size();
		m_goList.push_back(go);
	}
	return FetchGO(type);
}

//this transforms the signal to a range spanning 0 to 1. 
//large negative values get mapped to 0 and large position values get mapped to 1
float SceneFlappyBird::Sigmoid(float x)
{
	return 1.f / (1.f + exp(-x));
}

//not implementing this
float SceneFlappyBird::Derivative(float x)
{
	return 0.f;
}

void SceneFlappyBird::RandomizeWeight(GameObject* go)
{
	go->hiddenNode.resize(HIDDENSIZE);
	for (size_t i = 0; i < go->hiddenNode.size(); ++i)
	{
		//+1 for the additional bias. we'll apparently just have our bias be between fMinBias to fMaxBias
		//(could potentially try a different range of values for the bias)
		go->hiddenNode[i].weights.resize(INPUTSIZE + 1);
		for (size_t j = 0; j < INPUTSIZE + 1; ++j)
		{
			go->hiddenNode[i].weights[j] = Math::RandFloatMinMax(-1.f, 1.f);
		}

		//randomize bias
		go->hiddenNode[i].weights[INPUTSIZE] = Math::RandFloatMinMax(fMinBias, fMaxBias);
	}

	//Note: we only have 1 output node - acting as a jump button
	//+1 for the additional bias. we'll apparently just have our initial bias be between fMinBias to fMaxBias
	//(could potentially try a different range of values for the bias)
	go->outputNode.weights.resize(go->hiddenNode.size() + 1);
	for (size_t j = 0; j < go->outputNode.weights.size(); ++j)
	{
		go->outputNode.weights[j] = Math::RandFloatMinMax(-1.f, 1.f);
	}
	//randomize bias
	go->outputNode.weights[go->outputNode.weights.size() - 1] = Math::RandFloatMinMax(fMinBias, fMaxBias);
}

Pipes* SceneFlappyBird::FetchPipe()
{
	for (std::vector<Pipes*>::iterator it = m_pipesList.begin(); it != m_pipesList.end(); ++it)
	{
		Pipes* p = (Pipes*)*it;
		if (!p->m_active)
		{
			p->m_active = true;
			p->m_gap = Math::RandFloatMinMax(m_pipeGapMin, m_pipeGapMax);

			p->m_top.active = true;
			p->m_bottom.active = true;
			p->m_position.x = m_worldWidth * 0.75f;
			p->m_position.y = Math::RandFloatMinMax(m_worldHeight * 0.35f, m_worldHeight * 0.65f);

			p->m_aabbTop.Set(m_pipeScaleX - 2.0f, m_pipeScaleY - 1.0f, Vector3(p->m_position.x, p->m_position.y + p->m_gap, p->m_position.z));
			p->m_aabbBot.Set(m_pipeScaleX - 2.0f, m_pipeScaleY - 1.0f, Vector3(p->m_position.x, p->m_position.y - p->m_gap, p->m_position.z));
			return p;
		}
	}
	for (unsigned i = 0; i < 10; ++i)
	{
		Pipes* pipe = new Pipes();
		pipe->m_active = false;
		pipe->m_top.type = GameObject::GO_PIPE;
		pipe->m_bottom.type = GameObject::GO_PIPE;
		pipe->m_top.active = false;
		pipe->m_bottom.active = false;

		m_pipesList.push_back(pipe);
	}
	return FetchPipe();
}

void SceneFlappyBird::UpdatePipes(double dt)
{
	for (std::vector<Pipes*>::iterator it = m_pipesList.begin(); it != m_pipesList.end(); ++it)
	{
		Pipes* p = (Pipes*)*it;
		if (p->m_active)
		{
			float value = (float)(m_gameSpeed * m_speed * dt);
			p->m_position.x -= value;
			p->m_aabbTop.Update(Vector3(-value, 0, 0));
			p->m_aabbBot.Update(Vector3(-value, 0, 0));

			//Check if it should die by the X
			if (p->m_position.x < -m_pipeScaleX / 2.0f)
			{
				p->m_active = false;
			}
		}
	}

	//Collision Check
	for (std::vector<Pipes*>::iterator it = m_pipesList.begin(); it != m_pipesList.end(); ++it)
	{
		Pipes* p = (Pipes*)*it;
		if (p->m_active)
		{
			for (auto go : m_goList)
			{
				if (go->alive)
				{
					if (AABBvsCIRCLE(p->m_aabbTop, go->pos, m_characterRadius))
					{
						go->alive = false;
					}

					if (AABBvsCIRCLE(p->m_aabbBot, go->pos, m_characterRadius))
					{
						go->alive = false;
					}
				}
			}
		}
	}
}

Pipes* SceneFlappyBird::GetNextPipe(float x)
{
	Pipes* nextP = NULL;
	float nearestX = FLT_MAX;
	for (auto p : m_pipesList)
	{
		if (p->m_active && p->m_position.x > x)
		{
			if (p->m_position.x < nearestX)
			{
				nearestX = p->m_position.x;
				nextP = p;
			}
		}
	}
	return nextP;
}

void SceneFlappyBird::UpdateCharacter(GameObject* go, double dt)
{
	if (go->alive)
	{
		go->score = (float)m_score;

		go->vel.y += m_gravity * m_speed * static_cast<float>(dt);
		go->pos.y += go->vel.y * m_speed * static_cast<float>(dt);

		//Check if character hit limit
		if (go->pos.y < 0)
		{
			go->alive = false;
		}
		else if (go->pos.y > m_worldHeight)
		{
			go->alive = false;
		}
	}
}

void SceneFlappyBird::Update(double dt)
{
	//this makes the game's speed framerate dependant, 
	//but we're not concerned about this for the purpose of training weights
	dt = 1 / 60.0; //assume fixed time step of 1/60s

	//Calculating aspect ratio
	m_worldHeight = 100.f;
	m_worldWidth = m_worldHeight * (float)Application::GetWindowWidth() / Application::GetWindowHeight();

	if (Application::IsKeyPressed(VK_RETURN))
	{
		bResetiRandSeed = true;
	}
	if (Application::IsKeyPressed(VK_OEM_MINUS))
	{
		//m_speed = Math::Max(0.f, m_speed - 0.1f);
		--iSimulationSpeed;
		iSimulationSpeed = max(0, iSimulationSpeed);
	}
	if (Application::IsKeyPressed(VK_OEM_PLUS))
	{
		++iSimulationSpeed;
		//m_speed += 0.1f;
	}
	//SPACE to JUMP
	static bool bSpaceState = false;
	if ((!bSpaceState && Application::IsKeyPressed(VK_SPACE)))
	{
		bSpaceState = true;
		if (m_goList.size() > 0)
			m_goList[0]->vel.Set(0, m_jumpForce, 0);
	}
	else if (bSpaceState && !Application::IsKeyPressed(VK_SPACE))
	{
		bSpaceState = false;
	}

	//Input Section
	static bool bLButtonState = false;
	if (!bLButtonState && Application::IsMousePressed(0))
	{
		bLButtonState = true;
		std::cout << "LBUTTON DOWN" << std::endl;
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

	int framesToSimulate = static_cast<int>(std::ceil(iSimulationSpeed));
	while (framesToSimulate > 0)
	{
		--framesToSimulate;
		SceneBase::Update(dt);
		
		//To control the time between each pipe spawn
		m_spawnReducerTimer += (float)(dt * m_speed);
		if (m_spawnReducerTimer >= m_spawnReduceTime)
		{
			m_spawnReducerTimer = 0;
			m_spawnTime -= 0.1f;
			m_spawnTime = Math::Max(0.75f, m_spawnTime);
		}

		//To control the spawning of pipes
		m_spawnTimer += (float)(dt * m_speed);
		if (m_spawnTimer >= m_spawnTime)
		{
			m_spawnTimer = 0;
			FetchPipe();
		}

		//Update the pipes
		UpdatePipes(dt);

		m_liveObjectCount = 0;
		for (auto go : m_goList)
		{
			if (go->alive)
			{
				Pipes* p = GetNextPipe(go->pos.x); //next pipe
				Pipes* p2 = p != nullptr ? GetNextPipe(p->m_position.x) : nullptr; //next-next pipe

				//horizontal distance to next pipe
				float i0 = p != nullptr ? p->m_position.x - go->pos.x : m_worldWidth;
				//vertical distance to next pipes(mid-point betw 2 pipes)
				float i1 = p != nullptr ? p->m_position.y - go->pos.y : m_worldHeight * 0.5f - go->pos.y; //if no pipe available, use screen center
				//bot's velocity
				float i2 = go->vel.y;
				//size of gap of next pipe
				float i3 = p != nullptr ? p->m_gap : m_worldHeight;

				//NOTE: these 2 inputs are commented because the bots seems to behave very inconsistently with these 2 inputs.
				//      perhaps 2 hidden layers of neurons are needed (instead of 1) to support these?
				//horizontal distance to next-next pipe
				//float i4 = p2 != nullptr ? p2->m_position.x - go->pos.x : m_worldWidth;
				//vertical distance to next-next pipe
				//float i5 = p2 != nullptr ? p2->m_position.y - go->pos.y : m_worldHeight * 0.5f - go->pos.y; //if no pipe available, use screen center

				std::vector<float> inputList{ i0, i1, i2, i3, 1.f }; //1 is added for the bias (1 * weight) *the weight acts as the bias*
				float output = FeedNN(go, inputList);
				if (output > 0.5f) //if the signal is >0.5, we perform a jump
				{
					go->vel.Set(0, m_jumpForce, 0);
				}
				UpdateCharacter(go, dt);

				m_liveObjectCount++;
			}
		}
		//Update score & hiscore
		m_score += (int)(100.0f * dt * m_speed);
		if (m_highScore < m_score)
		{
			m_highScore = m_score;
		}
		bool allDead = true;
		for (auto go : m_goList)
		{
			if (go->alive)
			{
				allDead = false;
				break;
			}
		}
		if (allDead)
		{
			m_lastScore = m_score;
			Restart();
		}
	}
}

float SceneFlappyBird::FeedNN(GameObject* go, std::vector<float>& inputList)
{
	//layer 1 - input neurons
	for (NNode& node : go->hiddenNode)
	{
		node.z = node.weights * inputList; //dot product of the weights and the corresponding inputs + bias
		node.output = Sigmoid(node.z);     //map the result to (0, 1)
	}

	//layer 2 - hidden layer
	std::vector<float> input2;
	for (int i = 0; i < (int)go->hiddenNode.size(); ++i) //hidden layer is input to the output node
		input2.push_back(go->hiddenNode[i].output);
	input2.push_back(1.f); //1 added for bias
	go->outputNode.z = go->outputNode.weights * input2;
	go->outputNode.output = Sigmoid(go->outputNode.z);

	return go->outputNode.output;
}

void SceneFlappyBird::Restart()
{
	if (bResetiRandSeed == true)
	{
		iRandSeed = (unsigned int)time(0);
		bResetiRandSeed = false;
	}

	Math::InitRNG(iRandSeed);

	m_score = 0;
	m_pipesList.clear();

	//Reset Spawn Timings
	m_spawnTime = 2.25f;
	m_spawnTimer = 0;
	m_spawnReduceTime = 4.0f;
	m_spawnReducerTimer = 0.0;

	//code here assumes there are more than 1 bird

	//Feel free to experiment with how you want to train the next generation of flappy birds
	int birdCount = static_cast<int>(birds.size());
	int halfBirds = birdCount / 2;
	std::sort(birds.begin(), birds.end(), [](GameObject* lhs, GameObject* rhs) { return lhs->score < rhs->score; }); //sort the birds by their score
	GameObject* strongest = *(birds.end() - 1); //after sorting, the last bird is the "strongest"


#ifdef _ORIGINAL
	//NOTE: Mutate 50% bottom birds - reroll dice.

	//handle bottom 50%
	for (int i = 0; i < halfBirds; ++i)
	{
		//completely randomize the weights of the weaker half of the population.
		//that means half the population is discarded. we may end up with something as stupid, or we may have some better performing values
		RandomizeWeight(birds[i]);
	}
	//Copy from strongest bird to weakest bird, then mutate it a bit
	birds[halfBirds]->hiddenNode = strongest->hiddenNode;
	birds[halfBirds]->outputNode = strongest->outputNode;

	//Mutate rest a little
	float threshold = 0.05f; //we shall tweak the weights by this amount
	for (int i = halfBirds; i < birdCount - 2; ++i) //exclude strongest bird in the mutation. keep it as it is.
	{
		for (NNode& node : birds[i]->hiddenNode)
		{
			for (float& f : node.weights)
				f += Math::RandFloatMinMax(-threshold, threshold);
		}

		for (float& f : birds[i]->outputNode.weights)
			f += Math::RandFloatMinMax(-threshold, threshold);
	}
#else
	//completely reroll the weights for the last quarter of population
	for (int i = 0; i < halfBirds / 2; ++i)
	{
		//completely randomize the weights of the weaker half of the population.
		//that means half the population is discarded. we may end up with something as stupid, or we may have some better performing values
		RandomizeWeight(birds[i]);
	}
	//copy strongest bird to the next quarter
	for (int i = halfBirds / 2; i < halfBirds; ++i)
	{
		////Copy from strongest bird to this bird, then mutate it a bit
		birds[i]->hiddenNode = strongest->hiddenNode;
		birds[i]->outputNode = strongest->outputNode;

		//Copy from strongest quarter to this quarter
		//birds[i]->hiddenNode = birds[birdCount - i]->hiddenNode;
		//birds[i]->outputNode = birds[birdCount - i]->outputNode;
	}

	//Mutate rest a little
	float threshold = 0.15f; //we shall tweak the weights by this amount
	//float threshold = 0.025f; //we shall tweak the weights by this amount
	for (int i = halfBirds / 2; i < birdCount - iNumBirdsToKeep; ++i) //exclude strongest bird in the mutation. keep it as it is.
	{
		for (NNode& node : birds[i]->hiddenNode)
		{
			for (float& f : node.weights)
				f += Math::RandFloatMinMax(-threshold, threshold);
		}

		for (float& f : birds[i]->outputNode.weights)
			f += Math::RandFloatMinMax(-threshold, threshold);
	}
#endif

	//Reset the birds 
	++generation;
	for (auto go : m_goList)
	{
		go->pos.y = m_worldHeight * 0.5f + Math::RandFloatMinMax(-10.f, 10.f);
		go->vel.SetZero();
		go->alive = true;
		go->score = 0;
	}
}

void SceneFlappyBird::RenderGO(GameObject* go)
{
	switch (go->type)
	{
	case GameObject::GO_BIRD:
		modelStack.PushMatrix();
		modelStack.Translate(go->pos.x, go->pos.y, 0.01f * go->id);
		modelStack.Scale(m_characterRadius, m_characterRadius, 1);
			modelStack.PushMatrix();
			modelStack.Translate(0.0f, 0.8f, 0.0f);
			std::ostringstream ss;
			ss << go->id;
			RenderText(meshList[GEO_TEXT], ss.str(), Color(0, 0, 0));
			modelStack.PopMatrix();
		RenderMesh(meshList[GEO_CHARACTER], false);
		modelStack.PopMatrix();
		break;
	}
}

void SceneFlappyBird::RenderPipes()
{
	for (int i = 0; i < (int)m_pipesList.size(); ++i)
	{
		Pipes* p = m_pipesList[i];
		if (p->m_active)
		{
			modelStack.PushMatrix();
			modelStack.Translate(p->m_position.x, p->m_position.y + p->m_gap, 0.1f);
			modelStack.Rotate(180, 0, 0, 1);
			modelStack.Scale(m_pipeScaleX, m_pipeScaleY, 1);
			RenderMesh(meshList[GEO_PIPE], false);
			modelStack.PopMatrix();

			modelStack.PushMatrix();
			modelStack.Translate(p->m_position.x, p->m_position.y - p->m_gap, 0.1f);
			modelStack.Scale(m_pipeScaleX, m_pipeScaleY, 1);
			RenderMesh(meshList[GEO_PIPE], false);
			modelStack.PopMatrix();

			//modelStack.PushMatrix();
			//modelStack.Translate(p->m_position.x, p->m_position.y, 0.1f);
			//modelStack.Scale(10, 10, 10);
			//std::ostringstream ss;
			//ss << i;
			//RenderText(meshList[GEO_TEXT], ss.str(), Color(0, 0, 0));
			//modelStack.PopMatrix();
		}
	}
}

void SceneFlappyBird::Render()
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
	modelStack.Translate(m_worldWidth * 0.725f * 0.5f, m_worldHeight * 0.5f, -1.f);
	modelStack.Scale(m_worldWidth * 0.725f, m_worldHeight, m_worldHeight);
	RenderMesh(meshList[GEO_FLAPPYBG], false);
	modelStack.PopMatrix();

	modelStack.PushMatrix();
	modelStack.Translate(m_worldWidth * 0.725f + m_worldWidth * 0.375f / 2.0f, m_worldHeight * 0.5f, 10.f);
	modelStack.Scale(m_worldWidth * 0.375f, m_worldHeight, m_worldHeight);
	RenderMesh(meshList[GEO_SIDEBAR], false);
	modelStack.PopMatrix();

	RenderPipes();

	for (auto go : m_goList)
	{
		if (go->alive)
		{
			RenderGO(go);
		}
	}

	//On screen text
	std::ostringstream ss;

	ss.str("");
	ss.precision(5);
	ss << "FPS:" << fps;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 60, 0);

	ss.str("");
	ss.precision(3);
	ss << "Speed:" << iSimulationSpeed;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 60, 3);

	ss.str("");
	ss.precision(5);
	ss << "High Score:" << m_highScore;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 60, 6);

	ss.str("");
	ss << "Last Score:" << m_lastScore;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 60, 9);

	ss.str("");
	ss << "Score:" << m_score;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 60, 12);


	ss.str("");
	ss << "Spawn Time:" << m_spawnTime;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 60, 15);
		
	ss.str("");
	ss << "# of Birds:" << m_liveObjectCount << " / " << m_objectCount;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 60, 18);
	ss.str("");

	ss.str("");
	ss << "Generation:" << generation;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 60, 21);
	ss.str("");

	ss.str("");
	ss << "iRandSeed:" << iRandSeed;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 2, 60, 24);
	ss.str("");

	//Render score of each bird
}

void SceneFlappyBird::Exit()
{
	SceneBase::Exit();
	//Cleanup GameObjects
	while (m_goList.size() > 0)
	{
		GameObject* go = m_goList.back();
		delete go;
		m_goList.pop_back();
	}
}
