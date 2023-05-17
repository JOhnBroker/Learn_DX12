#include "Waves.h"
#include <ppl.h>
#include <algorithm>
#include <cassert>

Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
{
	m_NumRows = m;
	m_NumCols = n;

	m_VertexCount = m * n;
	m_TriangleCount = (m - 1) * (n - 1) * 2;

	m_TimeStep = dt;
	m_SpatialStep = dx;

	float d = damping * dt + 2.0f;
	float e = (speed * speed) * (dt * dt) / (dx * dx);

	m_K1 = (damping * dt - 2.0f) / d;
	m_K2 = (4.0f - 8.0f * e) / d;
	m_K3 = (2.0f * e) / d;

	m_PrevSolution.resize(m_VertexCount);
	m_CurrSolution.resize(m_VertexCount);
	m_Normals.resize(m_VertexCount);
	m_TangentX.resize(m_VertexCount);

	float halfWidth = (n - 1) * dx * 0.5f;
	float halfDepth = (m - 1) * dx * 0.5f;
	for (int i = 0; i < m; ++i) 
	{
		float z = halfDepth - i * dx;
		for (int j = 0; j < n; ++j) 
		{
			float x = -halfWidth + j * dx;
			m_PrevSolution[i * n + j] = DirectX::XMFLOAT3(x, 0.0f, z);
			m_CurrSolution[i * n + j] = DirectX::XMFLOAT3(x, 0.0f, z);
			m_Normals[i * n + j] = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
			m_TangentX[i * n + j] = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
		}
	}

}

Waves::~Waves()
{
}

void Waves::Update(float dt)
{
	static float t = 0.0f;

	t += dt;

	if (t >= m_TimeStep) 
	{
		concurrency::parallel_for(1, m_NumRows - 1, [this](int i)
		{
			for (int j = 1; j < m_NumCols - 1; ++j)
			{
				m_PrevSolution[i * m_NumCols + j].y =
					m_K1 * m_PrevSolution[i * m_NumCols + j].y +
					m_K2 * m_CurrSolution[i * m_NumCols + j].y +
					m_K3 * (m_CurrSolution[(i + 1) * m_NumCols + j].y +
							m_CurrSolution[(i - 1) * m_NumCols + j].y +
							m_CurrSolution[i * m_NumCols + j + 1].y +
							m_CurrSolution[i * m_NumCols + j - 1].y);
			}
		});

		std::swap(m_PrevSolution, m_CurrSolution);

		t = 0.0f;

		concurrency::parallel_for(1, m_NumRows - 1, [this](int i)
		{
			for (int j = 1; j < m_NumCols - 1; ++j)
			{
				float l = m_CurrSolution[i * m_NumCols + j - 1].y;
				float r = m_CurrSolution[i * m_NumCols + j + 1].y;
				float t = m_CurrSolution[(i - 1) * m_NumCols + j].y;
				float b = m_CurrSolution[(i + 1) * m_NumCols + j].y;
				m_Normals[i * m_NumCols + j].x = -r + l;
				m_Normals[i * m_NumCols + j].y = 2.0f * m_SpatialStep;
				m_Normals[i * m_NumCols + j].z = b - t;

				DirectX::XMVECTOR n = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&m_Normals[i * m_NumCols + j]));
				DirectX::XMStoreFloat3(&m_Normals[i * m_NumCols + j], n);
				
				m_TangentX[i * m_NumCols + j] = DirectX::XMFLOAT3(2.0f * m_SpatialStep, r - l, 0.0f);
				DirectX::XMVECTOR T = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&m_TangentX[i * m_NumCols + j]));
				DirectX::XMStoreFloat3(&m_TangentX[i * m_NumCols + j], T);
			}
		});
	}
}

void Waves::Disturb(int i, int j, float magnitude)
{
	assert(i > 1 && i < m_NumRows - 2);
	assert(j > 1 && j < m_NumCols - 2);

	float halfMag = 0.5f * magnitude;

	m_CurrSolution[i * m_NumCols + j].y += magnitude;
	m_CurrSolution[i * m_NumCols + j + 1].y += halfMag;
	m_CurrSolution[i * m_NumCols + j - 1].y += halfMag;
	m_CurrSolution[(i + 1) * m_NumCols + j].y += halfMag;
	m_CurrSolution[(i - 1) * m_NumCols + j].y += halfMag;
}
