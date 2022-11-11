#pragma once

#include "Mesh.h"
#include "SabaStructs.h"

class SabaMesh : public Mesh
{
private:
	std::shared_ptr<saba::PMXModel> model;
	bool isPmx;

public:
	SabaMesh(const char* path, const char* texpath, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> in_context);
	//Nothing to clean
	~SabaMesh(){}

	bool IsPmx() override { return true; }

	std::shared_ptr<saba::PMXModel> GetModel() {
		return model;
	}

	void Draw() override;
};

