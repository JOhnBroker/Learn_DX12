#include "CSApp.h"

CSApp::CSApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

CSApp::CSApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

CSApp::~CSApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool CSApp::Initialize()
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

	m_Waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
	m_GpuWaves = std::make_unique<GpuWaves>(m_pd3dDevice.Get(),
		m_CommandList.Get(), 256, 256, 0.25f, 0.03f, 2.0f, 0.2f);
	m_BlurFilter = std::make_unique<BlurFilter>(m_pd3dDevice.Get(),
		m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	m_SobelFilter = std::make_unique<SobelFilter>(m_pd3dDevice.Get(),
		m_ClientWidth, m_ClientHeight, m_BackBufferFormat);

	m_OffScreenRT = std::make_unique<RenderTarget>(m_pd3dDevice.Get(),
		m_ClientWidth, m_ClientHeight, m_BackBufferFormat);

	BuildRootSignature();
	BuildCSRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildLandGeometry();
	BuildWavesGeometry();
	BuildBoxGeometry();
	BuildMaterials();
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

bool CSApp::InitResource()
{
	bool bResult = false;

	LoadTexture("grassTex", L"..\\Textures\\grass.dds");
	LoadTexture("waterTex", L"..\\Textures\\water1.dds");
	LoadTexture("fenceTex", L"..\\Textures\\WireFence.dds");

	bResult = true;

	return bResult;
}

void CSApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);

	if (m_BlurFilter != nullptr)
	{
		m_BlurFilter->OnResize(m_ClientWidth, m_ClientHeight);
	}
	if (m_SobelFilter != nullptr) 
	{
		m_SobelFilter->OnResize(m_ClientWidth, m_ClientHeight);
	}
	if (m_OffScreenRT != nullptr) 
	{
		m_OffScreenRT->OnResize(m_ClientWidth, m_ClientHeight);
	}
}

