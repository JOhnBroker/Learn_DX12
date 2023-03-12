#include "GeometryGenerator.h"
#include <algorithm>

using namespace DirectX;

GeometryGenerator::MeshData GeometryGenerator::CreateBox(float width, float height, float depth, uint32 numSubdivisions)
{
    MeshData meshData;

    Vertex v[24];
    uint32 i[36];

    float w2 = 0.5f * width;
    float h2 = 0.5f * height;
    float d2 = 0.5f * depth;

    // front face
    v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    
    // back face
    v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // top face
    v[8]  = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[9]  = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

    // bottom face
    v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // left face
    v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
    v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
    v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
    v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

    // right face
    v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    meshData.Vertices.assign(&v[0], &v[24]);

    // front face
    i[0] = 0; i[1] = 1; i[2] = 2;
    i[3] = 0; i[4] = 2; i[5] = 3;

    // back face
    i[6] = 4; i[7] = 5; i[8] = 6;
    i[9] = 4; i[10] = 6; i[11] = 7;

    // top face
    i[12] = 8; i[13] = 9; i[14] = 10;
    i[15] = 8; i[16] = 10; i[17] = 11;

    // bottom face
    i[18] = 12; i[19] = 13; i[20] = 14;
    i[21] = 12; i[22] = 14; i[23] = 15;

    // left face
    i[24] = 16; i[25] = 17 ; i[26] = 18;
    i[27] = 16; i[28] = 18; i[29] = 19;

    // right face
    i[30] = 20; i[31] = 21; i[32] = 22;
    i[33] = 20; i[34] = 22; i[35] = 23;

    meshData.Indices32.assign(&i[0], &i[36]);

    numSubdivisions = std::min<uint32>(numSubdivisions, 0u);

	for (uint32 time = 0; time < numSubdivisions; ++time)
	{
		Subdivide(meshData);
	}

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateSphere(float radius, uint32 sliceCount, uint32 stackCount)
{
    MeshData meshData;

    Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI / sliceCount;

    meshData.Vertices.push_back(topVertex);

    for (uint32 i = 1; i <= stackCount - 1; ++i) 
    {
        float phi = i * phiStep;

        for (uint32 j = 0; j <= sliceCount; ++j) 
        {
            float theta = j * thetaStep;
            
            Vertex v;

            v.Position.x = radius * sinf(phi) * cosf(theta);
            v.Position.y = radius * cosf(phi);
            v.Position.z = radius * sinf(phi) * sinf(theta);

            v.TangentU.x = -radius * sinf(phi) * sinf(theta);
            v.TangentU.y = 0.0f;
            v.TangentU.z = +radius * sinf(phi) * cosf(theta);

            XMVECTOR T = XMLoadFloat3(&v.TangentU);
            XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

            XMVECTOR P = XMLoadFloat3(&v.Position);
            XMStoreFloat3(&v.Normal, XMVector3Normalize(P));

            v.TexC.x = theta / XM_2PI;
            v.TexC.y = phi / XM_PI;

            meshData.Vertices.push_back(v);
        }
    }

    meshData.Vertices.push_back(bottomVertex);

    // Compute indices for top stack.  The top stack was written first to the vertex buffer
    // and connects the top pole to the first ring.

    for (uint32 i = 1; i <= sliceCount; ++i) 
    {
        meshData.Indices32.push_back(0);
        meshData.Indices32.push_back(i + 1);
        meshData.Indices32.push_back(i);
    }

    // Offset the indices to the index of the first vertex in the first ring.
    // This is just skipping the top pole vertex.
    uint32 baseIndex = 1;
    uint32 ringVertexCount = sliceCount + 1;
    for (uint32 i = 0; i < stackCount - 2; ++i) 
    {
        for (uint32 j = 0; j < sliceCount; ++j) 
        {
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);

            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
            meshData.Indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    // Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
    // and connects the bottom pole to the bottom ring.
    uint32 southPoleIndex = (uint32)meshData.Vertices.size() - 1;

    baseIndex = southPoleIndex - ringVertexCount;

    for (uint32 i = 0; i < sliceCount; ++i) 
    {
        meshData.Indices32.push_back(southPoleIndex);
        meshData.Indices32.push_back(baseIndex + i);
        meshData.Indices32.push_back(baseIndex + i + 1);
    }

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGeosphere(float radius, uint32 numSubdivisions)
{
    MeshData meshData;

    // 确定细分的次数 
    numSubdivisions = std::min<uint32>(numSubdivisions, 0u);

    // 通过对正二十面体进行曲面细分 逼近 球体
    const float X = 0.525731f;
    const float Z = 0.850651f;

    XMFLOAT3 pos[12] =
    {
        XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
        XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
        XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
        XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
        XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
        XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
    };

    uint32 indices[60] =
    {
        1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
        1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
        3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
        10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
    };

    meshData.Vertices.resize(12);
    meshData.Indices32.assign(&indices[0], &indices[60]);

    for (uint32 i = 0; i < 12; ++i) 
    {
        meshData.Vertices[i].Position = pos[i];
    }

    for (uint32 i = 0; i < numSubdivisions; ++i) 
    {
        Subdivide(meshData);
    }

    // 将三维顶点投影到纹理坐标
    for (uint32 i = 0; i < meshData.Vertices.size(); ++i) 
    {
        // 映射到单位球上
        XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));

        // 投射到球面上
        XMVECTOR p = radius * n;

        XMStoreFloat3(&meshData.Vertices[i].Position, p);
        XMStoreFloat3(&meshData.Vertices[i].Normal, n);

        // 根据球面坐标推导纹理坐标
        float theta = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);

        if (theta < 0.0f) 
        {
            theta += XM_2PI;
        }

        float phi = acosf(meshData.Vertices[i].Position.y / radius);

        meshData.Vertices[i].TexC.x = theta / XM_2PI;
        meshData.Vertices[i].TexC.y = phi / XM_PI;

        meshData.Vertices[i].TangentU.x = -radius * sinf(phi) * sinf(theta);
        meshData.Vertices[i].TangentU.y = 0.0f;
        meshData.Vertices[i].TangentU.z = +radius * sinf(phi) * cosf(theta);

        XMVECTOR T = XMLoadFloat3(&meshData.Vertices[i].TangentU);
        XMStoreFloat3(&meshData.Vertices[i].TangentU, XMVector3Normalize(T));
    }
    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount)
{
    MeshData meshData;

    // 层高
    float stackHeight = height / stackCount;
    // 每层的半径
    float radiusStep = (topRadius - bottomRadius) / stackCount;
    uint32 ringCount = stackCount + 1;
    uint32 ringVertexCount = sliceCount + 1;

    // 从底部开始，技术堆叠层环的顶点坐标
    for (uint32 i = 0; i < ringCount; ++i) 
    {
        float y = -0.5f * height + i * stackHeight;
        float r = bottomRadius + i * radiusStep;

        float dTheta = 2.0f * XM_PI / sliceCount;
        for (uint32 j = 0; j <= sliceCount; ++j) 
        {
            Vertex vertex;

            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);
            
            vertex.Position = XMFLOAT3(r * c, y, r * s);
            vertex.TexC.x = (float)j / sliceCount;
            vertex.TexC.y = 1.0f - (float)i / stackCount;

            // Cylinder can be parameterized as follows, where we introduce v
            // parameter that goes in the same direction as the v tex-coord
            // so that the bitangent goes in the same direction as the v tex-coord.
            //   Let r0 be the bottom radius and let r1 be the top radius.
            //   y(v) = h - hv for v in [0,1].
            //   r(v) = r1 + (r0-r1)v
            //
            //   x(t, v) = r(v)*cos(t)
            //   y(t, v) = h - hv
            //   z(t, v) = r(v)*sin(t)
            // 
            //  dx/dt = -r(v)*sin(t)
            //  dy/dt = 0
            //  dz/dt = +r(v)*cos(t)
            //
            //  dx/dv = (r0-r1)*cos(t)
            //  dy/dv = -h
            //  dz/dv = (r0-r1)*sin(t)

            // This is unit length.
            vertex.TangentU = XMFLOAT3(-s, 0.0f, c);
            
            float dr = bottomRadius - topRadius;
            XMFLOAT3 bitangent(dr * c, -height, dr * s);

            XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
            XMVECTOR B = XMLoadFloat3(&bitangent);
            XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));

            XMStoreFloat3(&vertex.Normal, N);
        
            meshData.Vertices.push_back(vertex);
        }
    }

    for (uint32 i = 0; i < stackCount; ++i) 
    {
        for (uint32 j = 0; j < sliceCount; ++j) 
        {
            meshData.Indices32.push_back(i * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);

            meshData.Indices32.push_back(i * ringVertexCount + j);
            meshData.Indices32.push_back((i + 1) * ringVertexCount + j + 1);
            meshData.Indices32.push_back(i * ringVertexCount + j + 1);
        }
    }

    BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
    BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32 m, uint32 n)
{
    MeshData meshData;

    uint32 vertexCount = m * n;
    uint32 faceCount = (m - 1) * (n - 1) * 2;

    // vertices

    float halfWidth = 0.5f * width;
    float halfDepth = 0.5f * depth;

    float dx = width / (n - 1);
    float dz = depth / (m - 1);

    float du = 1.0f / (n - 1);
    float dv = 1.0f / (m - 1);

    meshData.Vertices.resize(vertexCount);

    for (uint32 i = 0; i < m; ++i)
    {
        float z = halfDepth - i * dz;
        for (uint32 j = 0; j < n; ++j) 
        {
            float x = -halfWidth + j * dx;
            meshData.Vertices[i * n + j].Position = XMFLOAT3(x, 0.0f, z);
            meshData.Vertices[i * n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
            meshData.Vertices[i * n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

            meshData.Vertices[i * n + j].TexC.x = j * du;
            meshData.Vertices[i * n + j].TexC.y = i * dv;
        }
    }

    // indices
    meshData.Indices32.resize(faceCount * 3);
    
    uint32 k = 0;
    for (uint32 i = 0; i < m - 1; ++i) 
    {
        for (uint32 j = 0; j < n - 1; ++j) 
        {
            meshData.Indices32[k] = i * n + j;
            meshData.Indices32[k + 1] = i * n + j + 1;
            meshData.Indices32[k + 2] = (i + 1) * n + j;

            meshData.Indices32[k + 3] = (i + 1) * n + j;
            meshData.Indices32[k + 4] = i * n + j + 1;
            meshData.Indices32[k + 5] = (i + 1) * n + j + 1;

            k += 6;
        }
    }

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateQuad(float x, float y, float w, float h, float depth)
{
    MeshData meshData;

    meshData.Vertices.resize(4);
    meshData.Indices32.resize(6);

    meshData.Vertices[0] = Vertex(
        x, y - h, depth,
        0.0f, 0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f);
    meshData.Vertices[1] = Vertex(
        x, y , depth,
        0.0f, 0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 0.0f);
    meshData.Vertices[2] = Vertex(
        x + w, y, depth,
        0.0f, 0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f);
    meshData.Vertices[3] = Vertex(
        x + w, y - h, depth,
        0.0f, 0.0f, -1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f);

    meshData.Indices32[0] = 0;
    meshData.Indices32[1] = 1;
    meshData.Indices32[2] = 2;

    meshData.Indices32[3] = 0;
    meshData.Indices32[4] = 2;
    meshData.Indices32[5] = 3;

    return meshData;
}

void GeometryGenerator::Subdivide(MeshData& meshData)
{

    MeshData inputCopy = meshData;

    meshData.Vertices.resize(0);
    meshData.Indices32.resize(0);

    //       v1
    //       *
    //      / \
	//     /   \
	//  m0*-----*m1
    //   / \   / \
	//  /   \ /   \
	// *-----*-----*
    // v0    m2     v2

    uint32 numTri = (uint32)inputCopy.Indices32.size() / 3;
    for (uint32 i = 0; i < numTri; ++i) 
    {
        Vertex v0 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 0]];
        Vertex v1 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 1]];
        Vertex v2 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 2]];

        // midpoints
        Vertex m0 = MidPoint(v0, v1);
        Vertex m1 = MidPoint(v1, v2);
        Vertex m2 = MidPoint(v0, v2);

        // Add new geometry
        meshData.Vertices.push_back(v0);
        meshData.Vertices.push_back(v1);
        meshData.Vertices.push_back(v2);
        meshData.Vertices.push_back(m0);
        meshData.Vertices.push_back(m1);
        meshData.Vertices.push_back(m2);

        meshData.Indices32.push_back(i * 6 + 0);
        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 5);

        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 4);
        meshData.Indices32.push_back(i * 6 + 5);

        meshData.Indices32.push_back(i * 6 + 5);
        meshData.Indices32.push_back(i * 6 + 4);
        meshData.Indices32.push_back(i * 6 + 2);

        meshData.Indices32.push_back(i * 6 + 3);
        meshData.Indices32.push_back(i * 6 + 1);
        meshData.Indices32.push_back(i * 6 + 4);
    }
}

