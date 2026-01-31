#pragma once
#include "Object2d.h"

namespace RMPG
{
	using GroupID = int;

	class Group
	{
	public:
		Group() = default;
		~Group() = default;

		void AddObject(ObjectID objectId);
		bool RemoveObject(ObjectID objectId);
		bool ContainsObject(ObjectID objectId) const;
		void Clear();

		void SetMatrix(const XMMATRIX& matrix);
		XMMATRIX GetMatrix() const;

		void SetPosition(float x, float y, float z);
		void SetPosition(const XMFLOAT3& position);
		void SetRotation(float pitch, float yaw, float roll);
		void SetRotation(const XMFLOAT3& rotation);
		void SetScale(float x, float y, float z);
		void SetScale(const XMFLOAT3& scale);
		void SetScale(float uniformScale);

		void AdjustPosition(float x, float y, float z);
		void AdjustPosition(const XMFLOAT3& position);
		void AdjustRotation(float pitch, float yaw, float roll);
		void AdjustRotation(const XMFLOAT3& rotation);
		void AdjustScale(float x, float y, float z);
		void AdjustScale(const XMFLOAT3& scale);
		void AdjustScale(float uniformScale);

		const std::vector<ObjectID>& GetObjects() const;
		size_t GetObjectCount() const;
		bool IsEmpty() const;

	private:
		void UpdateTransformMatrix();

		std::vector<ObjectID> objects;
		XMMATRIX transformMatrix = XMMatrixIdentity();
		XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	};
};
