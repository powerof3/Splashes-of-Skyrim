#pragma once

#include "Settings.h"

struct util
{
	template <typename First, typename... T>
	[[nodiscard]] static bool string_contains(First&& first, T&&... t)
	{
		return (string::icontains(first, t) || ...);
	}

	static std::uint32_t get_effect_type(const RE::NiAVObject* a_object)
	{
		static auto constexpr Fire{ "fire"sv };
		static auto constexpr Flame{ "flame"sv };
		static auto constexpr Dragon{ "dragon"sv };

		const std::string name(a_object->name);
		if (string_contains(name, Fire, Flame)) {
			if (string::icontains(name, Dragon)) {
				return 2;
			}
			return 1;
		}
		return 0;
	}

	static float get_water_height(const RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos)
	{
		float waterHeight = -RE::NI_INFINITY;

		const auto waterManager = RE::BGSWaterSystemManager::GetSingleton();
		if (waterManager && waterManager->enabled) {
			waterHeight = a_ref->GetWaterHeight();

			if (!numeric::essentially_equal(waterHeight, -RE::NI_INFINITY)) {
				return waterHeight;
			}

			const auto get_nearest_water_object_height = [&]() {
				const auto settings = Splash::Settings::GetSingleton();
				for (const auto& waterObject : waterManager->waterObjects) {
					if (waterObject) {
						if (!settings->GetAllowDamageWater()) {
							if (const auto waterForm = waterObject->waterForm; waterForm && waterForm->GetDangerous()) {
								continue;
							}
						}
						for (const auto& bound : waterObject->multiBounds) {
							if (bound) {
								if (auto size{ bound->size }; size.z <= 10.0f) {  //avoid sloped water
									auto center{ bound->center };
									const auto boundMin = center - size;
									const auto boundMax = center + size;
									if (!(a_pos.x < boundMin.x || a_pos.x > boundMax.x || a_pos.y < boundMin.y || a_pos.y > boundMax.y)) {
										return center.z;
									}
								}
							}
						}
					}
				}

				return -RE::NI_INFINITY;
			};

			waterHeight = get_nearest_water_object_height();
		}

		return waterHeight;
	}

	static std::pair<float, float> get_submerged_water_level(const RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos)
	{
		auto waterHeight = get_water_height(a_ref, a_pos);

		if (numeric::approximately_equal(waterHeight, -RE::NI_INFINITY) || waterHeight <= a_pos.z) {
			return std::make_pair(waterHeight, 0.0f);
		}

		auto level = (waterHeight - a_pos.z) / a_ref->GetHeight();
		return std::make_pair(waterHeight, level <= 1.0f ?
                                               level :
                                               1.0f);
	}

	static void create_ripple(const RE::NiPoint3& a_pos, float a_displacementMult, bool a_skipQueue = false)
	{
		const auto displacement = 0.0099999998f * a_displacementMult;

		if (!a_skipQueue || RE::TaskQueueInterface::ShouldUseTaskQueue()) {
			if (const auto taskPool = RE::TaskQueueInterface::GetSingleton(); taskPool) {
				taskPool->QueueBulletWaterDisplacementTask(displacement, a_pos);
			}
		} else {
			if (const auto waterManager = RE::BGSWaterSystemManager::GetSingleton(); waterManager) {
				waterManager->CreateBulletWaterDisplacement(a_pos, displacement);
			}
		}
	}
};

template <class T, Splash::TYPE type>
class ProjectileManager
{
public:
	static void Install();

private:
	ProjectileManager() = delete;
	ProjectileManager(const ProjectileManager&) = delete;
	ProjectileManager(ProjectileManager&&) = delete;

	~ProjectileManager() = delete;

	ProjectileManager& operator=(const ProjectileManager&) = delete;
	ProjectileManager& operator=(ProjectileManager&&) = delete;

