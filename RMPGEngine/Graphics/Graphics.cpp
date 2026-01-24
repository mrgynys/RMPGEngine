#include "Graphics.h"

bool RMPG::Graphics::Initialize(HWND hwnd, int width, int height)
{
	this->windowWidth = width;
	this->windowHeight = height;

	fpsTimer.Start();

	if (!InitializeDirectX(hwnd))
	{
		return false;
	}

	if (!InitializeShaders())
	{
		return false;
	}

	if (!InitializeScene())
	{
		return false;
	}

	return true;
}

void RMPG::Graphics::RenderFrame()
{
	float bgcolor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	this->deviceContext->ClearRenderTargetView(this->renderTargetView.Get(), bgcolor);
	this->deviceContext->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	this->deviceContext->IASetInputLayout(this->vertexshader.GetInputLayout());
	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->deviceContext->RSSetState(this->rasterizerState.Get());
	this->deviceContext->OMSetDepthStencilState(this->depthStencilState.Get(), 0);
	this->deviceContext->PSSetSamplers(0, 1, this->samplerState.GetAddressOf());
	this->deviceContext->VSSetShader(vertexshader.GetShader(), NULL, 0);
	this->deviceContext->PSSetShader(pixelshader.GetShader(), NULL, 0);

	UINT offset = 0;

	XMMATRIX view = this->camera.GetViewMatrix();
	XMMATRIX proj = this->camera.GetProjectionMatrix();

	std::vector<XMFLOAT4X4> tmp;
	tmp.reserve(objects.size());
	for (int i = 0; i < objects.size(); i++)
	{
		XMMATRIX world = objects[i]->GetMatrix();
		XMMATRIX wvp = XMMatrixTranspose(world * view * proj);
		XMFLOAT4X4 f;
		XMStoreFloat4x4(&f, wvp);
		tmp.push_back(f);
	}

	if (!objects.empty())
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = this->deviceContext->Map(this->perObjectStructuredBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(hr))
		{
			return;
		}
		memcpy(mapped.pData, tmp.data(), tmp.size() * sizeof(XMFLOAT4X4));
		this->deviceContext->Unmap(this->perObjectStructuredBuffer.Get(), 0);

		this->deviceContext->VSSetShaderResources(0, 1, this->perObjectSRV.GetAddressOf());
	}

	for (int i = 0; i < static_cast<int>(objects.size()); i++)
	{
		auto& obj = objects[i];

		this->drawBuffer.data.objectIndex = i;

		this->drawBuffer.data.uvOffset = XMFLOAT2(obj->GetCol() * obj->GetTileWidth(), obj->GetRow() * obj->GetTileHeight());
		this->drawBuffer.data.uvScale = XMFLOAT2(obj->GetScaleU(), obj->GetScaleV());

		if (!this->drawBuffer.ApplyChanges())
			return;

		this->deviceContext->VSSetConstantBuffers(0, 1, this->drawBuffer.GetAddressOf());
		this->deviceContext->PSSetConstantBuffers(0, 1, this->drawBuffer.GetAddressOf());

		this->deviceContext->PSSetShaderResources(0, 1, obj->texture->GetAddressOf());
		this->deviceContext->IASetVertexBuffers(0, 1, obj->vertexBuffer.GetAddressOf(), obj->vertexBuffer.StridePtr(), &offset);
		this->deviceContext->IASetIndexBuffer(indicesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		this->deviceContext->DrawIndexed(indicesBuffer.BufferSize(), 0, 0);
	}

	fpsCounter += 1;
	if (fpsTimer.GetMilisecondsElapsed() > 1000.0)
	{
		curFps = fpsCounter;
		fpsCounter = 0;
		fpsTimer.Restart();
	}

	this->swapchain->Present(this->vsync_on, NULL);
}

