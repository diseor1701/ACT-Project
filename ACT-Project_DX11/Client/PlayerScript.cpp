#include "pch.h"
#include "PlayerScript.h"
#include "Model.h"
#include "ModelAnimator.h"

void PlayerScript::Start()
{
}

void PlayerScript::Update()
{
	float dt = TIME->GetDeltaTime();

	//Vec3 pos = GetTransform()->GetPosition();
	bool hasMoved = false; // �̵��� �߻��ߴ��� ����

	if (INPUT->GetButton(KEY_TYPE::W))
	{
		//pos -= GetTransform()->GetLook() * _speed * dt;
		hasMoved = true;
	}

	if (INPUT->GetButton(KEY_TYPE::S))
	{
		//pos += GetTransform()->GetLook() * _speed * dt;
		hasMoved = true;
	}

	if (INPUT->GetButton(KEY_TYPE::A))
	{
		//pos += GetTransform()->GetRight() * _speed * dt;
		hasMoved = true;
	}

	if (INPUT->GetButton(KEY_TYPE::D))
	{
		//pos -= GetTransform()->GetRight() * _speed * dt;
		hasMoved = true;
	}

	if (hasMoved)
	{
		_modelAnimator->SetAnimationState(AnimationState::Walk);
	}
	else
	{
		_modelAnimator->SetAnimationState(AnimationState::Idle);
	}

	//GetTransform()->SetPosition(pos);
	
	/*
	if (INPUT->GetButton(KEY_TYPE::KEY_1))
	{
		Camera::S_IsWireFrame = false;
	}

	if (INPUT->GetButton(KEY_TYPE::KEY_2))
	{
		Camera::S_IsWireFrame = true;
	}*/
}
