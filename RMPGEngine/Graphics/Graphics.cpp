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
	this->deviceContext->VSSetShader(vertexshader.GetShader(), NULL, 0);
	this->deviceContext->PSSetShader(pixelshader.GetShader(), NULL, 0);

	UpdatePerObjectBuffer();

	if (!objects.empty())
	{
		this->deviceContext->VSSetShaderResources(0, 1, this->perObjectSRV.GetAddressOf());
	}

	UINT offset = 0;

	XMMATRIX view = this->camera.GetViewMatrix();
	XMMATRIX proj = this->camera.GetProjectionMatrix();

	for (auto& [objectId, obj] : objects)
	{
		RenderObject(objectId, obj.get());
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

// last object ".back()" will be nearest
std::vector<RMPG::ObjectID> RMPG::Graphics::PickObjectsAt(int mouseX, int mouseY)
{
	std::vector<ObjectID> result;

	if (mouseX < 0 || mouseY < 0 || mouseX > this->windowWidth || mouseY > this->windowHeight)
		return result;

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

	for (const auto& [id, obj] : objects)
	{
		XMMATRIX world = obj->GetMatrix();
		XMMATRIX invWorld = XMMatrixInverse(nullptr, world);

		XMVECTOR localOrigin = XMVector3TransformCoord(XMLoadFloat3(&rayOriginF), invWorld);
		XMVECTOR localDir = XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&rayDirF), invWorld));

		XMFLOAT3 lo, ld;
		XMStoreFloat3(&lo, localOrigin);
		XMStoreFloat3(&ld, localDir);

		float planeZ = obj->GetCoordZ();
		const float EPS = 1e-6f;

		if (fabsf(ld.z) < EPS)
			continue;

		float t = (planeZ - lo.z) / ld.z;
		if (t < 0.0f)
			continue;

		float hx = lo.x + ld.x * t;
		float hy = lo.y + ld.y * t;

		float halfW = obj->GetWidth() * 0.5f;
		float halfH = obj->GetHeight() * 0.5f;

		if (hx >= -halfW && hx <= halfW && hy >= -halfH && hy <= halfH)
		{
			result.push_back(id);
		}
	}

	std::sort(result.begin(), result.end(), [this](ObjectID a, ObjectID b) {
		auto objA = objects.find(a);
		auto objB = objects.find(b);
		if (objA != objects.end() && objB != objects.end()) {
			return objA->second->GetCoordZ() < objB->second->GetCoordZ();
		}
		return false;
		});

	return result;
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
	mouseX = max(0, min(mouseX, this->windowWidth));
	mouseY = max(0, min(mouseY, this->windowHeight));

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
	auto tex = std::make_unique<Texture2d>();
	HRESULT hr = tex->InitializeFromPicture(this->device.Get(), texFilePath);
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, L"Failed to create texture from file: " + texFilePath);
		return -1;
	}

	TextureID newId = nextTextureId++;
	textures[newId] = std::move(tex);
	return newId;
}

int RMPG::Graphics::AddObject(float width, float height, float coordZ, TextureID textureId)
{
	auto texIt = textures.find(textureId);
	if (texIt == textures.end())
	{
		ErrorLogger::Log("Invalid texture ID passed to AddObject.");
		return -1;
	}

	auto obj = std::make_unique<Object2d>();
	HRESULT hr = obj->Initialize(this->device.Get(), width, height, coordZ);
	if (FAILED(hr))
	{
		return -1;
	}

	obj->SetTexture(texIt->second.get());

	ObjectID newId = nextObjectId++;
	objects[newId] = std::move(obj);
	objectToTexture[newId] = textureId;

	textureUsage[textureId].insert(newId);

	HRESULT hr2 = EnsurePerObjectBufferCapacity(static_cast<int>(objects.size()));
	if (FAILED(hr2))
	{
		objects.erase(newId);
		objectToTexture.erase(newId);
		textureUsage[textureId].erase(newId);
		return -1;
	}

	return newId;
}

