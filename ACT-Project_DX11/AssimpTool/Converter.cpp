#include "pch.h"
#include "Converter.h"
#include <filesystem>
#include "Utils.h"
#include "tinyxml2.h"
#include "FileUtils.h"

Converter::Converter()
{
	_importer = make_shared<Assimp::Importer>();

}

Converter::~Converter()
{

}

void Converter::ReadAssetFile(wstring file)
{
	wstring fileStr = _assetPath + file;

	auto p = std::filesystem::path(fileStr);
	assert(std::filesystem::exists(p));

	_scene = _importer->ReadFile(
		Utils::ToString(fileStr),
		aiProcess_ConvertToLeftHanded |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace
	);

	assert(_scene != nullptr);
}

// �� �����͸� �������� �Լ�
void Converter::ExportModelData(wstring savePath)
{
	// ���� ����� ���� ��θ� �����մϴ�.
	wstring finalPath = _modelPath + savePath + L".mesh";

	// Assimp ���̺귯���� ����Ͽ� �о�� 3D ���� ��Ʈ ������ �����Ͽ�
	// �� �����͸� ��ȸ�ϸ� �о���Դϴ�.
	ReadModelData(_scene->mRootNode, -1, -1);

	// �޽ÿ� ����� ��Ų(��) �����͸� �о���Դϴ�.
	ReadSkinData();

	// ���� �޽� ������ ������ CSV ������ �ۼ��մϴ�.
	// �� ������ ���� ������ �����ϰų� ����뿡 �����ϰ� ���� �� �ֽ��ϴ�.
	{
		FILE* file;
		// CSV ������ ���� ���� ���ϴ�.
		::fopen_s(&file, "../Vertices.csv", "w");

		// ��� �� ������ ���Ͽ� ���ϴ�.
		for (shared_ptr<asBone>& bone : _bones)
		{
			string name = bone->name;
			// ���� �ε����� �̸��� ���Ͽ� ����մϴ�.
			::fprintf(file, "%d,%s\n", bone->index, bone->name.c_str());
		}

		// �޽� �����͸� ���Ͽ� ���ϴ�.
		::fprintf(file, "\n");

		for (shared_ptr<asMesh>& mesh : _meshes)
		{
			string name = mesh->name;
			// �޽� �̸��� �ֿܼ� ����մϴ�(����� �뵵).
			::printf("%s\n", name.c_str());

			// �޽��� �� ������ ���� ������ ���Ͽ� ����մϴ�.
			for (UINT i = 0; i < mesh->vertices.size(); i++)
			{
				Vec3 p = mesh->vertices[i].position; // ������ ��ġ
				Vec4 indices = mesh->vertices[i].blendIndices; // �� �ε���
				Vec4 weights = mesh->vertices[i].blendWeights; // �� ����ġ

				// ��ġ, �� �ε���, �� ����ġ�� ���Ͽ� ����մϴ�.
				::fprintf(file, "%f,%f,%f,", p.x, p.y, p.z);
				::fprintf(file, "%f,%f,%f,%f,", indices.x, indices.y, indices.z, indices.w);
				::fprintf(file, "%f,%f,%f,%f\n", weights.x, weights.y, weights.z, weights.w);
			}
		}

		// ���� �ۼ��� ��ġ�� ������ �ݽ��ϴ�.
		::fclose(file);
	}

	// ��ȯ�� �� �����͸� `.mesh` ���� �������� ���� �����մϴ�.
	WriteModelFile(finalPath);
}

// ���� �����͸� �������� �Լ�
void Converter::ExportMaterialData(wstring savePath)
{
	wstring finalPath = _texturePath + savePath + L".xml"; // ���� ���� ���
	ReadMaterialData(); // ���� ������ �б�
	WriteMaterialData(finalPath); // ���� ���� ����
}

// �ִϸ��̼� �����͸� �������� �Լ�
void Converter::ExportAnimationData(wstring savePath, uint32 index /*= 0*/)
{
	wstring finalPath = _modelPath + savePath + L".clip";
	assert(index < _scene->mNumAnimations);
	shared_ptr<asAnimation> animation = ReadAnimationData(_scene->mAnimations[index]);
	WriteAnimationData(animation, finalPath);
}

