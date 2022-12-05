#include "PhysXManager.h"

using namespace physx;

std::shared_ptr<PhysXManager> PhysXManager::s_instance;

PhysXManager::PhysXManager() 
{ 
	Init(); 
}

PhysXManager::~PhysXManager()
{
	PX_RELEASE(m_scene);
	PX_RELEASE(m_dispatcher);
	PX_RELEASE(m_physics);

#if USE_PVD
	if (m_pvd)
	{
		PxPvdTransport* transport = m_pvd->getTransport();
		m_pvd->release();	
		m_pvd = NULL;
		PX_RELEASE(transport);
	}
#endif

	PX_RELEASE(m_foundation);
}

void PhysXManager::Init()
{
    m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);

// PVD stuff is all Debug only. Connects to the visual debugger
#if USE_PVD
	m_pvd = PxCreatePvd(*m_foundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	m_pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale(), true, m_pvd);
#else
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale(), true);
#endif // USE_PVD

	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(m_gravity.x, m_gravity.y, m_gravity.z);
	m_dispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = m_dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	m_scene = m_physics->createScene(sceneDesc);

#if USE_PVD
	PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
#endif // USE_PVD

	m_materialTest = m_physics->createMaterial(0.5f, 0.5f, 0.6f);

	//PxRigidStatic* groundPlane = PxCreatePlane(*m_physics, PxPlane(0, 1, 0, 0), *m_materialTest);
	//m_scene->addActor(*groundPlane);

	//CreateDynamic(PxTransform(PxVec3(0, 40, 100)), PxSphereGeometry(10), PxVec3(0, -50, -100));

}

std::shared_ptr<PxRigidDynamic> PhysXManager::CreateDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity)
{
	// Sets a custom destructor that might cause a mem leak. PhysX destructors are private so this is the only way to utilize smart pointers
	// I could use raw pointers, but I feel that using raw pointers on objects that are referenced outside of this class will go against our current
	// coding conventions. If this ends up causing performance issues having the bonus Lambda then we can rotate to using raw pointers
	std::shared_ptr<PxRigidDynamic> dynamic(PxCreateDynamic(*m_physics, t, geometry, *m_materialTest, 10.0f), [](PxRigidDynamic* f) { 
		//PX_RELEASE(f); 
		});;

	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	m_scene->addActor(*dynamic);
	return dynamic;
}

std::shared_ptr<PxRigidStatic> PhysXManager::CreateStatic(const PxTransform& t, const PxGeometry& geometry)
{
	std::shared_ptr<PxRigidStatic> rigidStatic(PxCreateStatic(*m_physics, t, geometry, *m_materialTest), [](PxRigidStatic* f)
	{
		//PX_RELEASE(f);
	});;

	m_scene->addActor(*rigidStatic);
	return rigidStatic;
}

std::shared_ptr<PhysXManager> PhysXManager::GetInstance()
{
    if (!s_instance.get()) {
        std::shared_ptr<PhysXManager> newInstance(new PhysXManager());
        s_instance = newInstance;
    }

    return s_instance;
}

void PhysXManager::UpdatePhysics(float deltaTime, bool interactive)
{
	m_scene->simulate(FIXED_UPDATE_TIME);
	m_scene->fetchResults(true);
}