int RMPG::Graphics::AddTextObjectFromFontFile(const std::wstring& fontFilePath, const std::wstring& text, int fontPixelSize, float coordZ, float scale)
{
	std::vector<unsigned char> pixels;
	int w = 0, h = 0;
	HRESULT hr = RasterizeTextToBGRA(fontFilePath, text, fontPixelSize, pixels, w, h, 2);
	if (FAILED(hr) || w <= 0 || h <= 0)
		return -1;

	auto tex = std::make_unique<Texture2d>();
	hr = tex->InitializeFromMemory(this->device.Get(), w, h, pixels.data(), DXGI_FORMAT_B8G8R8A8_UNORM);
	if (FAILED(hr))
		return -1;

	TextureID dynTexId = nextTextureId++;
	dynamicTextures[dynTexId] = std::move(tex);

	DynamicTextMeta meta;
	meta.fontFile = fontFilePath;
	meta.fontPixelSize = fontPixelSize;
	meta.scale = scale;
	dynamicTextMeta[dynTexId] = meta;

	float width = static_cast<float>(w) * scale;
	float height = static_cast<float>(h) * scale;

	auto obj = std::make_unique<Object2d>();
	hr = obj->Initialize(this->device.Get(), width, height, coordZ);
	if (FAILED(hr))
	{
		dynamicTextures.erase(dynTexId);
		dynamicTextMeta.erase(dynTexId);
		return -1;
	}

	obj->SetTexture(dynamicTextures[dynTexId].get());

	ObjectID newId = nextObjectId++;
	objects[newId] = std::move(obj);
	objectToDynamicTexture[newId] = dynTexId;

	HRESULT hr2 = EnsurePerObjectBufferCapacity(static_cast<int>(objects.size()));
	if (FAILED(hr2))
	{
		objects.erase(newId);
		objectToDynamicTexture.erase(newId);
		dynamicTextures.erase(dynTexId);
		dynamicTextMeta.erase(dynTexId);
		return -1;
	}

	return newId;
}

RMPG::ObjectID RMPG::Graphics::AddStyledTextObject(const std::vector<TextRun>& runs, float coordZ, float scale)
{
	if (runs.empty())
		return -1;

	std::vector<unsigned char> pixels;
	int w = 0, h = 0;
	HRESULT hr = RasterizeTextRunsToBGRA(runs, pixels, w, h, 2);
	if (FAILED(hr) || w <= 0 || h <= 0)
		return -1;

	auto tex = std::make_unique<Texture2d>();
	hr = tex->InitializeFromMemory(this->device.Get(), w, h, pixels.data(), DXGI_FORMAT_B8G8R8A8_UNORM);
	if (FAILED(hr))
		return -1;

	// Создаем ID для динамической текстуры
	TextureID dynTexId = nextTextureId++;
	dynamicTextures[dynTexId] = std::move(tex);

	// Для styled text нет метаданных как в обычном тексте, но можно хранить runs
	// Временно - просто создаем пустые метаданные или можем хранить первую run
	DynamicTextMeta meta;
	if (!runs.empty())
	{
		meta.fontFile = runs[0].fontFile;
		meta.fontPixelSize = runs[0].fontPixelSize;
		meta.scale = scale;
	}
	dynamicTextMeta[dynTexId] = meta;

	float width = static_cast<float>(w) * scale;
	float height = static_cast<float>(h) * scale;

	// Создаем объект
	auto obj = std::make_unique<Object2d>();
	hr = obj->Initialize(this->device.Get(), width, height, coordZ);
	if (FAILED(hr))
	{
		dynamicTextures.erase(dynTexId);
		dynamicTextMeta.erase(dynTexId);
		return -1;
	}

	obj->SetTexture(dynamicTextures[dynTexId].get());

	ObjectID newId = nextObjectId++;
	objects[newId] = std::move(obj);
	objectToDynamicTexture[newId] = dynTexId;

	// Обновляем буферы
	HRESULT hr2 = EnsurePerObjectBufferCapacity(static_cast<int>(objects.size()));
	if (FAILED(hr2))
	{
		objects.erase(newId);
		objectToDynamicTexture.erase(newId);
		dynamicTextures.erase(dynTexId);
		dynamicTextMeta.erase(dynTexId);
		return -1;
	}

	return newId;
}

