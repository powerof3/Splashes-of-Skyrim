#pragma once

namespace Splash
{
	enum TYPE : std::uint32_t
	{
		kMissile = 0,
		kFlame = 1,
		kCone = 2,
		kArrow = 3,
		kExplosion = 4,

		kTotal = 5
	};

	enum SIZE : std::uint32_t
	{
		kHeavy = 0,
		kMedium = 1,
		kLight = 2
	};

	class Settings
	{
	public:
		using Setting = std::tuple<bool, bool, float, std::string, std::string, std::string>;

		static Settings* GetSingleton();

		void LoadSettings();

		float GetSplashRadius(SIZE a_size) const;
		float GetSplashScale(SIZE a_size) const;
		Setting GetSplashSetting(TYPE a_type) const;
		std::pair<bool, bool> GetInstallSetting(TYPE a_type) const;

		bool GetPatchDisplacement() const;
		bool GetAllowDamageWater() const;

		[[nodiscard]] float GetExplosionSplashRadius() const;

	private:
		static std::tuple<std::string, float> GetType(std::uint32_t a_type);
		static std::pair<std::string, float> GetSize(std::uint32_t a_size);

		bool patchDisplacement{ false };
		bool allowDamageWater{ false };

		std::vector<float> splashRadii;
		std::vector<float> splashScales;

		std::vector<Setting> splashSettings;

		float explosionSplashRadius{ 0.0 };
	};
}
