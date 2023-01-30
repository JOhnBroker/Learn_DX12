#include "ShapesApp.h"

ShapesApp::ShapesApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

ShapesApp::ShapesApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

ShapesApp::~ShapesApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool ShapesApp::Initialize()
{
	if (!D3DApp::Initialize()) 
	{
		return false;
	}

	if (!InitResource()) 
	{
		return false;
	}

	return true;
}

bool ShapesApp::InitResource()
{
	bool bResult = false;

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildShaderAndInputLayout();
	BuildShapeGeometry();
	BuildSkullGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBuffersViews();
	BuildPSOs();

	// execute initialization commands
	HR(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	bResult = true;

	return bResult;
}

void ShapesApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);
}

void ShapesApp::Update(const GameTimer& timer)
{
	// ImGui
	if (ImGui::Begin("ShapesDemo"))
	{
		// TODO: Add options for selecting models. 
		/*static int curr_mode_item = static_cast<int>(m_CurrMode);
		const char* mode_strs[] = {
			"Shapes",
			"Skull"
		};
		if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
		{
			if (curr_mode_item == 0 && m_CurrMode!= ShowMode::Shapes)
			{
				m_CurrMode = ShowMode::Shapes;
			}
			else if(curr_mode_item == 0 && m_CurrMode != ShowMode::Skull)
			{
				m_CurrMode = ShowMode::Skull;
			}
		}*/
		ImGui::Checkbox("Wireframe", &m_IsWireframe);
	}
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
	UpdateMainPassCB(timer);
}

void ShapesApp::Draw(const GameTimer& timer)
{
	auto cmdListAlloc = m_CurrFrameResource->CmdListAlloc;
	D3D12_RESOURCE_BARRIER present2render;
	D3D12_RESOURCE_BARRIER render2present;
	D3D12_CPU_DESCRIPTOR_HANDLE backbufferView;
	D3D12_CPU_DESCRIPTOR_HANDLE depthbufferView;

	HR(cmdListAlloc->Reset());

	if (m_IsWireframe) 
	{
		HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque_wireframe"].Get()));
	}
	else 
	{
		HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));
	}

	m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	present2render = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_CommandList->ResourceBarrier(1, &present2render);

	backbufferView = CurrentBackBufferView();
	depthbufferView = DepthStencilView();
	m_CommandList->ClearRenderTargetView(backbufferView, DirectX::Colors::LightSteelBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(depthbufferView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_CommandList->OMSetRenderTargets(1, &backbufferView, true, &depthbufferView);

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBVHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	int passCbvIndex = m_PassCbvOffset + m_CurrFrameResourceIndex;
	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CBVHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, m_CBVSRVDescriptorSize);
	m_CommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	DrawRenderItems(m_CommandList.Get(), m_OpaqueRitems);

	m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

	render2present = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_CommandList->ResourceBarrier(1, &render2present);

	HR(m_CommandList->Close());
	
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	HR(m_pSwapChain->Present(0, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % SwapChainBufferCount;
	m_CurrFrameResource->Fence = ++m_CurrentFence;

	m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);

}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void ShapesApp::UpdateCamera(const GameTimer& gt)
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

void ShapesApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = m_CurrFrameResource->ObjectCB.get();

	for (auto& obj : m_AllRitems) 
	{
		// 变化时，更新常量缓冲区的数据
		if (obj->NumFramesDirty > 0) 
		{
			XMMATRIX world = XMLoadFloat4x4(&obj->World);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

			currObjectCB->CopyData(obj->ObjCBIndex, objConstants);

			obj->NumFramesDirty--;
		}
	}
}

void ShapesApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&m_View);
	XMMATRIX proj = XMLoadFloat4x4(&m_Proj);
	XMMATRIX viewproj = XMMatrixMultiply(view, proj);
	XMVECTOR dView = XMMatrixDeterminant(view);
	XMVECTOR dProj = XMMatrixDeterminant(proj);
	XMVECTOR dViewProj = XMMatrixDeterminant(viewproj);
	XMMATRIX invView = XMMatrixInverse(&dView, view);
	XMMATRIX invProj = XMMatrixInverse(&dProj, proj);
	XMMATRIX invViewproj = XMMatrixInverse(&dViewProj, viewproj);

	XMStoreFloat4x4(&m_MainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_MainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_MainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_MainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_MainPassCB.ViewProj, XMMatrixTranspose(viewproj));
	XMStoreFloat4x4(&m_MainPassCB.InvViewProj, XMMatrixTranspose(invViewproj));
	m_MainPassCB.EyePosW = m_EyePos;
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
	m_MainPassCB.NearZ = 1.0f;
	m_MainPassCB.FarZ = 1000.0f;
	m_MainPassCB.TotalTime = gt.TotalTime();
	m_MainPassCB.DeltaTime = gt.DeltaTime();

	auto currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void ShapesApp::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)m_OpaqueRitems.size();

	// objcCunt + passCount
	UINT numDescriptors = (objCount + 1) * g_numFrameResources;

	m_PassCbvOffset = objCount * g_numFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	HR(m_pd3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CBVHeap)));
}

void ShapesApp::BuildConstantBuffersViews()
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	UINT objCount = (UINT)m_OpaqueRitems.size();

	// creat a CBV descriptor for frameResource
	for (int frameIndex = 0; frameIndex < g_numFrameResources; ++frameIndex) 
	{
		auto objectCB = m_FrameResources[frameIndex]->ObjectCB->Resource();
		for (UINT i = 0; i < objCount; ++i) 
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

			// 偏移到第i个物体的常量换冲区
			cbAddress += i * objCBByteSize;

			// 偏移到物体在描述符堆的CBV
			int heapIndex = frameIndex * objCount + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CBVHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, m_CBVSRVDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = objCBByteSize;

			m_pd3dDevice->CreateConstantBufferView(&cbvDesc, handle);

		}
	}

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Pass常量缓冲区
	for (int frameIndex = 0; frameIndex < g_numFrameResources; ++frameIndex) 
	{
		auto passCB = m_FrameResources[frameIndex]->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		int heapIndex = m_PassCbvOffset + frameIndex;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CBVHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, m_CBVSRVDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = passCBByteSize;

		m_pd3dDevice->CreateConstantBufferView(&cbvDesc, handle);

	}
}

void ShapesApp::BuildRootSignature()
{
	HRESULT hReturn = E_FAIL;
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	// 创建根描述符表
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	// 一个根签名由一组根参数组成
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 序列化处理
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

void ShapesApp::BuildShaderAndInputLayout()
{
	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter7\\color.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter7\\color.hlsl", nullptr, "PS", "ps_5_1");

	m_InputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
}

void ShapesApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateGeosphere(0.5f, 2);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	// define SubmeshGeometry
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	// pack vertices into one vertex buffer

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;

	UINT index = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++index) 
	{
		vertices[index].Pos = box.Vertices[i].Position;
		vertices[index].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
	}
	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++index)
	{
		vertices[index].Pos = grid.Vertices[i].Position;
		vertices[index].Color = XMFLOAT4(DirectX::Colors::ForestGreen);
	}
	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++index)
	{
		vertices[index].Pos = sphere.Vertices[i].Position;
		vertices[index].Color = XMFLOAT4(DirectX::Colors::Crimson);
	}
	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++index)
	{
		vertices[index].Pos = cylinder.Vertices[i].Position;
		vertices[index].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
	}

	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildSkullGeometry()
{
	std::vector<Vertex> vertices(1);
	std::vector<std::uint16_t> indices;

	// read from file 
	ReadDataFromFile(vertices, indices);

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skullGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["skull"] = submesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { m_InputLayout.data(),(UINT)m_InputLayout.size() };
	opaquePsoDesc.pRootSignature = m_RootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["standardVS"]->GetBufferPointer()),
		m_Shaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["opaquePS"]->GetBufferPointer()),
		m_Shaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = m_BackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = m_DepthStencilFormat;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque"])));

	// wireframe object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&m_PSOs["opaque_wireframe"])));

}

