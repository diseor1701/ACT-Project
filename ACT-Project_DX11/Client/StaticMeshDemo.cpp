#include "pch.h"
#include "StaticMeshDemo.h"
#include "GeometryHelper.h"
#include "Camera.h"
#include "GameObject.h"
#include "CameraScript.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Material.h"
#include "Model.h"
#include "ModelRenderer.h"
#include "Light.h"

void StaticMeshDemo::Init()
{
	RESOURCES->Init();
	_shader = make_shared<Shader>(L"23. RenderDemo.fx");

	// Camera
	{
		auto camera = make_shared<GameObject>();
		camera->GetOrAddTransform()->SetPosition(Vec3{ 0.f, 0.f, -5.f });
		camera->AddComponent(make_shared<Camera>());
		camera->AddComponent(make_shared<CameraScript>());

		CUR_SCENE->Add(camera);
	}
	// Light
	{
		auto light = make_shared<GameObject>();
		light->AddComponent(make_shared<Light>());
		LightDesc lightDesc;
		lightDesc.ambient = Vec4(0.4f);
		lightDesc.diffuse = Vec4(1.f);
		lightDesc.specular = Vec4(0.1f);
		lightDesc.direction = Vec3(1.f, 0.f, 1.f);
		light->GetLight()->SetLightDesc(lightDesc);

		CUR_SCENE->Add(light);
	}

	// Model
	{
		auto obj = make_shared<GameObject>();
		// CustomData -> Memory
		shared_ptr<class Model> m1 = make_shared<Model>();
		m1->ReadModel(L"Player/Player");
		m1->ReadMaterial(L"Player/Player");

		obj = make_shared<GameObject>();
		obj->GetOrAddTransform()->SetPosition(Vec3(0, 0, 50));
		obj->GetOrAddTransform()->SetScale(Vec3(0.01f));

		obj->AddComponent(make_shared<ModelRenderer>(_shader));
		{
			obj->GetModelRenderer()->SetModel(m1);
			obj->GetModelRenderer()->SetPass(1);
		}

		CUR_SCENE->Add(obj);
	}

	// Model
	{
		auto obj = make_shared<GameObject>();
		// CustomData -> Memory
		shared_ptr<class Model> m1 = make_shared<Model>();
		m1->ReadModel(L"Tower/Tower");
		m1->ReadMaterial(L"Tower/Tower");

		obj = make_shared<GameObject>();
		obj->GetOrAddTransform()->SetPosition(Vec3(10, 0, 50));
		obj->GetOrAddTransform()->SetScale(Vec3(0.01f));

		obj->AddComponent(make_shared<ModelRenderer>(_shader));
		{
			obj->GetModelRenderer()->SetModel(m1);
			obj->GetModelRenderer()->SetPass(1);
		}

		CUR_SCENE->Add(obj);
	}
}

void StaticMeshDemo::Update()
{

}

void StaticMeshDemo::Render()
{

}


