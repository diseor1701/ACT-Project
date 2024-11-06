#pragma once
#include "Component.h"

class Mesh;
class Shader;
class Material;

#define MAX_MESH_INSTANCE 500

class MeshRenderer : public Component
{
	using Super = Component;
public:
	MeshRenderer();
	virtual ~MeshRenderer();

	void SetMesh(shared_ptr<Mesh> mesh) { _mesh = mesh; }
	void SetMaterial(shared_ptr<Material> material) { _material = material; }
	void SetPass(uint8 pass) { _pass = pass; }
	void SetAlphaBlend(bool isAlphaBlend) { _isAlphaBlend = isAlphaBlend; }

	void RenderInstancing(shared_ptr<class InstancingBuffer>& buffer);
	void RenderSingle();
	InstanceID GetInstanceID();

private:
	shared_ptr<Mesh> _mesh;
	shared_ptr<Material> _material;
	uint8 _pass = 0;
	bool _isAlphaBlend = false;
};