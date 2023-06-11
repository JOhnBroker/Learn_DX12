#include "Octree.h"

#define LEAF_TRIANGLE_COUNT 60

using namespace DirectX;

Octree::Octree() 
	:m_Root(nullptr)
{
}

Octree::~Octree()
{
	if (m_Root) 
	{
		delete m_Root;
		m_Root = nullptr;
	}
}

void Octree::Build(const std::vector<XMFLOAT3>& vertices, const std::vector<UINT>& indices)
{
	m_Vertices = vertices;
	BoundingBox sceneBounds = BuildAABB();

	m_Root = new OctreeNode();
	m_Root->Bounds = sceneBounds;

	BuildOctree(m_Root, indices);
}

bool Octree::RayOctreeIntersect(XMVECTOR rayPos, XMVECTOR rayDir)
{
	return RayOctreeIntersect(m_Root, rayPos, rayDir);
}

BoundingBox Octree::BuildAABB()
{
	XMVECTOR vmin = XMVectorReplicate(+MathHelper::Infinity);
	XMVECTOR vmax = XMVectorReplicate(-MathHelper::Infinity);

	for (size_t i = 0; i < m_Vertices.size(); ++i) 
	{
		XMVECTOR P = XMLoadFloat3(&m_Vertices[i]);

		vmin = XMVectorMin(vmin, P);
		vmax = XMVectorMax(vmax, P);
	}
	BoundingBox bounds;
	XMVECTOR Center = 0.5f * (vmin + vmax);
	XMVECTOR Extent = 0.5f * (vmax - vmin);

	XMStoreFloat3(&bounds.Center, Center);
	XMStoreFloat3(&bounds.Extents, Extent);

	return bounds;
}

void Octree::BuildOctree(OctreeNode* parent, const std::vector<UINT>& indices)
{
	size_t triCount = indices.size() / 3;
	if (triCount < LEAF_TRIANGLE_COUNT)
	{
		parent->IsLeaf = true;
		parent->Indices = indices;
	}
	else 
	{
		parent->IsLeaf = false;
		BoundingBox aabb[8];
		parent->Subdivide(aabb);
		for (int i = 0; i < 8; ++i) 
		{
			parent->Children[i] = new OctreeNode;
			parent->Children[i]->Bounds = aabb[i];

			// 与这个节点的包围盒相交的三角形
			std::vector<UINT> intersectedTriangleIndices;
			for (size_t j = 0; j < triCount; ++j) 
			{
				UINT i0 = indices[j * 3 + 0];
				UINT i1 = indices[j * 3 + 1];
				UINT i2 = indices[j * 3 + 2];

				XMVECTOR v0 = XMLoadFloat3(&m_Vertices[i0]);
				XMVECTOR v1 = XMLoadFloat3(&m_Vertices[i1]);
				XMVECTOR v2 = XMLoadFloat3(&m_Vertices[i2]);

				if (aabb[i].Intersects(v0, v1, v2)) 
				{
					intersectedTriangleIndices.push_back(i0);
					intersectedTriangleIndices.push_back(i1);
					intersectedTriangleIndices.push_back(i2);
				}
			}
			BuildOctree(parent->Children[i], intersectedTriangleIndices);

		}
	}
}

bool Octree::RayOctreeIntersect(OctreeNode* parent, XMVECTOR rayPos, XMVECTOR rayDir)
{
	bool bResult = false;

	if(!parent->IsLeaf)
	{
		for (int i = 0; i < 8; ++i) 
		{
			float dist = 0.0f;
			if (parent->Children[i]->Bounds.Intersects(rayDir, rayDir, dist))
			{
				if (RayOctreeIntersect(parent->Children[i], rayPos, rayDir)) 
				{
					goto Exit1;
				}
			}
		}
		goto Exit0;
	}
	else 
	{
		size_t triCount = parent->Indices.size() / 3;
		for (size_t i = 0; i < triCount; ++i) 
		{
			UINT i0 = parent->Indices[i * 3 + 0];
			UINT i1 = parent->Indices[i * 3 + 1];
			UINT i2 = parent->Indices[i * 3 + 2];

			XMVECTOR v0 = XMLoadFloat3(&m_Vertices[i0]);
			XMVECTOR v1 = XMLoadFloat3(&m_Vertices[i1]);
			XMVECTOR v2 = XMLoadFloat3(&m_Vertices[i2]);

			float dist = 0.0f;
			if (TriangleTests::Intersects(rayPos, rayDir, v0, v1, v2, dist))
			{
				goto Exit1;
			}
		}
		goto Exit0;
	}

Exit1:
	bResult = true;
Exit0:
	return bResult;
}
