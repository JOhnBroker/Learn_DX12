#include "BoxApp.h"

BoxApp::BoxApp(HINSTANCE hInstance)
	:D3DApp(hInstance)
{
}

BoxApp::BoxApp(HINSTANCE hInstance, int witdh, int height)
	: D3DApp(hInstance, witdh, height)
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
	if(!D3DApp::Initialize())
		return false;

	if (!InitResource())
		return false;

	return true;
}

bool BoxApp::InitResource()
{
	bool bResult = false;

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShaderAndInputLayout();
	BuildBoxGeometry();
	BuildPSO();

	HR(m_CommandList->Close());

	ID3D12CommandList* cmdLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	bResult = true;
	return bResult;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);
}

void BoxApp::Update(const GameTimer& timer)
{
	static bool animateCube = true;
	static float phi = 0.0f, theta = 0.0f;
	phi += 0.0001f, theta += 0.00015f;

	// ImGui
	if (ImGui::Begin("Box demo")) 
	{
		static int curr_mode_item = static_cast<int>(m_CurrMode);
		const char* mode_strs[] = {
			"Box",
			"Pyramid"
		};
		if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs))) 
		{
			if (curr_mode_item == 0) 
			{
				m_CurrMode = ShowMode::Box;
			}
			else 
			{
				m_CurrMode = ShowMode::Pyramid;
			}
		}
		
		ImGui::Checkbox("Animate", &animateCube);
		ImGui::Checkbox("Line Mode", &m_LineMode);
		ImGui::Checkbox("Front Cull Mode", &m_FrontMode);

	}
	ImGui::End();
	ImGui::Render();

	float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
	float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
	float y = m_Radius * cosf(m_Phi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_View, view);

	XMMATRIX world;
	if (animateCube) 
	{
		world = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
	}
	else
	{
		world = XMLoadFloat4x4(&m_World);
	}
	XMMATRIX proj = XMLoadFloat4x4(&m_Proj);
	XMMATRIX worldViewProj = world * view * proj;

	// 
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	m_ObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& timer)
{
	// Reuse memory
	HR(m_DirectCmdListAlloc->Reset());

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), m_PSO.Get()));

	D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferView = CurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE currentDSBufferView = DepthStencilView();

	m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	CD3DX12_RESOURCE_BARRIER present2render = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_CommandList->ResourceBarrier(1, &present2render);

	m_CommandList->ClearRenderTargetView(currentBackBufferView, Colors::LightSteelBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(currentDSBufferView,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_CommandList->OMSetRenderTargets(1, &currentBackBufferView, true, &currentDSBufferView);

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBVHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	if (m_CurrMode == ShowMode::Box) 
	{
		vertexBufferView = m_BoxGeo->GetVertexBufferView();
		indexBufferView = m_BoxGeo->GetIndexBufferView();
	}
	else
	{
		vertexBufferView = m_PyramidGeo->GetVertexBufferView();
		indexBufferView = m_PyramidGeo->GetIndexBufferView();
	}
	m_CommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	m_CommandList->IASetIndexBuffer(&indexBufferView);
	m_CommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_CommandList->SetGraphicsRootDescriptorTable(0, m_CBVHeap->GetGPUDescriptorHandleForHeapStart());
	if (m_CurrMode == ShowMode::Box)
	{
		m_CommandList->DrawIndexedInstanced(m_BoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);
	}
	else
	{
		m_CommandList->DrawIndexedInstanced(m_PyramidGeo->DrawArgs["pyramid"].IndexCount, 1, 0, 0, 0);
	}
	CD3DX12_RESOURCE_BARRIER render2present = CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_CommandList->ResourceBarrier(1, &render2present);

	// 设置ImGui的SRV描述符堆
	m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
	// ImGui在Direct3D的绘制
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

	HR(m_CommandList->Close());

	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 交换buufer
	HR(m_pSwapChain->Present(0, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_LastMousePos.y));

		m_Theta += dx;
		m_Phi += dy;
		m_Phi = MathHelper::Clamp(m_Phi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = 0.005f * static_cast<float>(x - m_LastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - m_LastMousePos.y);

		m_Radius += dx - dy;
		m_Radius = MathHelper::Clamp(m_Radius, 3.0f, 15.0f);
	}
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void BoxApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	HR(m_pd3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CBVHeap)));
}

void BoxApp::BuildConstantBuffers()
{
	m_ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(m_pd3dDevice.Get(), 1, true);
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_ObjectCB->Resource()->GetGPUVirtualAddress();
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = objCBByteSize;

	m_pd3dDevice->CreateConstantBufferView(&cbvDesc, m_CBVHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT result = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
	if (errorBlob != nullptr) 
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	HR(result);

	HR(m_pd3dDevice->CreateRootSignature(0, 
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)));
}

