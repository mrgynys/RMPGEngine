#pragma once
#include "AdapterReader.h"
#include "Shaders.h"
#include "Vertex.h"
#include <WICTextureLoader.h>
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ConstantBuffer.h"
#include "Camera.h"
#include "../Timer.h"
#include "Texture2d.h"
#include "Object2d.h"
#include <algorithm>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include "FontRasterizer.h"
#include <locale>
#include <codecvt>
#include "Font.h"

namespace RMPG
{
	struct DynamicTextMeta
	{
		std::wstring fontFile;
		int fontPixelSize;
		float scale;
	};

	using ObjectID = int;
	using TextureID = int;

	class Graphics
	{
	public:
		bool Initialize(HWND hwnd, int width, int height);
		void RenderFrame();
		bool Resize(int width, int height);
		bool SetFullScreen(bool fullscreen);
		void SetVSync(bool vsync);

		ObjectID PickObjectAt(int mouseX, int mouseY);

		XMFLOAT3 ScreenToWorldOnPlane(int mouseX, int mouseY, float planeZ = 0.0f);
		void ScreenToWorldRay(int mouseX, int mouseY, XMFLOAT3& outOrigin, XMFLOAT3& outDir);

		int AddTexture(const std::wstring& texFilePath);

		ObjectID AddObject(float width, float height, float coordZ, TextureID textureId);
		ObjectID AddTextObjectFromFontFile(const std::wstring& fontFilePath, const std::wstring& text, int fontPixelSize, float coordZ, float scale = 1.0f);
		ObjectID AddStyledTextObject(const std::vector<TextRun>& runs, float coordZ, float scale = 1.0f);

		int UpdateTextObject(ObjectID objectId, const std::wstring& text);
		bool UpdateStyledTextObject(ObjectID objectId, const std::vector<TextRun>& runs);
		bool SetObjectMatrix(ObjectID objectId, XMMATRIX mat);

		bool RemoveObject(ObjectID objectId);
		bool RemoveAllObjects();
		bool RemoveTexture(TextureID textureId);
		bool RemoveAllTextures();

		RMPG::Object2d* GetObjectPtr(ObjectID objectId);
		RMPG::Texture2d* GetTexturePtr(TextureID textureId);

		bool ObjectExists(ObjectID objectId) const;
		bool TexturerExists(TextureID textureId) const;

		int GetFps();

		Camera camera;

	private:
		bool InitializeDirectX(HWND hwnd);
		bool InitializeShaders();
		bool InitializeScene();

		HRESULT EnsurePerObjectBufferCapacity(int requiredCount);

		void ReleaseObjectResources(Object2d* obj);
		void ReleaseTextureResources(Texture2d* tex);

		void UpdatePerObjectBuffer();
		void RenderObject(ObjectID id, Object2d* obj);

		Microsoft::WRL::ComPtr<ID3D11Device> device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;

		VertexShader vertexshader;
		PixelShader pixelshader;

		ConstantBuffer<CBDraw> drawBuffer;

		Microsoft::WRL::ComPtr<ID3D11Buffer> perObjectStructuredBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> perObjectSRV;
		int perObjectCapacity = 0;

		IndexBuffer indicesBuffer;

		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
		Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;

		int windowWidth = 0;
		int windowHeight = 0;

		int vsync_on = 1;

		Timer fpsTimer;
		int fpsCounter = 0;
		int curFps = 0;

		std::map<ObjectID, std::unique_ptr<Object2d>> objects;
		std::map<TextureID, std::unique_ptr<Texture2d>> textures;

		ObjectID nextObjectId = 0;
		TextureID nextTextureId = 0;

		std::map<TextureID, std::unique_ptr<Texture2d>> dynamicTextures;
		std::map<TextureID, DynamicTextMeta> dynamicTextMeta;
		std::map<ObjectID, TextureID> objectToDynamicTexture;
		std::map<ObjectID, TextureID> objectToTexture;
		std::map<TextureID, std::set<ObjectID>> textureUsage;

	};
};
