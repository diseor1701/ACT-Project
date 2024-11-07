#include "pch.h"
#include "ModelAnimator.h"
#include "Material.h"
#include "ModelMesh.h"
#include "Model.h"
#include "ModelAnimation.h"
#include "Camera.h"
#include "Light.h"

ModelAnimator::ModelAnimator(shared_ptr<Shader> shader)
	: Super(ComponentType::Animator), _shader(shader)
{
	
}

ModelAnimator::~ModelAnimator()
{

}

void ModelAnimator::SetModel(shared_ptr<Model> model)
{
	_model = model;

	const auto& materials = _model->GetMaterials();
	for (auto& material : materials)
	{
		material->SetShader(_shader);
	}
}

void ModelAnimator::SetAnimationState(AnimationState newState)
{
	int stateAsInt = static_cast<int>(newState);

	if (_tweenDesc.curr.state != stateAsInt) // 새로운 상태일 때만 전환
	{
		// 다음 애니메이션 예약
		//_tweenDesc.next.state = stateAsInt;
		//_tweenDesc.next.animIndex = _model->GetAnimationIndexByState(newState); // 상태에 맞는 애니메이션 인덱스를 가져옴
		
		_tweenDesc.curr.state = stateAsInt;
		_tweenDesc.curr.animIndex = _model->GetAnimationIndexByState(newState); // 상태에 맞는 애니메이션 인덱스를 가져옴
		_tweenDesc.tweenSumTime = 0; // 트윈 시작

		// 애니메이션이 변경될 때 첫 프레임 강제 설정
		{
			_tweenDesc.curr.currFrame = 0;
			_tweenDesc.curr.nextFrame = 1 % _model->GetAnimationByIndex(_tweenDesc.curr.animIndex)->frameCount;
			_tweenDesc.curr.sumTime = 0;
			_tweenDesc.curr.ratio = 0;
		}
	}
}

void ModelAnimator::Update()
{

}