void CSApp::Update(const GameTimer& timer)
{
	// ImGui
	ImGuiIO& io = ImGui::GetIO();
	
	const float dt = timer.DeltaTime();
	if (ImGui::Begin("Blend demo"))
	{
		static int curr_mode_item = static_cast<int>(m_CurrMode);
		const char* mode_strs[] = {
			"Wireframe",
			"Opaque",
			"Blur",
			"WavesCS",
			"SobelFilter"
		};
		if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
		{
			if (curr_mode_item == 0)
			{
				m_CurrMode = ShowMode::Wireframe;
			}
			else if (curr_mode_item == 1) 
			{
				m_CurrMode = ShowMode::Opaque;
			}
			else if (curr_mode_item == 2) 
			{
				m_CurrMode = ShowMode::Blur;
			}
			else if (curr_mode_item == 3)
			{
				m_CurrMode = ShowMode::WavesCS;
			}
			else if (curr_mode_item == 4)
			{
				m_CurrMode = ShowMode::SobelFilter;
			}
		}
		ImGui::Text("Blur count: %d", m_iBlurCount);
		ImGui::SliderInt("##1", &m_iBlurCount, 1, 8, "");
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
	if (m_CurrMode != ShowMode::WavesCS) 
	{
		UpdateWaves(timer);
	}
}

void CSApp::Draw(const GameTimer& timer)
{
	auto cmdListAlloc = m_CurrFrameResource->CmdListAlloc;
	D3D12_CPU_DESCRIPTOR_HANDLE backbufferView;
	D3D12_CPU_DESCRIPTOR_HANDLE depthbufferView;

	HR(cmdListAlloc->Reset());

	HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));

	m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	if (m_CurrMode == ShowMode::SobelFilter) 
	{
		backbufferView = m_OffScreenRT->GetRtv();
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_OffScreenRT->Resource(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}
	else 
	{
		backbufferView = CurrentBackBufferView();
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	depthbufferView = DepthStencilView();
	m_CommandList->ClearRenderTargetView(backbufferView, (float*)&m_MainPassCB.FogColor, 0, nullptr);
	m_CommandList->ClearDepthStencilView(depthbufferView, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_CommandList->OMSetRenderTargets(1, &backbufferView, true, &depthbufferView);

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_CommandList->SetGraphicsRootSignature(m_RootSignatures["wavesRender"].Get());

	auto passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

	switch (m_CurrMode)
	{
	case CSApp::ShowMode::Wireframe:
		m_CommandList->SetPipelineState(m_PSOs["opaque_wireframe"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTested]);
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Transparent]);

		m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		break;
	case CSApp::ShowMode::Opaque:
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);
		m_CommandList->SetPipelineState(m_PSOs["alphaTested"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTested]);
		m_CommandList->SetPipelineState(m_PSOs["transparent"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Transparent]);

		m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		break;
	case ShowMode::Blur:
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);
		m_CommandList->SetPipelineState(m_PSOs["alphaTested"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTested]);
		m_CommandList->SetPipelineState(m_PSOs["transparent"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Transparent]);

		m_BlurFilter->OnProcess(m_CommandList.Get(), m_RootSignatures["blur"].Get(), m_PSOs["horzBlur"].Get(),
			m_PSOs["vertBlur"].Get(), CurrentBackBuffer(), m_iBlurCount);
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

		m_CommandList->CopyResource(CurrentBackBuffer(), m_BlurFilter->Output());

		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));

		m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		break;
	case ShowMode::WavesCS:
		UpdateWavesGpu(timer);
		m_CommandList->SetGraphicsRootSignature(m_RootSignatures["wavesRender"].Get());
		m_CommandList->SetGraphicsRootDescriptorTable(4, m_GpuWaves->DisplacementMap());

		m_CommandList->SetPipelineState(m_PSOs["opaque"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);
		m_CommandList->SetPipelineState(m_PSOs["alphaTested"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTested]);
		m_CommandList->SetPipelineState(m_PSOs["wavesRender"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::GpuWaves]);

		m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		break;
	case ShowMode::SobelFilter:

		// draw opaque to offscreen
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);
		m_CommandList->SetPipelineState(m_PSOs["alphaTested"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::AlphaTested]);
		m_CommandList->SetPipelineState(m_PSOs["transparent"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Transparent]);

		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_OffScreenRT->Resource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
		// sobel process
		m_SobelFilter->OnProcess(m_CommandList.Get(), m_RootSignatures["sobel"].Get(), m_PSOs["sobel"].Get(), m_OffScreenRT->GetSrv());
		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// composite
		m_CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
		m_CommandList->SetGraphicsRootSignature(m_RootSignatures["sobel"].Get());
		m_CommandList->SetPipelineState(m_PSOs["composite"].Get());
		m_CommandList->SetGraphicsRootDescriptorTable(0, m_OffScreenRT->GetSrv());
		m_CommandList->SetGraphicsRootDescriptorTable(1, m_SobelFilter->OutputSrv());
		DrawFullScreenQuad(m_CommandList.Get());

		m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		break;
	}

	HR(m_CommandList->Close());
	
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	HR(m_pSwapChain->Present(0, 0));
	m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % SwapChainBufferCount;
	m_CurrFrameResource->Fence = ++m_CurrentFence;

	m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);

Exit0:
	return;
}

void CSApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void CSApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CSApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void CSApp::UpdateCamera(const GameTimer& gt)
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

void CSApp::UpdateObjectCBs(const GameTimer& gt)
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
			objConstants.DisplacementMapTexelSize = obj->DisplacementMapTexelSize;
			objConstants.GridSpatialStep = obj->GridSpatialStep;

			currObjectCB->CopyData(obj->ObjCBIndex, objConstants);

			obj->NumFramesDirty--;
		}
	}
}

void CSApp::AnimateMaterials(const GameTimer& gt)
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

void CSApp::UpdateMaterialCBs(const GameTimer& gt)
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

