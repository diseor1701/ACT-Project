#include "pch.h"
#include "Model.h"
#include "Utils.h"
#include "FileUtils.h"
#include "tinyxml2.h"
#include <filesystem>
#include "Material.h"
#include "ModelMesh.h"
#include "ModelAnimation.h"

Model::Model()
{

}

Model::~Model()
{

}

// XML 파일에서 재질 데이터를 읽어오는 함수
void Model::ReadMaterial(wstring filename)
{
	// 재질 파일의 전체 경로를 구성합니다.
	wstring fullPath = _texturePath + filename + L".xml";
	auto parentPath = filesystem::path(fullPath).parent_path();

	// XML 문서를 로드하기 위한 준비를 합니다.
	tinyxml2::XMLDocument* document = new tinyxml2::XMLDocument();
	tinyxml2::XMLError error = document->LoadFile(Utils::ToString(fullPath).c_str());
	assert(error == tinyxml2::XML_SUCCESS);

	// XML 문서의 루트 엘리먼트를 찾습니다.
	tinyxml2::XMLElement* root = document->FirstChildElement();
	tinyxml2::XMLElement* materialNode = root->FirstChildElement();

	// 모든 재질 노드를 순회합니다.
	while (materialNode)
	{
		shared_ptr<Material> material = make_shared<Material>();

		// 재질의 이름을 설정합니다.
		tinyxml2::XMLElement* node = nullptr;

		node = materialNode->FirstChildElement();
		material->SetName(Utils::ToWString(node->GetText()));

		// Diffuse Texture
		node = node->NextSiblingElement();
		if (node->GetText())
		{
			wstring textureStr = Utils::ToWString(node->GetText());
			if (textureStr.length() > 0)
			{
				auto texture = RESOURCES->GetOrAddTexture(textureStr, (parentPath / textureStr).wstring());
				material->SetDiffuseMap(texture);
			}
		}

		// Specular Texture
		node = node->NextSiblingElement();
		if (node->GetText())
		{
			wstring texture = Utils::ToWString(node->GetText());
			if (texture.length() > 0)
			{
				wstring textureStr = Utils::ToWString(node->GetText());
				if (textureStr.length() > 0)
				{
					auto texture = RESOURCES->GetOrAddTexture(textureStr, (parentPath / textureStr).wstring());
					material->SetSpecularMap(texture);
				}
			}
		}

		// Normal Texture
		node = node->NextSiblingElement();
		if (node->GetText())
		{
			wstring textureStr = Utils::ToWString(node->GetText());
			if (textureStr.length() > 0)
			{
				auto texture = RESOURCES->GetOrAddTexture(textureStr, (parentPath / textureStr).wstring());
				material->SetNormalMap(texture);
			}
		}

		// Ambient
		{
			node = node->NextSiblingElement();

			Color color;
			color.x = node->FloatAttribute("R");
			color.y = node->FloatAttribute("G");
			color.z = node->FloatAttribute("B");
			color.w = node->FloatAttribute("A");
			material->GetMaterialDesc().ambient = color;
		}

		// Diffuse
		{
			node = node->NextSiblingElement();

			Color color;
			color.x = node->FloatAttribute("R");
			color.y = node->FloatAttribute("G");
			color.z = node->FloatAttribute("B");
			color.w = node->FloatAttribute("A");
			material->GetMaterialDesc().diffuse = color;
		}

		// Specular
		{
			node = node->NextSiblingElement();

			Color color;
			color.x = node->FloatAttribute("R");
			color.y = node->FloatAttribute("G");
			color.z = node->FloatAttribute("B");
			color.w = node->FloatAttribute("A");
			material->GetMaterialDesc().specular = color;
		}

		// Emissive
		{
			node = node->NextSiblingElement();

			Color color;
			color.x = node->FloatAttribute("R");
			color.y = node->FloatAttribute("G");
			color.z = node->FloatAttribute("B");
			color.w = node->FloatAttribute("A");
			material->GetMaterialDesc().emissive = color;
		}

		_materials.push_back(material);

		// Next Material
		materialNode = materialNode->NextSiblingElement();
	}
	// 재질 정보를 기반으로 추가 처리를 수행합니다 (예: 캐시 정보 바인딩).
	BindCacheInfo();
}

