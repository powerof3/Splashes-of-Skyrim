#pragma once

namespace Splashes
{
	enum TYPE : std::uint32_t
	{
		kMissile = 0,
		kFlame,
		kCone,
		kArrow,
		kBeam,
		kExplosion,
	};

	enum SIZE : std::uint32_t
	{
		kHeavy = 0,
		kMedium,
		kLight
	};

	struct Base
	{
		Base(std::string_view a_type, float a_displacementMult, std::string_view a_nif, std::string_view a_nifFire, std::string_view a_nifDragon) :
			type(a_type),
			displacementMult(a_type, "fRippleDisplacementMult", a_displacementMult),
			modelPath(a_type, "sNifPath", std::string(a_nif)),
			modelPathFire(a_type, "sNifPathFire", std::string(a_nifFire)),
			modelPathDragon(a_type, "sNifPathDragonFire", std::string(a_nifDragon))
		{}

		// members
		std::string_view              type;
		REX::TIniSetting<float>       displacementMult;
		REX::TIniSetting<std::string> modelPath;
		REX::TIniSetting<std::string> modelPathFire;
		REX::TIniSetting<std::string> modelPathDragon;
	};

	struct Projectile : Base
	{
		Projectile(std::string_view a_type, float a_displacementMult) :
			Base(a_type, a_displacementMult,
				R"(Effects\waterSplash.NIF)",
				R"(Effects\ImpactEffects\ImpactWaterSplashFire.nif)",
				R"(Effects\ImpactEffects\FXDragonFireImpactWater.nif)"),
			enableSplash(a_type, "bWaterSplashes", true),
			enableRipple(a_type, "bWaterRipples", true)
		{}

		// members
		REX::TIniSetting<bool> enableSplash;
		REX::TIniSetting<bool> enableRipple;
	};

	struct Explosion : Base
	{
		Explosion(std::string_view a_type, float a_displacementMult) :
			Base(a_type, a_displacementMult,
				R"(Effects\ExplosionSplash.NIF)",
				R"(Effects\ExplosionSplash.NIF)",
				R"(Effects\ExplosionSplash.NIF)"),
			enable(a_type, "bEnable", true),
			fireOnly(a_type, "bFireExplosionsOnly", true),
			splashRadius(a_type, "fDefaultExplosionSplashRadius", 250.0f)
		{}

		// members
		REX::TIniSetting<bool>  enable;
		REX::TIniSetting<bool>  fireOnly;
		REX::TIniSetting<float> splashRadius;
	};

	class Settings : public REX::TSingleton<Settings>
	{
	public:
		void LoadSettings();

		[[nodiscard]] float GetSplashRadius(SIZE a_size) const;
		[[nodiscard]] float GetSplashScale(SIZE a_size) const;

		[[nodiscard]] const Projectile* GetProjectileSetting(TYPE a_type) const;
		[[nodiscard]] const Explosion*  GetExplosion() const;

		[[nodiscard]] std::pair<bool, bool> GetInstalled(TYPE a_type) const;

		[[nodiscard]] bool GetPatchDisplacement() const;
		[[nodiscard]] bool GetAllowDamageWater() const;

		[[nodiscard]] float GetExplosionSplashRadius() const;

	private:
		static constexpr auto path = R"(Data\SKSE\Plugins\po3_SplashesOfSkyrim.ini)";

		// members
		REX::TIniSetting<bool> patchDisplacement{ "Settings", "bWaterDisplacement", true };
		REX::TIniSetting<bool> allowDamageWater{ "Settings", "bSplashesOnDangerousWater", false };

		REX::TIniSetting<float> splashRadiusHeavy{ "Settings", "fProjectileSizeHeavy", 35.0f };
		REX::TIniSetting<float> splashRadiusMedium{ "Settings", "fProjectileSizeMedium", 20.0f };
		REX::TIniSetting<float> splashRadiusLight{ "Settings", "fProjectileSizeLight", 5.0f };

		REX::TIniSetting<float> splashScaleHeavy{ "Settings", "fSplashEffectScaleHeavy", 1.0f };
		REX::TIniSetting<float> splashScaleMedium{ "Settings", "fSplashEffectScaleMedium", 0.75f };
		REX::TIniSetting<float> splashScaleLight{ "Settings", "fSplashEffectScaleLight", 0.5f };

		Projectile missile{ "Missile"sv, 1.0f };
		Projectile flame{ "Flame"sv, 1.0f };
		Projectile cone{ "Cone"sv, 10.0f };
		Projectile arrow{ "Arrow"sv, 1.0f };
		Projectile beam{ "Beam"sv, 0.4f };
		Explosion  explosion{ "Explosion"sv, 5.0f };
	};
}
