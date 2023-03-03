#include "StencilApp.h"

StencilApp::StencilApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

StencilApp::StencilApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

StencilApp::~StencilApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool StencilApp::Initialize()
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

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildRoomGeometry();
	BuildSkullGeometry();
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

bool StencilApp::InitResource()
{
	bool bResult = false;

	LoadTexture("bricksTex", L"..\\Textures\\bricks3.dds");
	LoadTexture("checkboardTex", L"..\\Textures\\checkboard.dds");
	LoadTexture("iceTex", L"..\\Textures\\ice.dds");
	LoadTexture("white1x1Tex", L"..\\Textures\\white1x1.dds");

	bResult = true;

	return bResult;
}

void StencilApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_Proj, P);
}

void StencilApp::Update(const GameTimer& timer)
{
	// ImGui
	ImGuiIO& io = ImGui::GetIO();
	
	const float dt = timer.DeltaTime();
	if (ImGui::Begin("Stencil demo"))
	{
		static int curr_mode_item = static_cast<int>(m_CurrMode);
		const char* mode_strs[] = {
			"Wireframe",
			"WithoutStencil",
			"Other"
		};
		if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
		{
			if (curr_mode_item == 0)
			{
				m_CurrMode = ShowMode::Wireframe;
			}
			else if (curr_mode_item == 1) 
			{
				m_CurrMode = ShowMode::WithoutStencil;
			}
			else 
			{
				m_CurrMode = ShowMode::Other;
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

	if (ImGui::IsKeyDown(ImGuiKey_A))
		m_SkullTranslation.x -= 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_D))
		m_SkullTranslation.x += 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_W))
		m_SkullTranslation.y += 1.0f * dt;
	if (ImGui::IsKeyDown(ImGuiKey_S))
		m_SkullTranslation.y -= 1.0f * dt;

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

	UpdateSkull(timer);
	UpdateObjectCBs(timer);
	UpdateMaterialCBs(timer);
	UpdateMainPassCB(timer);
	UpdateReflectedPassCB(timer);
}

void StencilApp::Draw(const GameTimer& timer)
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

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	auto passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());

	// Select different PSO according to different modes 
	ComPtr<ID3D12PipelineState> opaquePSO = m_PSOs["opaque"].Get();
	ComPtr<ID3D12PipelineState> reflectionPSO = m_PSOs["reflection"].Get();
	ComPtr<ID3D12PipelineState> transparentPSO = m_PSOs["transparent"].Get();
	ComPtr<ID3D12PipelineState> shadowPSO = m_PSOs["shadow"].Get();;

	switch (m_CurrMode)
	{
	case ShowMode::Wireframe:
		opaquePSO = m_PSOs["opaque_wireframe"].Get();
		reflectionPSO = m_PSOs["reflection_wireframe"].Get();
		transparentPSO = m_PSOs["transparent_wireframe"].Get();
		shadowPSO = m_PSOs["shadow_wireframe"].Get();
		break;
	case ShowMode::WithoutStencil:
		reflectionPSO = m_PSOs["reflection_WithoutStencil"].Get();
		shadowPSO = m_PSOs["transparent"].Get();
		break;
	case ShowMode::Other:
		break;
	}

	// 1. Draw opaque objects
	m_CommandList->SetPipelineState(opaquePSO.Get());
	DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);

	// 2. Mark mirror area
	m_CommandList->OMSetStencilRef(1);
	m_CommandList->SetPipelineState(m_PSOs["markStencilMirrors"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Mirrors]);

	// 3. Draw reflection into the mirror
	m_CommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress() + 1 * passCBByteSize);
	m_CommandList->SetPipelineState(reflectionPSO.Get());
	DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Reflected]);

	// 3.1. Draw reflect shadow (option)
	m_CommandList->SetPipelineState(shadowPSO.Get());
	DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::ReflectedShadow]);

	// Reset pass constants and stencil ref
	m_CommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());
	m_CommandList->OMSetStencilRef(0);

	// 4. Draw mirror with transparency
	m_CommandList->SetPipelineState(transparentPSO.Get());
	DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Transparent]);

	// 5. Draw shadow
	m_CommandList->SetPipelineState(shadowPSO.Get());
	DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Shadow]);

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

void StencilApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void StencilApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void StencilApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void StencilApp::UpdateCamera(const GameTimer& gt)
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

void StencilApp::UpdateObjectCBs(const GameTimer& gt)
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

void StencilApp::UpdateSkull(const GameTimer& gt)
{
	m_SkullTranslation.y = MathHelper::Max(m_SkullTranslation.y, 0.0f);

	XMMATRIX skullRotate = XMMatrixRotationY(0.5 * MathHelper::Pi);
	XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	XMMATRIX skullOffset = XMMatrixTranslation(m_SkullTranslation.x, m_SkullTranslation.y, m_SkullTranslation.z);
	XMMATRIX skullWorld = skullScale * skullRotate * skullOffset;
	XMStoreFloat4x4(&m_SkullRitem->World, skullWorld);

	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	XMMATRIX R = XMMatrixReflect(mirrorPlane);
	XMStoreFloat4x4(&m_ReflectedSkullRitem->World, skullWorld * R);

	XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	XMVECTOR toMainLight = -XMLoadFloat3(&m_MainPassCB.Lights[0].m_Direction);
	XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
	XMMATRIX shadowOffsetY = XMMatrixTranslation(0.0f, 0.001f, 0.0f);
	XMStoreFloat4x4(&m_ShadowedSkullRitem->World, skullWorld * S * shadowOffsetY);

	XMStoreFloat4x4(&m_ReflectedShadowedSkullRitem->World, skullWorld * S * shadowOffsetY * R);

	m_SkullRitem->NumFramesDirty = g_numFrameResources;
	m_ReflectedSkullRitem->NumFramesDirty = g_numFrameResources;
	m_ShadowedSkullRitem->NumFramesDirty = g_numFrameResources;
	m_ReflectedShadowedSkullRitem->NumFramesDirty = g_numFrameResources;
}

void StencilApp::AnimateMaterials(const GameTimer& gt)
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

void StencilApp::UpdateMaterialCBs(const GameTimer& gt)
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

void StencilApp::UpdateMainPassCB(const GameTimer& gt)
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
	m_MainPassCB.Lights[0].m_Strength = { 0.6f, 0.6f, 0.6f };
	m_MainPassCB.Lights[1].m_Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[1].m_Strength = { 0.3f, 0.3f, 0.3f };
	m_MainPassCB.Lights[2].m_Direction = { 0.0f, -0.707f, -0.707f };
	m_MainPassCB.Lights[2].m_Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void StencilApp::UpdateReflectedPassCB(const GameTimer& gt)
{
	m_ReflectedPassCB = m_MainPassCB;

	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX R = XMMatrixReflect(mirrorPlane);

	// update light
	for (int i = 0; i < 3; ++i) 
	{
		XMVECTOR lightDir = XMLoadFloat3(&m_MainPassCB.Lights[i].m_Direction);
		XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&m_ReflectedPassCB.Lights[i].m_Direction, reflectedLightDir);
	}

	auto currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, m_ReflectedPassCB);
}

void StencilApp::BuildRootSignature()
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

void StencilApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	ComPtr<ID3D12Resource> bricksTex;
	ComPtr<ID3D12Resource> checkboardTex;
	ComPtr<ID3D12Resource> iceTex;
	ComPtr<ID3D12Resource> white1x1Tex;

	// SRV Heap
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	bricksTex = m_Textures["bricksTex"]->m_Resource;
	checkboardTex = m_Textures["checkboardTex"]->m_Resource;
	iceTex = m_Textures["iceTex"]->m_Resource;
	white1x1Tex = m_Textures["white1x1Tex"]->m_Resource;

	srvDesc.Format = bricksTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	m_pd3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = checkboardTex->GetDesc().Format;
	m_pd3dDevice->CreateShaderResourceView(checkboardTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = iceTex->GetDesc().Format;
	m_pd3dDevice->CreateShaderResourceView(iceTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.Format = white1x1Tex->GetDesc().Format;
	m_pd3dDevice->CreateShaderResourceView(white1x1Tex.Get(), &srvDesc, hDescriptor);
}

void StencilApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines [] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter11\\Default.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"]		= d3dUtil::CompileShader(L"..\\Shader\\Chapter11\\Default.hlsl", defines, "PS", "ps_5_1");
	m_Shaders["alphaTestedPS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter11\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	m_InputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL"  ,0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD"  ,0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
}

void StencilApp::BuildRoomGeometry()
{
	//   |--------------|
	//   |              |
	//   |----|----|----|
	//   |Wall|Mirr|Wall|
	//   |    | or |    |
	//   /--------------/
	//  /   Floor      /
	// /--------------/

	std::array<Vertex, 20> vertices =
	{
		// Floor: Observe we tile texture coordinates.
		Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
		Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
		Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

		// Wall: Observe we tile texture coordinates, and that we
		// leave a gap in the middle for the mirror.
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		// Mirror
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};

	std::array<std::int16_t, 30> indices =
	{
		// Floor
		0, 1, 2,
		0, 2, 3,

		// Walls
		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		// Mirror
		16, 17, 18,
		16, 18, 19
	};


	SubmeshGeometry floorSubmesh;
	floorSubmesh.IndexCount = 6;
	floorSubmesh.StartIndexLocation = 0;
	floorSubmesh.BaseVertexLocation = 0;

	SubmeshGeometry wallSubmesh;
	wallSubmesh.IndexCount = 18;
	wallSubmesh.StartIndexLocation = 6;
	wallSubmesh.BaseVertexLocation = 0;

	SubmeshGeometry mirrorSubmesh;
	mirrorSubmesh.IndexCount = 6;
	mirrorSubmesh.StartIndexLocation = 24;
	mirrorSubmesh.BaseVertexLocation = 0;

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "roomGeo";

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

	geo->DrawArgs["floor"] = floorSubmesh;
	geo->DrawArgs["wall"] = wallSubmesh;
	geo->DrawArgs["mirror"] = mirrorSubmesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void StencilApp::BuildSkullGeometry()
{
	std::vector<Vertex> vertices(1);
	std::vector<std::uint16_t> indices;

	ReadDataFromFile(vertices, indices);

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skullGeo";

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

	geo->DrawArgs["skull"] = submesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void StencilApp::BuildPSOs()
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

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentWireframePsoDesc = transparentPsoDesc;
	transparentWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&transparentWireframePsoDesc, IID_PPV_ARGS(&m_PSOs["transparent_wireframe"])));

	// mirror
	CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
	mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

	D3D12_DEPTH_STENCIL_DESC mirrorDSS;
	mirrorDSS.DepthEnable = true;
	mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	mirrorDSS.StencilEnable = true;
	mirrorDSS.StencilReadMask = 0xff;
	mirrorDSS.StencilWriteMask = 0xff;
	mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPsoDesc = opaquePsoDesc;
	markMirrorsPsoDesc.BlendState = mirrorBlendState;
	markMirrorsPsoDesc.DepthStencilState = mirrorDSS;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&m_PSOs["markStencilMirrors"])));

	// reflection
	D3D12_GRAPHICS_PIPELINE_STATE_DESC reflectionNoStencilPsoDesc = opaquePsoDesc;
	reflectionNoStencilPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	reflectionNoStencilPsoDesc.RasterizerState.FrontCounterClockwise = true;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&reflectionNoStencilPsoDesc, IID_PPV_ARGS(&m_PSOs["reflection_WithoutStencil"])));

	D3D12_DEPTH_STENCIL_DESC reflectionsDSS;
	reflectionsDSS.DepthEnable = true;
	reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	reflectionsDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	reflectionsDSS.StencilEnable = true;
	reflectionsDSS.StencilReadMask = 0xff;
	reflectionsDSS.StencilWriteMask = 0xff;
	reflectionsDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	reflectionsDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC reflectionPsoDesc = opaquePsoDesc;
	reflectionPsoDesc.DepthStencilState = reflectionsDSS;
	reflectionPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	reflectionPsoDesc.RasterizerState.FrontCounterClockwise = true;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&reflectionPsoDesc, IID_PPV_ARGS(&m_PSOs["reflection"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC reflectionWireframePsoDesc = opaquePsoDesc;
	reflectionWireframePsoDesc.DepthStencilState = reflectionsDSS;
	reflectionWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	reflectionWireframePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	reflectionWireframePsoDesc.RasterizerState.FrontCounterClockwise = true;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&reflectionWireframePsoDesc, IID_PPV_ARGS(&m_PSOs["reflection_wireframe"])));

	// shadow
	D3D12_DEPTH_STENCIL_DESC shadowDSS;
	shadowDSS.DepthEnable = true;
	shadowDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadowDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	shadowDSS.StencilEnable = true;
	shadowDSS.StencilReadMask = 0xff;
	shadowDSS.StencilWriteMask = 0xff;
	shadowDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	shadowDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = transparentPsoDesc;
	shadowPsoDesc.DepthStencilState = shadowDSS;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&m_PSOs["shadow"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowWireframePsoDesc = shadowPsoDesc;
	shadowWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&m_PSOs["shadow_wireframe"])));
}

