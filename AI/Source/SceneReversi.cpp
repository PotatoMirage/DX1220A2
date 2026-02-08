#include "SceneReversi.h"
#include "GL\glew.h"
#include "Application.h"
#include <sstream>

SceneReversi::SceneReversi()
	: m_bBlackTurn{}
	, m_bGameOver{}
	, m_winner{ WHO_CONTENT::WHO_NONE }
	, m_black{}
	, m_white{}
	, m_grid{}
	, m_whiteElapsed{}
	, m_blackElapsed{}
	, bAutoPlay(false)
{
}

SceneReversi::~SceneReversi()
{
}

void SceneReversi::Init()
{
	SceneBase::Init();

	//Calculating aspect ratio
	m_worldHeight = 100.f;
	m_worldWidth = m_worldHeight * (float)Application::GetWindowWidth() / Application::GetWindowHeight();

	//Physics code here
	m_speed = 1.f;

	Math::InitRNG();

	//for easy debugging, we work with a 4x4 board
	m_noGrid = 8;
	m_gridSize = m_worldHeight / m_noGrid;
	m_gridOffset = m_gridSize * 0.5f;

	//set up grid to contain necessary data
	m_grid.resize(m_noGrid * m_noGrid);
	Reset();

	srand(time(0));
}

int SceneReversi::GetIndex(int x, int y) const
{
	return y * m_noGrid + x;
}

void SceneReversi::Reset()
{
	// Exercise Week 14_15
	//3.	Implement Reset() method
	//reset state to beginning of a new game
	m_bGameOver = false;
	m_bBlackTurn = true;
	m_winner = WHO_CONTENT::WHO_NONE;
	m_black = m_white = 0;
	bAutoPlay = false;
	//empty the grid
	std::fill(m_grid.begin(), m_grid.end(), WHO_CONTENT::WHO_NONE);
	//place 4 pieces onto the board
	int minIdx = m_noGrid / 2 - 1; //calc indices for the center of the board
	int maxIdx = m_noGrid / 2; //calc indices for the center of the board
	m_grid[GetIndex(minIdx, minIdx)] = WHO_CONTENT::WHO_BLACK;
	m_grid[GetIndex(maxIdx, maxIdx)] = WHO_CONTENT::WHO_BLACK;
	m_grid[GetIndex(maxIdx, minIdx)] = WHO_CONTENT::WHO_WHITE;
	m_grid[GetIndex(minIdx, maxIdx)] = WHO_CONTENT::WHO_WHITE;
	


}

bool SceneReversi::IsIndexValid(int index) const
{
	return index >= 0 && index < m_noGrid;
}

bool SceneReversi::Move(std::vector<WHO_CONTENT>& grid, bool black, int index)
{
	// Exercise Week 14_15
	//4.	Implement SceneReversi::Move(). Game Logic - flip opposite color disks. 
	// Can you relate the following pseudo codes with the implementation in SceneReversi.cpp
	if (grid[index] != WHO_CONTENT::WHO_NONE)
		return false; //move not valid

	int tileX{ index % m_noGrid };
	int tileY{ index / m_noGrid };

	//store directional offsets
	static int offset[][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, 
							   {1, 1}, {-1, -1}, {-1, 1}, {1, -1} };
	WHO_CONTENT thisPiece{ black ? WHO_CONTENT::WHO_BLACK : WHO_CONTENT::WHO_WHITE };
	WHO_CONTENT other{ black ? WHO_CONTENT::WHO_WHITE : WHO_CONTENT::WHO_BLACK };

	//meant to store all pieces to be flipped
	//this container is static so it'll be instantiated only once no matter how many times Move() is invoked
	static std::vector<int> pieces{};
	
	bool valid{};
	//here, we check if move is valid, and update the state of the board accordingly
	for (int dir=0; dir <8; ++dir)
	{
		pieces.clear();

		int tx{ tileX }, ty{ tileY };
		do
		{
			//compute next position
			tx += offset[dir][0];
			ty += offset[dir][1];
			int nxtIdx{GetIndex(tx, ty)};
			//test next position for validity
			if (IsIndexValid(tx) && IsIndexValid(ty))
			{
				if (grid[nxtIdx] == other)
				{
					pieces.push_back(nxtIdx); //store piece that MAY need to flip
				}
				else if (grid[nxtIdx] == thisPiece)
				{
					//flip opponent pieces
					for (int p : pieces)
						grid[p] = thisPiece;

					//valid of this current direction is determined by where there are any 
					//pieces stored in the pieces container
					valid = valid || pieces.size() > 0;
					break; //proceed to next direction
				}
				else
				{
					//empty. proceed to next direction
					break;
				}
			}
		} while (IsIndexValid(tx) && IsIndexValid(ty));
	}

	//finally, place piece on empty slot
	if (valid)
		grid[index] = thisPiece;
	return valid;
}