void ShapesApp::BuildFrameResources()
{
	for(int i = 0;i<g_numFrameResources;++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get(),
			1, (UINT)m_AllRitems.size()));
	}
}

void ShapesApp::BuildRenderItems()
{
	if (m_CurrMode == ShowMode::Shapes) 
	{
		auto boxRitem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
		boxRitem->ObjCBIndex = 0;
		boxRitem->Geo = m_Geometries["shapeGeo"].get();
		boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
		m_AllRitems.push_back(std::move(boxRitem));

		auto gridRitem = std::make_unique<RenderItem>();
		gridRitem->World = MathHelper::Identity4x4();
		gridRitem->ObjCBIndex = 1;
		gridRitem->Geo = m_Geometries["shapeGeo"].get();
		gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
		gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
		gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
		m_AllRitems.push_back(std::move(gridRitem));

		UINT objCBIndex = 2;
		for (int i = 0; i < 5; ++i) 
		{
			auto leftCylRitem = std::make_unique<RenderItem>();
			auto rightCylRitem = std::make_unique<RenderItem>();
			auto leftSphereRitem = std::make_unique<RenderItem>();
			auto rightSphereRitem = std::make_unique<RenderItem>();

			XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
			XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
			XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
			XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

			XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
			leftCylRitem->ObjCBIndex = objCBIndex++;
			leftCylRitem->Geo = m_Geometries["shapeGeo"].get();
			leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
			leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
			leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

			XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
			rightCylRitem->ObjCBIndex = objCBIndex++;
			rightCylRitem->Geo = m_Geometries["shapeGeo"].get();
			rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
			rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
			rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

			XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
			leftSphereRitem->ObjCBIndex = objCBIndex++;
			leftSphereRitem->Geo = m_Geometries["shapeGeo"].get();
			leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
			leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
			leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

			XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
			rightSphereRitem->ObjCBIndex = objCBIndex++;
			rightSphereRitem->Geo = m_Geometries["shapeGeo"].get();
			rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
			rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
			rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

			m_AllRitems.push_back(std::move(leftCylRitem));
			m_AllRitems.push_back(std::move(rightCylRitem));
			m_AllRitems.push_back(std::move(leftSphereRitem));
			m_AllRitems.push_back(std::move(rightSphereRitem));
		}
	}
	else 
	{
		auto skullRitem = std::make_unique<RenderItem>();
		skullRitem->World = MathHelper::Identity4x4();
		skullRitem->ObjCBIndex = 0;
		skullRitem->Geo = m_Geometries["skullGeo"].get();
		skullRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
		skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
		skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;

		m_AllRitems.push_back(std::move(skullRitem));
	}

	for (auto& obj : m_AllRitems)
	{
		m_OpaqueRitems.push_back(obj.get());
	}

}

void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = m_CurrFrameResource->ObjectCB->Resource();
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	for (size_t i = 0; i < ritems.size(); ++i) 
	{
		auto ri = ritems[i];

		vertexBufferView = ri->Geo->GetVertexBufferView();
		indexBufferView = ri->Geo->GetIndexBufferView();

		cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
		cmdList->IASetIndexBuffer(&indexBufferView);
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		// 偏移到该物体在描述符表的位置
		UINT cbvIndex = m_CurrFrameResourceIndex * (UINT)m_OpaqueRitems.size() + ri->ObjCBIndex;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CBVHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, m_CBVSRVDescriptorSize);

		cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void ShapesApp::ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
	std::ifstream fin(m_VertexFileName);

	if (!fin) 
	{
		OutputDebugStringA(m_VertexFileName.c_str());
		OutputDebugStringA(" not found,\n");
		return;
	}

	UINT vcount = 0, tcount = 0;
	float normal = 0.0f;
	std::string ignore;
	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	vertices.resize(vcount);
	indices.resize(3 * tcount);

	for (UINT i = 0; i < vcount; ++i) 
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		fin >> normal >> normal >> normal;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();
}
