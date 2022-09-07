//Author: Cassiar Beaver
//this class represents a game object on the screen.
//This handles drawing object as well as transforming it.
//multiple objects can share the same mesh.
#pragma once
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory> //for shared pointers
#include <DirectXMath.h>

#include "Mesh.h"
#include "Transform.h"
#include "Camera.h"
#include "Material.h"

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> in_mesh, std::shared_ptr<Material> in_material, std::shared_ptr<Camera> in_camera);
	~GameEntity();

	//get pointer to mesh
	std::shared_ptr<Mesh> GetMesh();
	//get pointer to transform to allow changes outside
	Transform* GetTransform();

	//get pointer to material 
	std::shared_ptr<Material> GetMaterial();
	//change the material
	void SetMaterial(std::shared_ptr<Material> in_material);

	//will hold draw code
	void Draw();
private:

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Camera> camera;
	Transform transform;
	std::shared_ptr<Material> material;
};