int SceneReversi::CalculateScore(std::vector<WHO_CONTENT>& grid, bool black)
{
	// Count the number of white or black seeds
	int iCounter = 0;

	for (int row = 0; row < m_noGrid; ++row)
	{
		for (int col = 0; col < m_noGrid; ++col)
		{
			//std::cout << GetIndex(col, row) << std::endl;
			if (grid[GetIndex(col, row)] == (black ? WHO_CONTENT::WHO_BLACK : WHO_CONTENT::WHO_WHITE))
			{
				iCounter += 1 + gridWeights[GetIndex(col, row)];
			}
		}
	}

	return iCounter;
}

int SceneReversi::Count(std::vector<WHO_CONTENT>& grid, bool black)
{
	//Exercise Week 14_15
	//6.	Implement Count() - count number of black or white seeds
	return std::count(grid.begin(), grid.end(), black ? WHO_CONTENT::WHO_BLACK :
		WHO_CONTENT::WHO_WHITE);


}

bool SceneReversi::CheckGotMove(std::vector<WHO_CONTENT>& grid, bool black)
{
	//Exercise Week 14_15
	//7.	Implement these codes for CheckGotMove()
	size_t size = grid.size();
	//make a copy of the board just for the purpose of testing for a move
	//not performant, but irelevent to the purpose of this practical
	std::vector<WHO_CONTENT> tempGrid = grid;
	for (size_t index = 0; index < size; ++index)
	{
		if (Move(tempGrid, black, index))
			return true; //index is a valid move. thus, a move is available
	}
	return false;


}

//returns the index of the move that ai will make
//if no move is available, return -1
int SceneReversi::GetAIDecision(std::vector<WHO_CONTENT>& grid, bool black)
{
	// Exercise Week 14_15
	//9.	Game AI - implement GetAIDecision() and MinMax() using Minimax algorithm for white player
	std::vector<WHO_CONTENT> board = grid; //makes a copy of grid, we do not make changes to the real board until later
	size_t size = board.size();
	int move = -1; //-1 means no move is available
	//minimax player wants to maximize score for itself
	//in the case of this demo, the minimax player is white
	int bestScore = INT_MIN;
	for (size_t index = 0; index < size; ++index)
	{
		if (Move(board, black, index)) //check if move is valid
		{
			//compute the score associated with this branch using minimax
			int score = MinMax(board, !black, false, iMinMaxDepth);
			std::cout << (black ? "Black" : "White") << " score: " << score <<
				std::endl;
			std::cout << " " << (black ? "Black " : "White ") << "move: " << index
				<< std::endl;
			//take note of the best score out of all possible moves
			if (score > bestScore)
			{
				bestScore = score;
				move = index;
			}
			else if (score == bestScore)
			{
				// 50% chance to swap to the new score and index
				if ((rand() % 2) == 1)
				{
					bestScore = score;
					move = index;
				}
			}
			board = grid; //reset board
		}
	}
	std::cout << ">>>>>>" << (black ? "Black " : "White ") << "chose " << move << std::endl;
	return move;


}

