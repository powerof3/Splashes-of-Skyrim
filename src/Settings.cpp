#include "Settings.h"

namespace Splashes
{
	Base::Base(std::string_view a_type, float a_displacementMult) :
		type(a_type),
		displacementMult(a_displacementMult)
	{}

	Projectile::Projectile(std::string_view a_type, float a_displacementMult) :
		Base(a_type, a_displacementMult)
	{
		modelPath = R"(Effects\waterSplash.NIF)";
		modelPathFire = R"(Effects\ImpactEffects\ImpactWaterSplashFire.nif)";
		modelPathDragon = R"(Effects\ImpactEffects\FXDragonFireImpactWater.nif)";
	}

	Explosion::Explosion(std::string_view a_type, float a_displacementMult) :
		Base(a_type, a_displacementMult)
	{
		modelPath = R"(Effects\ExplosionSplash.NIF)";
		modelPathFire = R"(Effects\ExplosionSplash.NIF)";
		modelPathDragon = R"(Effects\ExplosionSplash.NIF)";
	}

	void Projectile::LoadSettings(CSimpleIniA& a_ini, bool a_writeComment)
	{
		ini::get_value(a_ini, enableSplash, type.c_str(), "bWaterSplashes", a_writeComment ? ";Enable water splashes (impact effects)." : nullptr);
		ini::get_value(a_ini, enableRipple, type.c_str(), "bWaterRipples", a_writeComment ? ";Enable water ripples." : nullptr);

		ini::get_value(a_ini, displacementMult, type.c_str(), "fRippleDisplacementMult", a_writeComment ? ";Ripple displacement multiplier. Higher values = more elongated ripples." : nullptr);
		ini::get_value(a_ini, modelPath, type.c_str(), "sNifPath", a_writeComment ? ";Path is relative to Data directory." : nullptr);
		ini::get_value(a_ini, modelPathFire, type.c_str(), "sNifPathFire", nullptr);
		ini::get_value(a_ini, modelPathDragon, type.c_str(), "sNifPathDragonFire", nullptr);
	}

	void Explosion::LoadSettings(CSimpleIniA& a_ini)
	{
		ini::get_value(a_ini, enable, type.c_str(), "bEnable", nullptr);
		ini::get_value(a_ini, fireOnly, type.c_str(), "bFireExplosionsOnly", nullptr);

		ini::get_value(a_ini, displacementMult, type.c_str(), "fRippleDisplacementMult", nullptr);
		ini::get_value(a_ini, modelPath, type.c_str(), "sNifPath", nullptr);
		ini::get_value(a_ini, modelPathFire, type.c_str(), "sNifPathFire", nullptr);
		ini::get_value(a_ini, modelPathDragon, type.c_str(), "sNifPathDragonFire", nullptr);

		ini::get_value(a_ini, splashRadius, type.c_str(), "fDefaultExplosionSplashRadius", nullptr);
	}

	void Settings::LoadSettings()
	{
		constexpr auto path = L"Data/SKSE/Plugins/po3_SplashesOfSkyrim.ini";

		CSimpleIniA ini;
		ini.SetUnicode();

		ini.LoadFile(path);

		ini::get_value(ini, patchDisplacement, "Settings", "bWaterDisplacement", ";Enables water displacement on all water surfaces.");
		ini::get_value(ini, allowDamageWater, "Settings", "bSplashesOnDangerousWater", ";Enables splashes on water marked as dangerous (i.e survival mods use this).\n;Lava and other non water surfaces may trigger water splashes.");

		ini::get_value(ini, splashRadii[SIZE::kHeavy], "Settings", "fProjectileSizeHeavy", ";Size of the projectile that will trigger splashes.");
		ini::get_value(ini, splashRadii[SIZE::kMedium], "Settings", "fProjectileSizeMedium", nullptr);
		ini::get_value(ini, splashRadii[SIZE::kLight], "Settings", "fProjectileSizeLight", nullptr);

		ini::get_value(ini, splashScales[SIZE::kHeavy], "Settings", "fSplashEffectScaleHeavy", ";Splash effect scale.");
		ini::get_value(ini, splashScales[SIZE::kMedium], "Settings", "fSplashEffectScaleMedium", nullptr);
		ini::get_value(ini, splashScales[SIZE::kLight], "Settings", "fSplashEffectScaleLight", nullptr);

		missile.LoadSettings(ini, true);
		flame.LoadSettings(ini);
		cone.LoadSettings(ini);
		arrow.LoadSettings(ini);
		beam.LoadSettings(ini);

		explosion.LoadSettings(ini);

		ini.SaveFile(path);
	}

	float Settings::GetSplashRadius(SIZE a_size) const
	{
		return splashRadii.find(a_size)->second;
	}

	float Settings::GetSplashScale(SIZE a_size) const
	{
		return splashScales.find(a_size)->second;
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
		return patchDisplacement;
	}

	bool Settings::GetAllowDamageWater() const
	{
		return allowDamageWater;
	}

	float Settings::GetExplosionSplashRadius() const
	{
		return explosion.splashRadius;
	}
}
