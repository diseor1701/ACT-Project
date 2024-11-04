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
	// 현재 상태에 따른 애니메이션 업데이트
	// 예를 들어, 현재 애니메이션 프레임을 증가시키는 등의 작업 수행
}

void AnimationStateMachine::PlayAnimationForState(AnimationState state)
{
	// 상태에 맞는 애니메이션 클립 재생 시작
	switch (state)
	{
	case AnimationState::Idle:
		// Idle 애니메이션 재생
		PlayAnimationClip("Idle");
		break;
	case AnimationState::Walk:
		// Walk 애니메이션 재생
		PlayAnimationClip("Walk");
		break;
	case AnimationState::Run:
		// Run 애니메이션 재생
		PlayAnimationClip("Run");
		break;
	case AnimationState::Attack:
		// Attack 애니메이션 재생
		//PlayAnimationClip("Attack");
		break;	
	case AnimationState::Jump:
		// Attack 애니메이션 재생
		//PlayAnimationClip("Jump");
		break;
		// 다른 상태 처리
	}
}

void AnimationStateMachine::PlayAnimationClip(const std::string& clipName)
{
	// clipName에 맞는 애니메이션 클립을 찾아 재생하는 로직
	// DirectX 애니메이션 데이터나 모델 애니메이터를 활용
}
