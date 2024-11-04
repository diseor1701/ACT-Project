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

// 모델 데이터를 내보내는 함수
void Converter::ExportModelData(wstring savePath)
{
	// 최종 저장될 파일 경로를 생성합니다.
	wstring finalPath = _modelPath + savePath + L".mesh";

	// Assimp 라이브러리를 사용하여 읽어온 3D 모델의 루트 노드부터 시작하여
	// 모델 데이터를 순회하며 읽어들입니다.
	ReadModelData(_scene->mRootNode, -1, -1);

	// 메시에 적용될 스킨(본) 데이터를 읽어들입니다.
	ReadSkinData();

	// 본과 메시 정보를 포함한 CSV 파일을 작성합니다.
	// 이 파일은 모델의 구조를 이해하거나 디버깅에 유용하게 사용될 수 있습니다.
	{
		FILE* file;
		// CSV 파일을 쓰기 모드로 엽니다.
		::fopen_s(&file, "../Vertices.csv", "w");

		// 모든 본 정보를 파일에 씁니다.
		for (shared_ptr<asBone>& bone : _bones)
		{
			string name = bone->name;
			// 본의 인덱스와 이름을 파일에 기록합니다.
			::fprintf(file, "%d,%s\n", bone->index, bone->name.c_str());
		}

		// 메시 데이터를 파일에 씁니다.
		::fprintf(file, "\n");

		for (shared_ptr<asMesh>& mesh : _meshes)
		{
			string name = mesh->name;
			// 메시 이름을 콘솔에 출력합니다(디버깅 용도).
			::printf("%s\n", name.c_str());

			// 메시의 각 정점에 대한 정보를 파일에 기록합니다.
			for (UINT i = 0; i < mesh->vertices.size(); i++)
			{
				Vec3 p = mesh->vertices[i].position; // 정점의 위치
				Vec4 indices = mesh->vertices[i].blendIndices; // 본 인덱스
				Vec4 weights = mesh->vertices[i].blendWeights; // 본 가중치

				// 위치, 본 인덱스, 본 가중치를 파일에 기록합니다.
				::fprintf(file, "%f,%f,%f,", p.x, p.y, p.z);
				::fprintf(file, "%f,%f,%f,%f,", indices.x, indices.y, indices.z, indices.w);
				::fprintf(file, "%f,%f,%f,%f\n", weights.x, weights.y, weights.z, weights.w);
			}
		}

		// 파일 작성을 마치고 파일을 닫습니다.
		::fclose(file);
	}

	// 변환된 모델 데이터를 `.mesh` 파일 형식으로 최종 저장합니다.
	WriteModelFile(finalPath);
}

// 재질 데이터를 내보내는 함수
void Converter::ExportMaterialData(wstring savePath)
{
	wstring finalPath = _texturePath + savePath + L".xml"; // 최종 파일 경로
	ReadMaterialData(); // 재질 데이터 읽기
	WriteMaterialData(finalPath); // 재질 파일 쓰기
}

// 애니메이션 데이터를 내보내는 함수
void Converter::ExportAnimationData(wstring savePath, uint32 index /*= 0*/)
{
	wstring finalPath = _modelPath + savePath + L".clip";
	assert(index < _scene->mNumAnimations);
	shared_ptr<asAnimation> animation = ReadAnimationData(_scene->mAnimations[index]);
	WriteAnimationData(animation, finalPath);
}

// 모델의 노드(본) 데이터를 읽고 처리하는 함수
void Converter::ReadModelData(aiNode* node, int32 index, int32 parent)
{
	// 새로운 본 객체를 생성하고 기본 정보를 설정합니다.
	shared_ptr<asBone> bone = make_shared<asBone>();
	bone->index = index; // 본의 고유 인덱스
	bone->parent = parent; // 부모 본의 인덱스
	bone->name = node->mName.C_Str(); // 본의 이름

	// 본의 로컬 변환 행렬을 가져오고, 전치(transpose)하여 저장합니다.
	// Assimp는 열 기준(column-major) 행렬을 사용하지만, DirectX나 OpenGL은 행 기준(row-major) 행렬을 사용할 수 있으므로, 전치가 필요할 수 있습니다.
	Matrix transform(node->mTransformation[0]);
	bone->transform = transform.Transpose();

	// 루트(혹은 부모) 본으로부터 상대적인 변환을 계산합니다.
	Matrix matParent = Matrix::Identity; // 기본값으로 단위 행렬을 사용
	if (parent >= 0) {
		matParent = _bones[parent]->transform; // 부모 본의 변환 행렬을 가져옵니다.
	}

	// 최종적으로 본의 변환 행렬을 계산합니다.
	// 본 자신의 로컬 변환에 부모의 변환 행렬을 곱합니다.
	bone->transform = bone->transform * matParent;

	// 처리된 본 정보를 내부 리스트에 추가합니다.
	_bones.push_back(bone);

	// 현재 노드(본)에 연결된 메시 데이터를 읽어들입니다.
	ReadMeshData(node, index);

	// 현재 노드의 모든 자식 노드를 재귀적으로 탐색하여 같은 처리를 반복합니다.
	for (uint32 i = 0; i < node->mNumChildren; i++) {
		ReadModelData(node->mChildren[i], _bones.size(), index);
	}
}


