#pragma once
class AnimationStateMachine
{
public:
	AnimationStateMachine();
	~AnimationStateMachine();
	
	void SetState(AnimationState newState);
	const AnimationState GetState() { return _currentState; }

	void Update(float deltaTime);

private:
	void PlayAnimationForState(AnimationState state);
	void PlayAnimationClip(const std::string& clipName);
private:
	AnimationState _currentState;
		
};