int RMPG::Graphics::UpdateTextObject(ObjectID objectId, const std::wstring& text)
{
	auto dynTexIt = objectToDynamicTexture.find(objectId);
	if (dynTexIt == objectToDynamicTexture.end())
		return false;

	auto metaIt = dynamicTextMeta.find(dynTexIt->second);
	if (metaIt == dynamicTextMeta.end())
		return false;

	auto objIt = objects.find(objectId);
	if (objIt == objects.end())
		return false;

	const DynamicTextMeta& meta = metaIt->second;

	std::vector<unsigned char> pixels;
	int w = 0, h = 0;
	HRESULT hr = RasterizeTextToBGRA(meta.fontFile, text, meta.fontPixelSize, pixels, w, h, 2);
	if (FAILED(hr) || w <= 0 || h <= 0)
		return false;

	auto texIt = dynamicTextures.find(dynTexIt->second);
	if (texIt == dynamicTextures.end())
		return false;

	hr = texIt->second->InitializeFromMemory(this->device.Get(), w, h, pixels.data(), DXGI_FORMAT_B8G8R8A8_UNORM);
	if (FAILED(hr))
		return false;

	float width = static_cast<float>(w) * meta.scale;
	float height = static_cast<float>(h) * meta.scale;

	hr = objIt->second->Initialize(this->device.Get(), width, height, objIt->second->GetCoordZ());
	if (FAILED(hr))
		return false;

	objIt->second->SetTexture(texIt->second.get());
	return true;
}

bool RMPG::Graphics::UpdateStyledTextObject(ObjectID objectId, const std::vector<TextRun>& runs)
{
	if (runs.empty())
		return false;

	auto dynTexIt = objectToDynamicTexture.find(objectId);
	if (dynTexIt == objectToDynamicTexture.end())
		return false;

	auto objIt = objects.find(objectId);
	if (objIt == objects.end())
		return false;

	std::vector<unsigned char> pixels;
	int w = 0, h = 0;
	HRESULT hr = RasterizeTextRunsToBGRA(runs, pixels, w, h, 2);
	if (FAILED(hr) || w <= 0 || h <= 0)
		return false;

	auto texIt = dynamicTextures.find(dynTexIt->second);
	if (texIt == dynamicTextures.end())
		return false;

	hr = texIt->second->InitializeFromMemory(this->device.Get(), w, h, pixels.data(), DXGI_FORMAT_B8G8R8A8_UNORM);
	if (FAILED(hr))
		return false;

	auto metaIt = dynamicTextMeta.find(dynTexIt->second);
	float scale = 1.0f;
	if (metaIt != dynamicTextMeta.end())
	{
		scale = metaIt->second.scale;
	}
	else if (!runs.empty())
	{
		scale = 1.0f;
	}

	float width = static_cast<float>(w) * scale;
	float height = static_cast<float>(h) * scale;

	hr = objIt->second->Initialize(this->device.Get(), width, height, objIt->second->GetCoordZ());
	if (FAILED(hr))
		return false;

	objIt->second->SetTexture(texIt->second.get());

	if (metaIt != dynamicTextMeta.end() && !runs.empty())
	{
		metaIt->second.fontFile = runs[0].fontFile;
		metaIt->second.fontPixelSize = runs[0].fontPixelSize;
	}

	return true;
}

bool RMPG::Graphics::SetObjectMatrix(ObjectID objectId, XMMATRIX mat)
{
	auto it = objects.find(objectId);
	if (it == objects.end())
		return false;

	it->second->SetMatrix(mat);
	return true;
}

bool RMPG::Graphics::SetObjectTintColor(ObjectID objectId, const XMFLOAT4& color, float intensity)
{
	auto it = objects.find(objectId);
	if (it == objects.end())
		return false;

	it->second->SetTintColor(color);
	it->second->SetTintIntensity(intensity);
	return true;
}

bool RMPG::Graphics::RemoveObject(ObjectID objectId)
{
	RemoveObjectFromAllGroups(objectId);
	auto objIt = objects.find(objectId);
	if (objIt == objects.end())
	{
		ErrorLogger::Log("Object ID not found for removal: " + std::to_string(objectId));
		return false;
	}

	auto dynTexIt = objectToDynamicTexture.find(objectId);
	if (dynTexIt != objectToDynamicTexture.end())
	{
		TextureID dynTexId = dynTexIt->second;

		bool usedElsewhere = false;
		for (const auto& [id, texId] : objectToDynamicTexture)
		{
			if (id != objectId && texId == dynTexId)
			{
				usedElsewhere = true;
				break;
			}
		}

		if (!usedElsewhere)
		{
			dynamicTextures.erase(dynTexId);
			dynamicTextMeta.erase(dynTexId);
		}

		objectToDynamicTexture.erase(objectId);
	}

	auto texIt = objectToTexture.find(objectId);
	if (texIt != objectToTexture.end())
	{
		TextureID texId = texIt->second;
		textureUsage[texId].erase(objectId);

		if (textureUsage[texId].empty())
		{
			textureUsage.erase(texId);
		}

		objectToTexture.erase(objectId);
	}

	ReleaseObjectResources(objIt->second.get());

	objects.erase(objIt);

	return true;
}

