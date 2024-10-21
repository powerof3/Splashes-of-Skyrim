#include "Manager.h"

namespace Splashes
{
    void InstallOnPostLoad()
    {
		Settings::GetSingleton()->LoadSettings();
		
		ProjectileManager<RE::MissileProjectile, kMissile>::Install();
		ProjectileManager<RE::FlameProjectile, kFlame>::Install();
		ProjectileManager<RE::ConeProjectile, kCone>::Install();
		ProjectileManager<RE::ArrowProjectile, kArrow>::Install();
		ProjectileManager<RE::BeamProjectile, kBeam>::Install();

		ExplosionManager::Install();
    }

    void InstallOnDataLoad()
    {
		DisplacementManager::Install();
    }
}