void ModelAnimator::UpdateTweenData()
{
	TweenDesc& desc = _tweenDesc;

	// 애니메이션 재생을 위한 시간 누적
	desc.curr.sumTime += DT;
	{
		// 현재 재생 중인 애니메이션 가져오기
		shared_ptr<ModelAnimation> currentAnim = _model->GetAnimationByState(static_cast<AnimationState>(desc.curr.state));
		if (currentAnim)
		{        
			// 프레임 당 시간 계산 (프레임레이트와 재생 속도 고려)
			float timePerFrame = 1 / (currentAnim->frameRate * desc.curr.speed);
			// 누적 시간이 프레임 당 시간을 초과하면 다음 프레임으로
			if (desc.curr.sumTime >= timePerFrame)
			{
				desc.curr.sumTime = 0;
				desc.curr.currFrame = (desc.curr.currFrame + 1) % currentAnim->frameCount;
				desc.curr.nextFrame = (desc.curr.currFrame + 1) % currentAnim->frameCount;
			}

			// 다음 프레임으로 넘어가는 비율 계산
			desc.curr.ratio = (desc.curr.sumTime / timePerFrame);
		}
	}

	// 다음 애니메이션이 예약 되어 있다면
	if (desc.next.animIndex >= 0)
	{
		desc.tweenSumTime += DT;
		desc.tweenRatio = desc.tweenSumTime / desc.tweenDuration;

		if (desc.tweenRatio >= 1.f)
		{
			// 애니메이션 교체 성공
			desc.curr = desc.next;
			desc.ClearNextAnim();
		}
		else
		{
			// 교체중
			shared_ptr<ModelAnimation> nextAnim = _model->GetAnimationByState(static_cast<AnimationState>(desc.next.state));
			desc.next.sumTime += DT;

			float timePerFrame = 1.f / (nextAnim->frameRate * desc.next.speed);

			if (desc.next.ratio >= 1.f)
			{
				desc.next.sumTime = 0;

				desc.next.currFrame = (desc.next.currFrame + 1) % nextAnim->frameCount;
				desc.next.nextFrame = (desc.next.currFrame + 1) % nextAnim->frameCount;
			}

			desc.next.ratio = desc.next.sumTime / timePerFrame;
		}
	}
}
void ModelAnimator::RenderSingle()
{
	if (_model == nullptr)
		return;
	if (_texture == nullptr)
		CreateTexture();

	UpdateTweenData();

	ImGui::InputFloat("Speed", &_tweenDesc.curr.speed, 0.5f, 4.f);

	// 현재 프레임 정보를 렌더러에 전달
	_shader->PushTweenData(GetTweenDesc());

	// GlobalData
	_shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	// Light
	auto lightObj = SCENE->GetCurrentScene()->GetLight();
	if (lightObj)
		_shader->PushLightData(lightObj->GetLight()->GetLightDesc());

	// SRV를 통해 정보 전달
	_shader->GetSRV("TransformMap")->SetResource(_srv.Get());

	// 본 데이터 준비
	BoneDesc boneDesc;

	// 모델의 본 갯수 가져오기
	const uint32 boneCount = _model->GetBoneCount();
	for (uint32 i = 0; i < boneCount; i++)
	{
		shared_ptr<ModelBone> bone = _model->GetBoneByIndex(i);
		// 각 본의 변환 정보를 본 설명자에 저장
		boneDesc.transforms[i] = bone->transform;
	}
	// 렌더러에 본 데이터 전달
	_shader->PushBoneData(boneDesc);

	const auto& meshes = _model->GetMeshes();
	for (auto& mesh : meshes)
	{
		if (mesh->material)
			mesh->material->Update();

		// BoneIndex
		_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		// Transform
		auto world = GetTransform()->GetWorldMatrix();
		_shader->PushTransformData(TransformDesc{ world });

		mesh->vertexBuffer->PushData();
		mesh->indexBuffer->PushData();

		if (Camera::S_IsWireFrame)
			_shader->DrawIndexed(3, _pass, mesh->indexBuffer->GetCount(), 0, 0);
		else
			_shader->DrawIndexed(1, _pass, mesh->indexBuffer->GetCount(), 0, 0);
	}
}

