#pragma once
#include <wrl/client.h>
#include "VertexBuffer.h"
#include "Vertex.h"
#include "../Timer.h"
#include "Texture2d.h"

#include <DirectXMath.h>
using namespace DirectX;

namespace RMPG
{
	class Object2d
	{
	public:
		Object2d();

		HRESULT Initialize(ID3D11Device* device, float width, float height, float coordZ);
		HRESULT SetTexture(Texture2d* texture2d);
		void SetAtlas(float cols, float rows);
		void SetMatrix(XMMATRIX mat);
		
		void SetTile(float col, float row);
		void SetCol(float col);
		void SetRow(float row);
		void Next();

		float GetCol() const;
		float GetRow() const;
		float GetTileWidth() const;
		float GetTileHeight() const;
		XMMATRIX GetMatrix() const;

		float GetWidth() const;
		float GetHeight() const;
		float GetCoordZ() const;

		float GetScaleU() const;
		float GetScaleV() const;

	public:
		VertexBuffer<Vertex> vertexBuffer;
		Texture2d* texture = nullptr;

	private:
		float atlasCols = 1.0f;
		float atlasRows = 1.0f;
		float tileWidth = 1.0f;
		float tileHeight = 1.0f;
		float curTileColOffset = 0.0f;
		float curTileRowOffset = 0.0f;

		XMMATRIX matrix;

		float width = 1.0f;
		float height = 1.0f;
		float coordZ = 0.0f;
	};
};
