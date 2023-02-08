#ifndef WAVES
#define WAVES

#include <vector>
#include <DirectXMath.h>

class Waves
{
public:
	Waves(int m, int n, float dx, float dt, float speed, float damping);
	Waves(const Waves& rhs) = delete;
	Waves& operator=(const Waves& rhs) = delete;
	~Waves();

	int GetRowCount()const { return m_NumRows; };
	int GetColCount()const { return m_NumCols; };
	int GetVertexCount()const { return m_VertexCount; };
	int GetTriangleCount()const { return m_TriangleCount; };
	float GetWidth() const { return m_NumCols * m_SpatialStep; };
	float GetHeight() const { return m_NumRows * m_SpatialStep; };

	const DirectX::XMFLOAT3& Position(int i)const { return m_CurrSolution[i]; };
	const DirectX::XMFLOAT3& Normal(int i)const { return m_Normals[i]; };
	const DirectX::XMFLOAT3& TangentX(int i)const { return m_TangentX[i]; };

	void Update(float dt);
	void Disturb(int i, int j, float magnitude);

private:
	int m_NumRows = 0;
	int m_NumCols = 0;

	int m_VertexCount = 0;
	int m_TriangleCount = 0;

	float m_K1 = 0.0f;
	float m_K2 = 0.0f;
	float m_K3 = 0.0f;

	float m_TimeStep = 0.0f;
	float m_SpatialStep = 0.0f;

	std::vector<DirectX::XMFLOAT3> m_PrevSolution;
	std::vector<DirectX::XMFLOAT3> m_CurrSolution;
	std::vector<DirectX::XMFLOAT3> m_Normals;
	std::vector<DirectX::XMFLOAT3> m_TangentX;
};

#endif