GeometryGenerator::Vertex GeometryGenerator::MidPoint(const Vertex& v0, const Vertex& v1)
{
    XMVECTOR p0 = XMLoadFloat3(&v0.Position);
    XMVECTOR p1 = XMLoadFloat3(&v1.Position);

    XMVECTOR n0 = XMLoadFloat3(&v0.Normal);
    XMVECTOR n1 = XMLoadFloat3(&v1.Normal);

    XMVECTOR tan0 = XMLoadFloat3(&v0.TangentU);
    XMVECTOR tan1 = XMLoadFloat3(&v1.TangentU);

    XMVECTOR tex0 = XMLoadFloat2(&v0.TexC);
    XMVECTOR tex1 = XMLoadFloat2(&v1.TexC);

    XMVECTOR pos = 0.5f * (p0 + p1);
    XMVECTOR normal = XMVector3Normalize(0.5f * (n0 + n1));
    XMVECTOR tangent = XMVector3Normalize(0.5f * (tan0 + tan1));
    XMVECTOR tex = 0.5f * (tex0 + tex1);

    Vertex v;
    XMStoreFloat3(&v.Position, pos);
    XMStoreFloat3(&v.Normal, normal);
    XMStoreFloat3(&v.TangentU, tangent);
    XMStoreFloat2(&v.TexC, tex);

    return v;
}

