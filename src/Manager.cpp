#include "Manager.h"

namespace Splashes
{
	FIRE_TYPE util::get_fire_type(const RE::NiAVObject* a_object)
	{
		std::string_view name = a_object->name;
		
		if (REX::STR::ICONTAINS(name, "fire"sv) || REX::STR::ICONTAINS(name, "flame"sv)) {
			if (REX::STR::ICONTAINS(name, "dragon"sv)) {
				return FIRE_TYPE::kDragon;
			}
			return FIRE_TYPE::kFire;
		}
		return FIRE_TYPE::kNone;
	}

	float util::get_water_height(const RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos)
	{
		float waterHeight = a_ref->GetWaterHeight();

		if (!REX::FLT::ESSENTIALLY_EQUAL(waterHeight, -RE::NI_INFINITY)) {
			return waterHeight;
		}

		if (const auto waterManager = RE::TESWaterSystem::GetSingleton()) {
			const RE::BSSpinLockGuard locker(waterManager->lock);  //serialize against water-system mutations

			const bool allowDamageWater = Settings::GetSingleton()->GetAllowDamageWater();
			
			const auto get_nearest_water_object_height = [&]() {
				for (const auto& waterObjectPtr : waterManager->waterObjects) {
					const auto waterObject = waterObjectPtr.get();  //read slot once to avoid data race
					if (!waterObject) {
						continue;
					}
					if (!allowDamageWater) {
						if (const auto waterForm = waterObject->waterType; waterForm && waterForm->GetDangerous()) {
							continue;
						}
					}
					for (const auto& boundPtr : waterObject->multiBounds) {
						const auto bound = boundPtr.get();
						if (!bound) {
							continue;
						}
						if (auto size{ bound->size }; size.z <= 10.0f) {  //avoid sloped water
							auto       center{ bound->center };
							const auto boundMin = center - size;
							const auto boundMax = center + size;
							if (!(a_pos.x < boundMin.x || a_pos.x > boundMax.x || a_pos.y < boundMin.y || a_pos.y > boundMax.y)) {
								return center.z;
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

	std::pair<float, float> util::get_submerged_water_level(const RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos)
	{
		auto waterHeight = get_water_height(a_ref, a_pos);

		if (REX::FLT::APPROXIMATELY_EQUAL(waterHeight, -RE::NI_INFINITY) || waterHeight <= a_pos.z) {
			return std::make_pair(waterHeight, 0.0f);
		}

		auto level = (waterHeight - a_pos.z) / a_ref->GetHeight();
		return std::make_pair(waterHeight, level <= 1.0f ?
											   level :
											   1.0f);
	}

	void util::create_ripple(const RE::NiPoint3& a_pos, float a_displacementMult)
	{
		const auto displacement = 0.0099999998f * a_displacementMult;

		if (RE::TaskQueueInterface::ShouldUseTaskQueue()) {
			RE::TaskQueueInterface::GetSingleton()->QueueAddRipple(displacement, a_pos);
		} else {
			RE::TESWaterSystem::GetSingleton()->AddRipple(a_pos, displacement);
		}
	}

	void InstallOnPostLoad()
	{
		Settings::GetSingleton()->LoadSettings();

		ProjectileManager<RE::MissileProjectile, kMissile>::Install();
		ProjectileManager<RE::FlameProjectile, kFlame>::Install();
		ProjectileManager<RE::ConeProjectile, kCone>::Install();
		ProjectileManager<RE::ArrowProjectile, kArrow>::Install();
		ProjectileManager<RE::BeamProjectile, kBeam>::Install();

		ExplosionManager::Install();

		CSInstalled = REX::W32::GetModuleHandleA("CommunityShaders.dll") != nullptr;
	}

	void InstallOnDataLoad()
	{
		const auto settings = Settings::GetSingleton();
		if (!settings->GetPatchDisplacement()) {
			return;
		}

		using Flag = RE::TESObjectACTI::ActiFlags;

		std::uint32_t count = 0;
		const auto    dataHandler = RE::TESDataHandler::GetSingleton();
		for (const auto& activator : dataHandler->GetFormArray<RE::TESObjectACTI>()) {
			if (activator && activator->IsWater() && !activator->QHasCurrents() && !activator->GetRandomAnim()) {
				activator->flags.reset(Flag::kNoDisplacement);
				count++;
			}
		}

		REX::INFO("Patching {} water records to use displacement", count);

		if (const auto setting = RE::GetINISetting("bUseBulletWaterDisplacements")) {
			setting->data.b = true;
		}
	}
}
