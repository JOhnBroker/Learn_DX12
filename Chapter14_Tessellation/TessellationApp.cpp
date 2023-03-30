#include "TessellationApp.h"

TessellationApp::TessellationApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

TessellationApp::TessellationApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

TessellationApp::~TessellationApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool TessellationApp::Initialize()
{
	bool bResult = false;

	if (!D3DApp::Initialize()) 
	{
		return bResult;
	}

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	if (!InitResource()) 
	{
		return bResult;
	}

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildQuadPatchGeometry();
	BuildSpherePatchGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	// execute initialization commands
	HR(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	switch (m_CurrMode)
	{
	case ShowMode::Basic:
		break;
	case ShowMode::Sphere:
		break;
	}

	bResult = true;

	return bResult;
}

bool TessellationApp::InitResource()
{
	LoadTexture("white", L"..\\Textures\\white1x1.dds");
	return true;
}

void TessellationApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);
}

void TessellationApp::Update(const GameTimer& timer)
{
	// ImGui
	ImGuiIO& io = ImGui::GetIO();
	
	const float dt = timer.DeltaTime();
	if (ImGui::Begin("Tessllation Demo"))
	{
		static int curr_mode_item = static_cast<int>(m_CurrMode);
		const char* mode_strs[] = {
			"Basic",
			"Sphere"
		};
		if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
		{
			if (curr_mode_item == 0)
			{
				m_CurrMode = ShowMode::Basic;
			}
			else if (curr_mode_item == 1) 
			{
				m_CurrMode = ShowMode::Sphere;
			}
		}
	}

	if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))
		m_SunTheta -= 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_RightArrow))
		m_SunTheta += 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_UpArrow))
		m_SunPhi -= 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_DownArrow))
		m_SunPhi += 1.0f * dt;

	ImGui::End();
	ImGui::Render();

	UpdateCamera(timer);
	m_CurrFrameResourceIndex = (m_CurrFrameResourceIndex + 1) % g_numFrameResources;
	m_CurrFrameResource = m_FrameResources[m_CurrFrameResourceIndex].get();

	// 等候GPU完成当前帧资源的命令
	if (m_CurrFrameResource->Fence != 0 && m_Fence->GetCompletedValue() < m_CurrFrameResource->Fence) 
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		HR(m_Fence->SetEventOnCompletion(m_CurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(timer);
	UpdateMaterialCBs(timer);
	UpdateMainPassCB(timer);

}

void TessellationApp::Draw(const GameTimer& timer)
{
	auto cmdListAlloc = m_CurrFrameResource->CmdListAlloc;

	HR(cmdListAlloc->Reset());

	HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["basic"].Get()));

	m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_CommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&m_MainPassCB.FogColor, 0, nullptr);
	m_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	auto passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

	// Todo
	switch (m_CurrMode)
	{
	case ShowMode::Basic:
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Basic]);
		break;
	case ShowMode::Sphere:
		m_CommandList->SetPipelineState(m_PSOs["sphere"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Sphere]);
		break;
	default:
		break;
	}	

	m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	HR(m_CommandList->Close());

	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	HR(m_pSwapChain->Present(0, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % SwapChainBufferCount;
	m_CurrFrameResource->Fence = ++m_CurrentFence;

	m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);
}

void TessellationApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		float dx = 0.05f * static_cast<float>(x - m_LastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - m_LastMousePos.y);

		m_Radius += dx - dy;
		m_Radius = MathHelper::Clamp(m_Radius, 5.0f, 150.0f);
	}
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void TessellationApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void TessellationApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void TessellationApp::UpdateCamera(const GameTimer& gt)
{
	m_EyePos.x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
	m_EyePos.z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
	m_EyePos.y = m_Radius * cosf(m_Phi);

	XMVECTOR pos = XMVectorSet(m_EyePos.x, m_EyePos.y, m_EyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_View, view);
}

void TessellationApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = m_CurrFrameResource->ObjectCB.get();

	for (auto& obj : m_AllRitems) 
	{
		if (obj->NumFramesDirty > 0) 
		{
			XMMATRIX world = XMLoadFloat4x4(&obj->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&obj->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(obj->ObjCBIndex, objConstants);

			obj->NumFramesDirty--;
		}
	}
}

void TessellationApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = m_CurrFrameResource->MaterialCB.get();

	for (auto& pair : m_Materials) 
	{
		Material* mat = pair.second.get();
		if (mat->m_NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->m_MatTransform);

			MaterialConstants matConstants;
			matConstants.m_DiffuseAlbedo = mat->m_DiffuseAlbedo;
			matConstants.m_FresnelR0 = mat->m_FresnelR0;
			matConstants.Roughness = mat->m_Roughness;
			XMStoreFloat4x4(&matConstants.m_MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->m_MatCBIndex, matConstants);

			mat->m_NumFramesDirty--;
		}
	}
}

void TessellationApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&m_View);
	XMMATRIX proj = XMLoadFloat4x4(&m_Proj);
	XMMATRIX viewproj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewproj), viewproj);

	XMStoreFloat4x4(&m_MainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_MainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_MainPassCB.ViewProj, XMMatrixTranspose(viewproj));
	XMStoreFloat4x4(&m_MainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_MainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_MainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	m_MainPassCB.EyePosW = m_EyePos;
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
	m_MainPassCB.NearZ = 1.0f;
	m_MainPassCB.FarZ = 1000.0f;
	m_MainPassCB.TotalTime = gt.TotalTime();
	m_MainPassCB.DeltaTime = gt.DeltaTime();
	m_MainPassCB.AmbientLight = { 0.25f,0.25f,0.35f,1.0f };

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, m_SunTheta, m_SunPhi);
	XMStoreFloat3(&m_MainPassCB.Lights[0].m_Direction, lightDir);
	m_MainPassCB.Lights[0].m_Strength = { 1.0f,1.0f,1.0f };

	auto currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void TessellationApp::BuildRootSignature()
{
	HRESULT hReturn = E_FAIL;
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(), 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hReturn = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	HR(hReturn);

	HR(m_pd3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())));
}

void TessellationApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};

	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto whiteTex = m_Textures["white"]->m_Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = whiteTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.PlaneSlice = 0;
	m_pd3dDevice->CreateShaderResourceView(whiteTex.Get(), &srvDesc, hDescriptor);
}

void TessellationApp::BuildShadersAndInputLayout()
{
	m_Shaders["basicVS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter14\\Tesselator.hlsl", nullptr, "Basic_VS", "vs_5_1");
	m_Shaders["basicHS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter14\\Tesselator.hlsl", nullptr, "Basic_HS", "hs_5_1");
	m_Shaders["basicDS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter14\\Tesselator.hlsl", nullptr, "Basic_DS", "ds_5_1");
	m_Shaders["basicPS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter14\\Tesselator.hlsl", nullptr, "Basic_PS", "ps_5_1");
	m_Shaders["sphereVS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter14\\Tesselator.hlsl", nullptr, "Sphere_VS", "vs_5_1");
	m_Shaders["sphereHS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter14\\Tesselator.hlsl", nullptr, "Sphere_HS", "hs_5_1");
	m_Shaders["sphereDS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter14\\Tesselator.hlsl", nullptr, "Sphere_DS", "ds_5_1");
	m_Shaders["spherePS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter14\\Tesselator.hlsl", nullptr, "Sphere_PS", "ps_5_1");

	m_InputLayouts["basic"] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
	m_InputLayouts["sphere"] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL"  ,0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD"  ,0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
}

void TessellationApp::BuildQuadPatchGeometry()
{
	std::array<XMFLOAT3, 4>vertices =
	{
		XMFLOAT3(-10.0f, 0.0f, +10.0f),
		XMFLOAT3(+10.0f, 0.0f, +10.0f),
		XMFLOAT3(-10.0f, 0.0f, -10.0f),
		XMFLOAT3(+10.0f, 0.0f, -10.0f)
	};
	std::array<std::uint16_t, 4> indices = { 0,1,2,3 };

	const UINT vbByteSize = vertices.size() * sizeof(XMFLOAT3);
	const UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "quadPatchGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(XMFLOAT3);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry quadSubmesh;
	quadSubmesh.BaseVertexLocation = 0;
	quadSubmesh.StartIndexLocation = 0;
	quadSubmesh.IndexCount = indices.size();
	geo->DrawArgs["quad"] = quadSubmesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void TessellationApp::BuildSpherePatchGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateGeosphere(1.0f, 0);

	std::vector<Vertex> vertices(sphere.Vertices.size());
	std::vector<std::uint16_t> indices = sphere.GetIndices16();

	for (size_t i = 0; i < sphere.Vertices.size(); ++i) 
	{
		vertices[i].Pos = sphere.Vertices[i].Position;
		vertices[i].Normal = sphere.Vertices[i].Normal;
		vertices[i].TexC = sphere.Vertices[i].TexC;
	}

	const UINT vbByteSize = vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "spherePatchGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(), vertices.data(),
		vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(), indices.data(),
		ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submeshGeo;
	submeshGeo.IndexCount = indices.size();
	submeshGeo.BaseVertexLocation = 0;
	submeshGeo.StartIndexLocation = 0;
	geo->DrawArgs["sphere"] = submeshGeo;

	m_Geometries[geo->Name] = std::move(geo);
}

void TessellationApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	opaquePsoDesc.InputLayout = { m_InputLayouts["basic"].data(),(UINT)m_InputLayouts["basic"].size()};
	opaquePsoDesc.pRootSignature = m_RootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["basicVS"]->GetBufferPointer()),
		m_Shaders["basicVS"]->GetBufferSize()
	};
	opaquePsoDesc.HS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["basicHS"]->GetBufferPointer()),
		m_Shaders["basicHS"]->GetBufferSize()
	};
	opaquePsoDesc.DS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["basicDS"]->GetBufferPointer()),
		m_Shaders["basicDS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["basicPS"]->GetBufferPointer()),
		m_Shaders["basicPS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = m_BackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = 1;
	opaquePsoDesc.SampleDesc.Quality = 0;
	opaquePsoDesc.DSVFormat = m_DepthStencilFormat;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["basic"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC spherePsoDesc = opaquePsoDesc;
	spherePsoDesc.InputLayout = { m_InputLayouts["sphere"].data(),(UINT)m_InputLayouts["sphere"].size() };
	spherePsoDesc.pRootSignature = m_RootSignature.Get();
	spherePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["sphereVS"]->GetBufferPointer()),
		m_Shaders["sphereVS"]->GetBufferSize()
	};
	spherePsoDesc.HS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["sphereHS"]->GetBufferPointer()),
		m_Shaders["sphereHS"]->GetBufferSize()
	};
	spherePsoDesc.DS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["sphereDS"]->GetBufferPointer()),
		m_Shaders["sphereDS"]->GetBufferSize()
	};
	spherePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["spherePS"]->GetBufferPointer()),
		m_Shaders["spherePS"]->GetBufferSize()
	};
	spherePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&spherePsoDesc, IID_PPV_ARGS(&m_PSOs["sphere"])));
}

void TessellationApp::BuildFrameResources()
{
	for (int i = 0; i < g_numFrameResources; ++i) 
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get()
			, 1, m_AllRitems.size(), m_Materials.size()));
	}
}

void TessellationApp::BuildMaterials()
{
	auto white = std::make_unique<Material>();
	white->m_Name = "white";
	white->m_MatCBIndex = 0;
	white->m_DiffuseSrvHeapIndex = 0;
	white->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	white->m_FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	white->m_Roughness = 0.5f;

	m_Materials[white->m_Name] = std::move(white);

}

void TessellationApp::BuildRenderItems()
{
	auto quadRitem = std::make_unique<RenderItem>();
	quadRitem->World = MathHelper::Identity4x4();
	quadRitem->TexTransform = MathHelper::Identity4x4();
	quadRitem->ObjCBIndex = 0;
	quadRitem->Mat = m_Materials["white"].get();
	quadRitem->Geo = m_Geometries["quadPatchGeo"].get();
	quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
	quadRitem->IndexCount = quadRitem->Geo->DrawArgs["quad"].IndexCount;
	quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["quad"].StartIndexLocation;
	quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["quad"].BaseVertexLocation;
	
	m_RitemLayer[(int)RenderLayer::Basic].push_back(quadRitem.get());

	auto sphereRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&sphereRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f));
	sphereRitem->TexTransform = MathHelper::Identity4x4();
	sphereRitem->ObjCBIndex = 1;
	sphereRitem->Mat = m_Materials["white"].get();
	sphereRitem->Geo = m_Geometries["spherePatchGeo"].get();
	sphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	sphereRitem->IndexCount = sphereRitem->Geo->DrawArgs["sphere"].IndexCount;
	sphereRitem->StartIndexLocation = sphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	sphereRitem->BaseVertexLocation = sphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Sphere].push_back(sphereRitem.get());

	m_AllRitems.push_back(std::move(quadRitem));
	m_AllRitems.push_back(std::move(sphereRitem));
}

void TessellationApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = m_CurrFrameResource->ObjectCB->Resource();
	auto matCB = m_CurrFrameResource->MaterialCB->Resource();

	for (size_t i = 0; i < ritems.size(); ++i) 
	{
		auto obj = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &obj->Geo->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&obj->Geo->GetIndexBufferView());
		cmdList->IASetPrimitiveTopology(obj->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(obj->Mat->m_DiffuseSrvHeapIndex, m_CBVSRVDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + obj->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + obj->Mat->m_MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(2, matCBAddress);

		cmdList->DrawIndexedInstanced(obj->IndexCount, 1, obj->StartIndexLocation, obj->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT> TessellationApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8);

	return { pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap ,anisotropicClamp  };
}

void TessellationApp::LoadTexture(std::string name, std::wstring filename)
{
	auto texture = std::make_unique<Texture>();

	texture->m_Name = name;
	texture->m_Filename = filename;
	HR(DirectX::CreateDDSTextureFromFile12(m_pd3dDevice.Get(), m_CommandList.Get(),
		texture->m_Filename.c_str(), texture->m_Resource, texture->m_UploadHeap));
	m_Textures[name] = std::move(texture);

}
