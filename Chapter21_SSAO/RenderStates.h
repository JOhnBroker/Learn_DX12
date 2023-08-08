#ifndef RENDERSTATES_H
#define RENDERSTATES_H

#include "d3dUtil.h"

class RenderStates 
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	static bool IsInit();
	static void InitAll();

public:
	static D3D12_RASTERIZER_DESC RSNormal;
	static D3D12_RASTERIZER_DESC RSWireframe;
	static D3D12_RASTERIZER_DESC RSNoCull;
	static D3D12_RASTERIZER_DESC RSShadow;

	static D3D12_STATIC_SAMPLER_DESC SSPointClamp;				// ������״̬���������Clampģʽ
	static D3D12_STATIC_SAMPLER_DESC SSPointWrap;				// ������״̬���������Wrapģʽ
	static D3D12_STATIC_SAMPLER_DESC SSLinearClamp;				// ������״̬���߹�����Clampģʽ
	static D3D12_STATIC_SAMPLER_DESC SSLinearWrap;				// ������״̬���߹�����Wrapģʽ
	static D3D12_STATIC_SAMPLER_DESC SSAnisotropicClamp8x;		// ������״̬��8���������Թ�����Clampģʽ
	static D3D12_STATIC_SAMPLER_DESC SSAnisotropicWrap8x;		// ������״̬��8���������Թ�����Wrapģʽ
	static D3D12_STATIC_SAMPLER_DESC SSShadowPCF;				// ������״̬����ȱȽ���Borderģʽ

	static D3D12_BLEND_DESC BSTransparent;		                // ���״̬��͸�����
	static D3D12_BLEND_DESC BSAlphaToCoverage;	                // ���״̬��Alpha-To-Coverage
	static D3D12_BLEND_DESC BSAdditive;			                // ���״̬���ӷ����
	static D3D12_BLEND_DESC BSAlphaWeightedAdditive;            // ���״̬����AlphaȨ�صļӷ����ģʽ

	static D3D12_DEPTH_STENCIL_DESC DSSEqual;					// ���/ģ��״̬��������������ֵ��ȵ�����
	static D3D12_DEPTH_STENCIL_DESC DSSLessEqual;				// ���/ģ��״̬�����ڴ�ͳ��ʽ��պл���
	static D3D12_DEPTH_STENCIL_DESC DSSGreaterEqual;			// ���/ģ��״̬�����ڷ���Z����
	static D3D12_DEPTH_STENCIL_DESC DSSNoDepthWrite;			// ���/ģ��״̬�������ԣ�����д�����ֵ
	static D3D12_DEPTH_STENCIL_DESC DSSNoDepthTest;				// ���/ģ��״̬���ر���Ȳ���
	static D3D12_DEPTH_STENCIL_DESC DSSWriteStencil;			// ���/ģ��״̬������Ȳ��ԣ�д��ģ��ֵ
	static D3D12_DEPTH_STENCIL_DESC DSSEqualStencil;			// ���/ģ��״̬������Z�����ģ��ֵ

private:
	 static bool bIsInit;
};



#endif // !STATICSAMPLER_H