void CSApp::UpdateMainPassCB(const GameTimer& gt)
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

void CSApp::UpdateWaves(const GameTimer& gt)
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

void CSApp::UpdateWavesGpu(const GameTimer& gt)
{
	static float t_base = 0.0f;

	if (m_Timer.TotalTime() - t_base >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, m_GpuWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, m_GpuWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(1.0f, 2.0f);
		m_GpuWaves->Disturb(m_CommandList.Get(), m_RootSignatures["wavesSim"].Get(), m_PSOs["wavesDisturb"].Get(), i, j, r);
	}
	m_GpuWaves->Update(gt, m_CommandList.Get(), m_RootSignatures["wavesSim"].Get(), m_PSOs["wavesUpdate"].Get());
}

void CSApp::BuildRootSignature()
{
	#pragma region Opaque
	HRESULT hReturn = E_FAIL;
	{
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
			IID_PPV_ARGS(m_RootSignatures["opaque"].GetAddressOf())));
	}
	#pragma endregion

	#pragma region WavesRender
	{
		CD3DX12_DESCRIPTOR_RANGE srvTable;
		srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE displacementMapTable;
		displacementMapTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

		CD3DX12_ROOT_PARAMETER slotRootParameter[5];
		slotRootParameter[0].InitAsDescriptorTable(1, &srvTable, D3D12_SHADER_VISIBILITY_ALL);
		slotRootParameter[1].InitAsConstantBufferView(0);
		slotRootParameter[2].InitAsConstantBufferView(1);
		slotRootParameter[3].InitAsConstantBufferView(2);
		slotRootParameter[4].InitAsDescriptorTable(1, &displacementMapTable, D3D12_SHADER_VISIBILITY_ALL);

		auto staticSamplers = GetStaticSamplers();

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
			(UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
			IID_PPV_ARGS(m_RootSignatures["wavesRender"].GetAddressOf())));
	}
	#pragma endregion
}

void CSApp::BuildCSRootSignature()
{
	HRESULT hReturn = E_FAIL;
	#pragma region Blur
	{
		CD3DX12_DESCRIPTOR_RANGE srvTable;
		srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE uavTable;
		uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

		CD3DX12_ROOT_PARAMETER slotRootParameter[3];
		slotRootParameter[0].InitAsConstants(12, 0);
		slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);
		slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
			IID_PPV_ARGS(m_RootSignatures["blur"].GetAddressOf())));
	}
	#pragma endregion

	#pragma region WavesSim
	{
		CD3DX12_DESCRIPTOR_RANGE uavTable0;
		uavTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE uavTable1;
		uavTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
		CD3DX12_DESCRIPTOR_RANGE uavTable2;
		uavTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);

		CD3DX12_ROOT_PARAMETER slotRootParameter[4];
		slotRootParameter[0].InitAsConstants(6, 0);
		slotRootParameter[1].InitAsDescriptorTable(1,&uavTable0);
		slotRootParameter[2].InitAsDescriptorTable(1,&uavTable1);
		slotRootParameter[3].InitAsDescriptorTable(1,&uavTable2);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
			0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

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
			IID_PPV_ARGS(m_RootSignatures["wavesSim"].GetAddressOf())));
	}
	#pragma endregion

#pragma region Sobel
	{
		CD3DX12_DESCRIPTOR_RANGE srvTable0;
		srvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		CD3DX12_DESCRIPTOR_RANGE srvTable1;
		srvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		CD3DX12_DESCRIPTOR_RANGE uavTable0;
		uavTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

		CD3DX12_ROOT_PARAMETER slotRootParameter[3];
		slotRootParameter[0].InitAsDescriptorTable(1, &srvTable0);
		slotRootParameter[1].InitAsDescriptorTable(1, &srvTable1);
		slotRootParameter[2].InitAsDescriptorTable(1, &uavTable0);

		auto staticSamplers = GetStaticSamplers();

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter,
			staticSamplers.size(), staticSamplers.data(), 
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
			IID_PPV_ARGS(m_RootSignatures["sobel"].GetAddressOf())));
	}