bool RMPG::Graphics::RemoveAllObjects()
{
	for (auto& [id, obj] : objects)
	{
		ReleaseObjectResources(obj.get());
	}

	for (auto& [id, tex] : dynamicTextures)
	{
		ReleaseTextureResources(tex.get());
	}

	objects.clear();
	objectToTexture.clear();
	objectToDynamicTexture.clear();
	dynamicTextures.clear();
	dynamicTextMeta.clear();
	textureUsage.clear();

	return true;
}

bool RMPG::Graphics::RemoveTexture(TextureID textureId)
{
	auto texIt = textures.find(textureId);
	if (texIt == textures.end())
	{
		ErrorLogger::Log("Texture ID not found for removal: " + std::to_string(textureId));
		return false;
	}

	auto usageIt = textureUsage.find(textureId);
	if (usageIt != textureUsage.end() && !usageIt->second.empty())
	{
		ErrorLogger::Log("Cannot remove texture: it is still in use by " +
			std::to_string(usageIt->second.size()) + " objects.");
		return false;
	}

	ReleaseTextureResources(texIt->second.get());

	textures.erase(texIt);

	if (usageIt != textureUsage.end())
	{
		textureUsage.erase(usageIt);
	}

	return true;
}

bool RMPG::Graphics::RemoveAllTextures()
{
	for (const auto& [texId, objSet] : textureUsage)
	{
		if (!objSet.empty())
		{
			ErrorLogger::Log("Cannot remove all textures: texture " +
				std::to_string(texId) + " is still in use.");
			return false;
		}
	}

	for (auto& [id, tex] : textures)
	{
		ReleaseTextureResources(tex.get());
	}

	textures.clear();
	textureUsage.clear();

	return true;
}

RMPG::Object2d* RMPG::Graphics::GetObjectPtr(ObjectID objectId)
{
	auto it = objects.find(objectId);
	return (it != objects.end()) ? it->second.get() : nullptr;
}

XMMATRIX RMPG::Graphics::GetObjectWorldMatrix(ObjectID objectId) const
{
	return CalculateObjectWorldMatrix(objectId);
}

RMPG::Texture2d* RMPG::Graphics::GetTexturePtr(TextureID textureId)
{
	auto it = textures.find(textureId);
	if (it != textures.end()) return it->second.get();
	auto dynIt = dynamicTextures.find(textureId);
	return (dynIt != dynamicTextures.end()) ? dynIt->second.get() : nullptr;
}

XMFLOAT4 RMPG::Graphics::GetObjectTintColor(ObjectID objectId) const
{
	auto it = objects.find(objectId);
	if (it == objects.end())
		return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	return it->second->GetTintColor();
}

float RMPG::Graphics::GetObjectTintIntensity(ObjectID objectId) const
{
	auto it = objects.find(objectId);
	if (it == objects.end())
		return 0.0f;

	return it->second->GetTintIntensity();
}

bool RMPG::Graphics::ObjectExists(ObjectID objectId) const
{
	return objects.find(objectId) != objects.end();
}

bool RMPG::Graphics::TexturerExists(TextureID textureId) const
{
	return textures.find(textureId) != textures.end() ||
		dynamicTextures.find(textureId) != dynamicTextures.end();
}

int RMPG::Graphics::GetFps()
{
	return this->curFps;
}

