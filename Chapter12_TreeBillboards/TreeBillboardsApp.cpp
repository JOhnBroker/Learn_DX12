#include "TreeBillboardsApp.h"

TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

TreeBillboardsApp::~TreeBillboardsApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool TreeBillboardsApp::Initialize()
{
	bool bResult = false;

	if (!D3DApp::Initialize()) 
	{
		return bResult;
	}

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	m_Waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
	
	if (!InitResource()) 
	{
		return bResult;
	}

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	// execute initialization commands
	HR(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	bResult = true;

	return bResult;
}

bool TreeBillboardsApp::InitResource()
{
	bool bResult = false;

	LoadTexture("grassTex", L"..\\Textures\\grass.dds");
	LoadTexture("waterTex", L"..\\Textures\\water1.dds");
	LoadTexture("fenceTex", L"..\\Textures\\WireFence.dds");
	LoadTexture("treeArrayTex", L"..\\Textures\\treeArray2.dds");
	LoadTexture("white1x1Tex", L"..\\Textures\\white1x1.dds");

	BuildLandGeometry();
	BuildWavesGeometry();
	BuildBoxGeometry();
	BuildTreeSpritesGeometry();
	BuildRoundWireGeometry();
	BuildSphereGeometry();
	BuildMaterials();

	bResult = true;

	return bResult;
}

void TreeBillboardsApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);
}

