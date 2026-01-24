#include "Texture2d.h"

HRESULT RMPG::Texture2d::InitializeFromPicture(ID3D11Device* device, std::wstring texpath)
{
	return DirectX::CreateWICTextureFromFile(device, texpath.c_str(), nullptr, texture.GetAddressOf());
}

HRESULT RMPG::Texture2d::InitializeFromMemory(ID3D11Device* device, int width, int height, const void* data, DXGI_FORMAT format)
{
	if (!device || !data || width <= 0 || height <= 0)
		return E_INVALIDARG;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = data;
	int bytesPerPixel = 4;
	initData.SysMemPitch = width * bytesPerPixel;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	HRESULT hr = device->CreateTexture2D(&desc, &initData, tex.GetAddressOf());
	if (FAILED(hr))
		return hr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, texture.GetAddressOf());
	return hr;
}

ID3D11ShaderResourceView* RMPG::Texture2d::Get() const
{
	return texture.Get();
}

ID3D11ShaderResourceView* const* RMPG::Texture2d::GetAddressOf() const
{
	return texture.GetAddressOf();
}
