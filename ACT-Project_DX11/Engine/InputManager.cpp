#include "pch.h"
#include "InputManager.h"

void InputManager::Init(HWND hwnd)
{
	_hwnd = hwnd;
	_states.resize(KEY_TYPE_COUNT, KEY_STATE::NONE);

	_prevMouseX = 0;
	_prevMouseY = 0;
	_deltaX = 0;
	_deltaY = 0;
}

void InputManager::Update()
{
	HWND hwnd = ::GetActiveWindow();
	if (_hwnd != hwnd)
	{
		for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
			_states[key] = KEY_STATE::NONE;

		return;
	}

	BYTE asciiKeys[KEY_TYPE_COUNT] = {};
	if (::GetKeyboardState(asciiKeys) == false)
		return;

	for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
	{
		// 키가 눌려 있으면 true
		if (asciiKeys[key] & 0x80)
		{
			KEY_STATE& state = _states[key];

			// 이전 프레임에 키를 누른 상태라면 PRESS
			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::PRESS;
			else
				state = KEY_STATE::DOWN;
		}
		else
		{
			KEY_STATE& state = _states[key];

			// 이전 프레임에 키를 누른 상태라면 UP
			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::UP;
			else
				state = KEY_STATE::NONE;
		}
	}

	// 마우스 커서의 현재 화면 좌표(스크린 좌표)를 _mousePos에 저장
	::GetCursorPos(&_mousePos);
	// 스크린 좌표를 윈도우 클라이언트 좌표로 변환
	::ScreenToClient(_hwnd, &_mousePos);

	// 마우스 이동량 계산
	_deltaX = _mousePos.x - _prevMouseX;
	_deltaY = _mousePos.y - _prevMouseY;

	// 현재 위치를 이전 위치로 저장
	_prevMouseX = _mousePos.x;
	_prevMouseY = _mousePos.y;
}