int SceneReversi::MinMax(std::vector<WHO_CONTENT>& grid, bool black, bool max, int depth)
{
	// Exercise Week 14_15
	//9.	Game AI - implement GetAIDecision() and MinMax() using Minimax algorithm for white player
	//reached a leaf node/maximum depth. time to compute the score on said node
	if (depth == 1 || !CheckGotMove(grid, black))
	{
		//char turn = m_bBlackTurn ? 'B' : 'W';
		//std::cout << "COUNT(" << turn << "): " << Count(grid, m_bBlackTurn) << std::endl;
		//return Count(grid, m_bBlackTurn); //count pieces belonging to minimax player
		return CalculateScore(grid, m_bBlackTurn); //count pieces belonging to minimax	player
	}
	size_t size = grid.size();
	int best = max ? INT_MIN : INT_MAX; //minimax player aims to maximize score. the opponent(human player) aims to minimize score
		std::vector<WHO_CONTENT> board = grid; //make a copy of board
	for (size_t i = 0; i < size; ++i)
	{
		// for each valid move
		if (Move(board, black, i))
		{
			int val = MinMax(board, !black, !max, depth - 1);
			if (max) //minimax player maximizes score
				best = Math::Max(best, val);
			else //assume opponent always minimizes score
				best = Math::Min(best, val);
			board = grid; //reset board for next sibling
		}
	}
	return best;


}

