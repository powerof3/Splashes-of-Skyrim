#pragma once

#include "Settings.h"

namespace Splashes
{
	struct util
	{
		static FIRE_TYPE               get_fire_type(const RE::NiAVObject* a_object);
		static float                   get_water_height(const RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos);
		static std::pair<float, float> get_submerged_water_level(const RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos);
		static void                    create_ripple(const RE::NiPoint3& a_pos, float a_displacementMult);
	};

	inline bool CSInstalled{ false };

	template <class T, TYPE type>
	class ProjectileManager
	{
	public:
		static void Install()
		{
			auto [enableSplash, enableRipple] = Settings::GetSingleton()->GetInstalled(type);
			if (!enableSplash && !enableRipple) {
				return;
			}

			stl::write_vfunc<T, Update>(0x0AB);

			//disabling cone water splash
			if constexpr (type == kCone) {
				REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(42638, 43806), OFFSET(0x48D, 0x3B7) };
				REL::WriteSafeData(target.address(), std::uint8_t{ 0xEB });
			}

			REX::INFO("Installed {}"sv, typeid(ProjectileManager).name());
		}

	private:
		struct Update
		{
			static void thunk(T* a_projectile, float a_delta)
			{
				func(a_projectile, a_delta);

				if (!a_projectile->IsDisabled() && !a_projectile->IsDeleted()) {
					if constexpr (type == kFlame || type == kBeam) {
						RE::NiPoint3 startPos = a_projectile->GetPosition();
						RE::NiPoint3 endPos;
						if constexpr (type == kFlame) {
							const auto data = !a_projectile->impacts.empty() ? a_projectile->impacts.front() : nullptr;
							if (!data) {
								return;
							}
							if (auto material = data->material; material && (material->materialID == RE::MATERIAL_ID::kWater || material->materialID == RE::MATERIAL_ID::kWaterPuddle)) {
								return;
							}
							endPos = data->desiredTargetLoc;
						} else {
							if (const auto data = !a_projectile->impacts.empty() ? a_projectile->impacts.front() : nullptr) {
								endPos = data->desiredTargetLoc;
							} else {
								const auto root = a_projectile->Get3D();
								if (!root) {
									return;
								}
								const auto beamEnd = root->GetObjectByName("BeamEnd");
								if (!beamEnd) {
									return;
								}
								endPos = beamEnd->world.translate;
							}
						}
						const auto waterHeight = util::get_water_height(a_projectile, endPos);
						if (!REX::FLT::APPROXIMATELY_EQUAL(endPos.z, startPos.z)) {
							const auto t = (waterHeight - startPos.z) / (endPos.z - startPos.z);
							endPos.x = (endPos.x - startPos.x) * t + startPos.x;
							endPos.y = (endPos.y - startPos.y) * t + startPos.y;
						}
						if (!REX::FLT::ESSENTIALLY_EQUAL(waterHeight, -RE::NI_INFINITY)) {
							const auto level = (waterHeight - endPos.z) / a_projectile->GetHeight();
							if (level >= 0.1f) {
								if constexpr (type == kBeam) {  // beam isn't focused when hitting water
									auto rng = REX::TRandom<float>();
									endPos.x += rng.Generate(-20.0f, 20.0f);
									endPos.y += rng.Generate(-20.0f, 20.0f);
								}
								endPos.z = waterHeight;
								create_splash(a_projectile, endPos);
							}
						}
					} else {
						if (const auto data = !a_projectile->impacts.empty() ? a_projectile->impacts.front() : nullptr; data) {
							if (const auto material = data->material; material && (material->materialID == RE::MATERIAL_ID::kWater || material->materialID == RE::MATERIAL_ID::kWaterPuddle)) {
								return;
							}
						}

						const auto startPos = a_projectile->GetPosition();
						auto [height, level] = util::get_submerged_water_level(a_projectile, startPos);
						if (level >= 0.01f && level < 1.0f) {
							RE::NiPoint3 splashPos{ startPos.x, startPos.y, height };
							create_splash(a_projectile, splashPos);
						}
					}
				}
			}
			static inline REL::Relocation<decltype(thunk)> func;

