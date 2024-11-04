#pragma once
#include "ConstantBuffer.h"

class Shader;

struct GlobalDesc
{
	Matrix V = Matrix::Identity;
	Matrix P = Matrix::Identity;
	Matrix VP = Matrix::Identity;
	Matrix VInv = Matrix::Identity;
};

struct TransformDesc
{
	Matrix W = Matrix::Identity;
};

// Light
struct LightDesc
{
	Color ambient = Color(1.f, 1.f, 1.f, 1.f);
	Color diffuse = Color(1.f, 1.f, 1.f, 1.f);
	Color specular = Color(1.f, 1.f, 1.f, 1.f);
	Color emissive = Color(1.f, 1.f, 1.f, 1.f);

	Vec3 direction;
	float padding0;
};

struct MaterialDesc
{
	Color ambient = Color(0.f, 0.f, 0.f, 1.f);
	Color diffuse = Color(1.f, 1.f, 1.f, 1.f);
	Color specular = Color(0.f, 0.f, 0.f, 1.f);
	Color emissive = Color(0.f, 0.f, 0.f, 1.f);
};

// Bone
#define MAX_MODEL_TRANSFORMS 250
#define MAX_MODEL_KEYFRAMES 500
#define MAX_MODEL_INSTANCE 500

struct BoneDesc
{
	Matrix transforms[MAX_MODEL_TRANSFORMS];
};

// Animation
struct KeyframeDesc
{
	AnimationState state = AnimationState::Idle;

	int32 animIndex = 0;	// �ִϸ��̼� �ε���
	uint32 currFrame = 0;	// ���� ������
	uint32 nextFrame = 0;	// ���� ������
	float ratio = 0.f;		// ���� �����Ӱ� ���� ������ ������ ���� ����
	float sumTime = 0.f;	// �ִϸ��̼� ���� �ð� �հ�
	float speed = 1.f;		// �ִϸ��̼� ��� �ӵ�
	float padding = 0.f; // 32����Ʈ�� ���߱� ���� �е�
};

struct TweenDesc
{
	TweenDesc()
	{
		curr.animIndex = 0;
		next.animIndex = -1;
	}

	void ClearNextAnim()
	{
		next.state = AnimationState::Idle;
		next.animIndex = -1;
		next.currFrame = 0;
		next.nextFrame = 0;
		next.sumTime = 0;
		tweenSumTime = 0;
		tweenRatio = 0;
	}

	float tweenDuration = 1.0f;	// Ʈ��(�߰� ����) ���� �ð�
	float tweenRatio = 0.f;		// Ʈ�� ����
	float tweenSumTime = 0.f;	// Ʈ�� ���� �ð� �հ�
	float padding = 0.f;		// �е�
	KeyframeDesc curr;			// ���� Ű������ ����
	KeyframeDesc next;			// ���� Ű������ ����
};

struct InstancedTweenDesc
{
	TweenDesc tweens[MAX_MODEL_INSTANCE];
};

struct SnowBillboardDesc
{
	Color color = Color(1, 1, 1, 1);

	Vec3 velocity = Vec3(0, -5, 0);
	float drawDistance = 0;

	Vec3 origin = Vec3(0, 0, 0);
	float turbulence = 5;

	Vec3 extent = Vec3(0, 0, 0);
	float time;
};