// ���� ���(��) �����͸� �а� ó���ϴ� �Լ�
void Converter::ReadModelData(aiNode* node, int32 index, int32 parent)
{
	// ���ο� �� ��ü�� �����ϰ� �⺻ ������ �����մϴ�.
	shared_ptr<asBone> bone = make_shared<asBone>();
	bone->index = index; // ���� ���� �ε���
	bone->parent = parent; // �θ� ���� �ε���
	bone->name = node->mName.C_Str(); // ���� �̸�

	// ���� ���� ��ȯ ����� ��������, ��ġ(transpose)�Ͽ� �����մϴ�.
	// Assimp�� �� ����(column-major) ����� ���������, DirectX�� OpenGL�� �� ����(row-major) ����� ����� �� �����Ƿ�, ��ġ�� �ʿ��� �� �ֽ��ϴ�.
	Matrix transform(node->mTransformation[0]);
	bone->transform = transform.Transpose();

	// ��Ʈ(Ȥ�� �θ�) �����κ��� ������� ��ȯ�� ����մϴ�.
	Matrix matParent = Matrix::Identity; // �⺻������ ���� ����� ���
	if (parent >= 0) {
		matParent = _bones[parent]->transform; // �θ� ���� ��ȯ ����� �����ɴϴ�.
	}

	// ���������� ���� ��ȯ ����� ����մϴ�.
	// �� �ڽ��� ���� ��ȯ�� �θ��� ��ȯ ����� ���մϴ�.
	bone->transform = bone->transform * matParent;

	// ó���� �� ������ ���� ����Ʈ�� �߰��մϴ�.
	_bones.push_back(bone);

	// ���� ���(��)�� ����� �޽� �����͸� �о���Դϴ�.
	ReadMeshData(node, index);

	// ���� ����� ��� �ڽ� ��带 ��������� Ž���Ͽ� ���� ó���� �ݺ��մϴ�.
	for (uint32 i = 0; i < node->mNumChildren; i++) {
		ReadModelData(node->mChildren[i], _bones.size(), index);
	}
}


// ��忡�� �޽� �����͸� �о�� ���� ����ü�� �����ϴ� �Լ�
void Converter::ReadMeshData(aiNode* node, int32 bone)
{
	if (node->mNumMeshes < 1)
		return; // �޽ð� ���� ���� ó������ ����

	shared_ptr<asMesh> mesh = make_shared<asMesh>(); // �� �޽� ��ü ����
	mesh->name = node->mName.C_Str(); // �޽� �̸� ����
	mesh->boneIndex = bone; // �޽ÿ� ����� �� �ε��� ����

	// ��忡 ���Ե� ��� �޽ÿ� ���� �ݺ�
	for (uint32 i = 0; i < node->mNumMeshes; i++) {
		uint32 index = node->mMeshes[i];
		const aiMesh* srcMesh = _scene->mMeshes[index]; // �ҽ� �޽� ����

		// �޽��� ���� �̸��� ������
		const aiMaterial* material = _scene->mMaterials[srcMesh->mMaterialIndex];
		mesh->materialName = material->GetName().C_Str(); // ���� �̸� ����

		const uint32 startVertex = mesh->vertices.size(); // ���� �޽��� ���� ���� �ε���

		// �޽��� ��� ������ ���� �ݺ�
		for (uint32 v = 0; v < srcMesh->mNumVertices; v++)
		{
			VertexType vertex; // �� ���� ��ü
			// ���� ��ġ, UV ��ǥ, ���� ���� ����
			::memcpy(&vertex.position, &srcMesh->mVertices[v], sizeof(Vec3));
			if (srcMesh->HasTextureCoords(0))
				::memcpy(&vertex.uv, &srcMesh->mTextureCoords[0][v], sizeof(Vec2));
			if (srcMesh->HasNormals())
				::memcpy(&vertex.normal, &srcMesh->mNormals[v], sizeof(Vec3));

			mesh->vertices.push_back(vertex); // ������ �޽ÿ� �߰�
		}

		// �޽��� ��� ��(���̽�)�� ���� �ݺ��Ͽ� �ε��� ���� ����
		for (uint32 f = 0; f < srcMesh->mNumFaces; f++)
		{
			aiFace& face = srcMesh->mFaces[f];
			for (uint32 k = 0; k < face.mNumIndices; k++)
				mesh->indices.push_back(face.mIndices[k] + startVertex); // �ε����� �޽ÿ� �߰�
		}
	}

	_meshes.push_back(mesh); // �޽ø� ���� �޽� ����Ʈ�� �߰�
}
// ���� ���� ���� ����ġ ������ �о�� ó���ϴ� �Լ�
void Converter::ReadSkinData()
{
	// ��� �޽ÿ� ���� �ݺ�
	for (uint32 i = 0; i < _scene->mNumMeshes; i++) {
		aiMesh* srcMesh = _scene->mMeshes[i];
		if (!srcMesh->HasBones()) continue; // ���� ������ �ǳʶ�

		shared_ptr<asMesh> mesh = _meshes[i]; // ���� �޽� ����

		vector<asBoneWeights> tempVertexBoneWeights;
		tempVertexBoneWeights.resize(mesh->vertices.size()); // ������ ����ġ ����Ʈ �ʱ�ȭ

		// ��� ���� ���� �ݺ��Ͽ� ����ġ ���� ����
		for (uint32 b = 0; b < srcMesh->mNumBones; b++) {
			aiBone* srcMeshBone = srcMesh->mBones[b];
			uint32 boneIndex = GetBoneIndex(srcMeshBone->mName.C_Str()); // �� �ε��� �˻�

			for (uint32 w = 0; w < srcMeshBone->mNumWeights; w++) {
				uint32 index = srcMeshBone->mWeights[w].mVertexId; // ���� �ε���
				float weight = srcMeshBone->mWeights[w].mWeight; // ����ġ

				tempVertexBoneWeights[index].AddWeights(boneIndex, weight); // ����ġ ���� �߰�
			}
		}

		// ���� ����ġ ������ ���� �����Ϳ� ����
		for (uint32 v = 0; v < tempVertexBoneWeights.size(); v++)
		{
			tempVertexBoneWeights[v].Normalize(); // ����ġ ����ȭ

			asBlendWeight blendWeight = tempVertexBoneWeights[v].GetBlendWeights();
			mesh->vertices[v].blendIndices = blendWeight.indices; // �� �ε��� ����
			mesh->vertices[v].blendWeights = blendWeight.weights; // �� ����ġ ����
		}
	}
}

