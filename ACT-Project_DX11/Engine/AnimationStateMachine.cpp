#include "pch.h"
#include "AnimationStateMachine.h"

AnimationStateMachine::AnimationStateMachine() : _currentState(AnimationState::Idle) {}

AnimationStateMachine::~AnimationStateMachine() {}

void AnimationStateMachine::SetState(AnimationState newState)
{
	if (_currentState != newState)
	{
		_currentState = newState;
		PlayAnimationForState(_currentState);
	}
}

void AnimationStateMachine::Update(float deltaTime)
{
	// ���� ���¿� ���� �ִϸ��̼� ������Ʈ
	// ���� ���, ���� �ִϸ��̼� �������� ������Ű�� ���� �۾� ����
}

void AnimationStateMachine::PlayAnimationForState(AnimationState state)
{
	// ���¿� �´� �ִϸ��̼� Ŭ�� ��� ����
	switch (state)
	{
	case AnimationState::Idle:
		// Idle �ִϸ��̼� ���
		PlayAnimationClip("Idle");
		break;
	case AnimationState::Walk:
		// Walk �ִϸ��̼� ���
		PlayAnimationClip("Walk");
		break;
	case AnimationState::Run:
		// Run �ִϸ��̼� ���
		PlayAnimationClip("Run");
		break;
	case AnimationState::Attack:
		// Attack �ִϸ��̼� ���
		//PlayAnimationClip("Attack");
		break;	
	case AnimationState::Jump:
		// Attack �ִϸ��̼� ���
		//PlayAnimationClip("Jump");
		break;
		// �ٸ� ���� ó��
	}
}

void AnimationStateMachine::PlayAnimationClip(const std::string& clipName)
{
	// clipName�� �´� �ִϸ��̼� Ŭ���� ã�� ����ϴ� ����
	// DirectX �ִϸ��̼� �����ͳ� �� �ִϸ����͸� Ȱ��
}
