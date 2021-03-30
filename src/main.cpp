#include "Settings.h"
#include "version.h"


namespace Splash
{
	using namespace SKSE::UTIL::FLOAT;
	using namespace SKSE::UTIL::STRING;


	std::uint32_t GetEffectType(RE::NiAVObject* a_object)
	{
		static auto constexpr Fire{ "fire"sv };
		static auto constexpr Flame{ "flame"sv };
		static auto constexpr Dragon{ "dragon"sv };

		std::string name(a_object->name);
		if (insenstiveStringFind(name, Fire) || insenstiveStringFind(name, Flame)) {
			if (insenstiveStringFind(name, Dragon)) {
				return 2;
			}
			return 1;
		}
		return 0;
	}


	float IsWithinBoundingBox(RE::BGSWaterSystemManager* a_manager, const RE::NiPoint3& a_pos)
	{
		auto settings = Settings::GetSingleton();
		for (auto& waterObject : a_manager->waterObjects) {
			if (waterObject) {
				if (!settings->GetAllowDamageWater()) {
					auto waterType = waterObject->waterForm;
					if (waterType && waterType->GetDangerous()) {
						break;
					}
				}
				for (auto& bound : waterObject->multiBounds) {
					if (bound) {
						auto size{ bound->size };
						auto center{ bound->center };
						if (size.z <= 10.0f) {	//avoid sloped water
							auto boundMin{ center - size };
							auto boundMax{ center + size };
							if (auto result = a_pos.x > boundMin.x && a_pos.x < boundMax.x && a_pos.y > boundMin.y && a_pos.y < boundMax.y; result) {
								return center.z;
							}
						}
					}
				}
			}
		}

		return -RE::NI_INFINITY;
	}


	float GetWaterHeight(RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos)
	{
		auto waterManager = RE::BGSWaterSystemManager::GetSingleton();
		if (waterManager && waterManager->state) {
			auto waterHeight = a_ref->GetWaterHeight();
			if (waterHeight != -RE::NI_INFINITY) {
				return waterHeight;
			}

			waterHeight = IsWithinBoundingBox(waterManager, a_pos);

			auto data = a_ref->loadedData;
			if (data) {
				data->relevantWaterHeight = waterHeight;
			}

			return waterHeight;
		}

		return -RE::NI_INFINITY;
	}


	std::pair<float, float> GetSubmergedWaterLevel(RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos)
	{
		auto waterHeight = GetWaterHeight(a_ref, a_pos);

		if (waterHeight == -RE::NI_INFINITY || waterHeight <= a_pos.z) {
			return std::make_pair(waterHeight, 0.0f);
		}

		auto level = (waterHeight - a_pos.z) / a_ref->GetHeight();
		return std::make_pair(waterHeight, level <= 1.0f ? level : 1.0f);
	}


	void CreateWaterRipple(const RE::NiPoint3& a_pos, float a_displacementMult)
	{
		auto const displacement = 0.0099999998f * a_displacementMult;

		if (RE::BSTaskPool::ShouldUseTaskPool()) {
			auto taskPool = RE::BSTaskPool::GetSingleton();
			if (taskPool) {
				taskPool->QueueBulletWaterDisplacementTask(displacement, a_pos);
			}
		} else {
			auto waterManager = RE::BGSWaterSystemManager::GetSingleton();
			if (waterManager) {
				waterManager->CreateBulletWaterDisplacement(a_pos, displacement);
			}
		}
	}


