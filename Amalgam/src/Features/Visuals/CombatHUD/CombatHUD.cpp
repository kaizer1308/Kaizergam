#include "CombatHUD.h"

#include "../../CritHack/CritHack.h"
#include "../../Ticks/Ticks.h"
#include "../../PacketManip/AntiAim/AntiAim.h"

void CCombatHUD::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::CombatHUD) || !I::EngineClient->IsInGame())
		return;

	if (!pLocal->IsAlive() || pLocal->IsAGhost())
		return;

	auto pWeapon = H::Entities.GetWeapon();
	if (!pWeapon)
		return;

	const DragBox_t dtPos = Vars::Menu::CombatHUDDisplay.Value;
	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);
	const int nTall = fFont.m_nTall + H::Draw.Scale(1);

	const int iPanelWidth = H::Draw.Scale(280, Scale_Round);
	const int iPanelPadding = H::Draw.Scale(8, Scale_Round);
	const int iBarHeight = H::Draw.Scale(18, Scale_Round);

	int x = dtPos.x;
	int y = dtPos.y;

	int iCrits = F::CritHack.GetAvailableCrits();
	int iMaxCrits = F::CritHack.GetPotentialCrits();
	bool bCritBanned = F::CritHack.IsCritBanned();
	bool bCritBoosted = pLocal->IsCritBoosted();
	float flTickBase = TICKS_TO_TIME(pLocal->m_nTickBase());
	bool bStreamingCrits = pWeapon->m_flCritTime() > flTickBase;

	Color_t tCritColor = iCrits > 0 ? Vars::Colors::IndicatorTextGood.Value : Vars::Colors::IndicatorTextBad.Value;
	H::Draw.StringOutlined(fFont, x - iPanelWidth / 2, y, tCritColor, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, 
		std::format("CRITS: {}/{}", iCrits, iMaxCrits).c_str());

	int iNextCrit = F::CritHack.GetNextCrit();
	if (bCritBoosted)
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextMisc.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, "CRIT BOOSTED");
	}
	else if (bStreamingCrits)
	{
		float flTime = pWeapon->m_flCritTime() - flTickBase;
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextMisc.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, 
			std::format("STREAMING {:.1f}s", flTime).c_str());
	}
	else if (bCritBanned)
	{
		float flDamageTilFlip = F::CritHack.GetDamageTilFlip();
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, 
			std::format("NEED {} DMG", static_cast<int>(ceilf(flDamageTilFlip))).c_str());
	}
	else if (iCrits > 0)
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, "CRIT READY");
	}
	else if (iNextCrit > 0 && iMaxCrits > 0)
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextMid.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, 
			std::format("NEXT IN {} SHOT{}", iNextCrit, iNextCrit == 1 ? "" : "S").c_str());
	}
	else
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Menu::Theme::Inactive.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, "NO CRITS");
	}

	y += nTall + H::Draw.Scale(4, Scale_Round);

	static auto tf_weapon_criticals_bucket_cap = H::ConVars.FindVar("tf_weapon_criticals_bucket_cap");
	const float flBucketCap = tf_weapon_criticals_bucket_cap ? tf_weapon_criticals_bucket_cap->GetFloat() : 1000.f;
	const float flBucket = pWeapon->m_flCritTokenBucket();
	float flCritRatioTarget = std::clamp(flBucket / flBucketCap, 0.f, 1.f);

	static float flCritRatioDisplay = 0.f;
	const float flLerpSpeed = 0.08f;
	flCritRatioDisplay = flCritRatioDisplay + (flCritRatioTarget - flCritRatioDisplay) * flLerpSpeed;
	float flCritRatio = flCritRatioDisplay;

	int iBarX = x - iPanelWidth / 2;
	int iBarY = y;

	H::Draw.FillRoundRect(iBarX - 1, iBarY - 1, iPanelWidth + 2, iBarHeight + 2, 5, Color_t(20, 20, 20, 100));

	if (flCritRatio > 0.001f)
	{
		int iBarFillWidth = static_cast<int>(iPanelWidth * flCritRatio);
		if (iBarFillWidth >= 5)
		{
			Color_t tColorStart = Color_t(100, 40, 140, 180);
			Color_t tColorEnd = Color_t(200, 100, 220, 180);
			Color_t tFillColor = tColorStart.Lerp(tColorEnd, flCritRatio);

			H::Draw.FillRoundRect(iBarX, iBarY, iBarFillWidth, iBarHeight, 5, tFillColor);
			
			H::Draw.FillRoundRect(iBarX, iBarY, iBarFillWidth, iBarHeight / 2, 5, Color_t(255, 255, 255, 30));
		}
		else if (iBarFillWidth > 0)
		{
			H::Draw.FillRect(iBarX, iBarY, iBarFillWidth, iBarHeight, Color_t(200, 100, 220, 180));
		}
	}

	y += iBarHeight + H::Draw.Scale(4, Scale_Round);

	int iCurrentDamage = static_cast<int>(F::CritHack.GetRangedDamage()) + F::CritHack.GetMeleeDamage();
	int iUnsafe = std::abs(F::CritHack.GetDesyncDamage());

	H::Draw.StringOutlined(fFont, x - iPanelWidth / 2, y, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, 
		std::format("DMG: {}/{}", iCurrentDamage, static_cast<int>(flBucketCap)).c_str());

	Color_t tUnsafeColor = iUnsafe > 0 ? Vars::Colors::IndicatorTextBad.Value : Vars::Colors::IndicatorTextGood.Value;
	H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, tUnsafeColor, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, 
		std::format("UNSAFE: {}", iUnsafe).c_str());

	y += nTall + H::Draw.Scale(4, Scale_Round);

	bool bTickShifting = F::Ticks.m_bDoubletap || F::Ticks.m_bWarp || F::Ticks.m_bSpeedhack;
	int iChoke = std::max(I::ClientState->chokedcommands - (F::AntiAim.YawOn() ? F::AntiAim.AntiAimTicks() : 0), 0);
	int iTicks = std::clamp(F::Ticks.m_iShiftedTicks + iChoke, 0, F::Ticks.m_iMaxShift);
	int iMaxTicks = F::Ticks.m_iMaxShift;

	Color_t tTickColor = bTickShifting ? Vars::Colors::IndicatorTextMisc.Value : Vars::Menu::Theme::Active.Value;
	H::Draw.StringOutlined(fFont, x - iPanelWidth / 2, y, tTickColor, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, 
		std::format("TICKS: {}/{}", iTicks, iMaxTicks).c_str());

	if (F::Ticks.m_bSpeedhack)
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextMisc.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, 
			std::format("SPEEDHACK x{}", Vars::Speedhack::Amount.Value).c_str());
	}
	else if (F::Ticks.m_iWait)
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, "NOT READY");
	}
	else if (iTicks >= iMaxTicks)
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, "READY");
	}
	else if (iTicks < iMaxTicks / 2)
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Colors::IndicatorTextMid.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, "CHARGING");
	}
	else
	{
		H::Draw.StringOutlined(fFont, x + iPanelWidth / 2, y, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPRIGHT, "CHARGING");
	}

	y += nTall + H::Draw.Scale(4, Scale_Round);

	float flTickRatioTarget = iMaxTicks > 0 ? static_cast<float>(iTicks) / iMaxTicks : 0.f;

	static float flTickRatioDisplay = 0.f;
	flTickRatioDisplay = flTickRatioDisplay + (flTickRatioTarget - flTickRatioDisplay) * flLerpSpeed;
	float flTickRatio = flTickRatioDisplay;

	iBarY = y;

	H::Draw.FillRoundRect(iBarX - 1, iBarY - 1, iPanelWidth + 2, iBarHeight + 2, 5, Color_t(20, 20, 20, 100));

	if (flTickRatio > 0.001f)
	{
		int iBarFillWidth = static_cast<int>(iPanelWidth * flTickRatio);
		if (iBarFillWidth >= 5)
		{
			Color_t tColorStart = Color_t(40, 100, 140, 180);
			Color_t tColorEnd = Color_t(100, 200, 240, 180);
			Color_t tFillColor = tColorStart.Lerp(tColorEnd, flTickRatio);

			H::Draw.FillRoundRect(iBarX, iBarY, iBarFillWidth, iBarHeight, 5, tFillColor);

			H::Draw.FillRoundRect(iBarX, iBarY, iBarFillWidth, iBarHeight / 2, 5, Color_t(255, 255, 255, 30));
		}
		else if (iBarFillWidth > 0)
		{
			H::Draw.FillRect(iBarX, iBarY, iBarFillWidth, iBarHeight, Color_t(100, 200, 240, 180));
		}
	}
}
