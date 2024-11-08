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
	transform = GetTransform();

	Vec3 moveDir = Vec3(0.0f);
	bool isRunning = INPUT->GetButton(KEY_TYPE::SHIFT);  // Shift 키로 달리기 모드 여부 확인
	bool isAttack = false;  // Shift 키로 달리기 모드 여부 확인


	// 이동 입력 처리
	if (INPUT->GetButton(KEY_TYPE::W))
		moveDir += Vec3(0.0f, 0.0f, 1.0f);
	if (INPUT->GetButton(KEY_TYPE::S))
		moveDir += Vec3(0.0f, 0.0f, -1.0f);
	if (INPUT->GetButton(KEY_TYPE::A))
		moveDir += Vec3(-1.0f, 0.0f, 0.0f);
	if (INPUT->GetButton(KEY_TYPE::D))
		moveDir += Vec3(1.0f, 0.0f, 0.0f);
	if (INPUT->GetButton(KEY_TYPE::LBUTTON))
		isAttack = true;

	if (isAttack)
	{
		_modelAnimator->SetAnimationState(AnimationState::Attack4);
	}

	// 이동 방향의 크기를 기준으로 애니메이션 상태 결정
	AnimationState targetAnimationState;
	if (moveDir.LengthSquared() > 0.0f)  // 이동 벡터가 0이 아니라면 이동 중으로 간주
	{
		moveDir.Normalize();
		float speed = isRunning ? _speed*2 : _speed;
		transform->SetPosition(transform->GetPosition() + moveDir * speed * dt);

		targetAnimationState = isRunning ? AnimationState::Run : AnimationState::Walk;


		// 이동 방향에 따라 회전 설정
		Vec3 targetForward = moveDir;					// 캐릭터가 이동하려는 방향
		Vec3 currentForward = transform->GetLook();	// 캐릭터가 현재 바라보는 방향

		// 두 벡터 사이의 각도를 계산하여 회전
		float angle = std::acos(currentForward.Dot(targetForward));	// 두 벡터 사이의 각도
		if (angle != 0.f)
		{
			Vec3 rotationAxis = currentForward.Cross(targetForward);	// 두 벡터가 이루는 평면의 법선벡터
			rotationAxis.Normalize();

			// 회전 축의 y 값으로 좌우 방향을 구분
			if (rotationAxis.y < 0) {
				angle = -angle;  // 왼쪽으로 회전
			}
			transform->SetRotation(transform->GetRotation() + Vec3(0, angle, 0));

		}

		// Debug Object
		{
			_look->GetTransform()->SetPosition(transform->GetPosition() + transform->GetLook() * 2.5f);
			_up->GetTransform()->SetPosition(transform->GetPosition() + transform->GetUp() * 2.5f);
			_right->GetTransform()->SetPosition(transform->GetPosition() + transform->GetRight() * 2.5f);

			_look->GetTransform()->SetRotation(transform->GetRotation());
			_up->GetTransform()->SetRotation(transform->GetRotation());
			_right->GetTransform()->SetRotation(transform->GetRotation());
		}
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
