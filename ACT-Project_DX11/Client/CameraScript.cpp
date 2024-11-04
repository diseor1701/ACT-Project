#include "pch.h"
#include "Camera.h"
#include "CameraScript.h"
#include "Transform.h"

void CameraScript::Start()
{

}

void CameraScript::Update()
{
	float dt = TIME->GetDeltaTime();

	// Right, Up, Look ���� ������Ʈ (�� ����� �� ���͸� ���)
	Vec3 right = Vec3(Camera::S_MatView._11, Camera::S_MatView._21, Camera::S_MatView._31);  // X�� ����
	Vec3 up = Vec3(Camera::S_MatView._12, Camera::S_MatView._22, Camera::S_MatView._32);     // Y�� ����
	Vec3 look = Vec3(Camera::S_MatView._13, Camera::S_MatView._23, Camera::S_MatView._33);   // Z�� ����

	// ���� ����ȭ (ī�޶��� ���� ���͵��� ���� ���Ͱ� �Ǿ�� ��)
	right.Normalize();
	up.Normalize();
	look.Normalize();


	Vec3 pos = GetTransform()->GetPosition();

	if (INPUT->GetButton(KEY_TYPE::W))
		pos += look * _speed * dt;

	if (INPUT->GetButton(KEY_TYPE::S))
		pos -= look * _speed * dt;

	if (INPUT->GetButton(KEY_TYPE::A))
		pos -= right * _speed * dt;

	if (INPUT->GetButton(KEY_TYPE::D))
		pos += right * _speed * dt;

	GetTransform()->SetPosition(pos);

	//if (INPUT->GetButton(KEY_TYPE::Q))
	//{
	//	Vec3 rotation = GetTransform()->GetLocalRotation();
	//	rotation.x += dt * 0.5f;
	//	GetTransform()->SetLocalRotation(rotation);
	//}

	//if (INPUT->GetButton(KEY_TYPE::E))
	//{
	//	Vec3 rotation = GetTransform()->GetLocalRotation();
	//	rotation.x -= dt * 0.5f;
	//	GetTransform()->SetLocalRotation(rotation);
	//}

	//if (INPUT->GetButton(KEY_TYPE::Z))
	//{
	//	Vec3 rotation = GetTransform()->GetLocalRotation();
	//	rotation.y += dt * 0.5f;
	//	GetTransform()->SetLocalRotation(rotation);
	//}

	//if (INPUT->GetButton(KEY_TYPE::C))
	//{
	//	Vec3 rotation = GetTransform()->GetLocalRotation();
	//	rotation.y -= dt * 0.5f;
	//	GetTransform()->SetLocalRotation(rotation);
	//}

	if (INPUT->GetButton(KEY_TYPE::KEY_1))
	{
		Camera::S_IsWireFrame = false;
	}

	if (INPUT->GetButton(KEY_TYPE::KEY_2))
	{
		Camera::S_IsWireFrame = true;
	}
}
