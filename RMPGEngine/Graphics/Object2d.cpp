#include "Object2d.h"

RMPG::Object2d::Object2d()
{
    matrix = XMMatrixIdentity();;
}

RMPG::Object2d::~Object2d()
{
    Release();
}

HRESULT RMPG::Object2d::Initialize(ID3D11Device* device, float width, float height, float coordZ)
{
    Vertex v[] =
    {
        Vertex(-width / 2, -height / 2, coordZ, 0.0f, 1.0f),
        Vertex(-width / 2,  height / 2, coordZ, 0.0f, 0.0f),
        Vertex( width / 2,  height / 2, coordZ, 1.0f, 0.0f),
        Vertex( width / 2, -height / 2, coordZ, 1.0f, 1.0f),
    };

    this->width = width;
    this->height = height;
    this->coordZ = coordZ;

    return this->vertexBuffer.Initialize(device, v, ARRAYSIZE(v));
}

HRESULT RMPG::Object2d::SetTexture(Texture2d* texture2d)
{
    if (!texture2d)
        return E_INVALIDARG;
    this->texture = texture2d;
    return S_OK;
}

void RMPG::Object2d::Release()
{
    texture = nullptr;
}

bool RMPG::Object2d::IsInitialized() const
{
    return this->texture != texture;
}

void RMPG::Object2d::SetAtlas(float cols, float rows)
{
    this->atlasCols = cols;
    this->atlasRows = rows;
    this->tileWidth = 1.0f / cols;
    this->tileHeight = 1.0f / rows;
}

void RMPG::Object2d::SetMatrix(XMMATRIX mat)
{
    this->matrix = mat;
}

void RMPG::Object2d::SetTile(float col, float row)
{
    this->curTileColOffset = col;
    this->curTileRowOffset = row;
}

void RMPG::Object2d::SetCol(float col)
{
    this->curTileColOffset = col;
}

void RMPG::Object2d::SetRow(float row)
{
    this->curTileRowOffset = row;
}

void RMPG::Object2d::Next()
{
    this->curTileColOffset += 1.0f;
    if (this->curTileColOffset == this->atlasCols)
        this->curTileColOffset = 0.0f;
}

float RMPG::Object2d::GetCol() const
{
    return this->curTileColOffset;
}

float RMPG::Object2d::GetRow() const
{
    return this->curTileRowOffset;
}

float RMPG::Object2d::GetTileWidth() const
{
    return this->tileWidth;
}

float RMPG::Object2d::GetTileHeight() const
{
    return this->tileHeight;
}

XMMATRIX RMPG::Object2d::GetMatrix() const
{
    return this->matrix;
}

float RMPG::Object2d::GetWidth() const
{
    return this->width;
}

float RMPG::Object2d::GetHeight() const
{
    return this->height;
}

float RMPG::Object2d::GetCoordZ() const
{
    return this->coordZ;
}

float RMPG::Object2d::GetScaleU() const
{
    return 1.0f / this->atlasCols;
}

float RMPG::Object2d::GetScaleV() const
{
    return 1.0f / this->atlasRows;
}

void RMPG::Object2d::SetTintColor(const XMFLOAT4& color)
{
    this->tintColor = color;
}

XMFLOAT4 RMPG::Object2d::GetTintColor() const
{
    return this->tintColor;
}

void RMPG::Object2d::SetTintIntensity(float intensity)
{
    this->tintIntensity = intensity;
}

float RMPG::Object2d::GetTintIntensity() const
{
    return this->tintIntensity;
}

