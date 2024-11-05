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

// XML ���Ͽ��� ���� �����͸� �о���� �Լ�
void Model::ReadMaterial(wstring filename)
{
	// ���� ������ ��ü ��θ� �����մϴ�.
	wstring fullPath = _texturePath + filename + L".xml";
	auto parentPath = filesystem::path(fullPath).parent_path();

	// XML ������ �ε��ϱ� ���� �غ� �մϴ�.
	tinyxml2::XMLDocument* document = new tinyxml2::XMLDocument();
	tinyxml2::XMLError error = document->LoadFile(Utils::ToString(fullPath).c_str());
	assert(error == tinyxml2::XML_SUCCESS);

	// XML ������ ��Ʈ ������Ʈ�� ã���ϴ�.
	tinyxml2::XMLElement* root = document->FirstChildElement();
	tinyxml2::XMLElement* materialNode = root->FirstChildElement();

	// ��� ���� ��带 ��ȸ�մϴ�.
	while (materialNode)
	{
		shared_ptr<Material> material = make_shared<Material>();

		// ������ �̸��� �����մϴ�.
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
	// ���� ������ ������� �߰� ó���� �����մϴ� (��: ĳ�� ���� ���ε�).
	BindCacheInfo();
}

// �� ������ �о� ���� ���� �޽� �����͸� �ε��ϴ� �Լ�
void Model::ReadModel(wstring filename)
{
	// �� ������ ��ü ��θ� �����մϴ�.
	wstring fullPath = _modelPath + filename + L".mesh";

	// ���� ��ƿ��Ƽ ��ü�� �����ϰ� ������ �б� ���� ���ϴ�.
	shared_ptr<FileUtils> file = make_shared<FileUtils>();
	file->Open(fullPath, FileMode::Read);

	// �� �����͸� �о���Դϴ�.
	{
		const uint32 count = file->Read<uint32>(); // ���� ������ �о�ɴϴ�.

		for (uint32 i = 0; i < count; i++) // �� ���� ���Ͽ� �ݺ��մϴ�.
		{
			shared_ptr<ModelBone> bone = make_shared<ModelBone>(); // ���ο� �� ��ü�� �����մϴ�.
			bone->index = file->Read<int32>(); // ���� �ε����� �о�ɴϴ�.
			bone->name = Utils::ToWString(file->Read<string>()); // ���� �̸��� �о�ɴϴ�.
			bone->parentIndex = file->Read<int32>(); // �θ� ���� �ε����� �о�ɴϴ�.
			bone->transform = file->Read<Matrix>(); // ���� ��ȯ ����� �о�ɴϴ�.

			_bones.push_back(bone); // ó���� �� ��ü�� ���� �� ��Ͽ� �߰��մϴ�.
		}
	}

	// �޽� �����͸� �о���Դϴ�.
	{
		const uint32 count = file->Read<uint32>(); // �޽��� ������ �о�ɴϴ�.

		for (uint32 i = 0; i < count; i++) // �� �޽ÿ� ���Ͽ� �ݺ��մϴ�.
		{
			shared_ptr<ModelMesh> mesh = make_shared<ModelMesh>(); // ���ο� �޽� ��ü�� �����մϴ�.

			mesh->name = Utils::ToWString(file->Read<string>()); // �޽��� �̸��� �о�ɴϴ�.
			mesh->boneIndex = file->Read<int32>(); // �޽ÿ� ������ ���� �ε����� �о�ɴϴ�.

			// �޽ð� ����ϴ� ������ �̸��� �о�ɴϴ�.
			mesh->materialName = Utils::ToWString(file->Read<string>());

			//VertexData �޽��� ���� �����͸� �о�ɴϴ�.
			{
				const uint32 count = file->Read<uint32>(); // ������ ������ �о�ɴϴ�.
				vector<ModelVertexType> vertices;// ���� �����͸� ������ ���͸� �����մϴ�.
				vertices.resize(count);

				void* data = vertices.data();// ���� �����͸� �о�ɴϴ�.
				file->Read(&data, sizeof(ModelVertexType) * count);
				mesh->geometry->AddVertices(vertices);// �о�� ���� �����͸� �޽ÿ� �߰��մϴ�.
			}

			//IndexData �޽��� �ε��� �����͸� �о�ɴϴ�.
			{
				const uint32 count = file->Read<uint32>();// �ε����� ������ �о�ɴϴ�.

				vector<uint32> indices;
				indices.resize(count);// �ε��� �����͸� ������ ���͸� �����մϴ�.

				void* data = indices.data();// �ε��� �����͸� �о�ɴϴ�.
				file->Read(&data, sizeof(uint32) * count);
				mesh->geometry->AddIndices(indices);// �о�� �ε��� �����͸� �޽ÿ� �߰��մϴ�.
			}

			mesh->CreateBuffers(); // �޽��� ���۸� �����մϴ�.

			_meshes.push_back(mesh); // ó���� �޽� ��ü�� ���� �޽� ��Ͽ� �߰��մϴ�.
		}
	}

	// �� �����Ϳ� ���õ� ĳ�� ������ ���ε��մϴ�.
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
//		// �ٸ� ���� �߰� ����
//	}
//	return -1; // ���� ���
//}

int Model::GetAnimationIndexByState(AnimationState state)
{
	for (size_t i = 0; i < _animations.size(); ++i)
	{
		if (_animations[i]->state == state)
		{
			return static_cast<int>(i); // �ε����� ���������� ��ȯ
		}
	}
	return -1; // �̸��� �ش��ϴ� �ִϸ��̼��� ���� ���
}
//int Model::FindAnimationIndex(AnimationState state)
//{
//	for (size_t i = 0; i < _animations.size(); ++i)
//	{
//		if (_animations[i]->state == state)
//		{
//			return static_cast<int>(i); // �ε����� ���������� ��ȯ
//		}
//	}
//	return -1; // �̸��� �ش��ϴ� �ִϸ��̼��� ���� ���
//}

// AnimationState ���� ���ڿ��� ��ȯ
string Model::AnimationStateToString(AnimationState state)
{
	switch (state)
	{
	case AnimationState::Idle:   return "Idle";
	case AnimationState::Walk:   return "Walk";
	case AnimationState::Run:    return "Run";
	case AnimationState::Attack: return "Attack";
	case AnimationState::Jump:   return "Jump";
		// �ٸ� ���� �߰� ����
	default: return "Unknown";
	}
}

// ���� �޽ÿ� ���� ���� ���� ������ ���ε��ϴ� �Լ�
void Model::BindCacheInfo()
{
	// �޽ÿ� ���� ������ ���ε��մϴ�.
	for (const auto& mesh : _meshes)
	{
		if (mesh->material != nullptr) // �޽ÿ� �̹� ������ ���ε��Ǿ� �ִٸ�, ���� �޽÷� �Ѿ�ϴ�.
			continue;

		// �޽��� �̸��� ���� �ش��ϴ� ������ ã�� �޽ÿ� ���ε��մϴ�.
		mesh->material = GetMaterialByName(mesh->materialName);
	}

	// �޽ÿ� �� ������ ���ε��մϴ�.
	for (const auto& mesh : _meshes)
	{
		if (mesh->bone != nullptr) // �޽ÿ� �̹� ���� ���ε��Ǿ� �ִٸ�, ���� �޽÷� �Ѿ�ϴ�.
			continue;

		// �޽ð� �����ϴ� ���� �ε����� ���� �ش� ���� ã�� �޽ÿ� ���ε��մϴ�.
		mesh->bone = GetBoneByIndex(mesh->boneIndex);
	}

	// �� ������ ���� ������ �����մϴ�.
	if (_root == nullptr && !_bones.empty()) // ��Ʈ ���� �������� �ʾҰ�, ���� �ϳ� �̻� �ִ� ���
	{
		_root = _bones[0]; // ù ��° ���� ��Ʈ ������ �����մϴ�.

		for (const auto& bone : _bones)
		{
			if (bone->parentIndex >= 0) // �θ� ���� �ִ� ���
			{
				// �θ� ���� �ε����� ���� �θ� ���� ã�� �����ϰ�, �θ� ���� �ڽ� ��Ͽ� ���� ���� �߰��մϴ�.
				bone->parent = _bones[bone->parentIndex];
				bone->parent->children.push_back(bone);
			}
			else
			{
				// �θ� ���� ���� ���(��Ʈ ��)
				bone->parent = nullptr;
			}
		}
	}
}