XMFLOAT2 RMPG::Graphics::GetWorldCoordAtScreenPoint(int screenX, int screenY, float worldZ)
{
	screenX = max(0, min(screenX, windowWidth));
	screenY = max(0, min(screenY, windowHeight));

	float px = (2.0f * static_cast<float>(screenX) / static_cast<float>(windowWidth)) - 1.0f;
	float py = 1.0f - (2.0f * static_cast<float>(screenY) / static_cast<float>(windowHeight));

	XMVECTOR nearNDC = XMVectorSet(px, py, 0.0f, 1.0f);
	XMVECTOR farNDC = XMVectorSet(px, py, 1.0f, 1.0f);

	XMMATRIX invViewProj = XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix());
	XMVECTOR nearWorld = XMVector3TransformCoord(nearNDC, invViewProj);
	XMVECTOR farWorld = XMVector3TransformCoord(farNDC, invViewProj);

	XMVECTOR rayOrigin = nearWorld;
	XMVECTOR rayDir = XMVector3Normalize(farWorld - nearWorld);

	XMFLOAT3 originF, dirF;
	XMStoreFloat3(&originF, rayOrigin);
	XMStoreFloat3(&dirF, rayDir);

	const float EPS = 1e-6f;
	if (fabsf(dirF.z) < EPS)
	{
		return XMFLOAT2(originF.x, originF.y);
	}

	float t = (worldZ - originF.z) / dirF.z;

	if (t < 0.0f)
	{
		return XMFLOAT2(originF.x, originF.y);
	}

	XMVECTOR hit = rayOrigin + rayDir * t;
	XMFLOAT3 hitF;
	XMStoreFloat3(&hitF, hit);

	return XMFLOAT2(hitF.x, hitF.y);
}

XMFLOAT2 RMPG::Graphics::GetTopLeftWorldCoord(float worldZ)
{
	return GetWorldCoordAtScreenPoint(0, 0, worldZ);
}

XMFLOAT2 RMPG::Graphics::GetTopRightWorldCoord(float worldZ)
{
	return GetWorldCoordAtScreenPoint(windowWidth, 0, worldZ);
}

XMFLOAT2 RMPG::Graphics::GetBottomLeftWorldCoord(float worldZ)
{
	return GetWorldCoordAtScreenPoint(0, windowHeight, worldZ);
}

XMFLOAT2 RMPG::Graphics::GetBottomRightWorldCoord(float worldZ)
{
	return GetWorldCoordAtScreenPoint(windowWidth, windowHeight, worldZ);
}

XMFLOAT2 RMPG::Graphics::GetCenterWorldCoord(float worldZ)
{
	return GetWorldCoordAtScreenPoint(windowWidth / 2, windowHeight / 2, worldZ);
}

RMPG::WorldBounds RMPG::Graphics::GetCameraWorldBounds(float worldZ)
{
	WorldBounds bounds;
	bounds.topLeft = GetTopLeftWorldCoord(worldZ);
	bounds.topRight = GetTopRightWorldCoord(worldZ);
	bounds.bottomLeft = GetBottomLeftWorldCoord(worldZ);
	bounds.bottomRight = GetBottomRightWorldCoord(worldZ);
	bounds.center = GetCenterWorldCoord(worldZ);

	bounds.width = bounds.topRight.x - bounds.topLeft.x;
	bounds.height = bounds.topLeft.y - bounds.bottomLeft.y;

	return bounds;
}

XMFLOAT2 RMPG::Graphics::WorldToScreenCoord(const XMFLOAT3& worldPos)
{
	XMVECTOR worldVec = XMLoadFloat3(&worldPos);

	XMMATRIX viewProj = camera.GetViewMatrix() * camera.GetProjectionMatrix();
	XMVECTOR projVec = XMVector3TransformCoord(worldVec, viewProj);

	XMFLOAT3 projPos;
	XMStoreFloat3(&projPos, projVec);

	XMFLOAT2 screenPos;
	screenPos.x = (projPos.x + 1.0f) * 0.5f * windowWidth;
	screenPos.y = (1.0f - projPos.y) * 0.5f * windowHeight;

	return screenPos;
}

bool RMPG::Graphics::IsWorldPointVisible(const XMFLOAT3& worldPos)
{
	XMVECTOR worldVec = XMLoadFloat3(&worldPos);
	XMMATRIX viewProj = camera.GetViewMatrix() * camera.GetProjectionMatrix();
	XMVECTOR projVec = XMVector3TransformCoord(worldVec, viewProj);

	XMFLOAT3 projPos;
	XMStoreFloat3(&projPos, projVec);

	return (projPos.x >= -1.0f && projPos.x <= 1.0f &&
		projPos.y >= -1.0f && projPos.y <= 1.0f &&
		projPos.z >= 0.0f && projPos.z <= 1.0f);
}