void TreeBillboardsApp::Update(const GameTimer& timer)
{
	// ImGui
	ImGuiIO& io = ImGui::GetIO();
	
	const float dt = timer.DeltaTime();
	if (ImGui::Begin("Blend demo"))
	{
		static int curr_mode_item = static_cast<int>(m_CurrMode);
		const char* mode_strs[] = {
			"TreeSprites",
			"Cylinder",
			"Sphere"
		};
		ImGui::Checkbox("VertexNormal", &m_VertexNormalEnable);
		ImGui::Checkbox("PlaneNormal", &m_PlaneNormalEnable);
		ImGui::Checkbox("MSAA enable", &m_4xMsaaState);
		if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
		{
			if (curr_mode_item == 0)
			{
				m_CurrMode = ShowMode::TreeSprites;
			}
			else if (curr_mode_item == 1) 
			{
				m_CurrMode = ShowMode::Cylinder;
			}
			else if (curr_mode_item == 2) 
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

	AnimateMaterials(timer);
	UpdateObjectCBs(timer);
	UpdateMaterialCBs(timer);
	UpdateMainPassCB(timer);
	UpdateWaves(timer);
}

void TreeBillboardsApp::Draw(const GameTimer& timer)
{
	auto cmdListAlloc = m_CurrFrameResource->CmdListAlloc;
	D3D12_RESOURCE_BARRIER present2render;
	D3D12_RESOURCE_BARRIER render2present;
	D3D12_CPU_DESCRIPTOR_HANDLE backbufferView;
	D3D12_CPU_DESCRIPTOR_HANDLE depthbufferView;

	HR(cmdListAlloc->Reset());

	HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));

	m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	if (m_4xMsaaState && m_CurrMode == ShowMode::TreeSprites)
	{
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_MsaaRenderTarget.Get(),
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		auto rtvDescriptor = m_MsaaRTVHeap->GetCPUDescriptorHandleForHeapStart();
		auto dsvDescriptor = m_MsaaDSVHeap->GetCPUDescriptorHandleForHeapStart();
		m_CommandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
		m_CommandList->ClearRenderTargetView(rtvDescriptor, (float*)&m_MainPassCB.FogColor, 0, nullptr);
		m_CommandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		auto passCB = m_CurrFrameResource->PassCB->Resource();
		m_CommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

		// draw
		m_CommandList->SetPipelineState(m_PSOs["opaque_4x"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);
		m_CommandList->SetPipelineState(m_PSOs["alphaTested_4x"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTested]);
		m_CommandList->SetPipelineState(m_PSOs["treeSprites_4x"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);
		m_CommandList->SetPipelineState(m_PSOs["transparent_4x"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Transparent]);

		// 将render target 解析到交换链的后台缓冲区
		D3D12_RESOURCE_BARRIER barriers[2] =
		{
			CD3DX12_RESOURCE_BARRIER::Transition(m_MsaaRenderTarget.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST)
		};
		m_CommandList->ResourceBarrier(2, barriers);

		m_CommandList->ResolveSubresource(CurrentBackBuffer(), 0, m_MsaaRenderTarget.Get(), 0, m_BackBufferFormat);

		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));
	}
	else 
	{
		present2render = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_CommandList->ResourceBarrier(1, &present2render);

		backbufferView = CurrentBackBufferView();
		depthbufferView = DepthStencilView();
		m_CommandList->ClearRenderTargetView(backbufferView, (float*)&m_MainPassCB.FogColor, 0, nullptr);
		m_CommandList->ClearDepthStencilView(depthbufferView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		m_CommandList->OMSetRenderTargets(1, &backbufferView, true, &depthbufferView);

		ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

		auto passCB = m_CurrFrameResource->PassCB->Resource();
		m_CommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

		switch (m_CurrMode)
		{
			//TreeSprites ,Cylinder, Sphere, VertexNormal, PlaneNormal
		case TreeBillboardsApp::ShowMode::TreeSprites:
			DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);
			if (m_VertexNormalEnable)
			{
				m_CommandList->SetPipelineState(m_PSOs["vertexNormal"].Get());
				DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::OpaqueVertNormal]);
			}
			if (m_PlaneNormalEnable)
			{
				m_CommandList->SetPipelineState(m_PSOs["planeNormal"].Get());
				DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::OpaquePlanNormal]);
			}
			m_CommandList->SetPipelineState(m_PSOs["alphaTested"].Get());
			DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTested]);
			m_CommandList->SetPipelineState(m_PSOs["treeSprites"].Get());
			DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);
			m_CommandList->SetPipelineState(m_PSOs["transparent"].Get());
			DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Transparent]);
			break;
		case TreeBillboardsApp::ShowMode::Cylinder:
			m_CommandList->SetPipelineState(m_PSOs["cylinder"].Get());
			DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Cylinder]);
			if (m_VertexNormalEnable)
			{
				m_CommandList->SetPipelineState(m_PSOs["vertexNormal"].Get());
				DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::CylinderVertexNormal]);
			}
			break;
		case TreeBillboardsApp::ShowMode::Sphere:
			m_CommandList->SetPipelineState(m_PSOs["sphere"].Get());
			DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Sphere]);
			if (m_PlaneNormalEnable)
			{
				m_CommandList->SetPipelineState(m_PSOs["planeNormal"].Get());
				DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::SpherePlaneNormal]);
			}
			break;
		}
		m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

		render2present = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_CommandList->ResourceBarrier(1, &render2present);
	}

	HR(m_CommandList->Close());
	
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	HR(m_pSwapChain->Present(0, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % SwapChainBufferCount;
	m_CurrFrameResource->Fence = ++m_CurrentFence;

	m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);

}

void TreeBillboardsApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void TreeBillboardsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void TreeBillboardsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void TreeBillboardsApp::UpdateCamera(const GameTimer& gt)
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

void TreeBillboardsApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = m_CurrFrameResource->ObjectCB.get();

	for (auto& obj : m_AllRitems) 
	{
		// 变化时，更新常量缓冲区的数据
		if (obj->NumFramesDirty > 0) 
		{
			XMMATRIX world = XMLoadFloat4x4(&obj->World);
			XMMATRIX texTansform = XMLoadFloat4x4(&obj->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTansform));

			currObjectCB->CopyData(obj->ObjCBIndex, objConstants);

			obj->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::AnimateMaterials(const GameTimer& gt)
{
	auto waterMat = m_Materials["water"].get();

	float& tu = waterMat->m_MatTransform(3, 0);
	float& tv = waterMat->m_MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if (tu >= 1.0f) 
	{
		tu -= 1.0f;
	}
	if (tv >= 1.0f) 
	{
		tv -= 1.0f;
	}

	waterMat->m_MatTransform(3, 0) = tu;
	waterMat->m_MatTransform(3, 1) = tv;
	waterMat->m_NumFramesDirty = g_numFrameResources;
}

void TreeBillboardsApp::RotationMaterials(const GameTimer& gt)
{
	// rotate box
	auto boxMat = m_Materials["wirefence"].get();

	static float phi = 0.0f;
	phi += 0.001f;

	XMMATRIX texRotation = XMMatrixTranslation(-0.5f, -0.5f, 0.0f) * XMMatrixRotationZ(phi) * XMMatrixTranslation(+0.5f, +0.5f, 0.0f);
	XMStoreFloat4x4(&boxMat->m_MatTransform, texRotation);
	boxMat->m_NumFramesDirty = g_numFrameResources;
}

void TreeBillboardsApp::UpdateMaterialCBs(const GameTimer& gt)
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

void TreeBillboardsApp::UpdateMainPassCB(const GameTimer& gt)
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
	m_MainPassCB.AmbientLight = { 0.25f,0.25f,0.35f,1.0f };

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, m_SunTheta, m_SunPhi);
	XMStoreFloat3(&m_MainPassCB.Lights[0].m_Direction, lightDir);
	m_MainPassCB.Lights[0].m_Strength = { 1.0f,1.0f,0.9f };
	m_MainPassCB.Lights[1].m_Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[1].m_Strength = { 0.3f, 0.3f, 0.3f };
	m_MainPassCB.Lights[2].m_Direction = { 0.0f, -0.707f, -0.707f };
	m_MainPassCB.Lights[2].m_Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void TreeBillboardsApp::UpdateWaves(const GameTimer& gt)
{
	static float t_base = 0.0f;

	if ((m_Timer.TotalTime() - t_base) >= 0.25f) 
	{
		t_base += 0.25f;
		int i = MathHelper::Rand(4, m_Waves->GetRowCount() - 5);
		int j = MathHelper::Rand(4, m_Waves->GetColCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		m_Waves->Disturb(i, j, r);
	}

	m_Waves->Update(gt.DeltaTime());

	auto currWavesVB = m_CurrFrameResource->WavesVB.get();
	for (int i = 0; i < m_Waves->GetVertexCount(); ++i) 
	{
		Vertex v;
		v.Pos = m_Waves->Position(i);
		v.Normal = m_Waves->Normal(i);

		v.TexC.x = 0.5f + v.Pos.x / m_Waves->GetWidth();
		v.TexC.y = 0.5f - v.Pos.z / m_Waves->GetDepth();

		currWavesVB->CopyData(i, v);
	}

	m_WavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void TreeBillboardsApp::BuildRootSignature()
{
	HRESULT hReturn = E_FAIL;
	CD3DX12_DESCRIPTOR_RANGE texTable;
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// 创建根描述符表
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// 一个根签名由一组根参数组成
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
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

void TreeBillboardsApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	// SRV Heap
	srvHeapDesc.NumDescriptors = 5;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto grassTex = m_Textures["grassTex"]->m_Resource;
	auto waterTex = m_Textures["waterTex"]->m_Resource;
	auto fenceTex = m_Textures["fenceTex"]->m_Resource;
	auto treeArrayTex = m_Textures["treeArrayTex"]->m_Resource;
	auto whiteTex = m_Textures["white1x1Tex"]->m_Resource;

	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	m_pd3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = waterTex->GetDesc().Format;
	m_pd3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = fenceTex->GetDesc().Format;
	m_pd3dDevice->CreateShaderResourceView(fenceTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = treeArrayTex->GetDesc().Format;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	m_pd3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = whiteTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	m_pd3dDevice->CreateShaderResourceView(whiteTex.Get(), &srvDesc, hDescriptor);
}

void TreeBillboardsApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i) 
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t>indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
		m_CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), 
		m_CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;
	m_Geometries["landGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildWavesGeometry()
{
	std::vector<std::uint16_t> indices(3 * m_Waves->GetTriangleCount());
	assert(m_Waves->GetVertexCount() < 0x0000ffff);

	int r = m_Waves->GetRowCount();
	int c = m_Waves->GetColCount();
	int k = 0;

	for (int i = 0; i < r - 1; ++i) 
	{
		for (int j = 0; j < c - 1; ++j) 
		{
			indices[k] = i * c + j;
			indices[k + 1] = i * c + j + 1;
			indices[k + 2] = (i + 1) * c + j;

			indices[k + 5] = i * c + j + 1;
			indices[k + 3] = (i + 1) * c + j + 1;
			indices[k + 4] = (i + 1) * c + j;

			k += 6;
		}
	}

	UINT vbByteSize = m_Waves->GetVertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
		m_CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	m_Geometries["waterGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 0);

	std::vector<Vertex> vertices(box.Vertices.size());
	std::vector<std::uint16_t> indices = box.GetIndices16();

	for (size_t i = 0; i < box.Vertices.size(); ++i) 
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertices.size());
	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), indices.size());

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry boxSubmesh;
	boxSubmesh.BaseVertexLocation = 0;
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.IndexCount = (UINT)indices.size();

	geo->DrawArgs["box"] = boxSubmesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void TreeBillboardsApp::BuildTreeSpritesGeometry()
{
	struct TreeSpriteVertex 
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 16;
	std::array<TreeSpriteVertex, treeCount> vertices;
	std::array<std::uint16_t, treeCount> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};
	for (int i = 0; i < treeCount; ++i) 
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);

		y += 8.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
	}

	const UINT vbByteSize = vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(), vertices.data(),
		vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(), indices.data(),
		ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	
	geo->DrawArgs["points"] = submesh;

	m_Geometries["trees"] = std::move(geo);

}