	struct ProcessInWater
	{
		static void create_splash(const T* a_projectile, const RE::NiPoint3& a_pos)
		{
			const auto root = a_projectile->Get3D();
			if (!root || root->GetAppCulled()) {
				return;
			}

			const auto setting = Splash::Settings::GetSingleton();
			auto [enableSplash, enableRipple, displacementMult, modelPath, modelPathFire, modelPathDragon] = setting->GetSplashSetting(type);

			if (enableSplash) {
				const auto rng = ::stl::RNG::GetSingleton();

				bool result = true;
				if constexpr (type == Splash::kFlame || type == Splash::kArrow) {
					if (rng->Generate<std::uint32_t>(0, 2) != 0) {
						result = false;
					}
				}

				if (result) {
					const float radius = root->worldBound.radius;
					if (const auto cell = radius > 0.0f ?
                                              a_projectile->parentCell :
                                              nullptr;
						cell) {
						float scale = 1.0f;

						const auto heavyRadius = setting->GetSplashRadius(Splash::kHeavy);
						const auto mediumRadius = setting->GetSplashRadius(Splash::kMedium);
						const auto lightRadius = setting->GetSplashRadius(Splash::kLight);

						if (radius <= heavyRadius) {
							if (radius <= mediumRadius) {
								if (radius > lightRadius) {
									scale = setting->GetSplashScale(Splash::kLight);
								}
							} else {
								scale = setting->GetSplashScale(Splash::kMedium);
							}
						} else {
							scale = setting->GetSplashScale(Splash::kHeavy);
						}

						if constexpr (type == Splash::kMissile) {
							RE::BSSoundHandle soundHandle{};

							if (const auto audioManager = RE::BSAudioManager::GetSingleton(); audioManager) {
								if (radius <= heavyRadius) {
									if (radius <= mediumRadius) {
										if (radius > lightRadius) {
											static constexpr auto smallSound{ "CWaterSmall" };
											audioManager->BuildSoundDataFromEditorID(soundHandle, smallSound, 17);
										}
									} else {
										static constexpr auto mediumSound{ "CWaterMedium" };
										audioManager->BuildSoundDataFromEditorID(soundHandle, mediumSound, 17);
									}
								} else {
									static constexpr auto largeSound{ "CWaterLarge" };
									audioManager->BuildSoundDataFromEditorID(soundHandle, largeSound, 17);
								}
								if (soundHandle.IsValid()) {
									soundHandle.SetPosition(a_pos);
									soundHandle.Play();
								}
							}
						}

						RE::NiMatrix3 matrix{};
						matrix.SetEulerAnglesXYZ(-0.0f, -0.0f, rng->Generate<float>(-RE::NI_PI, RE::NI_PI));

						const char* modelName;
						float time;

						switch (util::get_effect_type(root)) {
						case 2:
							modelName = modelPathDragon.c_str();
							time = 2.0f;
							break;
						case 1:
							modelName = modelPathFire.c_str();
							time = 1.0f;
							break;
						default:
							modelName = modelPath.c_str();
							time = 1.0f;
							break;
						}

						cell->PlaceParticleEffect(time, modelName, matrix, a_pos, scale, 7, nullptr);
					}
				}
			}

			if (enableRipple) {
				util::create_ripple(a_pos, displacementMult);
			}
		}