// 모델 파일을 읽어 모델의 본과 메시 데이터를 로드하는 함수
void Model::ReadModel(wstring filename)
{
	// 모델 파일의 전체 경로를 생성합니다.
	wstring fullPath = _modelPath + filename + L".mesh";

	// 파일 유틸리티 객체를 생성하고 파일을 읽기 모드로 엽니다.
	shared_ptr<FileUtils> file = make_shared<FileUtils>();
	file->Open(fullPath, FileMode::Read);

	// 본 데이터를 읽어들입니다.
	{
		const uint32 count = file->Read<uint32>(); // 본의 개수를 읽어옵니다.

		for (uint32 i = 0; i < count; i++) // 각 본에 대하여 반복합니다.
		{
			shared_ptr<ModelBone> bone = make_shared<ModelBone>(); // 새로운 본 객체를 생성합니다.
			bone->index = file->Read<int32>(); // 본의 인덱스를 읽어옵니다.
			bone->name = Utils::ToWString(file->Read<string>()); // 본의 이름을 읽어옵니다.
			bone->parentIndex = file->Read<int32>(); // 부모 본의 인덱스를 읽어옵니다.
			bone->transform = file->Read<Matrix>(); // 본의 변환 행렬을 읽어옵니다.

			_bones.push_back(bone); // 처리된 본 객체를 모델의 본 목록에 추가합니다.
		}
	}

	// 메시 데이터를 읽어들입니다.
	{
		const uint32 count = file->Read<uint32>(); // 메시의 개수를 읽어옵니다.

		for (uint32 i = 0; i < count; i++) // 각 메시에 대하여 반복합니다.
		{
			shared_ptr<ModelMesh> mesh = make_shared<ModelMesh>(); // 새로운 메시 객체를 생성합니다.

			mesh->name = Utils::ToWString(file->Read<string>()); // 메시의 이름을 읽어옵니다.
			mesh->boneIndex = file->Read<int32>(); // 메시와 연관된 본의 인덱스를 읽어옵니다.

			// 메시가 사용하는 재질의 이름을 읽어옵니다.
			mesh->materialName = Utils::ToWString(file->Read<string>());

			//VertexData 메시의 정점 데이터를 읽어옵니다.
			{
				const uint32 count = file->Read<uint32>(); // 정점의 개수를 읽어옵니다.
				vector<ModelVertexType> vertices;// 정점 데이터를 저장할 벡터를 생성합니다.
				vertices.resize(count);

				void* data = vertices.data();// 정점 데이터를 읽어옵니다.
				file->Read(&data, sizeof(ModelVertexType) * count);
				mesh->geometry->AddVertices(vertices);// 읽어온 정점 데이터를 메시에 추가합니다.
			}

			//IndexData 메시의 인덱스 데이터를 읽어옵니다.
			{
				const uint32 count = file->Read<uint32>();// 인덱스의 개수를 읽어옵니다.

				vector<uint32> indices;
				indices.resize(count);// 인덱스 데이터를 저장할 벡터를 생성합니다.

				void* data = indices.data();// 인덱스 데이터를 읽어옵니다.
				file->Read(&data, sizeof(uint32) * count);
				mesh->geometry->AddIndices(indices);// 읽어온 인덱스 데이터를 메시에 추가합니다.
			}

			mesh->CreateBuffers(); // 메시의 버퍼를 생성합니다.

			_meshes.push_back(mesh); // 처리된 메시 객체를 모델의 메시 목록에 추가합니다.
		}
	}

	// 모델 데이터와 관련된 캐시 정보를 바인딩합니다.
	BindCacheInfo();
}

void Model::ReadAnimation(wstring filename, AnimationState state)
{
	wstring fullPath = _modelPath + filename + L".clip";

	shared_ptr<FileUtils> file = make_shared<FileUtils>();
	file->Open(fullPath, FileMode::Read);

	shared_ptr<ModelAnimation> animation = make_shared<ModelAnimation>();

	animation->animationName = AnimationStateToString(state);

	animation->state = state;
	animation->name = Utils::ToWString(file->Read<string>());
	animation->duration = file->Read<float>();
	animation->frameRate = file->Read<float>();
	animation->frameCount = file->Read<uint32>();

	uint32 keyframesCount = file->Read<uint32>();

	for (uint32 i = 0; i < keyframesCount; i++)
	{
		shared_ptr<ModelKeyframe> keyframe = make_shared<ModelKeyframe>();
		keyframe->boneName = Utils::ToWString(file->Read<string>());
	
		uint32 size = file->Read<uint32>();

		if (size > 0)
		{
			keyframe->transforms.resize(size);
			void* ptr = &keyframe->transforms[0];
			file->Read(&ptr, sizeof(ModelKeyframeData) * size);
		}

		animation->keyframes[keyframe->boneName] = keyframe;
	}

	_animations.push_back(animation);
}