void GeometryGenerator::BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData)
{
    // 初始顶点数量
    uint32 baseIndex = (uint32)meshData.Vertices.size();

    float y = 0.5f * height;
    float dTheta = 2.0f * XM_PI / sliceCount;

    for (uint32 i = 0; i <= sliceCount; ++i) 
    {
        float x = topRadius * cosf(i * dTheta);
        float z = topRadius * sinf(i * dTheta);

        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }

    meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

    uint32 centerIndex = (uint32)meshData.Vertices.size() - 1;

    for (uint32 i = 0; i < sliceCount; ++i) 
    {
        meshData.Indices32.push_back(centerIndex);
        meshData.Indices32.push_back(baseIndex + i + 1);
        meshData.Indices32.push_back(baseIndex + i);
    }

}

void GeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData)
{
    uint32 baseIndex = (uint32)meshData.Vertices.size();

    float y = -0.5f * height;
    float dTheta = 2.0f * XM_PI / sliceCount;

    for (uint32 i = 0; i <= sliceCount; ++i) 
    {
        float x = bottomRadius * cosf(i * dTheta);
        float z = bottomRadius * sinf(i * dTheta);

        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
    }
    meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

    uint32 centerIndex = (uint32)meshData.Vertices.size() - 1;

    for (uint32 i = 0; i < sliceCount; ++i) 
    {
        meshData.Indices32.push_back(centerIndex);
        meshData.Indices32.push_back(baseIndex + i);
        meshData.Indices32.push_back(baseIndex + i + 1);
    }
}