void BoxApp::BuildShaderAndInputLayout()
{
	HRESULT hr = S_OK;

	m_VSByteCode = d3dUtil::CompileShader(L"..\\Shader\\Chapter6\\color.hlsl", nullptr, "VS", "vs_5_0");
	m_PSByteCode = d3dUtil::CompileShader(L"..\\Shader\\Chapter6\\color.hlsl", nullptr, "PS", "ps_5_0");

	m_InputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
}

void BoxApp::BuildBoxGeometry()
{
	// Pyramid demo
	std::array<Vertex, 8> pyramidGeoVertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)	}),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green)	}),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(+0.0f, +1.0f, +0.0f), XMFLOAT4(Colors::Red  ) })
	};
	
	std::array<std::uint16_t, 18> pyramidGeoIndices =
	{
		// bottom face
		0, 3, 1,
		1, 3, 2,

		// 
		0, 4, 3,
		3, 4, 2,
		2, 4, 1,
		1, 4, 0,
	};

	const UINT pvbByteSize = (UINT)pyramidGeoVertices.size() * sizeof(Vertex);
	const UINT pibByteSize = (UINT)pyramidGeoIndices.size() * sizeof(std::uint16_t);

	m_PyramidGeo = std::make_unique<MeshGeometry>();
	m_PyramidGeo->Name = "pyramidGeo";

	HR(D3DCreateBlob(pvbByteSize, &m_PyramidGeo->VertexBufferCPU));
	CopyMemory(m_PyramidGeo->VertexBufferCPU->GetBufferPointer(), pyramidGeoVertices.data(), pvbByteSize);

	HR(D3DCreateBlob(pibByteSize, &m_PyramidGeo->IndexBufferCPU));
	CopyMemory(m_PyramidGeo->IndexBufferCPU->GetBufferPointer(), pyramidGeoIndices.data(), pibByteSize);

	m_PyramidGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
		m_CommandList.Get(), pyramidGeoVertices.data(), pvbByteSize, m_PyramidGeo->VertexBufferUploader);

	m_PyramidGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
		m_CommandList.Get(), pyramidGeoIndices.data(), pibByteSize, m_PyramidGeo->IndexBufferUploader);
	m_PyramidGeo->VertexByteStride = sizeof(Vertex);
	m_PyramidGeo->VertexBufferByteSize = pvbByteSize;
	m_PyramidGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_PyramidGeo->IndexBufferByteSize = pibByteSize;

	SubmeshGeometry pysubmesh;
	pysubmesh.IndexCount = (UINT)pyramidGeoIndices.size();
	pysubmesh.StartIndexLocation = 0;
	pysubmesh.BaseVertexLocation = 0;
	m_PyramidGeo->DrawArgs["pyramid"] = pysubmesh;

	//Box demo
	std::array<Vertex, 8> boxVertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Red)	}),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Blue)	}),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)	}),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Red)	}),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)	}),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> boxIndices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	const UINT bvbByteSize = (UINT)boxVertices.size() * sizeof(Vertex);
	const UINT bibByteSize = (UINT)boxIndices.size() * sizeof(std::uint16_t);

	m_BoxGeo = std::make_unique<MeshGeometry>();
	m_BoxGeo->Name = "boxGeo";

	HR(D3DCreateBlob(bvbByteSize, &m_BoxGeo->VertexBufferCPU));
	CopyMemory(m_BoxGeo->VertexBufferCPU->GetBufferPointer(), boxVertices.data(), bvbByteSize);

	HR(D3DCreateBlob(bibByteSize, &m_BoxGeo->IndexBufferCPU));
	CopyMemory(m_BoxGeo->IndexBufferCPU->GetBufferPointer(), boxIndices.data(), bibByteSize);

	m_BoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
		m_CommandList.Get(), boxVertices.data(), bvbByteSize, m_BoxGeo->VertexBufferUploader);

	m_BoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
		m_CommandList.Get(), boxIndices.data(), bibByteSize, m_BoxGeo->IndexBufferUploader);
	m_BoxGeo->VertexByteStride = sizeof(Vertex);
	m_BoxGeo->VertexBufferByteSize = bvbByteSize;
	m_BoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_BoxGeo->IndexBufferByteSize = bibByteSize;

	SubmeshGeometry boxsubmesh;
	boxsubmesh.IndexCount = (UINT)boxIndices.size();
	boxsubmesh.StartIndexLocation = 0;
	boxsubmesh.BaseVertexLocation = 0;
	m_BoxGeo->DrawArgs["box"] = boxsubmesh;

}

void BoxApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { m_InputLayout.data(),(UINT)m_InputLayout.size() };
	psoDesc.pRootSignature = m_RootSignature.Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_VSByteCode->GetBufferPointer()),
		m_VSByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_PSByteCode->GetBufferPointer()),
		m_PSByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//// 线框模式
	//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//// 前向裁剪
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_BackBufferFormat;
	psoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = m_DepthStencilFormat;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PSO)));

}