void SceneReversi::Update(double dt)
{
	SceneBase::Update(dt);

	//Calculating aspect ratio
	m_worldHeight = 100.f;
	m_worldWidth = m_worldHeight * (float)Application::GetWindowWidth() / Application::GetWindowHeight();

	static bool aPressed{};
	if (Application::IsKeyPressed('A') && !aPressed)
	{
		aPressed = true;
		bAutoPlay = !bAutoPlay;
	}
	if (!Application::IsKeyPressed('A') && aPressed)
	{
		aPressed = false;
	}

	if (Application::IsKeyPressed(VK_OEM_MINUS))
	{
		m_speed = Math::Max(0.f, m_speed - 0.1f);
	}
	if (Application::IsKeyPressed(VK_OEM_PLUS))
	{
		m_speed += 0.1f;
	}
	if (Application::IsKeyPressed('R'))
	{
		Reset();
	}

	// If manual play, then run this part of the codes
	if (bAutoPlay == false)
	{
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
			if (posX > 0 && posX < m_noGrid * m_gridSize && posY > 0 && posY < m_noGrid * m_gridSize)
			{
				int gridX = static_cast<int>(posX / m_gridSize);
				int gridY = static_cast<int>(posY / m_gridSize);
				int index = GetIndex(gridX, gridY);
				//Exercise Week 14_15: Game Control
				//5.	Game Control - implement the game inputs for both Black and White players
				if (!m_bGameOver)
				{
					if (m_grid[index] == WHO_NONE)
					{
						//Check if valid move exists
						if (m_bBlackTurn)
						{
							if (CheckGotMove(m_grid, true))
							{
								//player places move
								if (Move(m_grid, true, index))
									m_bBlackTurn = !m_bBlackTurn;
							}
						}
						//white's turn is handled elsewhere
					}
				}


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
	}
	else
	{
		// If autoplay, then run this part of the codes

		//check if it's Player's turn
		if (m_bBlackTurn && !m_bGameOver)
		{
			m_blackElapsed += static_cast<float>(dt);
			if (m_blackElapsed >= 1.f)
			{
				int move = GetAIDecision(m_grid, m_bBlackTurn);
				if (move != -1)
				{
					Move(m_grid, m_bBlackTurn, move);
				}

				m_blackElapsed = 0.f;
				m_bBlackTurn = !m_bBlackTurn;
			}
		}
	}

	//check if it's AI's turn
	if (!m_bBlackTurn && !m_bGameOver)
	{
		m_whiteElapsed += static_cast<float>(dt);
		if (m_whiteElapsed >= 1.f)
		{
			int move = GetAIDecision(m_grid, m_bBlackTurn);
			if (move != -1)
			{
				Move(m_grid, m_bBlackTurn, move);
			}

			m_whiteElapsed = 0.f;
			m_bBlackTurn = !m_bBlackTurn;
		}
	}

	// Exercise Week 14_15
	//8.	Game Logic - After every move,
	//a.If a player cannot place any disks, the player�s turn is forfeited and pass the turn
	//b.If both players cannot place any disks, the game is over
	//c.Check for draw, black wins or white wins
	if (!m_bGameOver)
	{
		m_black = Count(m_grid, true);
		m_white = Count(m_grid, false);
		if (!CheckGotMove(m_grid, m_bBlackTurn))
		{
			m_bBlackTurn = !m_bBlackTurn; //no move available. pass the turn.
			//neither player could make a move. game ends.
			if (!CheckGotMove(m_grid, m_bBlackTurn))
			{
				m_bGameOver = true;
				if (m_black > m_white)
					m_winner = WHO_CONTENT::WHO_BLACK;
				else if (m_white > m_black)
					m_winner = WHO_CONTENT::WHO_WHITE;
			}
		}
	}


}

void SceneReversi::Render()
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

	// Exercise Week 14_15
	//a.	Render Game board
	modelStack.PushMatrix();
	modelStack.Translate(m_worldHeight * 0.5f, m_worldHeight * 0.5f, -1.f);
	modelStack.Scale(m_worldHeight, m_worldHeight, m_worldHeight);
	if (m_noGrid == 4)
		RenderMesh(meshList[GEO_REVERSIBOARD4x4], false);
	else if (m_noGrid == 8)
		RenderMesh(meshList[GEO_REVERSIBOARD], false);
	modelStack.PopMatrix();
	


	// Exercise Week 14_15
	//b.	Render black and white seeds without GameObject
	for (int row = 0; row < m_noGrid; ++row)
	{
		for (int col = 0; col < m_noGrid; ++col)
		{
			if (m_grid[GetIndex(col, row)] == WHO_NONE)
				continue;
			modelStack.PushMatrix();
			modelStack.Translate(col * m_gridSize + m_gridOffset, row * m_gridSize +
				m_gridOffset, 0.f);
			modelStack.Scale(m_gridSize, m_gridSize, 1.f);
			RenderMesh(meshList[m_grid[GetIndex(col, row)] == WHO_CONTENT::WHO_BLACK ?
				GEO_REVERSIBLACK : GEO_REVERSIWHITE], false);
			modelStack.PopMatrix();
		}
	}
	


	//On screen text
	std::ostringstream ss;
	ss.precision(3);
	ss << "Speed:" << m_speed;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 6);

	ss.str("");
	ss.precision(5);
	ss << "FPS:" << fps;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 3);

	// Exercise Week 14_15
	//c.	Render number of black and white seeds
	ss.str("");
	ss.precision(5);
	ss << "B: " << m_black << " W: " << m_white;
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 12);
	


	// Exercise Week 14_15
	//d.	Render whose turn (black or white)
	ss.str("");
	ss.precision(5);
	ss << "Turn: " << (m_bBlackTurn ? "Black" : "White");
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 15);
	


	if (bAutoPlay == true)
	{
		//Render whose turn (black or white)
		ss.str("");
		ss.precision(5);
		ss << "In Autoplay mode";
		RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 17);
	}

	// Exercise Week 14_15
	//e.	[When gameover] Render draw, black wins or white wins
	ss.str("");
	if (m_bGameOver)
	{
		if (m_winner == WHO_NONE)
			ss << "Winner: Draw";
		else
			ss << "Winner: " << (m_winner == WHO_BLACK ? "Black" : "White");
	}
	RenderTextOnScreen(meshList[GEO_TEXT], ss.str(), Color(0, 1, 0), 3, 50, 9);
	

	
	RenderTextOnScreen(meshList[GEO_TEXT], "Reversi (R to reset)", Color(0, 1, 0), 3, 50, 0);
}

void SceneReversi::Exit()
{
	SceneBase::Exit();
}
