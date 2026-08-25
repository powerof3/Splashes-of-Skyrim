#include "Settings.h"

namespace Splashes
{
	void Settings::LoadSettings()
	{
		const auto store = REX::FIniSettingStore::GetSingleton();
		store->Init(path, "");
		store->Load();
		store->Save();
	}

	float Settings::GetSplashRadius(SIZE a_size) const
	{
		switch (a_size) {
		case kHeavy:
			return splashRadiusHeavy.GetValue();
		case kMedium:
			return splashRadiusMedium.GetValue();
		default:
			return splashRadiusLight.GetValue();
		}
	}

	float Settings::GetSplashScale(SIZE a_size) const
	{
		switch (a_size) {
		case kHeavy:
			return splashScaleHeavy.GetValue();
		case kMedium:
			return splashScaleMedium.GetValue();
		default:
			return splashScaleLight.GetValue();
		}
	}

	const Projectile* Settings::GetProjectileSetting(TYPE a_type) const
	{
		switch (a_type) {
		case kMissile:
			return &missile;
		case kFlame:
			return &flame;
		case kCone:
			return &cone;
		case kArrow:
			return &arrow;
		case kBeam:
			return &beam;
		default:
			return nullptr;
		}
	}

	const Explosion* Settings::GetExplosion() const
	{
		return &explosion;
	}

	std::pair<bool, bool> Settings::GetInstalled(TYPE a_type) const
	{
		switch (a_type) {
		case kMissile:
			return { missile.enableSplash, missile.enableRipple };
		case kFlame:
			return { flame.enableSplash, flame.enableRipple };
		case kCone:
			return { cone.enableSplash, cone.enableRipple };
		case kArrow:
			return { arrow.enableSplash, arrow.enableRipple };
		case kBeam:
			return { beam.enableSplash, beam.enableRipple };
		case kExplosion:
			return { explosion.enable, true };
		default:
			return { false, false };
		}
	}

	bool Settings::GetPatchDisplacement() const
	{
		return patchDisplacement.GetValue();
	}

	bool Settings::GetAllowDamageWater() const
	{
		return allowDamageWater.GetValue();
	}

	float Settings::GetExplosionSplashRadius() const
	{
		return explosion.splashRadius.GetValue();
	}
}
