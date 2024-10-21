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
		Base(std::string_view a_type, float a_displacementMult);

		// members
		std::string type{};
		std::string modelPath{};
		std::string modelPathFire{};
		std::string modelPathDragon{};
		float       displacementMult{};
	};

	struct Projectile : Base
	{
		Projectile(std::string_view a_type, float a_displacementMult);

		void LoadSettings(CSimpleIniA& a_ini, bool a_writeComment = false);

		// members
		bool enableSplash{ true };
		bool enableRipple{ true };
	};

	struct Explosion : Base
	{
		Explosion(std::string_view a_type, float a_displacementMult);

		void LoadSettings(CSimpleIniA& a_ini);

		// members
		bool  enable{ true };
		bool  fireOnly{ true };
		float splashRadius{ 250.0f };
	};

	class Settings : public ISingleton<Settings>
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
		// members
		bool patchDisplacement{ true };
		bool allowDamageWater{ false };

		Projectile missile{ "Missile"sv, 1.0f };
		Projectile flame{ "Flame"sv, 1.0f };
		Projectile cone{ "Cone"sv, 10.0f };
		Projectile arrow{ "Arrow"sv, 1.0f };
		Projectile beam{ "Beam"sv, 0.4f };
		Explosion  explosion{ "Explosion", 5.0f };

		std::map<SIZE, float> splashRadii{
			{ kHeavy, 35.0f },
			{ kMedium, 20.0f },
			{ kLight, 5.0f }
		};

		std::map<SIZE, float> splashScales{
			{ kHeavy, 1.0f },
			{ kMedium, 0.75f },
			{ kLight, 0.5f }
		};
	};
}