void StencilApp::BuildFrameResources()
{
	for(int i = 0;i<g_numFrameResources;++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get(),
			2, (UINT)m_AllRitems.size(), (UINT)m_Materials.size(), m_Waves->GetVertexCount()));
	}
}

void StencilApp::BuildMaterials()
{
	auto bricks = std::make_unique<Material>();
	bricks->m_Name = "bricks";
	bricks->m_MatCBIndex = 0;
	bricks->m_DiffuseSrvHeapIndex = 0;
	bricks->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->m_FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	bricks->m_Roughness = 0.25f;

	auto checkertile = std::make_unique<Material>();
	checkertile->m_Name = "checkertile";
	checkertile->m_MatCBIndex = 1;
	checkertile->m_DiffuseSrvHeapIndex = 1;
	checkertile->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	checkertile->m_FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	checkertile->m_Roughness = 0.3f;

	auto icemirror = std::make_unique<Material>();
	icemirror->m_Name = "icemirror";
	icemirror->m_MatCBIndex = 2;
	icemirror->m_DiffuseSrvHeapIndex = 2;
	icemirror->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	icemirror->m_FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	icemirror->m_Roughness = 0.5f;

	auto skullMat = std::make_unique<Material>();
	skullMat->m_Name = "skullMat";
	skullMat->m_MatCBIndex = 3;
	skullMat->m_DiffuseSrvHeapIndex = 3;
	skullMat->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->m_FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->m_Roughness = 0.3f;

	auto shadowMat = std::make_unique<Material>();
	shadowMat->m_Name = "shadowMat";
	shadowMat->m_MatCBIndex = 4;
	shadowMat->m_DiffuseSrvHeapIndex = 3;
	shadowMat->m_DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadowMat->m_FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadowMat->m_Roughness = 0.0f;

	m_Materials["brick"] = std::move(bricks);
	m_Materials["checkertile"] = std::move(checkertile);
	m_Materials["icemirror"] = std::move(icemirror);
	m_Materials["skullMat"] = std::move(skullMat);
	m_Materials["shadowMat"] = std::move(shadowMat);
}

