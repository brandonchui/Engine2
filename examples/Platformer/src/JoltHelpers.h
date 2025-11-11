#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// Avoid naming collisions with Forge
using namespace JPH::literals;

namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

// Should objects collide?
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inObject1,
							   JPH::ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			return false;
		}
	}
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
	JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];

public:
	BPLayerInterfaceImpl()
	{
		mObjectToBroadPhase[Layers::NON_MOVING] = JPH::BroadPhaseLayer(0);
		mObjectToBroadPhase[Layers::MOVING] = JPH::BroadPhaseLayer(1);
	}

	virtual JPH::uint GetNumBroadPhaseLayers() const override { return 2; }

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
	{
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
	{
		switch ((JPH::BroadPhaseLayer::Type)inLayer)
		{
		case (JPH::BroadPhaseLayer::Type)JPH::BroadPhaseLayer(0):
			return "NON_MOVING";
		case (JPH::BroadPhaseLayer::Type)JPH::BroadPhaseLayer(1):
			return "MOVING";
		default:
			return "INVALID";
		}
	}
#endif
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1,
							   JPH::BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == JPH::BroadPhaseLayer(1);
		case Layers::MOVING:
			return true;
		default:
			return false;
		}
	}
};

inline JPH::PhysicsSystem*
CreatePhysicsSystem(BPLayerInterfaceImpl& broad_phase_layer_interface,
					ObjectVsBroadPhaseLayerFilterImpl& object_vs_broadphase,
					ObjectLayerPairFilterImpl& object_vs_object)
{
	JPH::PhysicsSystem* pPhysicsSystem = new JPH::PhysicsSystem();
	pPhysicsSystem->Init(1024, 0, 1024, 1024, broad_phase_layer_interface, object_vs_broadphase,
						 object_vs_object);
	return pPhysicsSystem;
}

inline JPH::BodyID CreateFloor(JPH::PhysicsSystem* pPhysicsSystem, float width, float thickness,
							   float depth, float yPos)
{
	JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(width, thickness, depth));
	JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	JPH::ShapeRefC floor_shape = floor_shape_result.Get();

	JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0_r, yPos, 0.0_r),
											 JPH::Quat::sIdentity(), JPH::EMotionType::Static,
											 Layers::NON_MOVING);

	JPH::Body* floor = pPhysicsSystem->GetBodyInterface().CreateBody(floor_settings);
	pPhysicsSystem->GetBodyInterface().AddBody(floor->GetID(), JPH::EActivation::DontActivate);
	return floor->GetID();
}

inline JPH::BodyID CreateDynamicBox(JPH::PhysicsSystem* pPhysicsSystem, float sizeX, float sizeY,
									float sizeZ, float posX, float posY, float posZ)
{
	JPH::BoxShapeSettings box_shape_settings(JPH::Vec3(sizeX, sizeY, sizeZ));
	JPH::ShapeSettings::ShapeResult box_shape_result = box_shape_settings.Create();
	JPH::ShapeRefC box_shape = box_shape_result.Get();

	JPH::BodyCreationSettings box_settings(box_shape, JPH::RVec3(posX, posY, posZ),
										   JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic,
										   Layers::MOVING);

	return pPhysicsSystem->GetBodyInterface().CreateAndAddBody(box_settings,
															   JPH::EActivation::Activate);
}
