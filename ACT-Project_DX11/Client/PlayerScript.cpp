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
	bool isRunning = INPUT->GetButton(KEY_TYPE::SHIFT);  // Shift Ű�� �޸��� ��� ���� Ȯ��


	// �̵� �Է� ó��
	if (INPUT->GetButton(KEY_TYPE::W))
		_moveDir += Vec3(0.0f, 0.0f, 1.0f);
	if (INPUT->GetButton(KEY_TYPE::S))
		_moveDir += Vec3(0.0f, 0.0f, -1.0f);
	if (INPUT->GetButton(KEY_TYPE::A))
		_moveDir += Vec3(-1.0f, 0.0f, 0.0f);
	if (INPUT->GetButton(KEY_TYPE::D))
		_moveDir += Vec3(1.0f, 0.0f, 0.0f);

	// �̵� ������ ũ�⸦ �������� �ִϸ��̼� ���� ����
	AnimationState targetAnimationState;
	if (_moveDir.LengthSquared() > 0.0f)  // �̵� ���Ͱ� 0�� �ƴ϶�� �̵� ������ ����
	{
		_moveDir.Normalize();
		float speed = isRunning ? _speed*2 : _speed;
		_transform->SetPosition(_transform->GetPosition() + _moveDir * speed * dt);

		targetAnimationState = isRunning ? AnimationState::Run : AnimationState::Walk;

		// �̵� ���⿡ ���� ȸ�� ����
		Vec3 targetForward = _moveDir;					// ĳ���Ͱ� �̵��Ϸ��� ����
		Vec3 currentForward = _transform->GetLook();	// ĳ���Ͱ� ���� �ٶ󺸴� ����

		// �� ���� ������ ������ ����Ͽ� ȸ��
		float angle = std::acos(currentForward.Dot(targetForward));	// �� ���� ������ ����
		Vec3 rotationAxis = currentForward.Cross(targetForward);	// �� ���Ͱ� �̷�� ����� ��������
		rotationAxis.Normalize();
		if (rotationAxis != Vec3(0.f))	// rotationAxis 0�϶� ��������
		{
			// rotationAxis ���� �������� angle��ŭ ȸ���ϴ� ���ʹϾ� �����ϰ� ���Ϸ� ������ ��ȯ �� ���� ����
			Quaternion rotation = Quaternion::CreateFromAxisAngle(rotationAxis, angle);
			_transform->SetLocalRotation(Transform::ToEulerAngles(rotation));
		}
	}
	else
	{
		targetAnimationState = AnimationState::Idle;
	}

	// �ִϸ��̼� ���°� ����Ǿ��� ���� ���� ��ȯ
	if (_currentAnimationState != targetAnimationState)
	{
		_modelAnimator->SetAnimationState(targetAnimationState);
		_currentAnimationState = targetAnimationState;  // ���� ���� ������Ʈ
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