void StencilApp::BuildRenderItems()
{
	auto floorRitem = std::make_unique<RenderItem>();
	floorRitem->World = MathHelper::Identity4x4();
	floorRitem->TexTransform = MathHelper::Identity4x4();
	floorRitem->ObjCBIndex = 0;
	floorRitem->Mat = m_Materials["checkertile"].get();
	floorRitem->Geo = m_Geometries["roomGeo"].get();
	floorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	floorRitem->IndexCount = floorRitem->Geo->DrawArgs["floor"].IndexCount;
	floorRitem->StartIndexLocation = floorRitem->Geo->DrawArgs["floor"].StartIndexLocation;
	floorRitem->BaseVertexLocation = floorRitem->Geo->DrawArgs["floor"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Opaque].push_back(floorRitem.get());

	auto wallRitem = std::make_unique<RenderItem>();
	wallRitem->World = MathHelper::Identity4x4();
	wallRitem->TexTransform = MathHelper::Identity4x4();
	wallRitem->ObjCBIndex = 1;
	wallRitem->Mat = m_Materials["brick"].get();
	wallRitem->Geo = m_Geometries["roomGeo"].get();
	wallRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wallRitem->IndexCount = wallRitem->Geo->DrawArgs["wall"].IndexCount;
	wallRitem->StartIndexLocation = wallRitem->Geo->DrawArgs["wall"].StartIndexLocation;
	wallRitem->BaseVertexLocation = wallRitem->Geo->DrawArgs["wall"].BaseVertexLocation;

	m_WallRitem = wallRitem.get();
	m_RitemLayer[(int)RenderLayer::Opaque].push_back(wallRitem.get());	

	auto skullRitem = std::make_unique<RenderItem>();
	skullRitem->World = MathHelper::Identity4x4();
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjCBIndex = 2;
	skullRitem->Mat = m_Materials["skullMat"].get();
	skullRitem->Geo = m_Geometries["skullGeo"].get();
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;

	m_SkullRitem = skullRitem.get();
	m_RitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());

	auto reflectedFloorRitem = std::make_unique<RenderItem>();
	*reflectedFloorRitem = *floorRitem;
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	XMMATRIX R = XMMatrixReflect(mirrorPlane);
	XMStoreFloat4x4(&reflectedFloorRitem->World,  R);
	reflectedFloorRitem->ObjCBIndex = 3;
	m_RitemLayer[(int)RenderLayer::Reflected].push_back(reflectedFloorRitem.get());

	auto reflectedSkullRitem = std::make_unique<RenderItem>();
	*reflectedSkullRitem = *skullRitem;
	reflectedSkullRitem->ObjCBIndex = 4;
	m_ReflectedSkullRitem = reflectedSkullRitem.get();
	m_RitemLayer[(int)RenderLayer::Reflected].push_back(reflectedSkullRitem.get());

	auto reflectedShadowedSkullRitem = std::make_unique<RenderItem>();
	*reflectedShadowedSkullRitem = *skullRitem;
	XMStoreFloat4x4(&reflectedShadowedSkullRitem->World, R);
	reflectedShadowedSkullRitem->ObjCBIndex = 5;
	reflectedShadowedSkullRitem->Mat = m_Materials["shadowMat"].get();
	m_ReflectedShadowedSkullRitem = reflectedShadowedSkullRitem.get();
	m_RitemLayer[(int)RenderLayer::ReflectedShadow].push_back(reflectedShadowedSkullRitem.get());

	auto shadowedSkullRitem = std::make_unique<RenderItem>();
	*shadowedSkullRitem = *skullRitem;
	shadowedSkullRitem->ObjCBIndex = 6;
	shadowedSkullRitem->Mat = m_Materials["shadowMat"].get();
	m_ShadowedSkullRitem = shadowedSkullRitem.get();
	m_RitemLayer[(int)RenderLayer::Shadow].push_back(shadowedSkullRitem.get());

	auto mirrorRitem = std::make_unique<RenderItem>();
	mirrorRitem->World = MathHelper::Identity4x4();
	mirrorRitem->TexTransform = MathHelper::Identity4x4();
	mirrorRitem->ObjCBIndex = 7;
	mirrorRitem->Mat = m_Materials["icemirror"].get();
	mirrorRitem->Geo = m_Geometries["roomGeo"].get();
	mirrorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorRitem->IndexCount = mirrorRitem->Geo->DrawArgs["mirror"].IndexCount;
	mirrorRitem->StartIndexLocation = mirrorRitem->Geo->DrawArgs["mirror"].StartIndexLocation;
	mirrorRitem->BaseVertexLocation = mirrorRitem->Geo->DrawArgs["mirror"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Mirrors].push_back(mirrorRitem.get());
	m_RitemLayer[(int)RenderLayer::Transparent].push_back(mirrorRitem.get());

	m_AllRitems.push_back(std::move(floorRitem));
	m_AllRitems.push_back(std::move(wallRitem));
	m_AllRitems.push_back(std::move(skullRitem));
	m_AllRitems.push_back(std::move(reflectedFloorRitem));
	m_AllRitems.push_back(std::move(reflectedSkullRitem));
	m_AllRitems.push_back(std::move(reflectedShadowedSkullRitem));
	m_AllRitems.push_back(std::move(shadowedSkullRitem));
	m_AllRitems.push_back(std::move(mirrorRitem));
}

void StencilApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT> StencilApp::GetStaticSamplers()
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

void StencilApp::LoadTexture(std::string name, std::wstring filename)
{
	auto texture = std::make_unique<Texture>();
	texture->m_Name = name;
	texture->m_Filename = filename;
	HR(DirectX::CreateDDSTextureFromFile12(m_pd3dDevice.Get(), m_CommandList.Get(),
		texture->m_Filename.c_str(), texture->m_Resource, texture->m_UploadHeap));

	m_Textures[name] = std::move(texture);
}

void StencilApp::ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
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
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
		vertices[i].TexC = { 0.0f,0.0f };
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