// �� ������ �ۼ��ϴ� �Լ�
void Converter::WriteModelFile(wstring finalPath)
{
	auto path = filesystem::path(finalPath);

	// ���� ������ ��ġ�� ���丮�� �����մϴ�. �̹� �����ϸ� �ǳʶݴϴ�.
	filesystem::create_directory(path.parent_path());

	shared_ptr<FileUtils> file = make_shared<FileUtils>();
	file->Open(finalPath, FileMode::Write); // ������ ���� ���� ���ϴ�.

	// �� �����͸� ���Ͽ� ���ϴ�.
	file->Write<uint32>(_bones.size()); // ���� ������ ����մϴ�.
	for (shared_ptr<asBone>& bone : _bones)
	{
		// �� ���� ���� ������ ���Ͽ� ����մϴ�.
		file->Write<int32>(bone->index); // ���� �ε���
		file->Write<string>(bone->name); // ���� �̸�
		file->Write<int32>(bone->parent); // �θ� ���� �ε���
		file->Write<Matrix>(bone->transform); // ���� ��ȯ ���
	}

	// �޽� �����͸� ���Ͽ� ���ϴ�.
	file->Write<uint32>(_meshes.size()); // �޽��� ������ ����մϴ�.
	for (shared_ptr<asMesh>& meshData : _meshes)
	{
		// �� �޽ÿ� ���� ������ ���Ͽ� ����մϴ�.
		file->Write<string>(meshData->name); // �޽� �̸�
		file->Write<int32>(meshData->boneIndex); // ����� ���� �ε���
		file->Write<string>(meshData->materialName); // ����ϴ� ������ �̸�

		// ���� �����͸� ���Ͽ� ���ϴ�.
		file->Write<uint32>(meshData->vertices.size()); // ������ ����
		file->Write(&meshData->vertices[0], sizeof(VertexType) * meshData->vertices.size()); // ���� ������

		// �ε��� �����͸� ���Ͽ� ���ϴ�.
		file->Write<uint32>(meshData->indices.size()); // �ε����� ����
		file->Write(&meshData->indices[0], sizeof(uint32) * meshData->indices.size()); // �ε��� ������
	}
}