bool RMPG::Graphics::Resize(int width, int height)
{
	if (width <= 0 || height <= 0)
		return false;

	if (width == this->windowWidth && height == this->windowHeight)
		return true;

	this->windowWidth = width;
	this->windowHeight = height;
	this->deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	this->renderTargetView.Reset();
	this->depthStencilView.Reset();
	this->depthStencilBuffer.Reset();
	HRESULT hr = this->swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to resize swapchain buffers.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	hr = this->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "GetBuffer failed after ResizeBuffers.");
		return false;
	}

	hr = this->device->CreateRenderTargetView(backBuffer.Get(), NULL, this->renderTargetView.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "FAiled to create render target view after resize.");
		return false;
	}

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D11_TEXTURE2D_DESC));
	depthStencilDesc.Width = this->windowWidth;
	depthStencilDesc.Height = this->windowHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	hr = this->device->CreateTexture2D(&depthStencilDesc, NULL, this->depthStencilBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil buffer after resize.");
		return false;
	}

	hr = this->device->CreateDepthStencilView(this->depthStencilBuffer.Get(), NULL, this->depthStencilView.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil view after resize.");
		return false;
	}

	this->deviceContext->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), this->depthStencilView.Get());

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(this->windowWidth);
	viewport.Height = static_cast<FLOAT>(this->windowHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	this->deviceContext->RSSetViewports(1, &viewport);

	this->camera.SetProjectionValues(90.0f, static_cast<float>(this->windowWidth) / static_cast<float>(windowHeight), 0.1f, 1000.0f);

	return true;
}

bool RMPG::Graphics::SetFullScreen(bool fullscreen)
{
	if (!this->swapchain)
	{
		ErrorLogger::Log("Swapchain not initialized -- cannot change fullscreen.");
		return false;
	}

	HRESULT hr = this->swapchain->SetFullscreenState(fullscreen, nullptr);
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to change swapchain fullscreen state.");
		return false;
	}

	return true;
}

void RMPG::Graphics::SetVSync(bool vsync)
{
	if (vsync)
		this->vsync_on = 1;
	else
		this->vsync_on = 0;
}

int RMPG::Graphics::PickObjectAt(int mouseX, int mouseY)
{
	using namespace DirectX;

	if (mouseX < 0 || mouseY < 0 || mouseX > this->windowWidth || mouseY > this->windowHeight)
		return -1;

	float px = (2.0f * static_cast<float>(mouseX) / static_cast<float>(this->windowWidth)) - 1.0f;
	float py = 1.0f - (2.0f * static_cast<float>(mouseY) / static_cast<float>(this->windowHeight));

	XMVECTOR nearNDC = XMVectorSet(px, py, 0.0f, 1.0f);
	XMVECTOR farNDC = XMVectorSet(px, py, 1.0f, 1.0f);

	XMMATRIX invViewProj = XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix());

	XMVECTOR nearWorld = XMVector3TransformCoord(nearNDC, invViewProj);
	XMVECTOR farWorld = XMVector3TransformCoord(farNDC, invViewProj);
	XMVECTOR rayDir = XMVector3Normalize(farWorld - nearWorld);

	XMFLOAT3 rayOriginF, rayDirF;
	XMStoreFloat3(&rayOriginF, nearWorld);
	XMStoreFloat3(&rayDirF, rayDir);

	for (int i = 0; i < this->objects.size(); ++i)
	{
		RMPG::Object2d* o = objects[i].get();
		XMMATRIX world = o->GetMatrix();
		XMMATRIX invWorld = XMMatrixInverse(nullptr, world);

		XMVECTOR localOrigin = XMVector3TransformCoord(XMLoadFloat3(&rayOriginF), invWorld);
		XMVECTOR localDir = XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&rayDirF), invWorld));

		XMFLOAT3 lo, ld;
		XMStoreFloat3(&lo, localOrigin);
		XMStoreFloat3(&ld, localDir);

		float planeZ = o->GetCoordZ();
		const float EPS = 1e-6f;
		if (fabsf(ld.z) < EPS)
			continue;

		float t = (planeZ - lo.z) / ld.z;
		if (t < 0.0f)
			continue;

		float hx = lo.x + ld.x * t;
		float hy = lo.y + ld.y * t;

		float halfW = o->GetWidth() * 0.5f;
		float halfH = o->GetHeight() * 0.5f;

		if (hx >= -halfW && hx <= halfW && hy >= -halfH && hy <= halfH)
			return i;
	}

	return -1;
}

XMFLOAT3 RMPG::Graphics::ScreenToWorldOnPlane(int mouseX, int mouseY, float planeZ)
{
	XMFLOAT3 originF, dirF;

	ScreenToWorldRay(mouseX, mouseY, originF, dirF);

	XMVECTOR origin = XMLoadFloat3(&originF);
	XMVECTOR dir = XMLoadFloat3(&dirF);

	const float EPS = 1e-6f;
	XMFLOAT3 dirTemp;
	XMStoreFloat3(&dirTemp, dir);
	if (fabsf(dirTemp.z) < EPS)
	{
		return originF;
	}

	XMFLOAT3 originTemp;
	XMStoreFloat3(&originTemp, origin);
	float t = (planeZ - originTemp.z) / dirTemp.z;
	if (t < 0.0f)
	{
		return originTemp;
	}
	
	XMVECTOR hit = origin + dir * t;
	XMFLOAT3 hitF;
	XMStoreFloat3(&hitF, hit);
	return hitF;
}