#pragma endregion

}

void CSApp::BuildDescriptorHeaps()
{
	const int textureDescriptorCount = 3;
	const int blurDescriptorCount = 4;
	const int rtDescriptorCount = 3;

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	ComPtr<ID3D12Resource> grassTex;
	ComPtr<ID3D12Resource> waterTex;
	ComPtr<ID3D12Resource> fenceTex;

	// SRV Heap
	srvHeapDesc.NumDescriptors = textureDescriptorCount + blurDescriptorCount +
		m_GpuWaves->DescriptorCount() + m_SobelFilter->DescriptorCount() +
		rtDescriptorCount;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	grassTex = m_Textures["grassTex"]->m_Resource;
	waterTex = m_Textures["waterTex"]->m_Resource;
	fenceTex = m_Textures["fenceTex"]->m_Resource;

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

	int blurOffset = textureDescriptorCount;
	int gpuWavesOffset = textureDescriptorCount + blurDescriptorCount;
	int SobelOffset = textureDescriptorCount + blurDescriptorCount + m_GpuWaves->DescriptorCount();
	int rtOffset = textureDescriptorCount + blurDescriptorCount + m_GpuWaves->DescriptorCount() + m_SobelFilter->DescriptorCount();

	m_BlurFilter->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), blurOffset, m_CBVSRVDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), blurOffset, m_CBVSRVDescriptorSize),
		m_CBVSRVDescriptorSize);

	m_GpuWaves->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), gpuWavesOffset, m_CBVSRVDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), gpuWavesOffset, m_CBVSRVDescriptorSize),
		m_CBVSRVDescriptorSize);

	m_SobelFilter->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), SobelOffset, m_CBVSRVDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), SobelOffset, m_CBVSRVDescriptorSize),
		m_CBVSRVDescriptorSize);

	m_OffScreenRT->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), rtOffset, m_CBVSRVDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), rtOffset, m_CBVSRVDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), SwapChainBufferCount, m_RTVDescriptorSize));
}

void CSApp::BuildLandGeometry()
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

void CSApp::BuildWavesGeometry()
{
	// Cpu wave
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

	// Gpu wave
	{
		GeometryGenerator geoGen;
		GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, m_GpuWaves->RowCount(), m_GpuWaves->ColumnCount());

		std::vector<Vertex> vertices(grid.Vertices.size());
		std::vector<std::uint32_t> indices = grid.Indices32;
		for (size_t i = 0; i < grid.Vertices.size(); ++i) 
		{
			vertices[i].Pos = grid.Vertices[i].Position;
			vertices[i].Normal = grid.Vertices[i].Normal;
			vertices[i].TexC = grid.Vertices[i].TexC;
		}

		UINT vbByteSize = m_GpuWaves->VertexCount() * sizeof(Vertex);
		UINT ibByteSize = indices.size() * sizeof(std::uint32_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "gpuWaterGeo";

		HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
		
		HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
			m_CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
			m_CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexBufferByteSize = vbByteSize;
		geo->VertexByteStride = sizeof(Vertex);
		geo->IndexFormat = DXGI_FORMAT_R32_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)indices.size();
		submesh.BaseVertexLocation = 0;
		submesh.StartIndexLocation = 0;

		geo->DrawArgs["grid"] = submesh;

		m_Geometries[geo->Name] = std::move(geo);
	}
}

void CSApp::BuildBoxGeometry()
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

