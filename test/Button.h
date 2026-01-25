#pragma once
#include "Engine.h"

class Button
{
public:
	void Initialize(RMPG::Object2d* buttonPtr, RMPG::Object2d* textPtr)
	{
		this->button = buttonPtr;
		this->text = textPtr;
	}

	RMPG::Object2d* button;
	RMPG::Object2d* text;
	RMPG::ObjectID bid;
	RMPG::ObjectID tid;

	bool hovered = false;

	XMMATRIX matrix = XMMatrixIdentity();
};