void RMPG::Graphics::ScreenToWorldRay(int mouseX, int mouseY, XMFLOAT3& outOrigin, XMFLOAT3& outDir)
{
	if (mouseX < 0) mouseX = 0;
	if (mouseY < 0) mouseY = 0;
	if (mouseX > this->windowWidth) mouseX = this->windowWidth;
	if (mouseY > this->windowHeight) mouseX = this->windowHeight;

	float px = (2.0f * static_cast<float>(mouseX) / static_cast<float>(this->windowWidth)) - 1.0f;
	float py = 1.0f - (2.0f * static_cast<float>(mouseY) / static_cast<float>(this->windowHeight));

	XMVECTOR nearNDC = XMVectorSet(px, py, 0.0f, 1.0f);
	XMVECTOR farNDC = XMVectorSet(px, py, 1.0f, 1.0f);

	XMMATRIX invViewProj = XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix());

	XMVECTOR nearWorld = XMVector3TransformCoord(nearNDC, invViewProj);
	XMVECTOR farWorld = XMVector3TransformCoord(farNDC, invViewProj);
	XMVECTOR dir = XMVector3Normalize(farWorld - nearWorld);

	XMStoreFloat3(&outOrigin, nearWorld);
	XMStoreFloat3(&outDir, dir);
}

int RMPG::Graphics::AddTexture(const std::wstring& texFilePath)
{
	auto tex = std::make_unique<RMPG::Texture2d>();
	HRESULT hr = tex->InitializeFromPicture(this->device.Get(), texFilePath);
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, L"Failed to create texture from file: " + texFilePath);
		return -1;
	}

	int index = static_cast<int>(textures.size());
	textures.push_back(std::move(tex));

	return index;
}

int RMPG::Graphics::AddObject(float width, float height, float coordZ, RMPG::Texture2d* texture)
{
	auto obj = std::make_unique<RMPG::Object2d>();
	HRESULT hr = obj->Initialize(this->device.Get(), width, height, coordZ);
	if (FAILED(hr))
	{
		return -1;
	}
	obj->SetTexture(texture);

	int index = static_cast<int>(objects.size());
	objects.push_back(std::move(obj));

	this->objectToDynamicTex.push_back(-1);

	hr = EnsurePerObjectBufferCapacity(static_cast<int>(objects.size()));
	if (FAILED(hr))
	{
		objects.pop_back();
		return -1;
	}

	return index;
}

int RMPG::Graphics::AddTextObjectFromFontFile(const std::wstring& fontFilePath, const std::wstring& text, int fontPixelSize, float coordZ, float scale)
{
	std::vector<unsigned char> pixels;
	int w = 0, h = 0;
	HRESULT hr = RasterizeTextToBGRA(fontFilePath, text, fontPixelSize, pixels, w, h, 2);
	if (FAILED(hr) || w <= 0 || h <= 0)
		return -1;

	auto tex = std::make_unique<RMPG::Texture2d>();
	hr = tex->InitializeFromMemory(this->device.Get(), w, h, pixels.data(), DXGI_FORMAT_B8G8R8A8_UNORM);
	if (FAILED(hr))
		return -1;

	RMPG::Texture2d* texPtr = tex.get();
	this->dynamicTextures.push_back(std::move(tex));

	DynamicTextMeta meta;
	meta.fontFile = fontFilePath;
	meta.fontPixelSize = fontPixelSize;
	meta.scale = scale;
	this->dynamicTextMeta.push_back(meta);

	float width = static_cast<float>(w) * scale;
	float height = static_cast<float>(h) * scale;
	int idx = AddObject(width, height, coordZ, texPtr);
	if (idx < 0)
	{
		this->dynamicTextures.pop_back();
		this->dynamicTextMeta.pop_back();
		return -1;
	}

	int dynamicIndex = static_cast<int>(this->dynamicTextures.size()) - 1;
	if (idx >= 0 && idx < static_cast<int>(this->objectToDynamicTex.size()))
		this->objectToDynamicTex[idx] = dynamicIndex;

	return idx;
}

