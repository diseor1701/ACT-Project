#pragma once
#include "Component.h"

enum class ProjectionType
{
	Perspective, // 원근 투영
	Orthographic, // 직교 투영
};

class Camera :  public Component
{
	using Super = Component;
public:
	Camera();
	virtual ~Camera();
	
	virtual void Update() override;
	void UpdateCameraWithMouseInput();
	void UpdateMatrix();

	void SetProjectionType(ProjectionType type) { _type = type; }
	ProjectionType GetProjectionType() { return _type; }


	void SetNear(float value) { _near = value; }
	void SetFar(float value) { _far = value; }
	void SetFOV(float value) { _fov = value; }
	void SetWidth(float value) { _width = value; }
	void SetHeight(float value) { _height = value; }

	void SetCameraOffset(Vec3 v);

	Matrix& GetViewMatrix() { return _matView; }
	Matrix& GetProjectionMatrix() { return _matProjection; }

	float GetWidth() { return _width; }
	float GetHeight() { return _height; }

	Vec3 GetCameraOffset() { return _cameraOffset; }

private:
	ProjectionType _type = ProjectionType::Perspective;
	Matrix _matView = Matrix::Identity;
	Matrix _matProjection = Matrix::Identity;

	float _near = 1.f;
	float _far = 1000.f;
	float _fov = XM_PI / 4.f;
	float _width = 0.f;
	float _height = 0.f;

	//카메라가 플레이어를 중심으로 어느 정도 떨어진 위치에 있을지 정하는 위치 벡터
	Vec3 _cameraOffset = Vec3(0.f);

	float _yaw = 0.0f;				// 좌우 회전 각도
	float _pitch = 0.0f;			// 상하 회전 각도
	float _cameraDistance = 5.0f;	// 플레이어와 카메라 간의 거리
	float _sensitivity = 0.005f;	// 마우스 감도

	// 카메라 위치
	Vec3 _cameraPosition = Vec3(0.f);
	// 초점 위치
	Vec3 _focusPosition = Vec3(0.f);

public:
	static bool S_IsWireFrame;
	static Matrix S_MatView;
	static Matrix S_MatProjection;

public:
	void SortGameObject();
	void Render_Forward();

	void SetCullingMaskLayerOnOff(uint8 layer, bool on)
	{
		if (on)
			_cullingMask |= (1 << layer);
		else
			_cullingMask &= ~(1 << layer);
	}

	void SetCullingMaskAll() { SetCullingMask(UINT32_MAX); }
	void SetCullingMask(uint32 mask) { _cullingMask = mask; }
	bool IsCulled(uint8 layer) { return (_cullingMask & (1 << layer)) != 0; }

private:
	uint32 _cullingMask = 0;
	vector<shared_ptr<GameObject>> _vecForward;
	shared_ptr<GameObject> _player = nullptr;
};