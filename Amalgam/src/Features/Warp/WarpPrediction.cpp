#include "WarpPrediction.h"
#include "../Backtrack/Backtrack.h"
#include "../Simulation/MovementSimulation/MovementSimulation.h"


// I don't know if I recoded this right -sean
void CWarpPrediction::Initialize()
{
	m_mPredictedPositions.clear();
	m_mMovementVectors.clear();
	m_mLastPositions.clear();
	m_mIsWarping.clear();
}

void CWarpPrediction::Update()
{
	auto pLocal = H::Entities.GetLocal();
	if (!pLocal || !pLocal->IsAlive())
		return;

	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerEnemy))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (!pPlayer || pPlayer->IsDormant() || !pPlayer->IsAlive() || pPlayer->IsAGhost())
		{
			int iIndex = pPlayer->entindex();
			m_mPredictedPositions.erase(iIndex);
			m_mMovementVectors.erase(iIndex);
			m_mLastPositions.erase(iIndex);
			m_mIsWarping.erase(iIndex);
			continue;
		}

		int iIndex = pPlayer->entindex();

		std::vector<TickRecord*> vRecords;
		if (!F::Backtrack.GetRecords(pPlayer, vRecords) || vRecords.size() < 2)
		{
			m_mIsWarping[iIndex] = false;
			continue;
		}

		m_mIsWarping[iIndex] = DetectWarping(iIndex, vRecords);

		if (m_mIsWarping[iIndex])
		{
			Vec3 vPredicted;
			if (PredictWarpPosition(iIndex, vPredicted))
				m_mPredictedPositions[iIndex] = vPredicted;
		}
	}
}

bool CWarpPrediction::DetectWarping(int iIndex, const std::vector<TickRecord*>& vRecords)
{
	if (vRecords.size() < 2)
		return false;

	// Check if the latest record broke lag compensation (teleported)
	Vec3 vDelta = vRecords[0]->m_vOrigin - vRecords[1]->m_vOrigin;
	if (vDelta.Length2DSqr() > 4096.0f) // 64^2
		return true;

	return false;
}

bool CWarpPrediction::PredictWarpPosition(int iIndex, Vec3& vPredictedPos)
{
	auto pEntity = I::ClientEntityList->GetClientEntity(iIndex);
	if (!pEntity || pEntity->GetClientClass()->m_ClassID != static_cast<int>(ETFClassID::CTFPlayer))
		return false;

	auto pPlayer = pEntity->As<CTFPlayer>();
	if (!pPlayer || !pPlayer->IsAlive())
		return false;

	std::vector<TickRecord*> vRecords;
	if (!F::Backtrack.GetRecords(pPlayer, vRecords) || vRecords.size() < 2)
		return false;

	// Calculate choked commands
	float flSimTime = vRecords[0]->m_flSimTime;
	float flOldSimTime = vRecords[1]->m_flSimTime;
	int iTicks = TIME_TO_TICKS(flSimTime - flOldSimTime);

	if (iTicks <= 0 || iTicks > 22)
		return false;

	MoveStorage tStorage;
	F::MoveSim.Initialize(pPlayer, tStorage);

	// Run simulation for choked ticks
	for (int i = 0; i < iTicks; i++)
		F::MoveSim.RunTick(tStorage);

	vPredictedPos = tStorage.m_vPredictedOrigin;

	F::MoveSim.Restore(tStorage);
	return true;
}

bool CWarpPrediction::IsWarping(int iIndex)
{
	return m_mIsWarping.contains(iIndex) && m_mIsWarping[iIndex];
}