// 노드에서 메시 데이터를 읽어와 내부 구조체에 저장하는 함수
void Converter::ReadMeshData(aiNode* node, int32 bone)
{
	if (node->mNumMeshes < 1)
		return; // 메시가 없는 노드는 처리하지 않음

	shared_ptr<asMesh> mesh = make_shared<asMesh>(); // 새 메시 객체 생성
	mesh->name = node->mName.C_Str(); // 메시 이름 설정
	mesh->boneIndex = bone; // 메시와 연결된 본 인덱스 설정

	// 노드에 포함된 모든 메시에 대해 반복
	for (uint32 i = 0; i < node->mNumMeshes; i++) {
		uint32 index = node->mMeshes[i];
		const aiMesh* srcMesh = _scene->mMeshes[index]; // 소스 메시 참조

		// 메시의 재질 이름을 가져옴
		const aiMaterial* material = _scene->mMaterials[srcMesh->mMaterialIndex];
		mesh->materialName = material->GetName().C_Str(); // 재질 이름 설정

		const uint32 startVertex = mesh->vertices.size(); // 현재 메시의 시작 정점 인덱스

		// 메시의 모든 정점에 대해 반복
		for (uint32 v = 0; v < srcMesh->mNumVertices; v++)
		{
			VertexType vertex; // 새 정점 객체
			// 정점 위치, UV 좌표, 법선 벡터 복사
			::memcpy(&vertex.position, &srcMesh->mVertices[v], sizeof(Vec3));
			if (srcMesh->HasTextureCoords(0))
				::memcpy(&vertex.uv, &srcMesh->mTextureCoords[0][v], sizeof(Vec2));
			if (srcMesh->HasNormals())
				::memcpy(&vertex.normal, &srcMesh->mNormals[v], sizeof(Vec3));

			mesh->vertices.push_back(vertex); // 정점을 메시에 추가
		}

		// 메시의 모든 면(페이스)에 대해 반복하여 인덱스 정보 추출
		for (uint32 f = 0; f < srcMesh->mNumFaces; f++)
		{
			aiFace& face = srcMesh->mFaces[f];
			for (uint32 k = 0; k < face.mNumIndices; k++)
				mesh->indices.push_back(face.mIndices[k] + startVertex); // 인덱스를 메시에 추가
		}
	}

	_meshes.push_back(mesh); // 메시를 내부 메시 리스트에 추가
}
// 모델의 본과 정점 가중치 정보를 읽어와 처리하는 함수
void Converter::ReadSkinData()
{
	// 모든 메시에 대해 반복
	for (uint32 i = 0; i < _scene->mNumMeshes; i++) {
		aiMesh* srcMesh = _scene->mMeshes[i];
		if (!srcMesh->HasBones()) continue; // 본이 없으면 건너뜀

		shared_ptr<asMesh> mesh = _meshes[i]; // 현재 메시 참조

		vector<asBoneWeights> tempVertexBoneWeights;
		tempVertexBoneWeights.resize(mesh->vertices.size()); // 정점별 가중치 리스트 초기화

		// 모든 본에 대해 반복하여 가중치 정보 추출
		for (uint32 b = 0; b < srcMesh->mNumBones; b++) {
			aiBone* srcMeshBone = srcMesh->mBones[b];
			uint32 boneIndex = GetBoneIndex(srcMeshBone->mName.C_Str()); // 본 인덱스 검색

			for (uint32 w = 0; w < srcMeshBone->mNumWeights; w++) {
				uint32 index = srcMeshBone->mWeights[w].mVertexId; // 정점 인덱스
				float weight = srcMeshBone->mWeights[w].mWeight; // 가중치

				tempVertexBoneWeights[index].AddWeights(boneIndex, weight); // 가중치 정보 추가
			}
		}

		// 최종 가중치 정보를 정점 데이터에 적용
		for (uint32 v = 0; v < tempVertexBoneWeights.size(); v++)
		{
			tempVertexBoneWeights[v].Normalize(); // 가중치 정규화

			asBlendWeight blendWeight = tempVertexBoneWeights[v].GetBlendWeights();
			mesh->vertices[v].blendIndices = blendWeight.indices; // 본 인덱스 설정
			mesh->vertices[v].blendWeights = blendWeight.weights; // 본 가중치 설정
		}
	}
}