		static void thunk(T* a_projectile, float a_delta)
		{
			func(a_projectile, a_delta);

			if (!a_projectile->IsDisabled() && !a_projectile->IsDeleted()) {
				if constexpr (type == Splash::kFlame) {
					const auto data = !a_projectile->impacts.empty() ? a_projectile->impacts.front() : nullptr;
					if (!data) {
						return;
					}

					if (auto material = data->material; material && ::stl::is_in(material->materialID, RE::MATERIAL_ID::kWater, RE::MATERIAL_ID::kWaterPuddle)) {
						return;
					}

					const auto startPos = a_projectile->GetPosition();
					auto endPos = data->desiredTargetLoc;

					const auto waterHeight = util::get_water_height(a_projectile, endPos);
					if (!numeric::approximately_equal(endPos.z, startPos.z)) {
						const auto t = (waterHeight - startPos.z) / (endPos.z - startPos.z);
						endPos.x = (endPos.x - startPos.x) * t + startPos.x;
						endPos.y = (endPos.y - startPos.y) * t + startPos.y;
					}

					if (!numeric::essentially_equal(waterHeight, -RE::NI_INFINITY)) {
						const auto level = (waterHeight - endPos.z) / a_projectile->GetHeight();
						if (level >= 0.1f) {
							endPos.z = waterHeight;
							create_splash(a_projectile, endPos);
						}
					}
				} else {
					if (const auto data = !a_projectile->impacts.empty() ? a_projectile->impacts.front() : nullptr; data) {
						if (const auto material = data->material; material && ::stl::is_in(material->materialID, RE::MATERIAL_ID::kWater, RE::MATERIAL_ID::kWaterPuddle)) {
							return;
						}
					}

					const auto startPos = a_projectile->GetPosition();
					auto [height, level] = util::get_submerged_water_level(a_projectile, startPos);
					if (level >= 0.01f && level < 1.0f) {
						create_splash(a_projectile, { startPos.x, startPos.y, height });
					}
				}
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
};

template <class T, Splash::TYPE type>
void ProjectileManager<T, type>::Install()
{
	const auto settings = Splash::Settings::GetSingleton();
	auto [enableSplash, enableRipple] = settings->GetInstallSetting(type);
	if (!enableSplash && !enableRipple) {
		return;
	}

	::stl::write_vfunc<T, ProcessInWater>(0x0AB);

	//disabling cone water splash
	if constexpr (type == Splash::kCone) {
		REL::Relocation<std::uintptr_t> target{ REL::ID(42638), 0x48D };
		REL::safe_write(target.address(), std::uint8_t{ 0xEB });
	}

	logger::info("Installed {}"sv, typeid(ProjectileManager).name());
}

class ExplosionManager
{
public:
	static void Install()
	{
		const auto settings = Splash::Settings::GetSingleton();
		auto [enableSplash, enableRipple] = settings->GetInstallSetting(Splash::kExplosion);
		if (!enableSplash && !enableRipple) {
			return;
		}

		REL::Relocation<std::uintptr_t> target{ REL::ID(42696) };
		::stl::write_thunk_call<UpdateSound>(target.address() + 0x33C);

		//skip vanilla explosion particle spawn (waste of emptyFX)
		constexpr std::uint8_t JMP[] = { 0x90, 0xE9 };
		REL::safe_write(target.address() + 0x3A3, JMP, 2);
	}

private:
	struct UpdateSound
	{
		static void thunk(RE::Explosion* a_explosion)
		{
			func(a_explosion);

			const auto cell = a_explosion->parentCell;
			const auto root = cell && a_explosion->flags.any(RE::Explosion::Flags::kInWater) ? a_explosion->Get3D() : nullptr;

			if (root) {
				const auto setting = Splash::Settings::GetSingleton();
				auto [enable, fireOnly, displacementMult, modelPath, modelPathFire, modelPathDragon] = setting->GetSplashSetting(Splash::kExplosion);

				const auto startPos = a_explosion->GetPosition();
				const RE::NiPoint3 pos{ startPos.x, startPos.y, util::get_water_height(a_explosion, startPos) };

				const auto type = util::get_effect_type(root);
				if (!fireOnly || type == Splash::kCone || type == Splash::kFlame) {
					root->SetAppCulled(true);

					const char* modelName;
					float time;

					switch (type) {
					case 2:
						modelName = modelPathDragon.c_str();
						time = 2.0f;
						break;
					case 1:
						modelName = modelPathFire.c_str();
						time = 1.0f;
						break;
					default:
						modelName = modelPath.c_str();
						time = 1.0f;
					}

					const auto scale = a_explosion->radius / setting->GetExplosionSplashRadius();

					RE::NiMatrix3 matrix{};
					matrix.SetEulerAnglesXYZ(-0.0f, -0.0f, ::stl::RNG::GetSingleton()->Generate<float>(-RE::NI_PI, RE::NI_PI));

					if (const auto effect = cell->PlaceParticleEffect(time, modelName, matrix, pos, scale, 7, nullptr); effect) {
						RE::BSSoundHandle soundHandle{};

						if (const auto audioManager = RE::BSAudioManager::GetSingleton()) {
							static constexpr auto explosionSound{ "CWaterExplosionSplash" };
							audioManager->BuildSoundDataFromEditorID(soundHandle, explosionSound, 17);
						}

						if (soundHandle.IsValid()) {
							soundHandle.SetPosition(pos);
							soundHandle.Play();
						}
					}
				}

				util::create_ripple(pos, displacementMult);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
};

//TBD
class RainManager
{
public:
	static void Install()
	{
		::stl::write_thunk_call<ToggleWaterRipples>(target.address());

		logger::info("installed rain manager");
	}

private:
	struct ToggleWaterRipples
	{
		static std::uint32_t thunk(RE::BSAudioManager* a_manager, RE::NiAVObject* a_object, RE::NiAVObject* a_object2)
		{
			return func(a_manager, a_object, a_object2);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
};

class DisplacementManager
{
public:
	static void Install()
	{
		const auto settings = Splash::Settings::GetSingleton();
		if (!settings->GetPatchDisplacement()) {
			return;
		}

		using Flag = RE::TESObjectACTI::ActiFlags;

		std::uint32_t count = 0;
		const auto dataHandler = RE::TESDataHandler::GetSingleton();
		for (const auto& activator : dataHandler->GetFormArray<RE::TESObjectACTI>()) {
			if (activator && activator->IsWater() && !activator->QHasCurrents() && !activator->GetRandomAnim()) {
				activator->flags.reset(Flag::kNoDisplacement);
				count++;
			}
		}

		logger::info("Patching {} water records to use displacement", count);

		const auto settingCollection = RE::INISettingCollection::GetSingleton();
		const auto setting = settingCollection ?
                                 settingCollection->GetSetting("bUseBulletWaterDisplacements") :
                                 nullptr;

		if (setting) {
			setting->data.b = true;
		}
	}
};
