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
#include "Group.h"

namespace RMPG
{
	struct DynamicTextMeta
	{
		std::wstring fontFile;
		int fontPixelSize;
		float scale;
	};

	struct WorldBounds {
		XMFLOAT2 topLeft;
		XMFLOAT2 topRight;
		XMFLOAT2 bottomLeft;
		XMFLOAT2 bottomRight;
		XMFLOAT2 center;
		float width;
		float height;
	};
	using TextureID = int;

	class Graphics
	{
	public:
		bool Initialize(HWND hwnd, int width, int height);
		void RenderFrame();
		bool Resize(int width, int height);
		bool SetFullScreen(bool fullscreen);
		void SetVSync(bool vsync);

		std::vector<ObjectID> PickObjectsAt(int mouseX, int mouseY);

		XMFLOAT3 ScreenToWorldOnPlane(int mouseX, int mouseY, float planeZ = 0.0f);
		void ScreenToWorldRay(int mouseX, int mouseY, XMFLOAT3& outOrigin, XMFLOAT3& outDir);

		int AddTexture(const std::wstring& texFilePath);

		ObjectID AddObject(float width, float height, float coordZ, TextureID textureId);
		ObjectID AddTextObjectFromFontFile(const std::wstring& fontFilePath, const std::wstring& text, int fontPixelSize, float coordZ, float scale = 1.0f);
		ObjectID AddStyledTextObject(const std::vector<TextRun>& runs, float coordZ, float scale = 1.0f);

		int UpdateTextObject(ObjectID objectId, const std::wstring& text);
		bool UpdateStyledTextObject(ObjectID objectId, const std::vector<TextRun>& runs);
		bool SetObjectMatrix(ObjectID objectId, XMMATRIX mat);
		bool SetObjectTintColor(ObjectID objectId, const XMFLOAT4& color, float intensity = 1.0f);

		bool RemoveObject(ObjectID objectId);
		bool RemoveAllObjects();
		bool RemoveTexture(TextureID textureId);
		bool RemoveAllTextures();

		RMPG::Object2d* GetObjectPtr(ObjectID objectId);
		XMMATRIX GetObjectWorldMatrix(ObjectID objectId) const;
		RMPG::Texture2d* GetTexturePtr(TextureID textureId);

		XMFLOAT4 GetObjectTintColor(ObjectID objectId) const;
		float GetObjectTintIntensity(ObjectID objectId) const;

		bool ObjectExists(ObjectID objectId) const;
		bool TexturerExists(TextureID textureId) const;

		int GetFps();

		Camera camera;

		XMFLOAT2 GetWorldCoordAtScreenPoint(int screenX, int screenY, float worldZ = 0.0f);
		XMFLOAT2 GetTopLeftWorldCoord(float worldZ = 0.0f);
		XMFLOAT2 GetTopRightWorldCoord(float worldZ = 0.0f);
		XMFLOAT2 GetBottomLeftWorldCoord(float worldZ = 0.0f);
		XMFLOAT2 GetBottomRightWorldCoord(float worldZ = 0.0f);
		XMFLOAT2 GetCenterWorldCoord(float worldZ = 0.0f);
		WorldBounds GetCameraWorldBounds(float worldZ = 0.0f);
		XMFLOAT2 WorldToScreenCoord(const XMFLOAT3& worldPos);
		bool IsWorldPointVisible(const XMFLOAT3& worldPos);

		GroupID CreateGroup();
		bool DestroyGroup(GroupID groupId);
		bool AddObjectToGroup(GroupID groupId, ObjectID objectId);
		bool RemoveObjectFromGroup(GroupID groupId, ObjectID objectId);
		bool RemoveObjectFromAllGroups(ObjectID objectId);

		bool SetGroupMatrix(GroupID groupId, const XMMATRIX& matrix);
		bool SetGroupPosition(GroupID groupId, float x, float y, float z);
		bool SetGroupRotation(GroupID groupId, float pitch, float yaw, float roll);
		bool SetGroupScale(GroupID groupId, float x, float y, float z);
		bool SetGroupScale(GroupID groupId, float scale);

		bool AdjustGroupPosition(GroupID groupId, float x, float y, float z);
		bool AdjustGroupRotation(GroupID groupId, float pitch, float yaw, float roll);
		bool AdjustGroupScale(GroupID groupId, float x, float y, float z);
		bool AdjustGroupScale(GroupID groupId, float scale);

		bool GroupExists(GroupID groupId) const;
		const Group* GetGroupPtr(GroupID groupId) const;
		size_t GetGroupCount() const;

	private:
		bool InitializeDirectX(HWND hwnd);
		bool InitializeShaders();
		bool InitializeScene();

		HRESULT EnsurePerObjectBufferCapacity(int requiredCount);

		void ReleaseObjectResources(Object2d* obj);
		void ReleaseTextureResources(Texture2d* tex);

		void UpdatePerObjectBuffer();
		void RenderObject(ObjectID id, Object2d* obj);
		void RenderDepthPass();
		void RenderColorPass();
		void RenderObjectForDepth(ObjectID id, Object2d* obj);

		bool CreateSamplerStates();

		XMMATRIX CalculateObjectWorldMatrix(ObjectID objectId) const;

		Microsoft::WRL::ComPtr<ID3D11Device> device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;

		VertexShader vertexshader;
		PixelShader pixelshader;
		VertexShader depth_vertexshader;
		PixelShader depth_pixelshader;

		ConstantBuffer<CBDraw> drawBuffer;

		Microsoft::WRL::ComPtr<ID3D11Buffer> perObjectStructuredBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> perObjectSRV;
		int perObjectCapacity = 0;

		IndexBuffer indicesBuffer;

		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSamplerState;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> linearSamplerState;
		Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;

		int windowWidth = 0;
		int windowHeight = 0;

		int vsync_on = 1;

		Timer fpsTimer;
		int fpsCounter = 0;
		int curFps = 0;

		std::map<ObjectID, std::unique_ptr<Object2d>> objects;
		std::map<TextureID, std::unique_ptr<Texture2d>> textures;
		std::map<GroupID, std::unique_ptr<Group>> groups;

		ObjectID nextObjectId = 0;
		TextureID nextTextureId = 0;
		GroupID nextGroupId = 0;

		std::map<TextureID, std::unique_ptr<Texture2d>> dynamicTextures;
		std::map<TextureID, DynamicTextMeta> dynamicTextMeta;
		std::map<ObjectID, TextureID> objectToDynamicTexture;
		std::map<ObjectID, TextureID> objectToTexture;
		std::map<TextureID, std::set<ObjectID>> textureUsage;

	};
};