void Converter::ReadMaterialData()
{
	for (uint32 i = 0; i < _scene->mNumMaterials; i++)
	{
		aiMaterial* srcMaterial = _scene->mMaterials[i];
		shared_ptr<asMaterial> material = make_shared<asMaterial>();
		material->name = srcMaterial->GetName().C_Str(); // ������ �̸�

		// �ֺ���, Ȯ�걤, �ݻ籤, ��ü�߱� �Ӽ��� �о���Դϴ�.
		aiColor3D color;
		//Ambient
		srcMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
		material->ambient = Color(color.r, color.g, color.b, 1.f);

		//Diffuse
		srcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		material->diffuse = Color(color.r, color.g, color.b, 1.f);

		//Specualar
		srcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
		material->specular = Color(color.r, color.g, color.b, 1.f);
		srcMaterial->Get(AI_MATKEY_SHININESS, material->specular.w);

		//Emissive
		srcMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color);
		material->emissive = Color(color.r, color.g, color.b, 1.f);

		// �ؽ�ó ���� ��θ� �о���Դϴ�.
		aiString file;

		//Diffuse Texture
		srcMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &file);
		material->diffuseFile = file.C_Str();

		//Specular Texture
		srcMaterial->GetTexture(aiTextureType_SPECULAR, 0, &file);
		material->specularFile = file.C_Str();

		//Normal Texture
		srcMaterial->GetTexture(aiTextureType_NORMALS, 0, &file);
		material->normalFile = file.C_Str();

		// ���� ������ ���� ����Ʈ�� �߰��մϴ�.
		_materials.push_back(material);
	}
}

// ���� �����͸� XML �������� �����ϴ� �Լ�
void Converter::WriteMaterialData(wstring finalPath)
{
	auto path = filesystem::path(finalPath);

	// ���� ���� ��ο� �ش��ϴ� ������ ������ �����մϴ�.
	filesystem::create_directory(path.parent_path());

	string folder = path.parent_path().string();
	// XML ���� ��ü�� �����մϴ�.
	shared_ptr<tinyxml2::XMLDocument> document = make_shared<tinyxml2::XMLDocument>();
	// XML ����θ� �߰��մϴ�.

	tinyxml2::XMLDeclaration* decl = document->NewDeclaration();
	document->LinkEndChild(decl);
	// ���� �������� ��Ʈ ������Ʈ�� �����ϰ� ������ �߰��մϴ�.
	tinyxml2::XMLElement* root = document->NewElement("Materials");
	document->LinkEndChild(root);
	// ��� ������ ���� �ݺ��մϴ�.
	for (shared_ptr<asMaterial> material : _materials)
	{
		// �� ������ ���� ������ XML ������Ʈ�� �߰��մϴ�.
		tinyxml2::XMLElement* node = document->NewElement("Material");
		root->LinkEndChild(node);
		// ������ �̸�, �ؽ�ó ���� ���, ���� ���� ���� ������Ʈ�� �Ӽ����� �߰��մϴ�.
		tinyxml2::XMLElement* element = nullptr;

		element = document->NewElement("Name");
		element->SetText(material->name.c_str());
		node->LinkEndChild(element);

		element = document->NewElement("DiffuseFile");
		element->SetText(WriteTexture(folder, material->diffuseFile).c_str());
		node->LinkEndChild(element);

		element = document->NewElement("SpecularFile");
		element->SetText(WriteTexture(folder, material->specularFile).c_str());
		node->LinkEndChild(element);

		element = document->NewElement("NormalFile");
		element->SetText(WriteTexture(folder, material->normalFile).c_str());
		node->LinkEndChild(element);
		// �� ���� �Ӽ�(ambient, diffuse, specular, emissive)�� ���� ������ �߰��մϴ�.
		element = document->NewElement("Ambient");
		element->SetAttribute("R", material->ambient.x);
		element->SetAttribute("G", material->ambient.y);
		element->SetAttribute("B", material->ambient.z);
		element->SetAttribute("A", material->ambient.w);
		node->LinkEndChild(element);

		element = document->NewElement("Diffuse");
		element->SetAttribute("R", material->diffuse.x);
		element->SetAttribute("G", material->diffuse.y);
		element->SetAttribute("B", material->diffuse.z);
		element->SetAttribute("A", material->diffuse.w);
		node->LinkEndChild(element);

		element = document->NewElement("Specular");
		element->SetAttribute("R", material->specular.x);
		element->SetAttribute("G", material->specular.y);
		element->SetAttribute("B", material->specular.z);
		element->SetAttribute("A", material->specular.w);
		node->LinkEndChild(element);

		element = document->NewElement("Emissive");
		element->SetAttribute("R", material->emissive.x);
		element->SetAttribute("G", material->emissive.y);
		element->SetAttribute("B", material->emissive.z);
		element->SetAttribute("A", material->emissive.w);
		node->LinkEndChild(element);
	}
	// XML ������ ���Ϸ� �����մϴ�.
	document->SaveFile(Utils::ToString(finalPath).c_str());
}

