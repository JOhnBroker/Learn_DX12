#ifndef OCTREE_H
#define OCTREE_H

#include <d3dUtil.h>

struct OctreeNode;

class Octree
{
public:
	Octree();
	~Octree();

	void Build(const std::vector<DirectX::XMFLOAT3>& vertices, const std::vector<UINT>& indices);
	bool RayOctreeIntersect(DirectX::XMVECTOR rayPos, DirectX::XMVECTOR rayDir);

private:
	DirectX::BoundingBox BuildAABB();
	void BuildOctree(OctreeNode* parent, const std::vector<UINT>& indices);
	bool RayOctreeIntersect(OctreeNode* parent, DirectX::XMVECTOR rayPos, DirectX::XMVECTOR rayDir);

private:
	OctreeNode* m_Root;
	std::vector<DirectX::XMFLOAT3> m_Vertices;
};

struct OctreeNode
{
#pragma region Properties
	DirectX::BoundingBox Bounds;
	std::vector<UINT> Indices;
	OctreeNode* Children[8];
	bool IsLeaf;
#pragma endregion

	OctreeNode() 
	{
		for (int i = 0; i < 8; ++i) 
		{
			Children[i] = nullptr;
		}
		Bounds.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		Bounds.Extents = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		IsLeaf = false;
	}

	~OctreeNode() 
	{
		for (int i = 0; i < 8; ++i) 
		{
			if (Children[i]) 
			{
				delete Children[i];
				Children[i] = nullptr;
			}
		}
	}

	void Subdivide(DirectX::BoundingBox box[8]) 
	{
		DirectX::XMFLOAT3 halfExtent(
			0.5f * Bounds.Extents.x,
			0.5f * Bounds.Extents.y,
			0.5f * Bounds.Extents.z);

		// Top
		box[0].Center = DirectX::XMFLOAT3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[0].Extents = halfExtent;

		box[1].Center = DirectX::XMFLOAT3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[1].Extents = halfExtent;

		box[2].Center = DirectX::XMFLOAT3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[2].Extents = halfExtent;

		box[3].Center = DirectX::XMFLOAT3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y + halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[3].Extents = halfExtent;

		// Bottom
		box[4].Center = DirectX::XMFLOAT3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[4].Extents = halfExtent;

		box[5].Center = DirectX::XMFLOAT3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z + halfExtent.z);
		box[5].Extents = halfExtent;

		box[6].Center = DirectX::XMFLOAT3(
			Bounds.Center.x - halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[6].Extents = halfExtent;

		box[7].Center = DirectX::XMFLOAT3(
			Bounds.Center.x + halfExtent.x,
			Bounds.Center.y - halfExtent.y,
			Bounds.Center.z - halfExtent.z);
		box[7].Extents = halfExtent;
	}

};

#endif // !OCTREE_H
