#include "RenderStates.h"

D3D12_RASTERIZER_DESC RenderStates::RSNormal = {};
D3D12_RASTERIZER_DESC RenderStates::RSWireframe = {};
D3D12_RASTERIZER_DESC RenderStates::RSNoCull = {};
D3D12_RASTERIZER_DESC RenderStates::RSShadow = {};

D3D12_STATIC_SAMPLER_DESC RenderStates::SSPointClamp = {};		
D3D12_STATIC_SAMPLER_DESC RenderStates::SSPointWrap = {};		
D3D12_STATIC_SAMPLER_DESC RenderStates::SSLinearClamp = {};		
D3D12_STATIC_SAMPLER_DESC RenderStates::SSLinearWrap = {};		
D3D12_STATIC_SAMPLER_DESC RenderStates::SSAnisotropicClamp8x = {};
D3D12_STATIC_SAMPLER_DESC RenderStates::SSAnisotropicWrap8x = {};
D3D12_STATIC_SAMPLER_DESC RenderStates::SSShadowPCF = {};		

D3D12_BLEND_DESC RenderStates::BSTransparent = {};		             
D3D12_BLEND_DESC RenderStates::BSAlphaToCoverage = {};	             
D3D12_BLEND_DESC RenderStates::BSAdditive = {};			         
D3D12_BLEND_DESC RenderStates::BSAlphaWeightedAdditive = {};        

D3D12_DEPTH_STENCIL_DESC RenderStates::DSSEqual = {};				
D3D12_DEPTH_STENCIL_DESC RenderStates::DSSLessEqual = {};			
D3D12_DEPTH_STENCIL_DESC RenderStates::DSSGreaterEqual = {};		
D3D12_DEPTH_STENCIL_DESC RenderStates::DSSNoDepthWrite = {};		
D3D12_DEPTH_STENCIL_DESC RenderStates::DSSNoDepthTest = {};		
D3D12_DEPTH_STENCIL_DESC RenderStates::DSSWriteStencil = {};		
D3D12_DEPTH_STENCIL_DESC RenderStates::DSSEqualStencil = {};		

bool RenderStates::bIsInit = false;

bool RenderStates::IsInit()
{
	return bIsInit;
}

void RenderStates::InitAll()
{
	if (IsInit)
		return;

	bIsInit = true;

	RSNormal = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RSWireframe = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RSWireframe.FillMode = D3D12_FILL_MODE_WIREFRAME;

	RSNoCull = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RSNoCull.CullMode = D3D12_CULL_MODE_NONE;

	RSShadow = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RSShadow.DepthBias = 100000;
	RSShadow.DepthBiasClamp = 0.0f;
	RSShadow.SlopeScaledDepthBias = 1.0f;

	SSPointClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	SSPointClamp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SSPointClamp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SSPointClamp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SSPointClamp.MipLODBias = 0.0f;
	SSPointClamp.MaxAnisotropy = 16;
	SSPointClamp.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	SSPointClamp.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	SSPointClamp.MinLOD = 0.0f;
	SSPointClamp.MaxLOD = D3D12_FLOAT32_MAX;
	SSPointClamp.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	// register and space set by shader reflect

	SSPointWrap = SSPointClamp;
	SSPointWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SSPointWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SSPointWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	SSLinearClamp = SSPointClamp;
	SSLinearClamp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	SSLinearWrap = SSPointWrap;
	SSLinearWrap.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	SSAnisotropicClamp8x = SSPointClamp;
	SSAnisotropicClamp8x.Filter = D3D12_FILTER_ANISOTROPIC;
	SSAnisotropicClamp8x.MaxAnisotropy = 8;

	SSAnisotropicWrap8x = SSPointWrap;
	SSAnisotropicWrap8x.Filter = D3D12_FILTER_ANISOTROPIC;
	SSAnisotropicWrap8x.MaxAnisotropy = 8;

	SSShadowPCF = SSPointClamp;
	SSShadowPCF.Filter = D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	SSShadowPCF.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SSShadowPCF.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SSShadowPCF.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SSShadowPCF.MaxAnisotropy = 16;
	SSShadowPCF.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	// Alpha-To-Coverage模式
	BSAlphaToCoverage = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	BSAlphaToCoverage.AlphaToCoverageEnable = TRUE;

	// Color = SrcAlpha * SrcColor + (1 - SrcAlpha) * DestColor 
	// Alpha = SrcAlpha
	BSTransparent = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	BSTransparent.RenderTarget[0].BlendEnable = TRUE;
	BSTransparent.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	BSTransparent.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	BSTransparent.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	BSTransparent.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	BSTransparent.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	BSTransparent.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	// Color = SrcColor + DestColor
	// Alpha = SrcAlpha + DestAlpha
	BSAdditive = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	BSAdditive.RenderTarget[0].BlendEnable = TRUE;
	BSAdditive.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	BSAdditive.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	BSAdditive.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	BSAdditive.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	BSAdditive.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	BSAdditive.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	// Color = SrcAlpha * SrcColor + DestColor
	// Alpha = SrcAlpha
	BSAlphaWeightedAdditive = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	BSAlphaWeightedAdditive.RenderTarget[0].BlendEnable = TRUE;
	BSAlphaWeightedAdditive.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	BSAlphaWeightedAdditive.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	BSAlphaWeightedAdditive.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	BSAlphaWeightedAdditive.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	BSAlphaWeightedAdditive.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	BSAlphaWeightedAdditive.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	// 仅允许深度值一致的像素进行写入的深度/模板状态
	// 没必要写入深度
	DSSEqual = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	DSSEqual.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	DSSEqual.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

	DSSLessEqual = DSSEqual;
	DSSLessEqual.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	DSSLessEqual.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	DSSGreaterEqual = DSSLessEqual;
	DSSGreaterEqual.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	// 进行深度测试，但不写入深度值的状态
	// 若绘制非透明物体时，应使用默认状态
	// 绘制透明物体时，使用该状态可以有效确保混合状态的进行
	// 并且确保较前的非透明物体可以阻挡较后的一切物体
	DSSNoDepthWrite = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	DSSNoDepthWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	DSSNoDepthWrite.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// 关闭深度测试的深度/模板状态
	// 若绘制非透明物体，务必严格按照绘制顺序
	// 绘制透明物体则不需要担心绘制顺序
	// 而默认情况下模板测试就是关闭的
	DSSNoDepthTest = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	DSSNoDepthTest.DepthEnable = FALSE;

	// 反向Z深度测试，模板值比较
	DSSWriteStencil = DSSGreaterEqual;
	DSSWriteStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	DSSWriteStencil.StencilEnable = TRUE;
	DSSWriteStencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	DSSWriteStencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	// 无深度测试，仅模板写入
	DSSEqualStencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	DSSEqualStencil.DepthEnable = FALSE;
	DSSEqualStencil.StencilEnable = TRUE;
	DSSEqualStencil.StencilReadMask = 0xff;
	DSSEqualStencil.StencilWriteMask = 0xff;
	DSSEqualStencil.FrontFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
	DSSEqualStencil.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
	DSSEqualStencil.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	DSSEqualStencil.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	DSSEqualStencil.BackFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
	DSSEqualStencil.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
	DSSEqualStencil.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	DSSEqualStencil.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

}