RMPG::GroupID RMPG::Graphics::CreateGroup()
{
	GroupID newId = nextGroupId++;
	groups[newId] = std::make_unique<Group>();
	return newId;
}

bool RMPG::Graphics::DestroyGroup(GroupID groupId)
{
	auto it = groups.find(groupId);
	if (it == groups.end())
		return false;

	groups.erase(it);
	return true;
}

bool RMPG::Graphics::AddObjectToGroup(GroupID groupId, ObjectID objectId)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	auto objIt = objects.find(objectId);
	if (objIt == objects.end())
		return false;

	RemoveObjectFromAllGroups(objectId);

	groupIt->second->AddObject(objectId);
	return true;
}

bool RMPG::Graphics::RemoveObjectFromGroup(GroupID groupId, ObjectID objectId)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	return groupIt->second->RemoveObject(objectId);
}

bool RMPG::Graphics::RemoveObjectFromAllGroups(ObjectID objectId)
{
	bool removed = false;
	for (auto& [groupId, group] : groups)
	{
		if (group->RemoveObject(objectId))
			removed = true;
	}
	return removed;
}

bool RMPG::Graphics::SetGroupMatrix(GroupID groupId, const XMMATRIX& matrix)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->SetMatrix(matrix);
	return true;
}

bool RMPG::Graphics::SetGroupPosition(GroupID groupId, float x, float y, float z)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->SetPosition(x, y, z);
	return true;
}

bool RMPG::Graphics::SetGroupRotation(GroupID groupId, float pitch, float yaw, float roll)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->SetRotation(pitch, yaw, roll);
	return true;
}

bool RMPG::Graphics::SetGroupScale(GroupID groupId, float x, float y, float z)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->SetScale(x, y, z);
	return true;
}

bool RMPG::Graphics::SetGroupScale(GroupID groupId, float scale)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->SetScale(scale);
	return true;
}

bool RMPG::Graphics::AdjustGroupPosition(GroupID groupId, float x, float y, float z)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->AdjustPosition(x, y, z);
	return true;
}

bool RMPG::Graphics::AdjustGroupRotation(GroupID groupId, float pitch, float yaw, float roll)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->AdjustRotation(pitch, yaw, roll);
	return true;
}

bool RMPG::Graphics::AdjustGroupScale(GroupID groupId, float x, float y, float z)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->AdjustScale(x, y, z);
	return true;
}

bool RMPG::Graphics::AdjustGroupScale(GroupID groupId, float scale)
{
	auto groupIt = groups.find(groupId);
	if (groupIt == groups.end())
		return false;

	groupIt->second->AdjustScale(scale);
	return true;
}

bool RMPG::Graphics::GroupExists(GroupID groupId) const
{
	return groups.find(groupId) != groups.end();
}

const RMPG::Group* RMPG::Graphics::GetGroupPtr(GroupID groupId) const
{
	auto it = groups.find(groupId);
	return (it != groups.end()) ? it->second.get() : nullptr;
}