// 모델 파일을 작성하는 함수
void Converter::WriteModelFile(wstring finalPath)
{
	auto path = filesystem::path(finalPath);

	// 최종 파일이 위치할 디렉토리를 생성합니다. 이미 존재하면 건너뜁니다.
	filesystem::create_directory(path.parent_path());

	shared_ptr<FileUtils> file = make_shared<FileUtils>();
	file->Open(finalPath, FileMode::Write); // 파일을 쓰기 모드로 엽니다.

	// 본 데이터를 파일에 씁니다.
	file->Write<uint32>(_bones.size()); // 본의 개수를 기록합니다.
	for (shared_ptr<asBone>& bone : _bones)
	{
		// 각 본에 대한 정보를 파일에 기록합니다.
		file->Write<int32>(bone->index); // 본의 인덱스
		file->Write<string>(bone->name); // 본의 이름
		file->Write<int32>(bone->parent); // 부모 본의 인덱스
		file->Write<Matrix>(bone->transform); // 본의 변환 행렬
	}

	// 메시 데이터를 파일에 씁니다.
	file->Write<uint32>(_meshes.size()); // 메시의 개수를 기록합니다.
	for (shared_ptr<asMesh>& meshData : _meshes)
	{
		// 각 메시에 대한 정보를 파일에 기록합니다.
		file->Write<string>(meshData->name); // 메시 이름
		file->Write<int32>(meshData->boneIndex); // 연결된 본의 인덱스
		file->Write<string>(meshData->materialName); // 사용하는 재질의 이름

		// 정점 데이터를 파일에 씁니다.
		file->Write<uint32>(meshData->vertices.size()); // 정점의 개수
		file->Write(&meshData->vertices[0], sizeof(VertexType) * meshData->vertices.size()); // 정점 데이터

		// 인덱스 데이터를 파일에 씁니다.
		file->Write<uint32>(meshData->indices.size()); // 인덱스의 개수
		file->Write(&meshData->indices[0], sizeof(uint32) * meshData->indices.size()); // 인덱스 데이터
	}
}

void Converter::ReadMaterialData()
{
	for (uint32 i = 0; i < _scene->mNumMaterials; i++)
	{
		aiMaterial* srcMaterial = _scene->mMaterials[i];
		shared_ptr<asMaterial> material = make_shared<asMaterial>();
		material->name = srcMaterial->GetName().C_Str(); // 재질의 이름

		// 주변광, 확산광, 반사광, 자체발광 속성을 읽어들입니다.
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

		// 텍스처 파일 경로를 읽어들입니다.
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

		// 재질 정보를 내부 리스트에 추가합니다.
		_materials.push_back(material);
	}
}