void TreeBillboardsApp::BuildRoundWireGeometry()
{
	std::vector<Vertex> vertices(41);
	std::vector<std::uint16_t> indices(41);

	for (int i = 0; i < 40; ++i) 
	{
		vertices[i].Pos = XMFLOAT3(cosf(XM_PI / 20 * i), -1.0f, -sinf(XM_PI / 20 * i));
		vertices[i].Normal = XMFLOAT3(cosf(XM_PI / 20 * i), 0.0f, -sinf(XM_PI / 20 * i));
		vertices[i].TexC = XMFLOAT2(0.0f, 0.0f);
		indices[i] = i;
	}
	vertices[40] = vertices[0];
	indices[40] = 0;

	const UINT vbByteSize = vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "roundGeo";

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

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["line"] = submesh;

	m_Geometries["roundWire"] = std::move(geo);
}

void TreeBillboardsApp::BuildSphereGeometry()
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
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "sphereGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertices.size());
	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), indices.size());

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(), m_CommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.BaseVertexLocation = 0;
	sphereSubmesh.StartIndexLocation = 0;
	sphereSubmesh.IndexCount = (UINT)indices.size();

	geo->DrawArgs["sphere"] = sphereSubmesh;

	m_Geometries["sphereGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Default.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Default.hlsl", defines, "PS", "ps_5_1");
	m_Shaders["alphaTestedPS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	m_Shaders["treeSpriteVS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["treeSpriteGS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	m_Shaders["treeSpritePS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");
	m_Shaders["exerciseVS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Exercise.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["sphereVS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Exercise.hlsl", nullptr, "Sphere_VS", "vs_5_1");
	m_Shaders["cylinderGS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Exercise.hlsl", nullptr, "Cylinder_GS", "gs_5_1");
	m_Shaders["sphereGS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Exercise.hlsl", nullptr, "Sphere_GS", "gs_5_1");
	m_Shaders["vertexNormalGS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Exercise.hlsl", nullptr, "VertexNormal_GS", "gs_5_1");
	m_Shaders["planeNormalGS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Exercise.hlsl", nullptr, "PlaneNormal_GS", "gs_5_1");
	m_Shaders["exercisePS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter12\\Exercise.hlsl", alphaTestDefines, "PS", "ps_5_1");

	m_InputLayouts["standard"] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL"  ,0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD"  ,0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	m_InputLayouts["treeSprite"] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"SIZE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
}

void TreeBillboardsApp::BuildPSOs()
{
	// opaque
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { m_InputLayouts["standard"].data(),(UINT)m_InputLayouts["standard"].size() };
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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaque4xPsoDesc = opaquePsoDesc;
	opaque4xPsoDesc.SampleDesc.Count = 4;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&opaque4xPsoDesc, IID_PPV_ARGS(&m_PSOs["opaque_4x"])));

	// transparent
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&m_PSOs["transparent"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparent4xPsoDesc = transparentPsoDesc;
	transparent4xPsoDesc.SampleDesc.Count = 4;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&transparent4xPsoDesc, IID_PPV_ARGS(&m_PSOs["transparent_4x"])));

	// alpha test
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["alphaTestedPS"]->GetBufferPointer()),
		m_Shaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&m_PSOs["alphaTested"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTested4xPsoDesc = alphaTestedPsoDesc;
	alphaTested4xPsoDesc.SampleDesc.Count = 4;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&alphaTested4xPsoDesc, IID_PPV_ARGS(&m_PSOs["alphaTested_4x"])));

	// tree sprite
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	CD3DX12_BLEND_DESC treeSpriteBlendDesc(D3D12_DEFAULT);
	treeSpriteBlendDesc.AlphaToCoverageEnable = true;
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["treeSpriteVS"]->GetBufferPointer()),
		m_Shaders["treeSpriteVS"]->GetBufferSize()
	}; 
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["treeSpriteGS"]->GetBufferPointer()),
		m_Shaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["treeSpritePS"]->GetBufferPointer()),
		m_Shaders["treeSpritePS"]->GetBufferSize()
	};
	treeSpritePsoDesc.BlendState = treeSpriteBlendDesc;
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { m_InputLayouts["treeSprite"].data(),(UINT)m_InputLayouts["treeSprite"].size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&m_PSOs["treeSprites"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSprite4xPsoDesc = treeSpritePsoDesc;
	treeSprite4xPsoDesc.SampleDesc.Count = 4;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&treeSprite4xPsoDesc, IID_PPV_ARGS(&m_PSOs["treeSprites_4x"])));

	// Cylinder
	D3D12_GRAPHICS_PIPELINE_STATE_DESC cylinderPsoDesc = opaquePsoDesc;
	cylinderPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["exerciseVS"]->GetBufferPointer()),
		m_Shaders["exerciseVS"]->GetBufferSize()
	};
	cylinderPsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["cylinderGS"]->GetBufferPointer()),
		m_Shaders["cylinderGS"]->GetBufferSize()
	};
	cylinderPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["exercisePS"]->GetBufferPointer()),
		m_Shaders["exercisePS"]->GetBufferSize()
	};
	cylinderPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	cylinderPsoDesc.InputLayout = { m_InputLayouts["standard"].data(),(UINT)m_InputLayouts["standard"].size() };
	cylinderPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&cylinderPsoDesc, IID_PPV_ARGS(&m_PSOs["cylinder"])));

	// Sphere
	D3D12_GRAPHICS_PIPELINE_STATE_DESC spherePsoDesc = cylinderPsoDesc;
	spherePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["sphereVS"]->GetBufferPointer()),
		m_Shaders["sphereVS"]->GetBufferSize()
	};
	spherePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["sphereGS"]->GetBufferPointer()),
		m_Shaders["sphereGS"]->GetBufferSize()
	};
	spherePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["exercisePS"]->GetBufferPointer()),
		m_Shaders["exercisePS"]->GetBufferSize()
	};
	spherePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	spherePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&spherePsoDesc, IID_PPV_ARGS(&m_PSOs["sphere"])));

	// Vertex normal
	D3D12_GRAPHICS_PIPELINE_STATE_DESC vertexNormalPsoDesc = spherePsoDesc;
	vertexNormalPsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["vertexNormalGS"]->GetBufferPointer()),
		m_Shaders["vertexNormalGS"]->GetBufferSize()
	};
	vertexNormalPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	vertexNormalPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&vertexNormalPsoDesc, IID_PPV_ARGS(&m_PSOs["vertexNormal"])));
	
	// Plane normal
	D3D12_GRAPHICS_PIPELINE_STATE_DESC planeNormalPsoDesc = spherePsoDesc;
	planeNormalPsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["planeNormalGS"]->GetBufferPointer()),
		m_Shaders["planeNormalGS"]->GetBufferSize()
	};
	planeNormalPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	planeNormalPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&planeNormalPsoDesc, IID_PPV_ARGS(&m_PSOs["planeNormal"])));
}

