#pragma once
#include <d3d11.h>
#include <string>
#include <wrl/client.h>
#include <WICTextureLoader.h>

namespace RMPG
{
	enum class TextureFilterMode
	{
		Point,
		Linear
	};

	class Texture2d
	{
	public:
		HRESULT InitializeFromPicture(ID3D11Device* device, std::wstring texpath);
		HRESULT InitializeFromMemory(ID3D11Device* device, int width, int height, const void* data, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
		void Release();
		bool IsInitialized() const;

		ID3D11ShaderResourceView* Get() const;
		ID3D11ShaderResourceView* const* GetAddressOf() const;

		TextureFilterMode filter = TextureFilterMode::Point;

	private:
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	};
};
