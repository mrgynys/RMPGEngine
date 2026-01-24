#pragma once
#include <DirectXMath.h>

struct CBDraw
{
	UINT objectIndex;
	UINT padding[3];

	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT2 uvScale;
};
