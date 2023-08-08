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

	static D3D12_STATIC_SAMPLER_DESC SSPointClamp;				// 采样器状态：点过滤与Clamp模式
	static D3D12_STATIC_SAMPLER_DESC SSPointWrap;				// 采样器状态：点过滤与Wrap模式
	static D3D12_STATIC_SAMPLER_DESC SSLinearClamp;				// 采样器状态：线过滤与Clamp模式
	static D3D12_STATIC_SAMPLER_DESC SSLinearWrap;				// 采样器状态：线过滤与Wrap模式
	static D3D12_STATIC_SAMPLER_DESC SSAnisotropicClamp8x;		// 采样器状态：8倍各向异性过滤与Clamp模式
	static D3D12_STATIC_SAMPLER_DESC SSAnisotropicWrap8x;		// 采样器状态：8倍各向异性过滤与Wrap模式
	static D3D12_STATIC_SAMPLER_DESC SSShadowPCF;				// 采样器状态：深度比较与Border模式

	static D3D12_BLEND_DESC BSTransparent;		                // 混合状态：透明混合
	static D3D12_BLEND_DESC BSAlphaToCoverage;	                // 混合状态：Alpha-To-Coverage
	static D3D12_BLEND_DESC BSAdditive;			                // 混合状态：加法混合
	static D3D12_BLEND_DESC BSAlphaWeightedAdditive;            // 混合状态：带Alpha权重的加法混合模式

	static D3D12_DEPTH_STENCIL_DESC DSSEqual;					// 深度/模板状态：仅允许绘制深度值相等的像素
	static D3D12_DEPTH_STENCIL_DESC DSSLessEqual;				// 深度/模板状态：用于传统方式天空盒绘制
	static D3D12_DEPTH_STENCIL_DESC DSSGreaterEqual;			// 深度/模板状态：用于反向Z绘制
	static D3D12_DEPTH_STENCIL_DESC DSSNoDepthWrite;			// 深度/模板状态：仅测试，但不写入深度值
	static D3D12_DEPTH_STENCIL_DESC DSSNoDepthTest;				// 深度/模板状态：关闭深度测试
	static D3D12_DEPTH_STENCIL_DESC DSSWriteStencil;			// 深度/模板状态：无深度测试，写入模板值
	static D3D12_DEPTH_STENCIL_DESC DSSEqualStencil;			// 深度/模板状态：反向Z，检测模板值

private:
	 static bool bIsInit;
};



#endif // !STATICSAMPLER_H
