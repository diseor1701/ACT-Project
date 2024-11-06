#include "pch.h"
#include "PlayerScript.h"
#include "Model.h"
#include "Camera.h"
#include "ModelAnimator.h"

void PlayerScript::Start()
{
}

void PlayerScript::Update()
{
	float dt = TIME->GetDeltaTime();
	_transform = GetTransform();

	Vec3 _moveDir = Vec3(0.0f);
	bool isRunning = INPUT->GetButton(KEY_TYPE::SHIFT);  // Shift 키로 달리기 모드 여부 확인


	// 이동 입력 처리
	if (INPUT->GetButton(KEY_TYPE::W))
		_moveDir += Vec3(0.0f, 0.0f, 1.0f);
	if (INPUT->GetButton(KEY_TYPE::S))
		_moveDir += Vec3(0.0f, 0.0f, -1.0f);
	if (INPUT->GetButton(KEY_TYPE::A))
		_moveDir += Vec3(-1.0f, 0.0f, 0.0f);
	if (INPUT->GetButton(KEY_TYPE::D))
		_moveDir += Vec3(1.0f, 0.0f, 0.0f);

	// 이동 방향의 크기를 기준으로 애니메이션 상태 결정
	AnimationState targetAnimationState;
	if (_moveDir.LengthSquared() > 0.0f)  // 이동 벡터가 0이 아니라면 이동 중으로 간주
	{
		_moveDir.Normalize();
		float speed = isRunning ? _speed*2 : _speed;
		_transform->SetPosition(_transform->GetPosition() + _moveDir * speed * dt);

		targetAnimationState = isRunning ? AnimationState::Run : AnimationState::Walk;
	}
	else
	{
		targetAnimationState = AnimationState::Idle;
	}

	// 애니메이션 상태가 변경되었을 때만 상태 전환
	if (_currentAnimationState != targetAnimationState)
	{
		_modelAnimator->SetAnimationState(targetAnimationState);
		_currentAnimationState = targetAnimationState;  // 현재 상태 업데이트
	}
	
	if (INPUT->GetButton(KEY_TYPE::KEY_1))
	{
		Camera::S_IsWireFrame = false;
	}

	if (INPUT->GetButton(KEY_TYPE::KEY_2))
	{
		Camera::S_IsWireFrame = true;
	}
}