std::shared_ptr<Material> Model::GetMaterialByName(const wstring& name)
{
	for (auto& material : _materials)
	{
		if (material->GetName() == name)
			return material;
	}

	return nullptr;
}

std::shared_ptr<ModelMesh> Model::GetMeshByName(const wstring& name)
{
	for (auto& mesh : _meshes)
	{
		if (mesh->name == name)
			return mesh;
	}

	return nullptr;
}

std::shared_ptr<ModelBone> Model::GetBoneByName(const wstring& name)
{
	for (auto& bone : _bones)
	{
		if (bone->name == name)
			return bone;
	}

	return nullptr;
}	
std::shared_ptr<ModelAnimation> Model::GetAnimationByState(AnimationState state)
{
	for (auto& animation : _animations)
	{
		if (animation->state == state)
			return animation;
	}

	return nullptr;
}

std::shared_ptr<ModelAnimation> Model::GetAnimationByName(wstring name)
{
	for (auto& animation : _animations)
	{
		if (animation->name == name)
			return animation;
	}

	return nullptr;
}

//int Model::GetAnimationIndexByState(AnimationState state)
//{
//	switch (state)
//	{
//	case AnimationState::Idle: return FindAnimationIndex("Idle");
//	case AnimationState::Walk: return FindAnimationIndex("Walk");
//	case AnimationState::Run: return FindAnimationIndex("Run");
//	case AnimationState::Attack: return FindAnimationIndex("Attack");
//		// 다른 상태 추가 가능
//	}
//	return -1; // 없는 경우
//}

int Model::GetAnimationIndexByState(AnimationState state)
{
	for (size_t i = 0; i < _animations.size(); ++i)
	{
		if (_animations[i]->state == state)
		{
			return static_cast<int>(i); // 인덱스를 정수형으로 반환
		}
	}
	return -1; // 이름에 해당하는 애니메이션이 없는 경우
}
//int Model::FindAnimationIndex(AnimationState state)
//{
//	for (size_t i = 0; i < _animations.size(); ++i)
//	{
//		if (_animations[i]->state == state)
//		{
//			return static_cast<int>(i); // 인덱스를 정수형으로 반환
//		}
//	}
//	return -1; // 이름에 해당하는 애니메이션이 없는 경우
//}

// AnimationState 값을 문자열로 반환
string Model::AnimationStateToString(AnimationState state)
{
	switch (state)
	{
	case AnimationState::Idle:   return "Idle";
	case AnimationState::Walk:   return "Walk";
	case AnimationState::Run:    return "Run";
	case AnimationState::Attack: return "Attack";
	case AnimationState::Jump:   return "Jump";
		// 다른 상태 추가 가능
	default: return "Unknown";
	}
}

// 모델의 메시와 본에 대한 참조 정보를 바인딩하는 함수
void Model::BindCacheInfo()
{
	// 메시에 재질 정보를 바인딩합니다.
	for (const auto& mesh : _meshes)
	{
		if (mesh->material != nullptr) // 메시에 이미 재질이 바인딩되어 있다면, 다음 메시로 넘어갑니다.
			continue;

		// 메시의 이름을 통해 해당하는 재질을 찾아 메시에 바인딩합니다.
		mesh->material = GetMaterialByName(mesh->materialName);
	}

	// 메시에 본 정보를 바인딩합니다.
	for (const auto& mesh : _meshes)
	{
		if (mesh->bone != nullptr) // 메시에 이미 본이 바인딩되어 있다면, 다음 메시로 넘어갑니다.
			continue;

		// 메시가 참조하는 본의 인덱스를 통해 해당 본을 찾아 메시에 바인딩합니다.
		mesh->bone = GetBoneByIndex(mesh->boneIndex);
	}

	// 본 구조의 계층 정보를 설정합니다.
	if (_root == nullptr && !_bones.empty()) // 루트 본이 설정되지 않았고, 본이 하나 이상 있는 경우
	{
		_root = _bones[0]; // 첫 번째 본을 루트 본으로 설정합니다.

		for (const auto& bone : _bones)
		{
			if (bone->parentIndex >= 0) // 부모 본이 있는 경우
			{
				// 부모 본의 인덱스를 통해 부모 본을 찾아 설정하고, 부모 본의 자식 목록에 현재 본을 추가합니다.
				bone->parent = _bones[bone->parentIndex];
				bone->parent->children.push_back(bone);
			}
			else
			{
				// 부모 본이 없는 경우(루트 본)
				bone->parent = nullptr;
			}
		}
	}
}
