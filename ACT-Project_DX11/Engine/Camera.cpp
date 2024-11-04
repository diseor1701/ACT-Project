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

	// 모든 게임오브젝트를 대상으로 내가 그려줘야할 대상인지 체크
	for (auto& gameObject : gameObjects)
	{
		if (IsCulled(gameObject->GetLayerIndex()))
			continue;

		if (gameObject->GetMeshRenderer() == nullptr
			&& gameObject->GetModelRenderer() == nullptr
			&& gameObject->GetModelAnimator() == nullptr)
			continue;

		// 내가 그려줘야할 대상인 gameObject
		_vecForward.push_back(gameObject);		
	}
}

void Camera::Render_Forward()
{
	// 내가 이제 그릴 카메라니까 나의 정보들로 갱신 
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
	// 마우스 입력을 통한 카메라 위치 및 방향 업데이트
	UpdateCameraWithMouseInput();
	// 업데이트된 카메라 위치와 방향을 기반으로 매트릭스 설정
	UpdateMatrix();
}

void Camera::UpdateCameraWithMouseInput()
{
	_player = CUR_SCENE->GetPlayer();

	float dx = INPUT->GetMouseDeltaX(); // x축 마우스 이동량
	float dy = INPUT->GetMouseDeltaY(); // y축 마우스 이동량

	// yaw와 pitch 각도를 마우스 이동에 따라 조절
	_yaw += dx * _sensitivity;
	_pitch += dy * _sensitivity;

	// pitch 값의 범위를 제한하여 카메라가 뒤집히지 않도록 조정 (-90도 ~ 90도 사이)
	_pitch = std::clamp(_pitch, -XM_PIDIV2 + 0.1f, XM_PIDIV2 - 0.1f);

	// 카메라 위치 계산
	float x = _cameraDistance * cosf(_pitch) * sinf(_yaw);
	float y = _cameraDistance * sinf(_pitch);
	float z = _cameraDistance * cosf(_pitch) * cosf(_yaw);

	// 카메라 위치를 플레이어 위치 기준으로 설정
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

	if (_type == ProjectionType::Perspective) // Main 카메라
	{
		// eyePosition과 focusPosition은 UpdateCameraWithMouseInput에서 설정된 값 사용
		eyePosition = _cameraPosition;
		focusPosition = _focusPosition;
		upDirection = Vec3(0.0f, 1.0f, 0.0f);
	}
	else // UI 카메라
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

