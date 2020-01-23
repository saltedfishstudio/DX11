#include "InputClass.h"

InputClass::InputClass()
{
}

InputClass::InputClass(const InputClass&)
{
}

InputClass::~InputClass()
{
}

void InputClass::Initialize()
{
	int i;

	// initialize all the keys to being release and not pressed.
	for (i = 0;i<256;i++)
	{
		m_keys[i] = false;
	}

	return;
}

void InputClass::KeyDown(unsigned input)
{
	m_keys[input] = true;
	return;
}

void InputClass::KeyUp(unsigned input)
{
	m_keys[input] = false;
	return;
}

bool InputClass::IsKeyDown(unsigned input)
{
	return m_keys[input];
}