void ModelAnimator::RenderInstancing(shared_ptr<class InstancingBuffer>& buffer)
{
	// 모델이 설정되지 않았다면 업데이트를 진행하지 않음
	if (_model == nullptr)
		return;
	// 텍스처가 없으면 생성
	if (_texture == nullptr)
		CreateTexture();

	// GlobalData
	_shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	// Light
	auto lightObj = SCENE->GetCurrentScene()->GetLight();
	if (lightObj)
		_shader->PushLightData(lightObj->GetLight()->GetLightDesc());

	// SRV를 통해 정보 전달
	_shader->GetSRV("TransformMap")->SetResource(_srv.Get());

	// Bones
	BoneDesc boneDesc;

	const uint32 boneCount = _model->GetBoneCount();
	for (uint32 i = 0; i < boneCount; i++)
	{
		shared_ptr<ModelBone> bone = _model->GetBoneByIndex(i);
		boneDesc.transforms[i] = bone->transform;
	}
	_shader->PushBoneData(boneDesc);

	const auto& meshes = _model->GetMeshes();
	for (auto& mesh : meshes)
	{
		if (mesh->material)
			mesh->material->Update();

		// BoneIndex
		_shader->GetScalar("BoneIndex")->SetInt(mesh->boneIndex);

		mesh->vertexBuffer->PushData();
		mesh->indexBuffer->PushData();

		buffer->PushData();

		if (Camera::S_IsWireFrame)
			_shader->DrawIndexedInstanced(2, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
		else
			_shader->DrawIndexedInstanced(0, _pass, mesh->indexBuffer->GetCount(), buffer->GetCount());
	}
}



InstanceID ModelAnimator::GetInstanceID()
{
	return make_pair((uint64)_model.get(), (uint64)_shader.get());
}

void ModelAnimator::CreateTexture()
{
	if (_model->GetAnimationCount() == 0)
		return;

	_animTransforms.resize(_model->GetAnimationCount());
	for (uint32 i = 0; i < _model->GetAnimationCount(); i++)
		CreateAnimationTransform(i);

	// Creature Texture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = MAX_MODEL_TRANSFORMS * 4;
		desc.Height = MAX_MODEL_KEYFRAMES;
		desc.ArraySize = _model->GetAnimationCount();
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // 16바이트
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;

		const uint32 dataSize = MAX_MODEL_TRANSFORMS * sizeof(Matrix);
		const uint32 pageSize = dataSize * MAX_MODEL_KEYFRAMES;
		void* mallocPtr = ::malloc(pageSize * _model->GetAnimationCount());

		// 파편화된 데이터를 조립한다.
		for (uint32 c = 0; c < _model->GetAnimationCount(); c++)
		{
			uint32 startOffset = c * pageSize;

			BYTE* pageStartPtr = reinterpret_cast<BYTE*>(mallocPtr) + startOffset;

			for (uint32 f = 0; f < MAX_MODEL_KEYFRAMES; f++)
			{
				void* ptr = pageStartPtr + dataSize * f;
				::memcpy(ptr, _animTransforms[c].transforms[f].data(), dataSize);
			}
		}

		// 리소스 만들기
		vector<D3D11_SUBRESOURCE_DATA> subResources(_model->GetAnimationCount());

		for (uint32 c = 0; c < _model->GetAnimationCount(); c++)
		{
			void* ptr = (BYTE*)mallocPtr + c * pageSize;
			subResources[c].pSysMem = ptr;
			subResources[c].SysMemPitch = dataSize;
			subResources[c].SysMemSlicePitch = pageSize;
		}

		HRESULT hr = DEVICE->CreateTexture2D(&desc, subResources.data(), _texture.GetAddressOf());
		CHECK(hr);

		::free(mallocPtr);
	}

	// Create SRV
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipLevels = 1;
		desc.Texture2DArray.ArraySize = _model->GetAnimationCount();

		HRESULT hr = DEVICE->CreateShaderResourceView(_texture.Get(), &desc, _srv.GetAddressOf());
		CHECK(hr);
	}
}

void ModelAnimator::CreateAnimationTransform(uint32 index)
{
	vector<Matrix> tempAnimBoneTransforms(MAX_MODEL_TRANSFORMS, Matrix::Identity);

	shared_ptr<ModelAnimation> animation = _model->GetAnimationByIndex(index);

	for (uint32 f = 0; f < animation->frameCount; f++)
	{
		for (uint32 b = 0; b < _model->GetBoneCount(); b++)
		{
			shared_ptr<ModelBone> bone = _model->GetBoneByIndex(b);

			Matrix matAnimation;

			shared_ptr<ModelKeyframe> frame = animation->GetKeyframe(bone->name);
			if (frame != nullptr)
			{
				ModelKeyframeData& data = frame->transforms[f];

				Matrix S, R, T;
				S = Matrix::CreateScale(data.scale.x, data.scale.y, data.scale.z);
				R = Matrix::CreateFromQuaternion(data.rotation);
				T = Matrix::CreateTranslation(data.translation.x, data.translation.y, data.translation.z);

				matAnimation = S * R * T;
			}
			else
			{
				matAnimation = Matrix::Identity;
			}

			// [ !!!!!!! ]
			Matrix toRootMatrix = bone->transform;
			Matrix invGlobal = toRootMatrix.Invert();

			int32 parentIndex = bone->parentIndex;

			Matrix matParent = Matrix::Identity;
			if (parentIndex >= 0)
				matParent = tempAnimBoneTransforms[parentIndex];
			
			tempAnimBoneTransforms[b] = matAnimation * matParent;

			// 결론
			_animTransforms[index].transforms[f][b] = invGlobal * tempAnimBoneTransforms[b];
		}
	}
}
