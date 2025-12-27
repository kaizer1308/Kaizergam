#pragma once
#include "../../SDK/SDK.h"
#include "../Backtrack/Backtrack.h"

class CWarpPrediction
{
private:
	std::unordered_map<int, Vec3> m_mPredictedPositions;
	std::unordered_map<int, Vec3> m_mMovementVectors;
	std::unordered_map<int, Vec3> m_mLastPositions;
	std::unordered_map<int, bool> m_mIsWarping;

	float m_flMinVelocity = 200.0f;
	float m_flWarpThreshold = 500.0f;
	float m_flMaxPredictionTime = 1.0f;

	bool DetectWarping(int iIndex, const std::vector<TickRecord*>& vRecords);

public:
	void Initialize();
	void Update();
	bool PredictWarpPosition(int iIndex, Vec3& vPredictedPos);
	bool IsWarping(int iIndex);
	void Draw();
	
	Vec3 GetPredictedPosition(int iIndex) { return m_mPredictedPositions.contains(iIndex) ? m_mPredictedPositions[iIndex] : Vec3(); }
};

ADD_FEATURE(CWarpPrediction, WarpPrediction);
