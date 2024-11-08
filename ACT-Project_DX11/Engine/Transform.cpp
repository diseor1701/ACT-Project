#include "pch.h"
#include "Transform.h"

Transform::Transform() : Super(ComponentType::Transform)
{

}

Transform::~Transform()
{

}

void Transform::Awake()
{
}

void Transform::Update()
{
}
// 쿼터니언 -> 오일러 각도 변환
Vec3 Transform::ToEulerAngles(Quaternion q)
{
	Vec3 angles;

	// roll (x-axis rotation)
	double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	angles.x = std::atan2(sinr_cosp, cosr_cosp);

	//// pitch (y-axis rotation)
	//double sinp = 2 * (q.w * q.y - q.z * q.x);
	//if (std::abs(sinp) >= 1)
	//	angles.y = std::copysign(XM_PI / 2, sinp); // use 90 degrees if out of range
	//else
	//	angles.y = std::asin(sinp);
	
	// pitch (y-axis rotation)
	double sinp = std::sqrt(1 + 2 * (q.w * q.y - q.x * q.z));
	double cosp = std::sqrt(1 - 2 * (q.w * q.y - q.x * q.z));
	angles.y = 2 * std::atan2(sinp, cosp) - XM_PI / 2;

	// yaw (z-axis rotation)
	double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	angles.z = std::atan2(siny_cosp, cosy_cosp);

	return angles;
}

//void Transform::SetLocalRotation(const Vec3& localRotation)
//{
//	// _localRotation의 각도 값을 라디안으로 변환
//	float rotationX = XMConvertToRadians(localRotation.x);
//	float rotationY = XMConvertToRadians(localRotation.y);
//	float rotationZ = XMConvertToRadians(localRotation.z);
//
//	_localRotation.x = rotationX;
//	_localRotation.y = rotationY;
//	_localRotation.z = rotationZ;
//
//	UpdateTransform();
//}

void Transform::UpdateTransform()
{
	Matrix matScale = Matrix::CreateScale(_localScale);
	Matrix matRotation = Matrix::CreateRotationX(_localRotation.x);
	matRotation *= Matrix::CreateRotationY(_localRotation.y);
	matRotation *= Matrix::CreateRotationZ(_localRotation.z);
	Matrix matTranslation = Matrix::CreateTranslation(_localPosition);

	_matLocal = matScale * matRotation * matTranslation;

	if (HasParent())
	{
		_matWorld = _matLocal * _parent->GetWorldMatrix();
	}
	else
	{
		_matWorld = _matLocal;
	}

	Quaternion quat;
	_matWorld.Decompose(_scale, quat, _position);
	_rotation = ToEulerAngles(quat);

	// Children
	for (const shared_ptr<Transform>& child : _children)
		child->UpdateTransform();
}

void Transform::SetScale(const Vec3& worldScale)
{
	if (HasParent())
	{
		Vec3 parentScale = _parent->GetScale();
		Vec3 scale = worldScale;
		scale.x /= parentScale.x;
		scale.y /= parentScale.y;
		scale.z /= parentScale.z;
		SetLocalScale(scale);
	}
	else
	{
		SetLocalScale(worldScale);
	}
}

void Transform::SetRotation(const Vec3& worldRotation)
{
	if (HasParent())
	{
		Matrix inverseMatrix = _parent->GetWorldMatrix().Invert();

		Vec3 rotation;
		rotation.TransformNormal(worldRotation, inverseMatrix);

		SetLocalRotation(rotation);
	}
	else
		SetLocalRotation(worldRotation);
}

void Transform::SetPosition(const Vec3& worldPosition)
{

	if (HasParent())
	{
		Matrix worldToParentLocalMatrix = _parent->GetWorldMatrix().Invert();

		Vec3 position;
		position.Transform(worldPosition, worldToParentLocalMatrix);

		SetLocalPosition(position);
	}
	else
	{
		SetLocalPosition(worldPosition);
	}
}

Vec3 Transform::GetRight()
{
	Vec3 right = _matWorld.Right();
	right.x = std::abs(right.x) < 1e-5 ? 0.0f : right.x;
	right.y = std::abs(right.y) < 1e-5 ? 0.0f : right.y;
	right.z = std::abs(right.z) < 1e-5 ? 0.0f : right.z;
	right.Normalize();
	return right;
}
Vec3 Transform::GetUp()
{
	Vec3 up = _matWorld.Up();
	up.x = std::abs(up.x) < 1e-5 ? 0.0f : up.x;
	up.y = std::abs(up.y) < 1e-5 ? 0.0f : up.y;
	up.z = std::abs(up.z) < 1e-5 ? 0.0f : up.z;
	up.Normalize();
	return up;
}
Vec3 Transform::GetLook()
{
	Vec3 backward = _matWorld.Backward(); 
	backward.x = std::abs(backward.x) < 1e-5 ? 0.0f : backward.x;
	backward.y = std::abs(backward.y) < 1e-5 ? 0.0f : backward.y;
	backward.z = std::abs(backward.z) < 1e-5 ? 0.0f : backward.z;
	backward.Normalize();
	return backward;
}