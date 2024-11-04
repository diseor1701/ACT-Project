#include "pch.h"
#include "Camera.h"
#include "Scene.h"

Matrix	Camera::S_MatView		= Matrix::Identity;
Matrix	Camera::S_MatProjection = Matrix::Identity;
bool	Camera::S_IsWireFrame	= false;

void Camera::SetCameraOffset(Vec3 v)
{	
	_cameraOffset.x = v.x; 
	_cameraOffset.y = v.y; 
	_cameraOffset.z = v.z; 

	_cameraDistance = _cameraOffset.Length();
}
void Camera::SortGameObject()
{
	shared_ptr<Scene> scene = CUR_SCENE;
	unordered_set<shared_ptr<GameObject>>& gameObjects = scene->GetObjects();

	_vecForward.clear();

	// ��� ���ӿ�����Ʈ�� ������� ���� �׷������ ������� üũ
	for (auto& gameObject : gameObjects)
	{
		if (IsCulled(gameObject->GetLayerIndex()))
			continue;

		if (gameObject->GetMeshRenderer() == nullptr
			&& gameObject->GetModelRenderer() == nullptr
			&& gameObject->GetModelAnimator() == nullptr)
			continue;

		// ���� �׷������ ����� gameObject
		_vecForward.push_back(gameObject);		
	}
}

void Camera::Render_Forward()
{
	// ���� ���� �׸� ī�޶�ϱ� ���� ������� ���� 
	S_MatView = _matView;
	S_MatProjection = _matProjection;

	GET_SINGLE(InstancingManager)->Render(_vecForward);
}

Camera::Camera() : Super(ComponentType::Camera)
{
	_width = static_cast<float>(GAME->GetGameDesc().width);
	_height = static_cast<float>(GAME->GetGameDesc().height);
}

Camera::~Camera()
{

}

void Camera::Update()
{
	// ���콺 �Է��� ���� ī�޶� ��ġ �� ���� ������Ʈ
	UpdateCameraWithMouseInput();
	// ������Ʈ�� ī�޶� ��ġ�� ������ ������� ��Ʈ���� ����
	UpdateMatrix();
}

void Camera::UpdateCameraWithMouseInput()
{
	_player = CUR_SCENE->GetPlayer();

	float dx = INPUT->GetMouseDeltaX(); // x�� ���콺 �̵���
	float dy = INPUT->GetMouseDeltaY(); // y�� ���콺 �̵���

	// yaw�� pitch ������ ���콺 �̵��� ���� ����
	_yaw += dx * _sensitivity;
	_pitch += dy * _sensitivity;

	// pitch ���� ������ �����Ͽ� ī�޶� �������� �ʵ��� ���� (-90�� ~ 90�� ����)
	_pitch = std::clamp(_pitch, -XM_PIDIV2 + 0.1f, XM_PIDIV2 - 0.1f);

	// ī�޶� ��ġ ���
	float x = _cameraDistance * cosf(_pitch) * sinf(_yaw);
	float y = _cameraDistance * sinf(_pitch);
	float z = _cameraDistance * cosf(_pitch) * cosf(_yaw);

	// ī�޶� ��ġ�� �÷��̾� ��ġ �������� ����
	if (_player == nullptr)
	{
		Vec3 cameraposition = GetTransform()->GetPosition();
		_cameraPosition = cameraposition + Vec3(x, y, z);
		_focusPosition = cameraposition;
	}
	else 
	{
		Vec3 playerPosition = _player->GetTransform()->GetPosition();
		_cameraPosition = playerPosition + Vec3(x, y, z);
		_focusPosition = playerPosition;
	}
}

void Camera::UpdateMatrix()
{
	Vec3 eyePosition;
	Vec3 focusPosition;
	Vec3 upDirection;

	if (_type == ProjectionType::Perspective) // Main ī�޶�
	{
		// eyePosition�� focusPosition�� UpdateCameraWithMouseInput���� ������ �� ���
		eyePosition = _cameraPosition;
		focusPosition = _focusPosition;
		upDirection = Vec3(0.0f, 1.0f, 0.0f);
	}
	else // UI ī�޶�
	{
		eyePosition = GetTransform()->GetPosition();
		focusPosition = eyePosition + GetTransform()->GetLook();
		upDirection = GetTransform()->GetUp();
	}

	_matView = ::XMMatrixLookAtLH(eyePosition, focusPosition, upDirection);

	if (_type == ProjectionType::Perspective)
	{
		_matProjection = ::XMMatrixPerspectiveFovLH(_fov, _width / _height, _near, _far);
	}
	else
	{
		_matProjection = ::XMMatrixOrthographicLH(_width, _height, _near, _far);
	}
	
}