int RMPG::Graphics::AddStyledTextObject(const std::vector<TextRun>& runs, float coordZ, float scale)
{
	std::vector<unsigned char> pixels;
	int w = 0, h = 0;
	HRESULT hr = RasterizeTextRunsToBGRA(runs, pixels, w, h, 2);
	if (FAILED(hr) || w <= 0 || h <= 0)
		return -1;

	auto tex = std::make_unique<RMPG::Texture2d>();
	hr = tex->InitializeFromMemory(this->device.Get(), w, h, pixels.data(), DXGI_FORMAT_B8G8R8A8_UNORM);
	if (FAILED(hr))
		return -1;

	RMPG::Texture2d* texPtr = tex.get();
	this->dynamicTextures.push_back(std::move(tex));

	float width = static_cast<float>(w) * scale;
	float height = static_cast<float>(h) * scale;
	int idx = AddObject(width, height, coordZ, texPtr);
	if (idx < 0)
	{
		this->dynamicTextures.pop_back();
		return -1;
	}

	if (idx >= static_cast<int>(this->objectToDynamicTex.size()))
		this->objectToDynamicTex.resize(idx + 1, -1);
	int dynIndex = static_cast<int>(this->dynamicTextures.size()) - 1;
	this->objectToDynamicTex[idx] = dynIndex;

	return idx;
}

int RMPG::Graphics::UpdateTextObject(int objectIndex, const std::wstring& text)
{
	if (objectIndex < 0 || objectIndex >= static_cast<int>(objects.size()))
		return -1;

	int dynIdx = this->objectToDynamicTex[objectIndex];
	if (dynIdx < 0 || dynIdx >= static_cast<int>(this->dynamicTextures.size()))
		return -1;

	const DynamicTextMeta& meta = this->dynamicTextMeta[dynIdx];

	std::vector<unsigned char> pixels;
	int w = 0, h = 0;
	HRESULT hr = RasterizeTextToBGRA(meta.fontFile, text, meta.fontPixelSize, pixels, w, h, 2);
	if (FAILED(hr) || w <= 0 || h <= 0)
		return -1;

	hr = this->dynamicTextures[dynIdx]->InitializeFromMemory(this->device.Get(), w, h, pixels.data(), DXGI_FORMAT_B8G8R8A8_UNORM);
	if (FAILED(hr))
		return -1;

	float width = static_cast<float>(w) * meta.scale;
	float height = static_cast<float>(h) * meta.scale;
	auto obj = this->objects[objectIndex].get();
	if (obj)
	{
		obj->Initialize(this->device.Get(), width, height, obj->GetCoordZ());
		obj->SetTexture(this->dynamicTextures[dynIdx].get());
	}

	return objectIndex;
}

int RMPG::Graphics::UpdateStyledTextObject(int objectIndex, const std::vector<TextRun>& runs)
{
	if (objectIndex < 0 || objectIndex >= static_cast<int>(objects.size()))
		return -1;
	if (objectIndex >= static_cast<int>(this->objectToDynamicTex.size()))
		return -1;
	int dynIdx = this->objectToDynamicTex[objectIndex];
	if (dynIdx < 0 || dynIdx >= static_cast<int>(this->dynamicTextures.size()))
		return -1;

	std::vector<unsigned char> pixels;
	int w = 0, h = 0;
	HRESULT hr = RasterizeTextRunsToBGRA(runs, pixels, w, h, 2);
	if (FAILED(hr) || w <= 0 || h <= 0)
		return -1;

	hr = this->dynamicTextures[dynIdx]->InitializeFromMemory(this->device.Get(), w, h, pixels.data(), DXGI_FORMAT_B8G8R8A8_UNORM);
	if (FAILED(hr))
		return -1;

	auto obj = this->objects[objectIndex].get();
	if (obj)
	{
		float width = static_cast<float>(w) * 0.01f;
		float height = static_cast<float>(h) * 0.01f;
		obj->Initialize(this->device.Get(), width, height, obj->GetCoordZ());
		obj->SetTexture(this->dynamicTextures[dynIdx].get());
	}

	return objectIndex;
}

void RMPG::Graphics::SetObjectMatrix(int objectIndex, XMMATRIX mat)
{
	if (objectIndex < 0 || objectIndex >= static_cast<int>(this->objects.size()))
		return;
	auto obj = this->objects[objectIndex].get();
	if (!obj)
		return;
	obj->SetMatrix(mat);
}

int RMPG::Graphics::GetFps()
{
	return this->curFps;
}

