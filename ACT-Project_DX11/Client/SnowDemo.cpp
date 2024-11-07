#include "pch.h"
#include "RawBuffer.h"
#include "TextureBuffer.h"
#include "Material.h"
#include "SnowDemo.h"
#include "GeometryHelper.h"
#include "Camera.h"
#include "GameObject.h"
#include "CameraScript.h"
#include "PlayerScript.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Material.h"
#include "Model.h"
#include "ModelRenderer.h"
#include "ModelAnimator.h"
#include "Mesh.h"
#include "Transform.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Light.h"
#include "Graphics.h"
#include "SphereCollider.h"
#include "Scene.h"
#include "AABBBoxCollider.h"
#include "OBBBoxCollider.h"
#include "Terrain.h"
#include "Camera.h"
#include "Button.h"
#include "Billboard.h"
#include "SnowBillboard.h"

void SnowDemo::Init()
{
	_shader = make_shared<Shader>(L"29. SnowDemo.fx");
	_renderShader = make_shared<Shader>(L"23. RenderDemo.fx");

	// Camera
	{
		auto camera = make_shared<GameObject>();
		camera->GetOrAddTransform()->SetPosition(Vec3(0.f));
		camera->GetOrAddTransform()->SetLocalRotation(Vec3{ XMConvertToRadians(15), 0.f, 0.f });
		camera->AddComponent(make_shared<Camera>());
		{
			camera->GetCamera()->SetCameraOffset(Vec3(0.f, 7.f, -14.f));
		}
		//camera->AddComponent(make_shared<CameraScript>());
		camera->GetCamera()->SetCullingMaskLayerOnOff(Layer_UI, true);

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

	// Billboard
	{
		auto obj = make_shared<GameObject>();
		obj->GetOrAddTransform()->SetLocalPosition(Vec3(0.f));
		obj->AddComponent(make_shared<SnowBillboard>(Vec3(100, 100, 100), 10000));
		{
			// Material
			{
				shared_ptr<Material> material = make_shared<Material>();
				material->SetShader(_shader);
				auto texture = RESOURCES->Load<Texture>(L"bubble", L"..\\Resources\\Textures\\bubble.png");
				material->SetDiffuseMap(texture);
				MaterialDesc& desc = material->GetMaterialDesc();
				desc.ambient = Vec4(1.f);
				desc.diffuse = Vec4(1.f);
				desc.specular = Vec4(1.f);
				RESOURCES->Add(L"bubble", material);

				obj->GetSnowBillboard()->SetMaterial(material);
			}
		}

		CUR_SCENE->Add(obj);
	}

	// Player
	auto player = make_shared<GameObject>();
	{
		player->GetOrAddTransform()->SetPosition(Vec3(0, 0, 0));
		player->GetOrAddTransform()->SetRotation(Vec3(0, 0, 0));
		player->GetOrAddTransform()->SetScale(Vec3(0.01f));

		shared_ptr<Model> m1 = make_shared<Model>();
		// Model
		{
			m1->ReadModel(L"Player/Player");
			m1->ReadMaterial(L"Player/Player");

			m1->ReadAnimation(L"Player/Idle", AnimationState::Idle);
			m1->ReadAnimation(L"Player/Walk", AnimationState::Walk);
			m1->ReadAnimation(L"Player/Run", AnimationState::Run);

			//m1->ReadModel(L"Kachujin/Kachujin");
			//m1->ReadMaterial(L"Kachujin/Kachujin");
			//m1->ReadAnimation(L"Kachujin/Idle", AnimationState::Idle);
			//m1->ReadAnimation(L"Kachujin/Run", AnimationState::Walk);

			//m1->ReadAnimation(L"Player/Crab_Atk_Combo1");
			//m1->ReadAnimation(L"Player/Crab_Atk_Combo2");
			//m1->ReadAnimation(L"Player/Crab_Atk_Combo3");
			//m1->ReadAnimation(L"Player/Crab_Atk_Combo4");

			//m1->ReadAnimation(L"Player/Crab_Death");
			//m1->ReadAnimation(L"Player/Crab_GetUp");
		}
		shared_ptr<ModelAnimator> ma1 = make_shared<ModelAnimator>(_renderShader);
		player->AddComponent(ma1);
		{
			player->GetModelAnimator()->SetModel(m1);
			player->GetModelAnimator()->SetPass(2);
		}

		// PlayerScript
		shared_ptr<PlayerScript> playerScript = make_shared<PlayerScript>();
		playerScript->SetPlayer(m1);
		playerScript->SetModelAnimator(ma1);
		player->AddComponent(playerScript);

		CUR_SCENE->Add(player);
		CUR_SCENE->SetPlayer(player);
	}

	// Debug Object
	{
		auto transform = player->GetTransform();

		auto mesh = RESOURCES->Get<Mesh>(L"Cube");
		// Look 방향 큐브
		{
			// Look 방향 표시용 빨간색 재질
			auto material = make_shared<Material>();
			material->SetShader(_renderShader);
			material->GetMaterialDesc().diffuse = Vec4(1.0f, 0.0f, 0.0f, 1.0f);  // 빨간색

			auto lookCube = make_shared<GameObject>();
			lookCube->GetOrAddTransform()->SetPosition(transform->GetPosition() + transform->GetLook() * 1.5f);
			lookCube->GetOrAddTransform()->SetScale(Vec3(0.1f, 0.1f, 5.0f));  // Z 방향으로 길쭉하게
			lookCube->AddComponent(make_shared<MeshRenderer>());
			{
				lookCube->GetMeshRenderer()->SetMaterial(material);
				lookCube->GetMeshRenderer()->SetMesh(mesh);
				lookCube->GetMeshRenderer()->SetPass(0);
			}
			CUR_SCENE->Add(lookCube);
		}

		// Up 방향 큐브
		{
			// Up 방향 표시용 초록색 재질
			auto material = make_shared<Material>();
			material->SetShader(_renderShader);
			material->GetMaterialDesc().diffuse = Vec4(0.0f, 1.0f, 0.0f, 1.0f);  // 초록색
			
			auto upCube = make_shared<GameObject>();
			upCube->GetOrAddTransform()->SetPosition(transform->GetPosition() + transform->GetUp() * 1.5f);
			upCube->GetOrAddTransform()->SetScale(Vec3(0.1f, 5.0f, 0.1f));  // Y 방향으로 길쭉하게
			upCube->AddComponent(make_shared<MeshRenderer>());
			{
				upCube->GetMeshRenderer()->SetMaterial(material);
				upCube->GetMeshRenderer()->SetMesh(mesh);
				upCube->GetMeshRenderer()->SetPass(0);
			}

			CUR_SCENE->Add(upCube);
		}

		// Right 방향 큐브
		{
			// Right 방향 표시용 파란색 재질
			auto material = make_shared<Material>();
			material->SetShader(_renderShader);
			material->GetMaterialDesc().diffuse = Vec4(0.0f, 0.0f, 1.0f, 1.0f);  // 파란색

			auto rightCube = make_shared<GameObject>();
			rightCube->GetOrAddTransform()->SetPosition(transform->GetPosition() + transform->GetRight() * 1.5f);
			rightCube->GetOrAddTransform()->SetScale(Vec3(5.0f, 0.1f, 0.1f));  // X 방향으로 길쭉하게
			rightCube->AddComponent(make_shared<MeshRenderer>());
			{
				rightCube->GetMeshRenderer()->SetMaterial(material);
				rightCube->GetMeshRenderer()->SetMesh(mesh);
				rightCube->GetMeshRenderer()->SetPass(0);
			}
		
			CUR_SCENE->Add(rightCube);
		}
	}

	// Terrain
	{
		// Material
		{
			shared_ptr<Material> material = make_shared<Material>();
			material->SetShader(_renderShader);
			auto texture = RESOURCES->Load<Texture>(L"grass", L"..\\Resources\\Textures\\Terrain\\grass.jpg");
			material->SetDiffuseMap(texture);
			MaterialDesc& desc = material->GetMaterialDesc();
			desc.ambient = Vec4(1.f);
			desc.diffuse = Vec4(1.f);
			desc.specular = Vec4(1.f);
			RESOURCES->Add(L"grass", material);
		}

		auto obj = make_shared<GameObject>();
		obj->AddComponent(make_shared<Terrain>());
		obj->GetTerrain()->Create(100, 100, RESOURCES->Get<Material>(L"grass"));

		CUR_SCENE->Add(obj);
	}
}

void SnowDemo::Update()
{
}

void SnowDemo::Render()
{

}

void SnowDemo::CreatePlayer()
{




}
