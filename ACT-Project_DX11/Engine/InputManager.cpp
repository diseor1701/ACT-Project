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
		// Ű�� ���� ������ true
		if (asciiKeys[key] & 0x80)
		{
			KEY_STATE& state = _states[key];

			// ���� �����ӿ� Ű�� ���� ���¶�� PRESS
			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::PRESS;
			else
				state = KEY_STATE::DOWN;
		}
		else
		{
			KEY_STATE& state = _states[key];

			// ���� �����ӿ� Ű�� ���� ���¶�� UP
			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
				state = KEY_STATE::UP;
			else
				state = KEY_STATE::NONE;
		}
	}

	// ���콺 Ŀ���� ���� ȭ�� ��ǥ(��ũ�� ��ǥ)�� _mousePos�� ����
	::GetCursorPos(&_mousePos);
	// ��ũ�� ��ǥ�� ������ Ŭ���̾�Ʈ ��ǥ�� ��ȯ
	::ScreenToClient(_hwnd, &_mousePos);

	// ���콺 �̵��� ���
	_deltaX = _mousePos.x - _prevMouseX;
	_deltaY = _mousePos.y - _prevMouseY;

	// ���� ��ġ�� ���� ��ġ�� ����
	_prevMouseX = _mousePos.x;
	_prevMouseY = _mousePos.y;
}