// �ؽ�ó ������ �����ϰų� ���� �ؽ�ó ������ �� ��ġ�� �����ϴ� �Լ�
string Converter::WriteTexture(string saveFolder, string file)
{
	string fileName = filesystem::path(file).filename().string();
	string folderName = filesystem::path(saveFolder).filename().string();
	// �Ӻ���� �ؽ�ó �Ǵ� �ܺ� �ؽ�ó ������ ���� ���θ� Ȯ���մϴ�.
	const aiTexture* srcTexture = _scene->GetEmbeddedTexture(file.c_str());
	if (srcTexture) {
		// �ؽ�ó �����Ͱ� �Ӻ����Ǿ� ������ �� ���Ϸ� �����մϴ�.
		string pathStr = (filesystem::path(saveFolder) / fileName).string();

		if (srcTexture->mHeight == 0)
		{
			shared_ptr<FileUtils> file = make_shared<FileUtils>();
			file->Open(Utils::ToWString(pathStr), FileMode::Write);
			file->Write(srcTexture->pcData, srcTexture->mWidth);
		}
		else
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
			desc.Width = srcTexture->mWidth;
			desc.Height = srcTexture->mHeight;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			D3D11_SUBRESOURCE_DATA subResource = { 0 };
			subResource.pSysMem = srcTexture->pcData;

			ComPtr<ID3D11Texture2D> texture;
			HRESULT hr = DEVICE->CreateTexture2D(&desc, &subResource, texture.GetAddressOf());
			CHECK(hr);

			DirectX::ScratchImage img;
			::CaptureTexture(DEVICE.Get(), DC.Get(), texture.Get(), img);

			// Save To File
			hr = DirectX::SaveToDDSFile(*img.GetImages(), DirectX::DDS_FLAGS_NONE, Utils::ToWString(fileName).c_str());
			CHECK(hr);
		}
	}
	else
	{
		// �ܺ� �ؽ�ó �����̸� �� ��ġ�� �����մϴ�.
		string originStr = (filesystem::path(_assetPath) / folderName / file).string();
		Utils::Replace(originStr, "\\", "/");

		string pathStr = (filesystem::path(saveFolder) / fileName).string();
		Utils::Replace(pathStr, "\\", "/");

		::CopyFileA(originStr.c_str(), pathStr.c_str(), false);
	}

	return fileName;// ó���� �ؽ�ó ������ �̸��� ��ȯ�մϴ�.
}

// Assimp �ִϸ��̼� �����͸� �о� ����� ���� �ִϸ��̼� ��ü�� ��ȯ�ϴ� �Լ�
std::shared_ptr<asAnimation> Converter::ReadAnimationData(aiAnimation* srcAnimation)
{
	// ���ο� �ִϸ��̼� ��ü�� �����մϴ�.
	shared_ptr<asAnimation> animation = make_shared<asAnimation>();
	// �ִϸ��̼� �̸��� �����մϴ�.
	animation->name = srcAnimation->mName.C_Str();
	// �ִϸ��̼��� ������ �ӵ��� �����մϴ�. (�ʴ� ƽ ��)
	animation->frameRate = (float)srcAnimation->mTicksPerSecond;
	// �ִϸ��̼��� ������ ���� �����մϴ�. (���� �ð� ���)
	animation->frameCount = (uint32)srcAnimation->mDuration + 1;

	// �ִϸ��̼� ��带 ĳ���ϱ� ���� ���� �����մϴ�.
	map<string, shared_ptr<asAnimationNode>> cacheAnimNodes;
	// Assimp �ִϸ��̼� ä���� ��ȸ�ϸ� ��庰 �ִϸ��̼� �����͸� ó���մϴ�.
	for (uint32 i = 0; i < srcAnimation->mNumChannels; i++)
	{
		aiNodeAnim* srcNode = srcAnimation->mChannels[i];

		// �ִϸ��̼� ��� ������ �Ľ�
		shared_ptr<asAnimationNode> node = ParseAnimationNode(animation, srcNode);
		if (node->keyframe.size() == 0)
			continue;

		// ���� ã�� ��� �߿� ���� �� �ð����� �ִϸ��̼� �ð� ����
		animation->duration = max(animation->duration, node->keyframe.back().time);
		// �Ľ̵� ��带 ĳ�ÿ� �߰��մϴ�.
		cacheAnimNodes[srcNode->mNodeName.C_Str()] = node;
	}
	// �ִϸ��̼� Ű������ �����͸� ó���մϴ�
	ReadKeyframeData(animation, _scene->mRootNode, cacheAnimNodes);

	return animation;
}