		private:
			static void create_splash(const T* a_projectile, const RE::NiPoint3& a_pos)
			{
				const auto root = a_projectile->Get3D();
				if (!root || root->GetAppCulled()) {
					return;
				}

				const auto setting = Settings::GetSingleton();
				const auto projectile = setting->GetProjectileSetting(type);

				if (projectile->enableSplash) {
					const float radius = root->worldBound.radius;
					if (const auto cell = radius > 0.0f ?
					                          a_projectile->GetParentCell() :
					                          nullptr;
						cell) {
						float scale = 1.0f;

						if constexpr (type != kBeam) {
							const auto heavyRadius = setting->GetSplashRadius(kHeavy);
							const auto mediumRadius = setting->GetSplashRadius(kMedium);
							const auto lightRadius = setting->GetSplashRadius(kLight);

							if (radius <= heavyRadius) {
								if (radius <= mediumRadius) {
									if (radius > lightRadius) {
										scale = setting->GetSplashScale(kLight);
									}
								} else {
									scale = setting->GetSplashScale(kMedium);
								}
							} else {
								scale = setting->GetSplashScale(kHeavy);
							}

							if constexpr (type == kMissile) {
								RE::BSSoundHandle soundHandle{};

								if (const auto audioManager = RE::BSAudioManager::GetSingleton(); audioManager) {
									if (radius <= heavyRadius) {
										if (radius <= mediumRadius) {
											if (radius > lightRadius) {
												audioManager->GetSoundHandleByName(soundHandle, "CWaterSmall", 17);
											}
										} else {
											audioManager->GetSoundHandleByName(soundHandle, "CWaterMedium", 17);
										}
									} else {
										audioManager->GetSoundHandleByName(soundHandle, "CWaterLarge", 17);
									}
									if (soundHandle.IsValid()) {
										soundHandle.SetPosition(a_pos);
										soundHandle.Play();
									}
								}
							}
						}

						RE::NiMatrix3 matrix{};
						matrix.SetEulerAnglesXYZ(-0.0f, -0.0f, REX::TRandom<float>().Generate(-RE::NI_PI, RE::NI_PI));

						const std::string* modelName = nullptr;
						float              time = 1.0f;

						if constexpr (type == kMissile || type == kCone || type == kFlame) {
							const auto fireType = util::get_fire_type(root);
							modelName = &projectile->GetModel(fireType);
							time = Projectile::GetDuration(fireType);
						} else {
							modelName = &projectile->GetModel(FIRE_TYPE::kNone);
						}

						RE::BSTempEffectParticle::Spawn(cell, time, modelName->c_str(), matrix, a_pos, scale, CSInstalled ? 1 : 7, nullptr);
					}
				}

				if (projectile->enableRipple) {
					util::create_ripple(a_pos, projectile->displacementMult);
				}
			}
		};
	};

	class ExplosionManager
	{
	public:
		static void Install()
		{
			auto [enableSplash, enableRipple] = Settings::GetSingleton()->GetInstalled(kExplosion);
			if (!enableSplash && !enableRipple) {
				return;
			}

			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(42696, 43869) };
#ifndef SKYRIM_AE
			stl::write_thunk_call<UpdateSound>(target.address() + 0x33C);
#else
			stl::write_thunk_call<UpdateSoundHandle>(target.address() + OFFSET_VERSIONED(0x528, 0x545));
#endif

			//skip vanilla explosion particle spawn (waste of emptyFX)
			constexpr std::uint8_t JMP[] = { 0x90, 0xE9 };
			REL::WriteSafe(target.address() + OFFSET_3_VERSIONED(0x3A3, 0x57F, 0x59C), JMP, 2);

			REX::INFO("Installed {}"sv, typeid(ExplosionManager).name());
		}

	private:
#ifndef SKYRIM_AE
		struct UpdateSound
		{
			static void thunk(RE::Explosion* a_explosion)
			{
				func(a_explosion);

				const auto cell = a_explosion->parentCell;
				const auto root = cell && a_explosion->flags.any(RE::Explosion::Flags::kUnderwater) ? a_explosion->Get3D() : nullptr;

				create_explosion(a_explosion, cell, root);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};
#else
		//UpdateSound got inlined
		struct UpdateSoundHandle
		{
			static bool thunk(const RE::BSSoundHandle& a_handle)
			{
				auto result = func(a_handle);

				const auto explosion = REX::ADJUST_POINTER<RE::Explosion>(&a_handle, -0xD8);
				const auto cell = explosion ? explosion->GetParentCell() : nullptr;
				const auto root = cell && explosion->flags.any(RE::Explosion::Flags::kUnderwater) ? explosion->Get3D() : nullptr;

				create_explosion(explosion, cell, root);

				return result;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};
#endif
		static void create_explosion(const RE::Explosion* a_explosion, RE::TESObjectCELL* a_cell, RE::NiAVObject* a_root)
		{
			if (a_root) {
				const auto setting = Settings::GetSingleton();
				const auto explosionSetting = setting->GetExplosion();

				if (!explosionSetting) {
					return;
				}

				const auto         startPos = a_explosion->GetPosition();
				const RE::NiPoint3 pos{ startPos.x, startPos.y, util::get_water_height(a_explosion, startPos) };

				const auto fireType = util::get_fire_type(a_root);
				if (!explosionSetting->fireOnly || fireType != FIRE_TYPE::kNone) {
					a_root->SetAppCulled(true);

					const auto& modelName = explosionSetting->GetModel(fireType);
					const float time = Explosion::GetDuration(fireType);

					const auto scale = a_explosion->radius / setting->GetExplosionSplashRadius();

					RE::NiMatrix3 matrix{};
					matrix.SetEulerAnglesXYZ(-0.0f, -0.0f, REX::TRandom<float>().Generate(-RE::NI_PI, RE::NI_PI));

					if (const auto effect = RE::BSTempEffectParticle::Spawn(a_cell, time, modelName.c_str(), matrix, pos, scale, 7, nullptr)) {
						RE::BSSoundHandle soundHandle{};

						if (const auto audioManager = RE::BSAudioManager::GetSingleton()) {
							audioManager->GetSoundHandleByName(soundHandle, "CWaterExplosionSplash", 17);
						}

						if (soundHandle.IsValid()) {
							soundHandle.SetPosition(pos);
							soundHandle.Play();
						}
					}
				}

				util::create_ripple(pos, explosionSetting->displacementMult);
			}
		}
	};

	void InstallOnPostLoad();

	void InstallOnDataLoad();
}
