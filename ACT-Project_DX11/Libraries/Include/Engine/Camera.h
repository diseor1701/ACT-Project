#pragma once
#include "Component.h"

enum class ProjectionType
{
	Perspective, // ���� ����
	Orthographic, // ���� ����
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

	//ī�޶� �÷��̾ �߽����� ��� ���� ������ ��ġ�� ������ ���ϴ� ��ġ ����
	Vec3 _cameraOffset = Vec3(0.f);

	float _yaw = 0.0f;				// �¿� ȸ�� ����
	float _pitch = 0.0f;			// ���� ȸ�� ����
	float _cameraDistance = 5.0f;	// �÷��̾�� ī�޶� ���� �Ÿ�
	float _sensitivity = 0.005f;	// ���콺 ����

	// ī�޶� ��ġ
	Vec3 _cameraPosition = Vec3(0.f);
	// ���� ��ġ
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