void CSApp::BuildShadersAndInputLayout()
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

	const D3D_SHADER_MACRO waveDefines[] =
	{
		"DISPLACEMENT_MAP","1",
		NULL,NULL
	};

	m_Shaders["standardVS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Default.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["wavesVS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Default.hlsl", waveDefines, "VS", "vs_5_1");
	m_Shaders["opaquePS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Default.hlsl", defines, "PS", "ps_5_1");
	m_Shaders["alphaTestedPS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	m_Shaders["horzBlurCS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Blur.hlsl", nullptr, "HorzBlurCS", "cs_5_1");
	m_Shaders["vertBlurCS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Blur.hlsl", nullptr, "VertBlurCS", "cs_5_1");
	m_Shaders["wavesUpdateCS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\WaveSim.hlsl", nullptr, "UpdateWavesCS", "cs_5_1");
	m_Shaders["wavesDisturbCS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\WaveSim.hlsl", nullptr, "DisturbWavesCS", "cs_5_1");
	m_Shaders["compositeVS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\SobelVS.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["compositePS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\SobelVS.hlsl", nullptr, "PS", "ps_5_1");
	m_Shaders["sobelCS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Sobel.hlsl", nullptr, "SobelCS", "cs_5_1");


	m_InputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL"  ,0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD"  ,0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
}

void CSApp::BuildPSOs()
{
	// opaque
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { m_InputLayout.data(),(UINT)m_InputLayout.size() };
	opaquePsoDesc.pRootSignature = m_RootSignatures["wavesRender"].Get();
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

	// alpha test
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["alphaTestedPS"]->GetBufferPointer()),
		m_Shaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&m_PSOs["alphaTested"])));

	// HorzBlur
	D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPso = {};
	horzBlurPso.pRootSignature = m_RootSignatures["blur"].Get();
	horzBlurPso.CS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["horzBlurCS"]->GetBufferPointer()),
		m_Shaders["horzBlurCS"]->GetBufferSize()
	};
	horzBlurPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	HR(m_pd3dDevice->CreateComputePipelineState(&horzBlurPso, IID_PPV_ARGS(&m_PSOs["horzBlur"])));
	
	// VertBlur
	D3D12_COMPUTE_PIPELINE_STATE_DESC vertBlurPso = {};
	vertBlurPso.pRootSignature = m_RootSignatures["blur"].Get();
	vertBlurPso.CS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["vertBlurCS"]->GetBufferPointer()),
		m_Shaders["vertBlurCS"]->GetBufferSize()
	};
	vertBlurPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	HR(m_pd3dDevice->CreateComputePipelineState(&vertBlurPso, IID_PPV_ARGS(&m_PSOs["vertBlur"])));
	
	// Draw waves
	D3D12_GRAPHICS_PIPELINE_STATE_DESC wavesRenderPso = transparentPsoDesc;
	wavesRenderPso.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["wavesVS"]->GetBufferPointer()),
		m_Shaders["wavesVS"]->GetBufferSize()
	};
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&wavesRenderPso, IID_PPV_ARGS(&m_PSOs["wavesRender"])));

	// WaveUpdate
	D3D12_COMPUTE_PIPELINE_STATE_DESC wavesUpdatePso = {};
	wavesUpdatePso.pRootSignature = m_RootSignatures["wavesSim"].Get();
	wavesUpdatePso.CS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["wavesUpdateCS"]->GetBufferPointer()),
		m_Shaders["wavesUpdateCS"]->GetBufferSize()
	};
	wavesUpdatePso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	HR(m_pd3dDevice->CreateComputePipelineState(&wavesUpdatePso, IID_PPV_ARGS(&m_PSOs["wavesUpdate"])));

	// WaveDisturb
	D3D12_COMPUTE_PIPELINE_STATE_DESC wavesDisturbPso = {};
	wavesDisturbPso.pRootSignature = m_RootSignatures["wavesSim"].Get();
	wavesDisturbPso.CS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["wavesDisturbCS"]->GetBufferPointer()),
		m_Shaders["wavesDisturbCS"]->GetBufferSize()
	};
	wavesDisturbPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	HR(m_pd3dDevice->CreateComputePipelineState(&wavesDisturbPso, IID_PPV_ARGS(&m_PSOs["wavesDisturb"])));

	// Composite
	D3D12_GRAPHICS_PIPELINE_STATE_DESC compositePso = opaquePsoDesc;
	compositePso.pRootSignature = m_RootSignatures["sobel"].Get();
	// disable depth test
	compositePso.DepthStencilState.DepthEnable = false;
	compositePso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	compositePso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	compositePso.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["compositeVS"]->GetBufferPointer()),
		m_Shaders["compositeVS"]->GetBufferSize()
	};
	compositePso.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["compositePS"]->GetBufferPointer()),
		m_Shaders["compositePS"]->GetBufferSize()
	};
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&compositePso, IID_PPV_ARGS(&m_PSOs["composite"])));

	// Sobel
	D3D12_COMPUTE_PIPELINE_STATE_DESC sobelPso = {};
	sobelPso.pRootSignature = m_RootSignatures["sobel"].Get();
	sobelPso.CS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["sobelCS"]->GetBufferPointer()),
		m_Shaders["sobelCS"]->GetBufferSize()
	};
	sobelPso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	HR(m_pd3dDevice->CreateComputePipelineState(&sobelPso, IID_PPV_ARGS(&m_PSOs["sobel"])));

}

