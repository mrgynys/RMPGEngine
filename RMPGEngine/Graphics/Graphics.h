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

	class Graphics
	{
	public:
		bool Initialize(HWND hwnd, int width, int height);
		void RenderFrame();
		bool Resize(int width, int height);
		bool SetFullScreen(bool fullscreen);
		void SetVSync(bool vsync);

		int PickObjectAt(int mouseX, int mouseY);

		XMFLOAT3 ScreenToWorldOnPlane(int mouseX, int mouseY, float planeZ = 0.0f);
		void ScreenToWorldRay(int mouseX, int mouseY, XMFLOAT3& outOrigin, XMFLOAT3& outDir);

		int AddTexture(const std::wstring& texFilePath);

		int AddObject(float width, float height, float coordZ, RMPG::Texture2d* texture);
		int AddTextObjectFromFontFile(const std::wstring& fontFilePath, const std::wstring& text, int fontPixelSize, float coordZ, float scale = 1.0f);
		int AddStyledTextObject(const std::vector<TextRun>& runs, float coordZ, float scale = 1.0f);

		int UpdateTextObject(int objectIndex, const std::wstring& text);
		int UpdateStyledTextObject(int objectIndex, const std::vector<TextRun>& runs);
		void SetObjectMatrix(int objectIndex, XMMATRIX mat);

		int GetFps();

		Camera camera;

		std::vector<std::unique_ptr<RMPG::Object2d>> objects;
		std::vector<std::unique_ptr<RMPG::Texture2d>> textures;

	private:
		bool InitializeDirectX(HWND hwnd);
		bool InitializeShaders();
		bool InitializeScene();

		HRESULT EnsurePerObjectBufferCapacity(int requiredCount);

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

		std::vector<std::unique_ptr<RMPG::Texture2d>> dynamicTextures;

		std::vector<DynamicTextMeta> dynamicTextMeta;
		std::vector<int> objectToDynamicTex;

	};
};