void TreeBillboardsApp::BuildFrameResources()
{
	for(int i = 0;i<g_numFrameResources;++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get(),
			1, (UINT)m_AllRitems.size(), (UINT)m_Materials.size(), m_Waves->GetVertexCount()));
	}
}

void TreeBillboardsApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->m_Name = "grass";
	grass->m_MatCBIndex = 0;
	grass->m_DiffuseSrvHeapIndex = 0;
	grass->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->m_FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->m_Roughness = 0.125f;

	auto water = std::make_unique<Material>();
	water->m_Name = "water";
	water->m_MatCBIndex = 1;
	water->m_DiffuseSrvHeapIndex = 1;
	water->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water->m_FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->m_Roughness = 0.0f;

	auto wirefence = std::make_unique<Material>();
	wirefence->m_Name = "wirefence";
	wirefence->m_MatCBIndex = 2;
	wirefence->m_DiffuseSrvHeapIndex = 2;
	wirefence->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence->m_FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	wirefence->m_Roughness = 0.25f;

	auto treeSprites = std::make_unique<Material>();
	treeSprites->m_Name = "treeSprite";
	treeSprites->m_MatCBIndex = 3;
	treeSprites->m_DiffuseSrvHeapIndex = 3;
	treeSprites->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->m_FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->m_Roughness = 0.125f;

	auto exercise = std::make_unique<Material>();
	exercise->m_Name = "exercise";
	exercise->m_MatCBIndex = 4;
	exercise->m_DiffuseSrvHeapIndex = 4;
	exercise->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	exercise->m_FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	exercise->m_Roughness = 0.25f;

	m_Materials["grass"] = std::move(grass);
	m_Materials["water"] = std::move(water);
	m_Materials["wirefence"] = std::move(wirefence);
	m_Materials["treeSprites"] = std::move(treeSprites);
	m_Materials["exercise"] = std::move(exercise);
}