// �ִϸ��̼� ��带 �Ľ��ϴ� �Լ�
std::shared_ptr<asAnimationNode> Converter::ParseAnimationNode(shared_ptr<asAnimation> animation, aiNodeAnim* srcNode)
{
	// ���ο� �ִϸ��̼� ��� ��ü�� ����
	std::shared_ptr<asAnimationNode> node = make_shared<asAnimationNode>();
	// ��� �̸� ����
	node->name = srcNode->mNodeName;

	// ��ġ, ȸ��, ������ �� ���� ���� Ű�������� ���� ���� �������� �� Ű������ ���� ����
	uint32 keyCount = max(max(srcNode->mNumPositionKeys, srcNode->mNumScalingKeys), srcNode->mNumRotationKeys);

	// �� Ű�����ӿ� ���� �ݺ�
	for (uint32 k = 0; k < keyCount; k++)
	{
		// Ű������ ������ ��ü
		asKeyframeData frameData;

		// Ű�������� �߰ߵǾ����� ����
		bool found = false;
		// ���� Ű�������� �ε���
		uint32 t = node->keyframe.size();

		// ��ġ, ȸ��, ������ Ű�������� ó���մϴ�. �� Ű�������� �ð��� �����͸� �����Ͽ� frameData�� �����մϴ�.
		// Position
		if (::fabsf((float)srcNode->mPositionKeys[k].mTime - (float)t) <= 0.0001f)
		{
			aiVectorKey key = srcNode->mPositionKeys[k];
			frameData.time = (float)key.mTime;
			::memcpy_s(&frameData.translation, sizeof(Vec3), &key.mValue, sizeof(aiVector3D));

			found = true;
		}

		// Rotation
		if (::fabsf((float)srcNode->mRotationKeys[k].mTime - (float)t) <= 0.0001f)
		{
			aiQuatKey key = srcNode->mRotationKeys[k];
			frameData.time = (float)key.mTime;

			frameData.rotation.x = key.mValue.x;
			frameData.rotation.y = key.mValue.y;
			frameData.rotation.z = key.mValue.z;
			frameData.rotation.w = key.mValue.w;

			found = true;
		}

		// Scale
		if (::fabsf((float)srcNode->mScalingKeys[k].mTime - (float)t) <= 0.0001f)
		{
			aiVectorKey key = srcNode->mScalingKeys[k];
			frameData.time = (float)key.mTime;
			::memcpy_s(&frameData.scale, sizeof(Vec3), &key.mValue, sizeof(aiVector3D));

			found = true;
		}

		if (found == true)
			node->keyframe.push_back(frameData);
	}

	// �ִϸ��̼��� Ű������ ������ ����� Ű������ ���� ���� ���, ������ Ű�������� �����Ͽ� ä��ϴ�.
	if (node->keyframe.size() < animation->frameCount && node->keyframe.size() != 0)
	{
		uint32 count = animation->frameCount - node->keyframe.size(); // ä������ Ű������ ��
		asKeyframeData keyFrame = node->keyframe.back(); // ������ Ű������

		for (uint32 n = 0; n < count; n++)
			node->keyframe.push_back(keyFrame); // Ű������ �����Ͽ� �߰�
	}

	return node;
}