// 재질 데이터를 XML 형식으로 저장하는 함수
void Converter::WriteMaterialData(wstring finalPath)
{
	auto path = filesystem::path(finalPath);

	// 최종 저장 경로에 해당하는 폴더가 없으면 생성합니다.
	filesystem::create_directory(path.parent_path());

	string folder = path.parent_path().string();
	// XML 문서 객체를 생성합니다.
	shared_ptr<tinyxml2::XMLDocument> document = make_shared<tinyxml2::XMLDocument>();
	// XML 선언부를 추가합니다.

	tinyxml2::XMLDeclaration* decl = document->NewDeclaration();
	document->LinkEndChild(decl);
	// 재질 데이터의 루트 엘리먼트를 생성하고 문서에 추가합니다.
	tinyxml2::XMLElement* root = document->NewElement("Materials");
	document->LinkEndChild(root);
	// 모든 재질에 대해 반복합니다.
	for (shared_ptr<asMaterial> material : _materials)
	{
		// 각 재질에 대한 정보를 XML 엘리먼트로 추가합니다.
		tinyxml2::XMLElement* node = document->NewElement("Material");
		root->LinkEndChild(node);
		// 재질의 이름, 텍스처 파일 경로, 색상 정보 등을 엘리먼트의 속성으로 추가합니다.
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
		// 각 색상 속성(ambient, diffuse, specular, emissive)에 대한 정보를 추가합니다.
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
	// XML 문서를 파일로 저장합니다.
	document->SaveFile(Utils::ToString(finalPath).c_str());
}

// 텍스처 파일을 저장하거나 기존 텍스처 파일을 새 위치로 복사하는 함수
string Converter::WriteTexture(string saveFolder, string file)
{
	string fileName = filesystem::path(file).filename().string();
	string folderName = filesystem::path(saveFolder).filename().string();
	// 임베디드 텍스처 또는 외부 텍스처 파일의 존재 여부를 확인합니다.
	const aiTexture* srcTexture = _scene->GetEmbeddedTexture(file.c_str());
	if (srcTexture) {
		// 텍스처 데이터가 임베디드되어 있으면 새 파일로 저장합니다.
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
		// 외부 텍스처 파일이면 새 위치로 복사합니다.
		string originStr = (filesystem::path(_assetPath) / folderName / file).string();
		Utils::Replace(originStr, "\\", "/");

		string pathStr = (filesystem::path(saveFolder) / fileName).string();
		Utils::Replace(pathStr, "\\", "/");

		::CopyFileA(originStr.c_str(), pathStr.c_str(), false);
	}

	return fileName;// 처리된 텍스처 파일의 이름을 반환합니다.
}

// Assimp 애니메이션 데이터를 읽어 사용자 정의 애니메이션 객체로 변환하는 함수
std::shared_ptr<asAnimation> Converter::ReadAnimationData(aiAnimation* srcAnimation)
{
	// 새로운 애니메이션 객체를 생성합니다.
	shared_ptr<asAnimation> animation = make_shared<asAnimation>();
	// 애니메이션 이름을 설정합니다.
	animation->name = srcAnimation->mName.C_Str();
	// 애니메이션의 프레임 속도를 설정합니다. (초당 틱 수)
	animation->frameRate = (float)srcAnimation->mTicksPerSecond;
	// 애니메이션의 프레임 수를 설정합니다. (지속 시간 기반)
	animation->frameCount = (uint32)srcAnimation->mDuration + 1;

	// 애니메이션 노드를 캐싱하기 위한 맵을 선언합니다.
	map<string, shared_ptr<asAnimationNode>> cacheAnimNodes;
	// Assimp 애니메이션 채널을 순회하며 노드별 애니메이션 데이터를 처리합니다.
	for (uint32 i = 0; i < srcAnimation->mNumChannels; i++)
	{
		aiNodeAnim* srcNode = srcAnimation->mChannels[i];

		// 애니메이션 노드 데이터 파싱
		shared_ptr<asAnimationNode> node = ParseAnimationNode(animation, srcNode);
		if (node->keyframe.size() == 0)
			continue;

		// 현재 찾은 노드 중에 제일 긴 시간으로 애니메이션 시간 갱신
		animation->duration = max(animation->duration, node->keyframe.back().time);
		// 파싱된 노드를 캐시에 추가합니다.
		cacheAnimNodes[srcNode->mNodeName.C_Str()] = node;
	}
	// 애니메이션 키프레임 데이터를 처리합니다
	ReadKeyframeData(animation, _scene->mRootNode, cacheAnimNodes);

	return animation;
}

// 애니메이션 노드를 파싱하는 함수
std::shared_ptr<asAnimationNode> Converter::ParseAnimationNode(shared_ptr<asAnimation> animation, aiNodeAnim* srcNode)
{
	// 새로운 애니메이션 노드 객체를 생성
	std::shared_ptr<asAnimationNode> node = make_shared<asAnimationNode>();
	// 노드 이름 설정
	node->name = srcNode->mNodeName;

	// 위치, 회전, 스케일 중 가장 많은 키프레임을 가진 것을 기준으로 총 키프레임 수를 결정
	uint32 keyCount = max(max(srcNode->mNumPositionKeys, srcNode->mNumScalingKeys), srcNode->mNumRotationKeys);

	// 각 키프레임에 대해 반복
	for (uint32 k = 0; k < keyCount; k++)
	{
		// 키프레임 데이터 객체
		asKeyframeData frameData;

		// 키프레임이 발견되었는지 여부
		bool found = false;
		// 현재 키프레임의 인덱스
		uint32 t = node->keyframe.size();

		// 위치, 회전, 스케일 키프레임을 처리합니다. 각 키프레임의 시간과 데이터를 추출하여 frameData에 저장합니다.
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

	// 애니메이션의 키프레임 수보다 노드의 키프레임 수가 적은 경우, 마지막 키프레임을 복제하여 채웁니다.
	if (node->keyframe.size() < animation->frameCount && node->keyframe.size() != 0)
	{
		uint32 count = animation->frameCount - node->keyframe.size(); // 채워야할 키프레임 수
		asKeyframeData keyFrame = node->keyframe.back(); // 마지막 키프레임

		for (uint32 n = 0; n < count; n++)
			node->keyframe.push_back(keyFrame); // 키프레임 복제하여 추가
	}

	return node;
}

// 애니메이션 데이터에서 특정 노드의 키프레임 데이터를 읽어 내부 데이터 구조에 저장하는 함수
void Converter::ReadKeyframeData(shared_ptr<asAnimation> animation, aiNode* srcNode, map<string, shared_ptr<asAnimationNode>>& cache)
{
	// 새로운 키프레임 객체를 생성합니다.
	shared_ptr<asKeyframe> keyframe = make_shared<asKeyframe>();
	// 현재 노드(본)의 이름을 키프레임의 본 이름으로 설정합니다.
	keyframe->boneName = srcNode->mName.C_Str();

	// 현재 노드에 해당하는 애니메이션 노드를 찾습니다.
	shared_ptr<asAnimationNode> findNode = cache[srcNode->mName.C_Str()];

	// 애니메이션의 모든 프레임에 대해 반복합니다.
	for (uint32 i = 0; i < animation->frameCount; i++)
	{
		asKeyframeData frameData; // 키프레임 데이터 객체를 생성합니다.

		// 만약 현재 노드에 대한 애니메이션 노드가 캐시에서 찾아지지 않는 경우
		if (findNode == nullptr)
		{
			// 노드의 변환 행렬을 가져와 전치한 뒤, 이를 기반으로 위치, 회전, 스케일 데이터를 추출합니다.
			Matrix transform(srcNode->mTransformation[0]);
			transform = transform.Transpose();
			frameData.time = (float)i;	// 프레임 시간을 설정합니다.
			transform.Decompose(OUT frameData.scale, OUT frameData.rotation, OUT frameData.translation);
		}
		else
		{
			// 캐시에서 찾아진 애니메이션 노드에 이미 키프레임 데이터가 있으면, 해당 데이터를 사용합니다.
			frameData = findNode->keyframe[i];
		}
		// 처리된 키프레임 데이터를 키프레임 객체에 추가합니다.
		keyframe->transforms.push_back(frameData);
	}

	// 처리된 키프레임 객체를 애니메이션의 키프레임 목록에 추가합니다.
	animation->keyframes.push_back(keyframe);

	// 현재 노드의 모든 자식 노드에 대해 재귀적으로 동일한 처리를 수행합니다.
	for (uint32 i = 0; i < srcNode->mNumChildren; i++)
		ReadKeyframeData(animation, srcNode->mChildren[i], cache);
}

// 애니메이션 데이터를 파일로 저장하는 함수
void Converter::WriteAnimationData(shared_ptr<asAnimation> animation, wstring finalPath)
{
	// 최종 파일 경로를 생성하고, 해당 경로의 부모 디렉토리를 만듭니다.
	auto path = filesystem::path(finalPath);

	// 폴더가 없으면 만든다.
	filesystem::create_directory(path.parent_path());

	// 파일 작성을 위한 FileUtils 객체를 생성하고 파일을 엽니다.
	shared_ptr<FileUtils> file = make_shared<FileUtils>();
	file->Open(finalPath, FileMode::Write);

	// 애니메이션의 기본 정보를 파일에 기록합니다.
	file->Write<string>(animation->name); // 애니메이션 이름
	file->Write<float>(animation->duration); // 애니메이션의 총 지속 시간
	file->Write<float>(animation->frameRate); // 애니메이션의 프레임 속도 (초당 틱 수)
	file->Write<uint32>(animation->frameCount); // 애니메이션의 총 프레임 수

	// 애니메이션 키프레임 데이터를 파일에 기록합니다.
	file->Write<uint32>(animation->keyframes.size()); // 키프레임의 수

	// 각 키프레임에 대한 정보를 순회하며 파일에 기록합니다.
	for (shared_ptr<asKeyframe> keyframe : animation->keyframes)
	{
		file->Write<string>(keyframe->boneName); // 키프레임이 속한 본의 이름

		// 키프레임 변환 데이터의 크기와 데이터 자체를 파일에 기록합니다.
		file->Write<uint32>(keyframe->transforms.size()); // 변환 데이터의 수
		file->Write(&keyframe->transforms[0], sizeof(asKeyframeData) * keyframe->transforms.size()); // 변환 데이터
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
