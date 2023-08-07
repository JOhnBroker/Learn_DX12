#ifndef PIPELINESTATEOBJECT_H
#define PIPELINESTATEOBJECT_H

#include "d3dUtil.h"
#include "Shader.h"

//-------------------------------------------------------------------------//
// GraphicsPSO
//-------------------------------------------------------------------------//

struct GraphicsPSODescriptor
{
public:
	Shader* shader = nullptr;
	D3D12_BLEND_DESC BlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_RASTERIZER_DESC RasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	D3D12_INPUT_LAYOUT_DESC InputLayout;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	UINT NumRenderTarget = 1;
	DXGI_FORMAT RTVFormat[8] = { DXGI_FORMAT_R8G8B8A8_UNORM };
	DXGI_FORMAT DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	bool MsaaState = false;
	UINT MsaaQuality = 0;

};

class GraphicsPSOManager
{
public:

	GraphicsPSOManager(ID3D12Device* device);
	~GraphicsPSOManager();
	GraphicsPSOManager(const GraphicsPSOManager&) = delete;
	GraphicsPSOManager& operator=(const GraphicsPSOManager&) = delete;
	GraphicsPSOManager(GraphicsPSOManager&&) = default;
	GraphicsPSOManager& operator=(GraphicsPSOManager&&) = default;

	HRESULT AddShader(std::string name, ID3DBlob* blob);

	HRESULT AddPass(std::string& passName, const GraphicsPSODescriptor* pDesc);

	



	


	//set resource
	//bind param

private:


};


#endif // !PIPELINESTATEOBJECT_H