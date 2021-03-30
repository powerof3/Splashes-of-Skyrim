#include "Settings.h"

namespace Splash
{
	Settings* Settings::GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}


	std::tuple<std::string, float> Settings::GetType(std::uint32_t a_type)
	{
		switch (a_type) {
		case kMissile:
			{
				const std::string name{ "Missile" };
				return std::make_tuple(name, 1.0f);
			}
		case kFlame:
			{
				const std::string name{ "Flame" };
				return std::make_tuple(name, 1.0f);
			}
		case kCone:
			{
				const std::string name{ "Cone" };
				return std::make_tuple(name, 10.0f);
			}
		case kArrow:
			{
				const std::string name{ "Arrow" };
				return std::make_tuple(name, 1.0f);
			}
		case kExplosion:
			{
				const std::string name{ "Explosion" };
				return std::make_tuple(name, 5.0f);
			}
		default:
			return std::make_tuple(std::string(), 1.0f);
		}
	}


	std::pair<std::string, float> Settings::GetSize(std::uint32_t a_size)
	{
		switch (a_size) {
		case 0:
			{
				std::string name{ "Heavy Splash Projectile Size" };
				return std::make_pair(name, 35.0f);
			}
		case 1:
			{
				std::string name{ "Medium Splash Projectile Size" };
				return std::make_pair(name, 20.0f);
			}
		case 2:
			{
				std::string name{ "Light Splash Projectile Size" };
				return std::make_pair(name, 5.0f);
			}
		case 3:
			{
				std::string name{ "Heavy Splash Scale" };
				return std::make_pair(name, 1.0f);
			}
		case 4:
			{
				std::string name{ "Medium Splash Scale" };
				return std::make_pair(name, 0.75f);
			}
		case 5:
			{
				std::string name{ "Light Splash Scale" };
				return std::make_pair(name, 0.4f);
			}
		default:
			return std::make_pair(std::string(), 0.0f);
		}
	}


	void Settings::LoadSettings()
	{
		CSimpleIniA ini;
		ini.SetUnicode();
		ini.SetMultiKey();

		ini.LoadFile(L"Data/SKSE/Plugins/po3_SplashesOfSkyrim.ini");

		patchDisplacement = ini.GetBoolValue("Settings", "Enable Full Displacement", true);
		ini.SetBoolValue("Settings", "Enable Full Displacement", patchDisplacement, ";Enables water displacement on all water surfaces", true);

		allowDamageWater = ini.GetBoolValue("Settings", "Enable On Dangerous Water", false);
		ini.SetBoolValue("Settings", "Enable On Dangerous Water", allowDamageWater, ";Enables splashes on water marked as dangerous (i.e survival mods use this).\nLava and other non water surfaces may trigger water splashes!", true);

		
		for (auto i = 0; i < 3; i++) {
			auto [name, def] = GetSize(i);

			auto value = static_cast<float>(ini.GetDoubleValue("Settings", name.c_str(), def));
			ini.SetDoubleValue("Settings", name.c_str(), static_cast<double>(value), i == 0 ? ";Size of the projectile that will trigger splashes" : "", true);

			splashRadii.push_back(value);
		}

		for (auto i = 3; i < 6; i++) {
			auto [name, def] = GetSize(i);

			auto value = static_cast<float>(ini.GetDoubleValue("Settings", name.c_str(), def));
			ini.SetDoubleValue("Settings", name.c_str(), static_cast<double>(value), i == 3 ? ";Splash effect scale" : "", true);

			splashScales.push_back(value);
		}

		for (auto i = TYPE::kMissile; i < TYPE::kExplosion; i++) {
			auto [name, defMult] = GetType(i);

			Setting setting;
			auto& [enableSplash, enableRipple, displacementMult, modelPath, modelPathFire, modelPathDragon] = setting;

			enableSplash = ini.GetBoolValue(name.c_str(), "Enable Splashes", true);
			ini.SetBoolValue(name.c_str(), "Enable Splashes", enableSplash, i == TYPE::kMissile ? ";Enable water splashes (impact effects)" : "", true);

			enableRipple = ini.GetBoolValue(name.c_str(), "Enable Ripples", true);
			ini.SetBoolValue(name.c_str(), "Enable Ripples", enableRipple, i == TYPE::kMissile ? ";Enable water ripples (water displacement)" : "", true);

			displacementMult = static_cast<float>(ini.GetDoubleValue(name.c_str(), "Displacement Mult", defMult));
			ini.SetDoubleValue(name.c_str(), "Displacement Mult", static_cast<double>(displacementMult), i == TYPE::kMissile ? ";Ripple displacement multiplier. Higher values = more elongated ripples" : "", true);

			modelPath = ini.GetValue(name.c_str(), "NIF Path", R"(Effects\waterSplash.NIF)");
			ini.SetValue(name.c_str(), "NIF Path", modelPath.c_str(), i == TYPE::kMissile ? ";Path is relative to Data directory\n;Regular splash impact effect" : "", true);

			modelPathFire = ini.GetValue(name.c_str(), "NIF Path (Fire)", R"(Effects\ImpactEffects\ImpactWaterSplashFire.nif)");
			ini.SetValue(name.c_str(), "NIF Path (Fire)", modelPathFire.c_str(), i == TYPE::kMissile ? ";Fire impact effect" : "", true);
	
			modelPathDragon = ini.GetValue(name.c_str(), "NIF Path (Dragon)", R"(Effects\ImpactEffects\FXDragonFireImpactWater.nif)");
			ini.SetValue(name.c_str(), "NIF Path (Dragon)", modelPathDragon.c_str(), i == TYPE::kMissile ? ";Dragon fire impact effect" : "", true);

			splashSettings.push_back(setting);
		}

		Setting setting;
		auto& [enable, fireOnly, displacementMult, modelPath, modelPathFire, modelPathDragon] = setting;

		enable = ini.GetBoolValue("Explosion", "Enable", true);
		ini.SetBoolValue("Explosion", "Enable", enable, ";Enable underwater explosions", true);

		fireOnly = ini.GetBoolValue("Explosion", "Limit To Fire", true);
		ini.SetBoolValue("Explosion", "Limit To Fire", fireOnly, ";Only fire explosions will cause underwater explosions", true);

		displacementMult = static_cast<float>(ini.GetDoubleValue("Explosion", "Displacement Mult", 5.0f));
		ini.SetDoubleValue("Explosion", "Displacement Mult", static_cast<double>(displacementMult), "", true);

		auto constexpr explosionPath{ R"(Effects\ExplosionSplash.NIF)" };
		
		modelPath = ini.GetValue("Explosion", "NIF Path", explosionPath);
		ini.SetValue("Explosion", "NIF Path", modelPath.c_str(), ";Regular explosion", true);

		modelPathFire = ini.GetValue("Explosion", "NIF Path (Fire)", explosionPath);
		ini.SetValue("Explosion", "NIF Path (Fire)", modelPathFire.c_str(), ";Fire explosion", true);

		modelPathDragon = ini.GetValue("Explosion", "NIF Path (Dragon)", explosionPath);
		ini.SetValue("Explosion", "NIF Path (Dragon)", modelPathDragon.c_str(), ";Dragon fire explosion", true);

		splashSettings.push_back(setting);

		ini.SaveFile(L"Data/SKSE/Plugins/po3_SplashesOfSkyrim.ini");
	}


	float Settings::GetSplashRadius(SIZE a_size) const
	{
		return splashRadii[a_size];
	}

	float Settings::GetSplashScale(SIZE a_size) const
	{
		return splashScales[a_size];
	}


	Settings::Setting Settings::GetSplashSetting(TYPE a_type) const
	{
		return splashSettings[a_type];
	}


	bool Settings::GetPatchDisplacement() const
	{
		return patchDisplacement;
	}
	
	
	bool Settings::GetAllowDamageWater() const
	{
		return allowDamageWater;
	}
}
