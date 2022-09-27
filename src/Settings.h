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

		std::string type{};

		float displacementMult{};

		std::string modelPath{};
		std::string modelPathFire{};
		std::string modelPathDragon{};
	};

	struct Projectile : Base
	{
		Projectile(std::string_view a_type, float a_displacementMult);

		void LoadSettings(CSimpleIniA& a_ini, bool a_writeComment = false);

		bool enableSplash{ true };
		bool enableRipple{ true };
	};

	struct Explosion : Base
	{
		Explosion(std::string_view a_type, float a_displacementMult);

		void LoadSettings(CSimpleIniA& a_ini);

		bool enable{ true };
		bool fireOnly{ true };
		float splashRadius{ 250.0f };
	};

	namespace INI
	{
		template <class T>
		void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
		{
			if constexpr (std::is_same_v<bool, T>) {
				a_value = a_ini.GetBoolValue(a_section, a_key, a_value);
				a_ini.SetBoolValue(a_section, a_key, a_value, a_comment);
			} else if constexpr (std::is_floating_point_v<T>) {
				a_value = static_cast<T>(a_ini.GetDoubleValue(a_section, a_key, a_value));
				a_ini.SetDoubleValue(a_section, a_key, a_value, a_comment);
			} else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
				a_value = string::lexical_cast<T>(a_ini.GetValue(a_section, a_key, std::to_string(a_value).c_str()));
				a_ini.SetValue(a_section, a_key, std::to_string(a_value).c_str(), a_comment);
			} else {
				a_value = a_ini.GetValue(a_section, a_key, a_value.c_str());
				a_ini.SetValue(a_section, a_key, a_value.c_str(), a_comment);
			}
		}
	}

	class Settings
	{
	public:
		static Settings* GetSingleton();

		[[nodiscard]] float GetSplashRadius(SIZE a_size) const;
		[[nodiscard]] float GetSplashScale(SIZE a_size) const;

		[[nodiscard]] const Projectile* GetProjectileSetting(TYPE a_type) const;
		[[nodiscard]] const Explosion* GetExplosion() const;

		[[nodiscard]] std::pair<bool, bool> GetInstalled(TYPE a_type) const;

		[[nodiscard]] bool GetPatchDisplacement() const;
		[[nodiscard]] bool GetAllowDamageWater() const;

		[[nodiscard]] float GetExplosionSplashRadius() const;

	private:
		Settings();

		void LoadSettings();

		bool patchDisplacement{ true };
		bool allowDamageWater{ false };

		Projectile missile{ "Missile"sv, 1.0f };
		Projectile flame{ "Flame"sv, 1.0f };
		Projectile cone{ "Cone"sv, 10.0f };
		Projectile arrow{ "Arrow"sv, 1.0f };
		Projectile beam{ "Beam"sv, 0.4f };
		Explosion explosion{ "Explosion", 5.0f };

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
