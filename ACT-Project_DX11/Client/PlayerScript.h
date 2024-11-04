#pragma once
#include "MonoBehaviour.h"
class PlayerScript : public MonoBehaviour
{
	virtual void Start() override;
	virtual void Update() override;

	float _speed = 400.f;
};

