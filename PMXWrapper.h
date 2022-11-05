#pragma once

//@author Cassiar Beaver
//@purpose Wrap the saba library pmx class to allow easy testing
#include <DirectXMath.h>
#include <d3d11.h>
#include <memory> //for shared pointers
#include <unordered_map> //to avoid searching in ImGui debug sphere drawing
#include <vector> //for vector
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "PMXModel.h"

class PMXWrapper
{
public:
	PMXWrapper(const char* path, const char* texpath);
	~PMXWrapper();
	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

private:
	std::shared_ptr<saba::PMXModel> model;
};

