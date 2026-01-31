#include "Group.h"

namespace RMPG
{
    void Group::AddObject(ObjectID objectId)
    {
        for (const auto& id : objects)
        {
            if (id == objectId)
                return;
        }
        objects.push_back(objectId);
    }

    bool Group::RemoveObject(ObjectID objectId)
    {
        for (auto it = objects.begin(); it < objects.end(); it++)
        {
            if (*it == objectId)
            {
                objects.erase(it);
                return true;
            }
        }
        return false;
    }

    bool Group::ContainsObject(ObjectID objectId) const
    {
        for (const auto& id : objects)
        {
            if (id == objectId)
                return true;
        }
        return false;
    }

    void Group::Clear()
    {
        objects.clear();
    }

    void Group::SetMatrix(const XMMATRIX& matrix)
    {
        transformMatrix = matrix;
    }

    XMMATRIX Group::GetMatrix() const
    {
        return this->transformMatrix;
    }

    void Group::SetPosition(float x, float y, float z)
    {
        this->position = XMFLOAT3(x, y, z);
        UpdateTransformMatrix();
    }

    void Group::SetPosition(const XMFLOAT3& position)
    {
        this->position = position;
        UpdateTransformMatrix();
    }

    void Group::SetRotation(float pitch, float yaw, float roll)
    {
        this->rotation = XMFLOAT3(pitch, yaw, roll);
        UpdateTransformMatrix();
    }

    void Group::SetRotation(const XMFLOAT3& rotation)
    {
        this->rotation = rotation;
        UpdateTransformMatrix();
    }

    void Group::SetScale(float x, float y, float z)
    {
        this->scale = XMFLOAT3(x, y, z);
        UpdateTransformMatrix();
    }

    void Group::SetScale(const XMFLOAT3& scale)
    {
        this->scale = scale;
        UpdateTransformMatrix();
    }

    void Group::SetScale(float uniformScale)
    {
        this->scale = XMFLOAT3(uniformScale, uniformScale, uniformScale);
        UpdateTransformMatrix();
    }

    void Group::AdjustPosition(float x, float y, float z)
    {
        this->position.x += x;
        this->position.y += y;
        this->position.z += z;
        UpdateTransformMatrix();
    }

    void Group::AdjustPosition(const XMFLOAT3& position)
    {
        this->position.x += position.x;
        this->position.y += position.y;
        this->position.z += position.z;
        UpdateTransformMatrix();
    }

    void Group::AdjustRotation(float pitch, float yaw, float roll)
    {
        this->rotation.x += pitch;
        this->rotation.y += yaw;
        this->rotation.z += roll;
        UpdateTransformMatrix();
    }

    void Group::AdjustRotation(const XMFLOAT3& rotation)
    {
        this->rotation.x += rotation.x;
        this->rotation.y += rotation.y;
        this->rotation.z += rotation.z;
        UpdateTransformMatrix();
    }

    void Group::AdjustScale(float x, float y, float z)
    {
        scale.x += x;
        scale.y += y;
        scale.z += z;
        UpdateTransformMatrix();
    }

    void Group::AdjustScale(const XMFLOAT3& scale)
    {
        this->scale.x += scale.x;
        this->scale.y += scale.y;
        this->scale.z += scale.z;
        UpdateTransformMatrix();
    }

    void Group::AdjustScale(float uniformScale)
    {
        scale.x += uniformScale;
        scale.y += uniformScale;
        scale.z += uniformScale;
        UpdateTransformMatrix();
    }

    const std::vector<ObjectID>& Group::GetObjects() const
    {
        return this->objects;
    }

    size_t Group::GetObjectCount() const
    {
        return this->objects.size();
    }

    bool Group::IsEmpty() const
    {
        return this->objects.empty();
    }

    void Group::UpdateTransformMatrix()
    {
        XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
        XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
        transformMatrix = scaleMatrix * rotationMatrix * translationMatrix;
    }
}
