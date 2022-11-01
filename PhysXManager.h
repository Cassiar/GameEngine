#pragma once

#include <memory>

class PhysXManager
{
private:
	static std::shared_ptr<PhysXManager> s_instance;

	PhysXManager();

public:
	~PhysXManager();

	static std::shared_ptr<PhysXManager> GetInstance();
};