size_t RMPG::Graphics::GetGroupCount() const
{
	return groups.size();
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

	if (!CreateSamplerStates())
	{
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
		shaderfolder = L"Shaders\\x64\\Debug\\";
	#else //x32
		shaderfolder = L"Shaders\\x86\\Debug\\";
	#endif //RELEASE
#else
	#ifdef _WIN64 //x64
		shaderfolder = L"Shaders\\x64\\Release\\";
	#else //x32
		shaderfolder = L"Shaders\\x86\\Release\\";
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

void RMPG::Graphics::ReleaseObjectResources(Object2d* obj)
{
	if (!obj) return;
	obj->texture = nullptr;
}

void RMPG::Graphics::ReleaseTextureResources(Texture2d* tex)
{
	if (!tex) return;
	tex->Release(); //!!!
}

void RMPG::Graphics::UpdatePerObjectBuffer()
{
	if (objects.empty()) return;

	std::vector<XMFLOAT4X4> wvpMatrices;
	wvpMatrices.reserve(objects.size());

	XMMATRIX view = this->camera.GetViewMatrix();
	XMMATRIX proj = this->camera.GetProjectionMatrix();

	for (const auto& [id, obj] : objects)
	{
		XMMATRIX world = CalculateObjectWorldMatrix(id);
		XMMATRIX wvp = XMMatrixTranspose(world * view * proj);
		XMFLOAT4X4 f;
		XMStoreFloat4x4(&f, wvp);
		wvpMatrices.push_back(f);
	}

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = this->deviceContext->Map(this->perObjectStructuredBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (SUCCEEDED(hr))
	{
		memcpy(mapped.pData, wvpMatrices.data(), wvpMatrices.size() * sizeof(XMFLOAT4X4));
		this->deviceContext->Unmap(this->perObjectStructuredBuffer.Get(), 0);
	}
}

void RMPG::Graphics::RenderObject(ObjectID id, Object2d* obj)
{
	if (!obj) return;

	UINT offset = 0;

	int index = 0;
	for (auto it = objects.begin(); it != objects.end(); ++it, ++index)
	{
		if (it->first == id) break;
	}

	if (obj->texture->filter == RMPG::TextureFilterMode::Linear)
	{
		this->deviceContext->PSSetSamplers(0, 1, this->linearSamplerState.GetAddressOf());
	}
	else
	{
		this->deviceContext->PSSetSamplers(0, 1, this->pointSamplerState.GetAddressOf());
	}

	this->drawBuffer.data.objectIndex = index;
	this->drawBuffer.data.uvOffset = XMFLOAT2(obj->GetCol() * obj->GetTileWidth(), obj->GetRow() * obj->GetTileHeight());
	this->drawBuffer.data.uvScale = XMFLOAT2(obj->GetScaleU(), obj->GetScaleV());
	this->drawBuffer.data.tintColor = obj->GetTintColor();
	this->drawBuffer.data.tintIntensity = obj->GetTintIntensity();

	if (!this->drawBuffer.ApplyChanges())
		return;

	this->deviceContext->VSSetConstantBuffers(0, 1, this->drawBuffer.GetAddressOf());
	this->deviceContext->PSSetConstantBuffers(0, 1, this->drawBuffer.GetAddressOf());

	if (obj->texture)
	{
		this->deviceContext->PSSetShaderResources(0, 1, obj->texture->GetAddressOf());
	}

	this->deviceContext->IASetVertexBuffers(0, 1, obj->vertexBuffer.GetAddressOf(), obj->vertexBuffer.StridePtr(), &offset);
	this->deviceContext->IASetIndexBuffer(indicesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	this->deviceContext->DrawIndexed(indicesBuffer.BufferSize(), 0, 0);
}

bool RMPG::Graphics::CreateSamplerStates()
{
	D3D11_SAMPLER_DESC pointDesc;
	ZeroMemory(&pointDesc, sizeof(pointDesc));
	pointDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	pointDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	pointDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	pointDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	pointDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	pointDesc.MinLOD = 0;
	pointDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr = this->device->CreateSamplerState(&pointDesc, this->pointSamplerState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create point sampler state.");
		return false;
	}

	D3D11_SAMPLER_DESC linearDesc;
	ZeroMemory(&linearDesc, sizeof(linearDesc));
	linearDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	linearDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	linearDesc.MinLOD = 0;
	linearDesc.MaxLOD = D3D11_FLOAT32_MAX;
	linearDesc.MipLODBias = 0.0f;
	linearDesc.MaxAnisotropy = 1;

	hr = this->device->CreateSamplerState(&linearDesc, this->linearSamplerState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create linear sampler state.");
		return false;
	}

	return true;
}

XMMATRIX RMPG::Graphics::CalculateObjectWorldMatrix(ObjectID objectId) const
{
	auto objIt = objects.find(objectId);
	if (objIt == objects.end())
		return XMMatrixIdentity();

	Group* containingGroup = nullptr;
	for (const auto& [groupId, group] : groups)
	{
		if (group->ContainsObject(objectId))
		{
			containingGroup = group.get();
			break;
		}
	}

	XMMATRIX objectMatrix = objIt->second->GetMatrix();

	if (containingGroup)
	{
		return objectMatrix * containingGroup->GetMatrix();
	}

	return objectMatrix;
}