void CSApp::BuildFrameResources()
{
	for(int i = 0;i<g_numFrameResources;++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get(),
			1, (UINT)m_AllRitems.size(), (UINT)m_Materials.size(), m_Waves->GetVertexCount()));
	}
}

void CSApp::BuildMaterials()
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

	m_Materials["grass"] = std::move(grass);
	m_Materials["water"] = std::move(water);
	m_Materials["wirefence"] = std::move(wirefence);
}

void CSApp::BuildRenderItems()
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

	auto gpuWavesRitem = std::make_unique<RenderItem>();
	gpuWavesRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gpuWavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gpuWavesRitem->DisplacementMapTexelSize.x = 1.0f / m_GpuWaves->ColumnCount();
	gpuWavesRitem->DisplacementMapTexelSize.y = 1.0f / m_GpuWaves->RowCount();
	gpuWavesRitem->GridSpatialStep = m_GpuWaves->SpatialStep();
	gpuWavesRitem->ObjCBIndex = 1;
	gpuWavesRitem->Mat = m_Materials["water"].get();
	gpuWavesRitem->Geo = m_Geometries["gpuWaterGeo"].get();
	gpuWavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gpuWavesRitem->IndexCount = gpuWavesRitem->Geo->DrawArgs["grid"].IndexCount;
	gpuWavesRitem->StartIndexLocation = gpuWavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gpuWavesRitem->BaseVertexLocation = gpuWavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::GpuWaves].push_back(gpuWavesRitem.get());

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridRitem->ObjCBIndex = 2;
	gridRitem->Mat = m_Materials["grass"].get();
	gridRitem->Geo = m_Geometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

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

	m_AllRitems.push_back(std::move(wavesRitem));
	m_AllRitems.push_back(std::move(gpuWavesRitem));
	m_AllRitems.push_back(std::move(gridRitem));
	m_AllRitems.push_back(std::move(boxRitem));
}

void CSApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
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

void CSApp::DrawFullScreenQuad(ID3D12GraphicsCommandList* cmdList)
{
	// use SV_VertexID
	cmdList->IASetVertexBuffers(0, 1, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->DrawInstanced(6, 1, 0, 0);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT> CSApp::GetStaticSamplers()
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

float CSApp::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 CSApp::GetHillsNormal(float x, float z) const
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

void CSApp::LoadTexture(std::string name, std::wstring filename)
{
	auto texture = std::make_unique<Texture>();
	texture->m_Name = name;
	texture->m_Filename = filename;
	HR(DirectX::CreateDDSTextureFromFile12(m_pd3dDevice.Get(), m_CommandList.Get(),
		texture->m_Filename.c_str(), texture->m_Resource, texture->m_UploadHeap));

	m_Textures[name] = std::move(texture);
}