// �ִϸ��̼� �����Ϳ��� Ư�� ����� Ű������ �����͸� �о� ���� ������ ������ �����ϴ� �Լ�
void Converter::ReadKeyframeData(shared_ptr<asAnimation> animation, aiNode* srcNode, map<string, shared_ptr<asAnimationNode>>& cache)
{
	// ���ο� Ű������ ��ü�� �����մϴ�.
	shared_ptr<asKeyframe> keyframe = make_shared<asKeyframe>();
	// ���� ���(��)�� �̸��� Ű�������� �� �̸����� �����մϴ�.
	keyframe->boneName = srcNode->mName.C_Str();

	// ���� ��忡 �ش��ϴ� �ִϸ��̼� ��带 ã���ϴ�.
	shared_ptr<asAnimationNode> findNode = cache[srcNode->mName.C_Str()];

	// �ִϸ��̼��� ��� �����ӿ� ���� �ݺ��մϴ�.
	for (uint32 i = 0; i < animation->frameCount; i++)
	{
		asKeyframeData frameData; // Ű������ ������ ��ü�� �����մϴ�.

		// ���� ���� ��忡 ���� �ִϸ��̼� ��尡 ĳ�ÿ��� ã������ �ʴ� ���
		if (findNode == nullptr)
		{
			// ����� ��ȯ ����� ������ ��ġ�� ��, �̸� ������� ��ġ, ȸ��, ������ �����͸� �����մϴ�.
			Matrix transform(srcNode->mTransformation[0]);
			transform = transform.Transpose();
			frameData.time = (float)i;	// ������ �ð��� �����մϴ�.
			transform.Decompose(OUT frameData.scale, OUT frameData.rotation, OUT frameData.translation);
		}
		else
		{
			// ĳ�ÿ��� ã���� �ִϸ��̼� ��忡 �̹� Ű������ �����Ͱ� ������, �ش� �����͸� ����մϴ�.
			frameData = findNode->keyframe[i];
		}
		// ó���� Ű������ �����͸� Ű������ ��ü�� �߰��մϴ�.
		keyframe->transforms.push_back(frameData);
	}

	// ó���� Ű������ ��ü�� �ִϸ��̼��� Ű������ ��Ͽ� �߰��մϴ�.
	animation->keyframes.push_back(keyframe);

	// ���� ����� ��� �ڽ� ��忡 ���� ��������� ������ ó���� �����մϴ�.
	for (uint32 i = 0; i < srcNode->mNumChildren; i++)
		ReadKeyframeData(animation, srcNode->mChildren[i], cache);
}

// �ִϸ��̼� �����͸� ���Ϸ� �����ϴ� �Լ�
void Converter::WriteAnimationData(shared_ptr<asAnimation> animation, wstring finalPath)
{
	// ���� ���� ��θ� �����ϰ�, �ش� ����� �θ� ���丮�� ����ϴ�.
	auto path = filesystem::path(finalPath);

	// ������ ������ �����.
	filesystem::create_directory(path.parent_path());

	// ���� �ۼ��� ���� FileUtils ��ü�� �����ϰ� ������ ���ϴ�.
	shared_ptr<FileUtils> file = make_shared<FileUtils>();
	file->Open(finalPath, FileMode::Write);

	// �ִϸ��̼��� �⺻ ������ ���Ͽ� ����մϴ�.
	file->Write<string>(animation->name); // �ִϸ��̼� �̸�
	file->Write<float>(animation->duration); // �ִϸ��̼��� �� ���� �ð�
	file->Write<float>(animation->frameRate); // �ִϸ��̼��� ������ �ӵ� (�ʴ� ƽ ��)
	file->Write<uint32>(animation->frameCount); // �ִϸ��̼��� �� ������ ��

	// �ִϸ��̼� Ű������ �����͸� ���Ͽ� ����մϴ�.
	file->Write<uint32>(animation->keyframes.size()); // Ű�������� ��

	// �� Ű�����ӿ� ���� ������ ��ȸ�ϸ� ���Ͽ� ����մϴ�.
	for (shared_ptr<asKeyframe> keyframe : animation->keyframes)
	{
		file->Write<string>(keyframe->boneName); // Ű�������� ���� ���� �̸�

		// Ű������ ��ȯ �������� ũ��� ������ ��ü�� ���Ͽ� ����մϴ�.
		file->Write<uint32>(keyframe->transforms.size()); // ��ȯ �������� ��
		file->Write(&keyframe->transforms[0], sizeof(asKeyframeData) * keyframe->transforms.size()); // ��ȯ ������
	}
}


uint32 Converter::GetBoneIndex(const string& name)
{
	for (shared_ptr<asBone>& bone : _bones)
	{
		if (bone->name == name)
			return bone->index;
	}

	assert(false);
	return 0;
}