	void CreateWaterSplash(RE::Projectile* a_ref, const RE::NiPoint3& a_pos, Splash::TYPE a_type, bool a_noSound = false)
	{
		auto root = a_ref->Get3D();
		if (!root || root->GetAppCulled()) {
			return;
		}

		auto setting = Settings::GetSingleton();
		auto [enableSplash, enableRipple, displacementMult, modelPath, modelPathFire, modelPathDragon] = setting->GetSplashSetting(a_type);

		if (enableSplash) {
			auto RNG = SKSE::RNG::GetSingleton();

			bool result = true;
			if (a_type == Splash::TYPE::kFlame || a_type == Splash::TYPE::kArrow) {
				auto chance = RNG->Generate<std::uint32_t>(0, 2);
				if (chance != 0) {
					result = false;
				}
			}

			if (result) {
				float radius = root->worldBound.radius;
				if (radius > 0.0f) {
					auto cell = a_ref->parentCell;
					if (cell) {
						float scale = 1.0f;

						auto heavyRadius = setting->GetSplashRadius(kHeavy);
						auto mediumRadius = setting->GetSplashRadius(kMedium);
						auto lightRadius = setting->GetSplashRadius(kLight);

						RE::BSSoundHandle soundHandle{};

						if (radius <= heavyRadius) {
							if (radius <= mediumRadius) {
								if (radius > lightRadius) {
									scale = setting->GetSplashScale(kLight);

									if (auto audioManager = RE::BSAudioManager::GetSingleton(); audioManager) {
										audioManager->BuildSoundDataFromEditorID(soundHandle, "CWaterSmall", 17);
									}
								}
							} else {
								scale = setting->GetSplashScale(kMedium);

								if (auto audioManager = RE::BSAudioManager::GetSingleton(); audioManager) {
									audioManager->BuildSoundDataFromEditorID(soundHandle, "CWaterMedium", 17);
								}
							}
						} else {
							scale = setting->GetSplashScale(kHeavy);
							auto audioManager = RE::BSAudioManager::GetSingleton();
							if (audioManager) {
								audioManager->BuildSoundDataFromEditorID(soundHandle, "CWaterLarge", 17);
							}
						}

						if (!a_noSound && soundHandle.IsValid()) {
							soundHandle.SetPosition(a_pos.x, a_pos.y, a_pos.z);
							soundHandle.Play();
						}

						RE::NiMatrix3 matrix;
						matrix.SetEulerAnglesXYZ(-0.0f, -0.0f, RNG->Generate<float>(-RE::NI_PI, RE::NI_PI));

						const char* modelName = nullptr;
						float time = 1.0f;

						switch (GetEffectType(root)) {
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
		}

		if (enableRipple) {
			CreateWaterRipple(a_pos, displacementMult);
		}
	}
}


class ProjectileRange
{
public:
	static void Hook()
	{
		REL::Relocation<std::uintptr_t> ProjectileInit{ REL::ID(43030) };

		auto& trampoline = SKSE::GetTrampoline();
		_UpdateCombatThreat = trampoline.write_call<5>(ProjectileInit.address() + 0x3CB, UpdateCombatThreat);
	}

private:
	static void UpdateCombatThreat(/*RE::CombatManager::CombatThreats*/ std::uintptr_t a_threats, RE::Projectile* a_projectile)
	{
		using Type = RE::FormType;

		if (a_projectile && a_projectile->Is(Type::ProjectileMissile) || a_projectile->Is(Type::ProjectileCone)) {
			const auto base = a_projectile->GetBaseObject();
			const auto projectileBase = base ? base->As<RE::BGSProjectile>() : nullptr;
			if (projectileBase) {
				const auto baseSpeed = projectileBase->data.speed;
				if (baseSpeed > 0.0f) {
					const auto velocity = a_projectile->linearVelocity;
					a_projectile->range *= velocity.Length() / baseSpeed;
				}
			}
		}

		_UpdateCombatThreat(a_threats, a_projectile);
	}
	static inline REL::Relocation<decltype(UpdateCombatThreat)> _UpdateCombatThreat;
};


class Missile
{
public:
	static void Hook()
	{
		auto settings = Splash::Settings::GetSingleton();
		auto [enableSplash, enableRipple, displacementMult, modelPath, modelPathFire, modelPathDragon] = settings->GetSplashSetting(Splash::kMissile);
		if (!enableSplash && !enableRipple) {
			return;
		}

		logger::info("Hooked missile projectiles");

		REL::Relocation<std::uintptr_t> vtbl{ REL::ID(263942) };  //Missile vtbl
		_UpdateImpl = vtbl.write_vfunc(0x0AB, UpdateImpl);
	}

private:
	Missile() = default;

	static void UpdateImpl(RE::MissileProjectile* a_this, float a_delta)
	{
		using namespace SKSE::UTIL::FLOAT;

		_UpdateImpl(a_this, a_delta);

		if (!a_this->IsDisabled() && !a_this->IsDeleted()) {
			auto startPos{ a_this->GetPosition() };
			auto [height, level] = Splash::GetSubmergedWaterLevel(a_this, startPos);
			if (level >= 0.01f && level < 1.0f) {
				RE::NiPoint3 pos{ startPos.x, startPos.y, height };
				Splash::CreateWaterSplash(a_this, pos, Splash::kMissile);
			}
		}
	}

	using UpdateImpl_t = decltype(&RE::MissileProjectile::UpdateImpl);	// 0AE
	static inline REL::Relocation<UpdateImpl_t> _UpdateImpl;
};


class Flame
{
public:
	static void Hook()
	{
		auto settings = Splash::Settings::GetSingleton();
		auto [enableSplash, enableRipple, displacementMult, modelPath, modelPathFire, modelPathDragon] = settings->GetSplashSetting(Splash::kFlame);
		if (!enableSplash && !enableRipple) {
			return;
		}

		logger::info("Hooked flame projectiles");

		REL::Relocation<std::uintptr_t> flameVtbl{ REL::ID(263884) };  //Flame vtbl
		_UpdateImpl = flameVtbl.write_vfunc(0x0AB, UpdateImpl);
	}

private:
	Flame() = default;

	static void UpdateImpl(RE::FlameProjectile* a_this, float a_delta)
	{
		using namespace SKSE::UTIL::FLOAT;

		_UpdateImpl(a_this, a_delta);

		if (!a_this->IsDisabled() && !a_this->IsDeleted()) {
			auto data = !a_this->impacts.empty() ? a_this->impacts.front() : nullptr;
			if (data) {
				auto startPos{ a_this->GetPosition() };
				auto endPos{ data->desiredTargetLoc };

				auto waterHeight = Splash::GetWaterHeight(a_this, endPos);
				if (endPos.z != startPos.z) {
					auto t = (waterHeight - startPos.z) / (endPos.z - startPos.z);
					endPos.x = (endPos.x - startPos.x) * t + startPos.x;
					endPos.y = (endPos.y - startPos.y) * t + startPos.y;
				}

				if (waterHeight != -RE::NI_INFINITY) {
					auto level = (waterHeight - endPos.z) / a_this->GetHeight();
					if (level >= 0.1f) {
						endPos.z = waterHeight;
						Splash::CreateWaterSplash(a_this, endPos, Splash::kFlame, true);
					}
				}
			}
		}
	}
	using UpdateImpl_t = decltype(&RE::FlameProjectile::UpdateImpl);  // 0AE
	static inline REL::Relocation<UpdateImpl_t> _UpdateImpl;
};


class Cone
{
public:
	static void Hook()
	{
		auto settings = Splash::Settings::GetSingleton();
		auto [enableSplash, enableRipple, displacementMult, modelPath, modelPathFire, modelPathDragon] = settings->GetSplashSetting(Splash::kCone);
		if (!enableSplash && !enableRipple) {
			return;
		}

		logger::info("Hooked cone projectiles");

		REL::Relocation<std::uintptr_t> vtbl{ REL::ID(263822) };  //Cone vtbl
		_UpdateImpl = vtbl.write_vfunc(0x0AB, UpdateImpl);

		auto& trampoline = SKSE::GetTrampoline();
		REL::Relocation<std::uintptr_t> DoCollisionHitUpdate{ REL::ID(42638) };
		trampoline.write_call<5>(DoCollisionHitUpdate.address() + 0x4C3, CreateWaterSplash_Cone);
	}

private:
	Cone() = default;

	static void UpdateImpl(RE::ConeProjectile* a_this, float a_delta)
	{
		_UpdateImpl(a_this, a_delta);

		if (!a_this->IsDisabled() && !a_this->IsDeleted()) {
			auto [height, level] = Splash::GetSubmergedWaterLevel(a_this, a_this->GetPosition());
			if (level >= 0.01f && level < 1.0f) {
				RE::NiPoint3 pos{ a_this->GetPositionX(), a_this->GetPositionY(), height };
				Splash::CreateWaterSplash(a_this, pos, Splash::kCone, true);
			}
		}
	}
	using UpdateImpl_t = decltype(&RE::ConeProjectile::UpdateImpl);	 // 0AE
	static inline REL::Relocation<UpdateImpl_t> _UpdateImpl;


	static void CreateWaterSplash_Cone([[maybe_unused]] float a_radius, [[maybe_unused]] const RE::NiPoint3& a_pos)
	{
		return;
	}
};


class Arrow
{
public:
	static void Hook()
	{
		auto settings = Splash::Settings::GetSingleton();
		auto [enableSplash, enableRipple, displacementMult, modelPath, modelPathFire, modelPathDragon] = settings->GetSplashSetting(Splash::kArrow);
		if (!enableSplash && !enableRipple) {
			return;
		}

		logger::info("Hooked arrow projectiles");

		REL::Relocation<std::uintptr_t> vtbl{ REL::ID(263776) };  //Arrow vtbl
		_UpdateImpl = vtbl.write_vfunc(0x0AB, UpdateImpl);
	}

private:
	Arrow() = default;

	static void UpdateImpl(RE::ArrowProjectile* a_this, float a_delta)
	{
		_UpdateImpl(a_this, a_delta);

		if (!a_this->IsDisabled() && !a_this->IsDeleted()) {
			auto [height, level] = Splash::GetSubmergedWaterLevel(a_this, a_this->GetPosition());
			if (level >= 0.01f && level < 1.0f) {
				RE::NiPoint3 pos{ a_this->GetPositionX(), a_this->GetPositionY(), height };
				Splash::CreateWaterSplash(a_this, pos, Splash::kArrow, true);
			}
		}
	}
	using UpdateImpl_t = decltype(&RE::ArrowProjectile::UpdateImpl);  // 0AE
	static inline REL::Relocation<UpdateImpl_t> _UpdateImpl;
};


class Explosion
{
public:
	static void Hook()
	{
		auto settings = Splash::Settings::GetSingleton();
		auto [enableSplash, enableRipple, displacementMult, modelPath, modelPathFire, modelPathDragon] = settings->GetSplashSetting(Splash::kExplosion);
		if (!enableSplash && !enableRipple) {
			return;
		}

		logger::info("Hooked explosions");

		REL::Relocation<std::uintptr_t> ExplosionUpdate{ REL::ID(42696) };

		auto& trampoline = SKSE::GetTrampoline();
		_UpdateSound = trampoline.write_call<5>(ExplosionUpdate.address() + 0x33C, UpdateSound);

		constexpr std::array<std::uint8_t, 2> JMP{ 0x90, 0xE9 };
		REL::safe_write(ExplosionUpdate.address() + 0x3A3, JMP);  //skip vanilla explosion particle spawn (waste if emptyFX)
	}

private:
	static void UpdateSound(RE::Explosion* a_explosion)
	{
		_UpdateSound(a_explosion);

		auto cell = a_explosion->parentCell;
		if (cell && a_explosion->flags.any(RE::Explosion::Flags::kInWater)) {
			auto root = a_explosion->Get3D();
			if (root) {
				auto setting = Splash::Settings::GetSingleton();
				auto [enable, fireOnly, displacementMult, modelPath, modelPathFire, modelPathDragon] = setting->GetSplashSetting(Splash::kExplosion);

				auto startPos{ a_explosion->GetPosition() };
				RE::NiPoint3 pos{ startPos.x, startPos.y, Splash::GetWaterHeight(a_explosion, startPos) };
				
				auto type = Splash::GetEffectType(root);
				if (!fireOnly || type == 2 || type == 1) {
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

					auto scale = a_explosion->radius / 250.0f;

					RE::NiMatrix3 matrix;
					matrix.SetEulerAnglesXYZ(-0.0f, -0.0f, SKSE::RNG::GetSingleton()->Generate<float>(-RE::NI_PI, RE::NI_PI));

					auto effect = cell->PlaceParticleEffect(time, modelName, matrix, pos, scale, 7, nullptr);
					if (effect) {
						RE::BSSoundHandle soundHandle{};

						auto audioManager = RE::BSAudioManager::GetSingleton();
						if (audioManager) {
							static constexpr auto explosionSound{ "CWaterExplosionSplash" };
							audioManager->BuildSoundDataFromEditorID(soundHandle, explosionSound, 17);
						}

						if (soundHandle.IsValid()) {
							soundHandle.SetPosition(pos.x, pos.y, pos.z);
							soundHandle.Play();
						}
					}
				}

				Splash::CreateWaterRipple(pos, displacementMult);
			}
		}
	}
	static inline REL::Relocation<decltype(UpdateSound)> _UpdateSound;
};


class Displacement
{
public:
	static void Patch()
	{
		auto settings = Splash::Settings::GetSingleton();
		if (!settings->GetPatchDisplacement()) {
			return;
		}

		logger::info("Patching water records to use displacement");

		using Flag = RE::TESObjectACTI::ActiFlags;

		auto dataHandler = RE::TESDataHandler::GetSingleton();
		for (auto& activator : dataHandler->GetFormArray<RE::TESObjectACTI>()) {
			if (activator && activator->IsWater() && !activator->QHasCurrents() && !activator->GetRandomAnim()) {
				activator->flags.reset(Flag::kNoDisplacement);
			}
		}
	}
};


void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			Displacement::Patch();

			auto settingCollection = RE::INISettingCollection::GetSingleton();
			if (settingCollection) {
				auto setting = settingCollection->GetSetting("bUseBulletWaterDisplacements");
				if (setting) {
					setting->data.b = true;
				}
			}
		}
		break;
	default:
		break;
	}
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	try {
		auto path = logger::log_directory().value() / "po3_SplashesOfSkyrim.log";
		auto log = spdlog::basic_logger_mt("global log", path.string(), true);
		log->flush_on(spdlog::level::info);

#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
		log->sinks().push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
		log->set_level(spdlog::level::info);

#endif
		set_default_logger(log);
		spdlog::set_pattern("[%H:%M:%S] %v");

		logger::info("po3_SplashesOfSkyrim v{}", VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "Splashes Of Skyrim";
		a_info->version = VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			logger::critical("Loaded in editor, marking as incompatible");
			return false;
		}

		const auto ver = a_skse->RuntimeVersion();
		if (ver < SKSE::RUNTIME_1_5_39) {
			logger::critical("Unsupported runtime version {}", ver.string());
			return false;
		}
	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	try {
		logger::info("po3_SplashesOfSkyrim loaded");

		Init(a_skse);
		SKSE::AllocTrampoline(30);

		auto settings = Splash::Settings::GetSingleton();
		settings->LoadSettings();

		auto messaging = SKSE::GetMessagingInterface();
		if (!messaging->RegisterListener("SKSE", OnInit)) {
			return false;
		}

		//ProjectileRange::Hook();
		//Beam::Hook();

		Missile::Hook();
		Flame::Hook();
		Arrow::Hook();
		Cone::Hook();
		Explosion::Hook();

	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}