#pragma once

#include "GameEntity.h"

#include <memory>
#include <PxPhysicsAPI.h>

#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL;	}

#ifdef DEBUG
#define USE_PVD	 true;
#define PVD_HOST "127.0.0.1" //Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.
#endif // DEBUG

#ifndef USE_PVD
#define USE_PVD  false;
#endif // !USE_PVD


class PhysXManager
{
private:
	static std::shared_ptr<PhysXManager> s_instance;
	PhysXManager();

	// Based on variables from PhysX SnippetHelloWorld.cpp
	physx::PxDefaultAllocator		m_allocator;
	physx::PxDefaultErrorCallback	m_errorCallback;

	physx::PxFoundation* m_foundation = NULL;
	physx::PxPhysics*	 m_physics	  = NULL;

	physx::PxDefaultCpuDispatcher* m_dispatcher = NULL;
	physx::PxScene*				   m_scene		= NULL;

	physx::PxMaterial* m_materialTest = NULL;

#if USE_PVD
	physx::PxPvd* m_pvd = NULL;
#endif //  USE_PVD

	physx::PxReal stackZ = 10.0f;
	DirectX::XMFLOAT3 m_gravity = { 0.0f,9.81f,0.0f };

	void Init();
	void CreateStack(const PxTransform& t, PxU32 size, PxReal halfExtent);
	PxRigidDynamic* CreateDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity = PxVec3(0));
	
public:
	~PhysXManager();

	static std::shared_ptr<PhysXManager> GetInstance();

	void UpdatePhysics(float deltaTime, bool interactive = false);
};