void TreeBillboardsApp::BuildRenderItems()
{
	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->ObjCBIndex = 0;
	wavesRitem->Mat = m_Materials["water"].get();
	wavesRitem->Geo = m_Geometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_WavesRitem = wavesRitem.get();
	m_RitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = m_Materials["grass"].get();
	gridRitem->Geo = m_Geometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	m_RitemLayer[(int)RenderLayer::OpaquePlanNormal].push_back(gridRitem.get());

	auto gridNormalRitem = std::make_unique<RenderItem>();
	gridNormalRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridNormalRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridNormalRitem->ObjCBIndex = 2;
	gridNormalRitem->Mat = m_Materials["grass"].get();
	gridNormalRitem->Geo = m_Geometries["landGeo"].get();
	gridNormalRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	gridNormalRitem->IndexCount = gridNormalRitem->Geo->DrawArgs["grid"].IndexCount;
	gridNormalRitem->StartIndexLocation = gridNormalRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridNormalRitem->BaseVertexLocation = gridNormalRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::OpaqueVertNormal].push_back(gridNormalRitem.get());

	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation(3.0f, 2.0f, -9.0f));
	//XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(2.0f, 2.0f, 2.0f));
	boxRitem->ObjCBIndex = 3;
	boxRitem->Mat = m_Materials["wirefence"].get();
	boxRitem->Geo = m_Geometries["boxGeo"].get();
	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::AlphaTested].push_back(boxRitem.get());
	m_RitemLayer[(int)RenderLayer::OpaquePlanNormal].push_back(boxRitem.get());

	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->ObjCBIndex = 4;
	treeSpritesRitem->Mat = m_Materials["treeSprites"].get();
	treeSpritesRitem->Geo = m_Geometries["trees"].get();
	treeSpritesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());
	
	// Cylinder
	auto cylinderRitem = std::make_unique<RenderItem>();
	cylinderRitem->ObjCBIndex = 5;
	cylinderRitem->Mat = m_Materials["exercise"].get();
	cylinderRitem->Geo = m_Geometries["roundWire"].get();
	cylinderRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	cylinderRitem->IndexCount = cylinderRitem->Geo->DrawArgs["line"].IndexCount;
	cylinderRitem->StartIndexLocation = cylinderRitem->Geo->DrawArgs["line"].StartIndexLocation;
	cylinderRitem->BaseVertexLocation = cylinderRitem->Geo->DrawArgs["line"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Cylinder].push_back(cylinderRitem.get());
	
	auto cylinderNormalRitem = std::make_unique<RenderItem>();
	cylinderNormalRitem->ObjCBIndex = 6;
	cylinderNormalRitem->Mat = m_Materials["exercise"].get();
	cylinderNormalRitem->Geo = m_Geometries["roundWire"].get();
	cylinderNormalRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	cylinderNormalRitem->IndexCount = cylinderNormalRitem->Geo->DrawArgs["line"].IndexCount;
	cylinderNormalRitem->StartIndexLocation = cylinderNormalRitem->Geo->DrawArgs["line"].StartIndexLocation;
	cylinderNormalRitem->BaseVertexLocation = cylinderNormalRitem->Geo->DrawArgs["line"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::CylinderVertexNormal].push_back(cylinderNormalRitem.get());
	
	// Sphere
	auto sphereRitem = std::make_unique<RenderItem>();
	sphereRitem->ObjCBIndex = 7;
	sphereRitem->Mat = m_Materials["exercise"].get();
	sphereRitem->Geo = m_Geometries["sphereGeo"].get();
	sphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	sphereRitem->IndexCount = sphereRitem->Geo->DrawArgs["sphere"].IndexCount;
	sphereRitem->StartIndexLocation = sphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	sphereRitem->BaseVertexLocation = sphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Sphere].push_back(sphereRitem.get());
	m_RitemLayer[(int)RenderLayer::SpherePlaneNormal].push_back(sphereRitem.get());

	auto sphereNorRitem = std::make_unique<RenderItem>();
	sphereNorRitem->ObjCBIndex = 8;
	sphereNorRitem->Mat = m_Materials["exercise"].get();
	sphereNorRitem->Geo = m_Geometries["sphereGeo"].get();
	sphereNorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	sphereNorRitem->IndexCount = sphereNorRitem->Geo->DrawArgs["sphere"].IndexCount;
	sphereNorRitem->StartIndexLocation = sphereNorRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	sphereNorRitem->BaseVertexLocation = sphereNorRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;


	m_AllRitems.push_back(std::move(wavesRitem));
	m_AllRitems.push_back(std::move(gridRitem));
	m_AllRitems.push_back(std::move(gridNormalRitem));
	m_AllRitems.push_back(std::move(boxRitem));
	m_AllRitems.push_back(std::move(treeSpritesRitem));
	m_AllRitems.push_back(std::move(cylinderRitem));
	m_AllRitems.push_back(std::move(cylinderNormalRitem));
	m_AllRitems.push_back(std::move(sphereRitem));
	m_AllRitems.push_back(std::move(sphereNorRitem));

}

void TreeBillboardsApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = m_CurrFrameResource->ObjectCB->Resource();
	auto materialCB = m_CurrFrameResource->MaterialCB->Resource();
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

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->m_DiffuseSrvHeapIndex, m_CBVSRVDescriptorSize);

		// 使用描述符
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
		objCBAddress += ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = materialCB->GetGPUVirtualAddress();
		matCBAddress += ri->Mat->m_MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(2, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT> TreeBillboardsApp::GetStaticSamplers()
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

float TreeBillboardsApp::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 TreeBillboardsApp::GetHillsNormal(float x, float z) const
{
	// n = (-df/dx, 1, -df/dz)
	// f(x,z) = 0.3z * sin(0.1f * x) + x * cos(0.1f * x) 
	XMFLOAT3 N(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&N));
	XMStoreFloat3(&N, unitNormal);
	return N;
}

void TreeBillboardsApp::LoadTexture(std::string name, std::wstring filename)
{
	auto texture = std::make_unique<Texture>();
	texture->m_Name = name;
	texture->m_Filename = filename;
	HR(DirectX::CreateDDSTextureFromFile12(m_pd3dDevice.Get(), m_CommandList.Get(),
		texture->m_Filename.c_str(), texture->m_Resource, texture->m_UploadHeap));

	m_Textures[name] = std::move(texture);
}