bool RMPG::Graphics::InitializeDirectX(HWND hwnd)
{
	std::vector<AdapterData> adapters = AdapterReader::GetAdapters();

	if (adapters.size() < 1)
	{
		ErrorLogger::Log("No DXGI Adapters found.");
		return false;
	}

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
	
	scd.BufferDesc.Width = this->windowWidth;
	scd.BufferDesc.Height = this->windowHeight;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;

	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = 1;
	scd.OutputWindow = hwnd;
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain(
		adapters[0].pAdapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		NULL,
		NULL,
		NULL,
		0,
		D3D11_SDK_VERSION,
		&scd,
		this->swapchain.GetAddressOf(),
		this->device.GetAddressOf(),
		NULL,
		this->deviceContext.GetAddressOf()
	);

	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create device and swapchain.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	hr = this->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "GetBuffer Failed.");
		return false;
	}

	hr = this->device->CreateRenderTargetView(backBuffer.Get(), NULL, this->renderTargetView.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create render target view.");
		return false;
	}

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = this->windowWidth;
	depthStencilDesc.Height = this->windowHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	hr = this->device->CreateTexture2D(&depthStencilDesc, NULL, this->depthStencilBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil buffer.");
		return false;
	}

	hr = this->device->CreateDepthStencilView(this->depthStencilBuffer.Get(), NULL, this->depthStencilView.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil view.");
		return false;
	}

	this->deviceContext->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), this->depthStencilView.Get());

	D3D11_DEPTH_STENCIL_DESC depthstencildesc;
	ZeroMemory(&depthstencildesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthstencildesc.DepthEnable = true;
	depthstencildesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
	depthstencildesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

	hr = this->device->CreateDepthStencilState(&depthstencildesc, this->depthStencilState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil state.");
		return false;
	}

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = this->windowWidth;
	viewport.Height = this->windowHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	this->deviceContext->RSSetViewports(1, &viewport);

	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
	hr = this->device->CreateRasterizerState(&rasterizerDesc, this->rasterizerState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create rasterizer state.");
		return false;
	}

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	//sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	
	hr = this->device->CreateSamplerState(&sampDesc, this->samplerState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create sampler state.");
		return false;
	}

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = this->device->CreateBlendState(&blendDesc, this->blendState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create blend state.");
		return false;
	}

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	this->deviceContext->OMSetBlendState(this->blendState.Get(), blendFactor, 0xffffffff);

	return true;
}

bool RMPG::Graphics::InitializeShaders()
{
	std::wstring shaderfolder = L"";
#pragma region DetermineShaderPath
	if (IsDebuggerPresent() == TRUE)
	{
#ifdef _DEBUG //DEBUG
	#ifdef _WIN64 //x64
		shaderfolder = L"..\\x64\\Debug\\";
	#else //x32
		shaderfolder = L"..\\Debug\\";
	#endif //RELEASE
#else
	#ifdef _WIN64 //x64
		shaderfolder = L"..\\x64\\Release\\";
	#else //x32
		shaderfolder = L"..\\Release\\";
	#endif
#endif
	}

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

	if (!vertexshader.Initialize(this->device, shaderfolder + L"vertexshader.cso", layout, numElements))
		return false;

	if (!pixelshader.Initialize(this->device, shaderfolder + L"pixelshader.cso"))
		return false;

	return true;
}

bool RMPG::Graphics::InitializeScene()
{
	DWORD indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	HRESULT hr = this->indicesBuffer.Initialize(this->device.Get(), indices, ARRAYSIZE(indices));
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create indices buffer.");
		return false;
	}

	hr = this->drawBuffer.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to initialize draw-index constant buffer.");
		return false;
	}

	camera.SetPosition(0.0f, 0.0f, -2.0f);
	camera.SetProjectionValues(90.0f, static_cast<float>(windowWidth) / static_cast<float>(windowHeight), 0.1f, 1000.0f);

	return true;
}

HRESULT RMPG::Graphics::EnsurePerObjectBufferCapacity(int requiredCount)
{
	if (requiredCount <= perObjectCapacity)
		return S_OK;

	int newCapacity = max(4, max(perObjectCapacity * 2, requiredCount));
	
	perObjectSRV.Reset();
	perObjectStructuredBuffer.Reset();

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(XMFLOAT4X4);
	bd.ByteWidth = static_cast<UINT>(bd.StructureByteStride * newCapacity);

	HRESULT hr = this->device->CreateBuffer(&bd, nullptr, this->perObjectStructuredBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		return hr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.ElementWidth = newCapacity;

	hr = this->device->CreateShaderResourceView(this->perObjectStructuredBuffer.Get(), &srvDesc, this->perObjectSRV.GetAddressOf());
	if (FAILED(hr))
	{
		return hr;
	}

	perObjectCapacity = newCapacity;

	return S_OK;
}
