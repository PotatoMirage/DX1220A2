#ifndef SCENE_REVERSI_H
#define SCENE_REVERSI_H

#include <vector>
#include "SceneBase.h"

class SceneReversi : public SceneBase
{
	enum WHO_CONTENT
	{
		WHO_NONE = 0,
		WHO_BLACK,
		WHO_WHITE,
	};
public:
	SceneReversi();
	~SceneReversi();

	virtual void Init();
	virtual void Update(double dt);
	virtual void Render();
	virtual void Exit();

	void Reset();

	// Exercise Week 14_15
	//1. SceneReversi.h, add the following methods.
	bool Move(std::vector<WHO_CONTENT>& grid, bool black, int index);
	int Count(std::vector<WHO_CONTENT>& grid, bool black);
	bool CheckGotMove(std::vector<WHO_CONTENT>& grid, bool black);
	int GetAIDecision(std::vector<WHO_CONTENT>& grid, bool black);
	int MinMax(std::vector<WHO_CONTENT>& grid, bool black, bool max, int depth);
	int CalculateScore(std::vector<WHO_CONTENT>& grid, bool black);

protected:
	int GetIndex(int x, int y) const;
	bool IsIndexValid(int index) const;

	bool bAutoPlay;

	float m_speed;
	float m_worldWidth;
	float m_worldHeight;
	int m_noGrid;
	float m_gridSize;
	float m_gridOffset;

	const int iMinMaxDepth = 4;
	std::vector<WHO_CONTENT> m_grid;
	bool m_bBlackTurn;
	bool m_bGameOver;
	WHO_CONTENT m_winner;
	int m_black, m_white;
	float m_whiteElapsed; //stores the elapsed time since start of white's turn
	float m_blackElapsed; //stores the elapsed time since start of black's turn during AutoPlay

	const int gridWeights[64] = {    2, -1,  1,  1,  1,  1, -1,  2,
									-1, -2, -1, -1, -1, -1, -1, -1,
									 1, -1,  0,  0,  0,  0, -1,  1,
									 1, -1,  0,  0,  0,  0, -1,  1,
									 1, -1,  0,  0,  0,  0, -1,  1,
									 1, -1,  0,  0,  0,  0, -1,  1,
									-1, -2, -1, -1, -1, -1, -2, -1,
									 2, -1,  1,  1,  1,  1, -2,  2 };
};

#endif