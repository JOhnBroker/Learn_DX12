#include "GameApp.h"

GameApp::GameApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

GameApp::GameApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

GameApp::~GameApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool GameApp::Initialize()
{
	if (!D3DApp::Initialize()) 
	{
		return false;
	}

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	if (!InitResource()) 
	{
		return false;
	}

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildCubeDepthStencil();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
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

	return true;
}

bool GameApp::InitResource()
{
	bool bResult = false;

	if (m_CameraMode == CameraMode::FirstPerson)
	{
		auto camera = std::make_shared<FirstPersonCamera>();
		m_pCamera = camera;
		camera->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		//camera->LookAt(XMFLOAT3(0.0f, 2.0f, -15.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
		camera->LookAt(
			XMFLOAT3(0.0f, 4.0f, -5.0f),
			XMFLOAT3(0.0f, 1.0f, 0.0f),
			XMFLOAT3(0.0f, 1.0f, 0.0f));
		BuildCubeFaceCamera(0.0f, 2.5f, 0.0f);
	}
	else if (m_CameraMode == CameraMode::ThirdPerson)
	{
		auto camera = std::make_shared<ThirdPersonCamera>();
		m_pCamera = camera;
		camera->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		camera->SetTarget(XMFLOAT3(0.0f, 0.0f, 1.0f));
		camera->SetDistance(10.0f);
		camera->SetDistanceMinMax(3.0f, 20.0f);
	}

	m_DynamicCubeMap = std::make_unique<CubeRenderTarget>(m_pd3dDevice.Get(),
		CUBEMAP_SIZE, CUBEMAP_SIZE, DXGI_FORMAT_R8G8B8A8_UNORM);

	LoadTexture("bricksTex", L"..\\Textures\\bricks2.dds");
	LoadTexture("bricksNorTex", L"..\\Textures\\bricks2_nmap.dds");
	LoadTexture("tileTex", L"..\\Textures\\tile.dds");
	LoadTexture("tileNorTex", L"..\\Textures\\tile_nmap.dds");
	LoadTexture("whiteTex", L"..\\Textures\\white1x1.dds");
	LoadTexture("whiteNorTex", L"..\\Textures\\default_nmap.dds");
	LoadTexture("grassCube", L"..\\Textures\\grasscube1024.dds");
	LoadTexture("snowCube", L"..\\Textures\\snowcube1024.dds");
	LoadTexture("sunsetCube", L"..\\Textures\\sunset1024.dds");

	bResult = true;

	return bResult;
}

void GameApp::CreateRTVAndDSVDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 6;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	HR(m_pd3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(m_RTVHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	HR(m_pd3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(m_DSVHeap.GetAddressOf())));

	m_CubeDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DSVHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_DSVDescriptorSize);

	// 为ImGui创建SRV堆
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	HR(m_pd3dDevice->CreateDescriptorHeap(
		&srvHeapDesc, IID_PPV_ARGS(m_SRVHeap.GetAddressOf())));
}

void GameApp::OnResize()
{
	D3DApp::OnResize();

	if (m_pCamera != nullptr) 
	{
		m_pCamera->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	}
}

void GameApp::Update(const GameTimer& timer)
{
	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);
	UpdateCamera(timer);

	// animate skull

	XMMATRIX skullScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	XMMATRIX skullOffset = XMMatrixTranslation(3.0f, 2.0f, 0.0f);
	XMMATRIX skullLocalRotate = XMMatrixRotationY(2.0f * timer.TotalTime());
	XMMATRIX skullGlobalRotate = XMMatrixRotationY(0.5f * timer.TotalTime());
	XMStoreFloat4x4(&m_SkullRitem->World, skullScale * skullLocalRotate * skullOffset * skullGlobalRotate);
	m_SkullRitem->NumFramesDirty = g_numFrameResources;

	// ImGui
	ImGuiIO& io = ImGui::GetIO();
	
	const float dt = timer.DeltaTime();
	if (ImGui::Begin("NormalMap demo"))
	{
		ImGui::Checkbox("Wireframe", &m_WireframeEnable);

		static int curr_cameramode = static_cast<int>(m_CameraMode);
		static const char* cameraMode[] = {
				"First Person",
				"Third Person",
		};
		if (ImGui::Combo("Camera Mode", &curr_cameramode, cameraMode, ARRAYSIZE(cameraMode)))
		{
			if (curr_cameramode == 0 && m_CameraMode != CameraMode::FirstPerson)
			{
				if (!cam1st)
				{
					cam1st = std::make_shared<FirstPersonCamera>();
					cam1st->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
					m_pCamera = cam1st;
				}

				cam1st->LookAt(XMFLOAT3(0.0f, 2.0f, -15.0f),
					XMFLOAT3(0.0f, 0.0f, -1.0f),
					XMFLOAT3(0.0f, 1.0f, 0.0f));
				cam1st->UpdateViewMatrix();

				m_CameraMode = CameraMode::FirstPerson;
			}
			else if (curr_cameramode == 1 && m_CameraMode != CameraMode::ThirdPerson)
			{
				if (!cam3rd)
				{
					cam3rd = std::make_shared<ThirdPersonCamera>();
					cam3rd->SetFrustum(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
					m_pCamera = cam3rd;
				}
				cam3rd->SetTarget(XMFLOAT3(0.0f, 0.0f, 1.0f));
				cam3rd->SetDistance(10.0f);
				cam3rd->SetDistanceMinMax(3.0f, 20.0f);
				cam3rd->UpdateViewMatrix();
				m_CameraMode = CameraMode::ThirdPerson;
			}
		}
		
		static int curr_showmode = static_cast<int>(m_ShowMode);
		static const char* showMode[] = {
				"Reflection",
				"Refraction",
		};
		if (ImGui::Combo("Show Mode", &curr_showmode, showMode, ARRAYSIZE(showMode)))
		{
			if (curr_showmode == 0 && m_ShowMode != ShowMode::Reflection)
			{
				m_ShowMode = ShowMode::Reflection;
				// TODO
			}
			else if (curr_showmode == 1 && m_ShowMode != ShowMode::Refraction) 
			{
				m_ShowMode = ShowMode::Refraction;
				// TODO
			}
		}
		static int curr_skymode = static_cast<int>(m_SkyMode);
		static const char* skyMode[] = {
				"StaticSky",
				"DynamicSky",
		};
		if (ImGui::Combo("Sky Mode", &curr_skymode, skyMode, ARRAYSIZE(skyMode)))
		{
			if (curr_skymode == 0 && m_SkyMode != SkyMode::StaticSky)
			{
				m_SkyMode = SkyMode::StaticSky;
			}
			else if (curr_skymode == 1 && m_SkyMode != SkyMode::DynamicSky)
			{
				m_SkyMode = SkyMode::DynamicSky;
			}
		}
		static int curr_skyCubemode = max(0, static_cast<int>(m_SkyTexHeapIndex) - 6);
		static const char* skyCubeMode[] = {
				"grass",
				"snow",
				"sunset"
		};
		if (ImGui::Combo("SkyCube", &curr_skyCubemode, skyCubeMode, ARRAYSIZE(skyCubeMode)))
		{
			if (curr_skyCubemode == 0 )
			{
				m_SkyTexHeapIndex = 6;
			}
			else if (curr_skyCubemode == 1)
			{
				m_SkyTexHeapIndex = 7;
			}
			else if (curr_skyCubemode == 2) 
			{
				m_SkyTexHeapIndex = 8;
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
	UpdateMaterialBuffer(timer);
	UpdateMainPassCB(timer);
}

void GameApp::Draw(const GameTimer& timer)
{
	auto cmdListAlloc = m_CurrFrameResource->CmdListAlloc;

	HR(cmdListAlloc->Reset());

	if (m_WireframeEnable)
	{
		HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque_wireframe"].Get()));
	}
	else 
	{
		HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));
	}

	ID3D12DescriptorHeap* descriptorHeap[] = { m_SrvDescriptorHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeap), descriptorHeap);
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	auto matBuffer = m_CurrFrameResource->MaterialBuffer->Resource();
	m_CommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(m_SkyTexHeapIndex, m_CBVSRVDescriptorSize);
	m_CommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	m_CommandList->SetGraphicsRootDescriptorTable(4, m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	if (m_SkyMode == SkyMode::DynamicSky && m_CameraMode == CameraMode::FirstPerson) 
	{
		// draw dynamic skybox
		DrawSceneToCubeMap();
	}

	m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_CommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	auto passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
	
	if (m_ShowMode == ShowMode::Refraction)
	{
		m_CommandList->SetPipelineState(m_PSOs["refraction"].Get());
	}
	if (m_SkyMode == SkyMode::DynamicSky && m_CameraMode == CameraMode::FirstPerson)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE dynamicTexDescriptor(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		dynamicTexDescriptor.Offset(m_DynamicSkyTexHeapIndex, m_CBVSRVDescriptorSize);
		m_CommandList->SetGraphicsRootDescriptorTable(3, dynamicTexDescriptor);
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::DynamicSky]);
	}
	else 
	{
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::StaticSky]);
	}
	DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);

	m_CommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);
	m_CommandList->SetPipelineState(m_PSOs["sky"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Sky]);

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

void GameApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_RBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_LastMousePos.y));

		//m_Camera.Pitch(dx);
		//m_Camera.RotateY(dy);
	}
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void GameApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void GameApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void GameApp::UpdateCamera(const GameTimer& gt)
{
	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

	ImGuiIO& io = ImGui::GetIO();
	if (m_CameraMode == CameraMode::FirstPerson) 
	{
		float d1 = 0.0f, d2 = 0.0f;
		if (ImGui::IsKeyDown(ImGuiKey_W))
			d1 += gt.DeltaTime();
		if (ImGui::IsKeyDown(ImGuiKey_S))
			d1 -= gt.DeltaTime();
		if (ImGui::IsKeyDown(ImGuiKey_A))
			d2 -= gt.DeltaTime();
		if (ImGui::IsKeyDown(ImGuiKey_D))
			d2 += gt.DeltaTime();

		cam1st->Walk(d1 * 6.0f);
		cam1st->Strafe(d2 * 6.0f);

		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
		{
			cam1st->Pitch(io.MouseDelta.y * 0.01f);
			cam1st->RotateY(io.MouseDelta.x * 0.01f);
		}
	}
	else if (m_CameraMode == CameraMode::ThirdPerson) 
	{
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
		{
			cam3rd->RotateX(io.MouseDelta.y * 0.01f);
			cam3rd->RotateY(io.MouseDelta.x * 0.01f);
		}
		cam3rd->Approach(-io.MouseWheel * 1.0f);
	}
	m_pCamera->UpdateViewMatrix();
}

void GameApp::UpdateObjectCBs(const GameTimer& gt)
{	
	auto currObjectCB = m_CurrFrameResource->ObjectCB.get();
	for (auto& obj : m_AllRitems) 
	{
		if (obj->NumFramesDirty > 0) 
		{
			XMMATRIX world = XMLoadFloat4x4(&obj->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&obj->TexTransform);

			ObjectConstants objConstant;
			XMStoreFloat4x4(&objConstant.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstant.TexTransform, XMMatrixTranspose(texTransform));
			objConstant.MaterialIndex = obj->Mat->m_MatCBIndex;

			currObjectCB->CopyData(obj->ObjCBIndex, objConstant);

			obj->NumFramesDirty--;
		}
	}
}

void GameApp::UpdateMaterialBuffer(const GameTimer& gt)
{
	auto currMaterialCB = m_CurrFrameResource->MaterialBuffer.get();
	for (auto& pair : m_Materials) 
	{
		Material* mat = pair.second.get();
		if (mat->m_NumFramesDirty > 0) 
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->m_MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->m_DiffuseAlbedo;
			matData.FresnelR0 = mat->m_FresnelR0;
			matData.Roughness = mat->m_Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->m_DiffuseSrvHeapIndex;
			matData.NormalMapIndex = mat->m_NormalSrvHeapIndex;
			matData.Eta = mat->m_Eta;

			currMaterialCB->CopyData(mat->m_MatCBIndex, matData);

			mat->m_NumFramesDirty--;
		}
	}
}

void GameApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX skyboxWorld = XMMatrixScaling(m_SkyBoxScale.x, m_SkyBoxScale.y, m_SkyBoxScale.z);
	XMVECTOR dSkyboxWorld = XMMatrixDeterminant(skyboxWorld);
	XMMATRIX invSkyboxWorld = XMMatrixInverse(&dSkyboxWorld, skyboxWorld);
	XMMATRIX view = m_pCamera->GetViewXM();
	XMMATRIX proj = m_pCamera->GetProjXM();

	XMMATRIX viewproj = XMMatrixMultiply(view, proj);
	XMVECTOR dView = XMMatrixDeterminant(view);
	XMVECTOR dProj = XMMatrixDeterminant(proj);
	XMVECTOR dViewProj = XMMatrixDeterminant(viewproj);
	XMMATRIX invView = XMMatrixInverse(&dView, view);
	XMMATRIX invProj = XMMatrixInverse(&dProj, proj);
	XMMATRIX invViewproj = XMMatrixInverse(&dViewProj, viewproj);

	XMStoreFloat4x4(&m_MainPassCB.InvSkyBoxWorld, XMMatrixTranspose(invSkyboxWorld));
	XMStoreFloat4x4(&m_MainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_MainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_MainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&m_MainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&m_MainPassCB.ViewProj, XMMatrixTranspose(viewproj));
	XMStoreFloat4x4(&m_MainPassCB.InvViewProj, XMMatrixTranspose(invViewproj));
	m_MainPassCB.EyePosW = m_pCamera->GetPosition();
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_ClientWidth, 1.0f / m_ClientHeight);
	m_MainPassCB.NearZ = 1.0f;
	m_MainPassCB.FarZ = 1000.0f;
	m_MainPassCB.TotalTime = gt.TotalTime();
	m_MainPassCB.DeltaTime = gt.DeltaTime();
	//m_MainPassCB.SkyboxExtents = m_SkyBoxScale;
	m_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	m_MainPassCB.Lights[0].m_Direction = { 0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[0].m_Strength = { 0.8f, 0.8f, 0.8f };
	m_MainPassCB.Lights[1].m_Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[1].m_Strength = { 0.4f, 0.4f, 0.4f };
	m_MainPassCB.Lights[2].m_Direction = { 0.0f, -0.707f, -0.707f };
	m_MainPassCB.Lights[2].m_Strength = { 0.2f, 0.2f, 0.2f };
	
	auto currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);

	UpdateCubeMapFacePassCBs();
}

void GameApp::UpdateCubeMapFacePassCBs()
{
	// 指定位置
	float x = 0.0f, y = 2.5f, z = 0.0f;
	XMFLOAT3 center(x, y, z);
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);

	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f,y,z),	//+x
		XMFLOAT3(x - 1.0f,y,z),	//-x
		XMFLOAT3(x,y + 1.0f,z),	//+y
		XMFLOAT3(x,y - 1.0f,z),	//-y
		XMFLOAT3(x,y,z + 1.0f),	//+z
		XMFLOAT3(x,y,z - 1.0f) 	//-z
	};

	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f,1.0f,0.0f),	 //+x
		XMFLOAT3(0.0f,1.0f,0.0f),	 //-x
		XMFLOAT3(0.0f,0.0f,-1.0f),	 //+y
		XMFLOAT3(0.0f,0.0f,+1.0f),	 //-y
		XMFLOAT3(0.0f,1.0f,0.0f),	 //+z
		XMFLOAT3(0.0f,1.0f,0.0f) 	 //-z
	};

	for (int i = 0; i < 6; ++i) 
	{
		m_CubeCamera = std::make_shared<FirstPersonCamera>();
		m_CubeCamera->SetFrustum(0.5f * MathHelper::Pi, 1.0f, 0.1f, 1000.0f);
		m_CubeCamera->LookAt(center, targets[i], ups[i]);
		m_CubeCamera->UpdateViewMatrix();

		PassConstants cubeFacePassCB = m_MainPassCB;
		XMMATRIX view = m_CubeCamera->GetViewXM();
		XMMATRIX proj = m_CubeCamera->GetProjXM();

		XMMATRIX viewproj = XMMatrixMultiply(view, proj);
		XMVECTOR dView = XMMatrixDeterminant(view);
		XMVECTOR dProj = XMMatrixDeterminant(proj);
		XMVECTOR dViewProj = XMMatrixDeterminant(viewproj);
		XMMATRIX invView = XMMatrixInverse(&dView, view);
		XMMATRIX invProj = XMMatrixInverse(&dProj, proj);
		XMMATRIX invViewproj = XMMatrixInverse(&dViewProj, viewproj);

		XMStoreFloat4x4(&cubeFacePassCB.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&cubeFacePassCB.Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&cubeFacePassCB.InvView, XMMatrixTranspose(invView));
		XMStoreFloat4x4(&cubeFacePassCB.InvProj, XMMatrixTranspose(invProj));
		XMStoreFloat4x4(&cubeFacePassCB.ViewProj, XMMatrixTranspose(viewproj));
		XMStoreFloat4x4(&cubeFacePassCB.InvViewProj, XMMatrixTranspose(invViewproj));
		cubeFacePassCB.EyePosW = m_CubeCamera->GetPosition();
		cubeFacePassCB.RenderTargetSize = XMFLOAT2((float)CUBEMAP_SIZE, (float)CUBEMAP_SIZE);
		cubeFacePassCB.InvRenderTargetSize = XMFLOAT2(1.0f / CUBEMAP_SIZE, 1.0f / CUBEMAP_SIZE);

		auto currPassCB = m_CurrFrameResource->PassCB.get();
		currPassCB->CopyData(1 + i, cubeFacePassCB);
	}
}

void GameApp::BuildRootSignature()
{
	HRESULT hReturn = E_FAIL;
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	// 创建根描述符表
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

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

void GameApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 10;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	std::vector<ComPtr<ID3D12Resource>> tex2DList =
	{
		m_Textures["bricksTex"]->m_Resource,
		m_Textures["bricksNorTex"]->m_Resource,
		m_Textures["tileTex"]->m_Resource,
		m_Textures["tileNorTex"]->m_Resource,
		m_Textures["whiteTex"]->m_Resource,
		m_Textures["whiteNorTex"]->m_Resource,
	};
	auto grassSkyMap = m_Textures["grassCube"]->m_Resource;
	auto snowSkyMap = m_Textures["snowCube"]->m_Resource;
	auto sunsetSkyMap = m_Textures["sunsetCube"]->m_Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0;

	for (UINT i = 0; i < (UINT)tex2DList.size(); ++i) 
	{
		srvDesc.Format = tex2DList[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
		m_pd3dDevice->CreateShaderResourceView(tex2DList[i].Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	}

	m_SkyTexHeapIndex = 6;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = grassSkyMap->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = grassSkyMap->GetDesc().Format;
	m_pd3dDevice->CreateShaderResourceView(grassSkyMap.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.TextureCube.MipLevels = snowSkyMap->GetDesc().MipLevels;
	srvDesc.Format = snowSkyMap->GetDesc().Format;
	m_pd3dDevice->CreateShaderResourceView(snowSkyMap.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, m_CBVSRVDescriptorSize);
	srvDesc.TextureCube.MipLevels = sunsetSkyMap->GetDesc().MipLevels;
	srvDesc.Format = sunsetSkyMap->GetDesc().Format;
	m_pd3dDevice->CreateShaderResourceView(sunsetSkyMap.Get(), &srvDesc, hDescriptor);

	m_DynamicSkyTexHeapIndex = m_SkyTexHeapIndex + 3;

	auto srvCpuStart = m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto rtvCpuStart = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();

	int rtvOffset = SwapChainBufferCount;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cubeRtvHandles[6];
	for (int i = 0; i < 6; ++i) 
	{
		cubeRtvHandles[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, rtvOffset + i, m_RTVDescriptorSize);
	}
	m_DynamicCubeMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, m_DynamicSkyTexHeapIndex, m_CBVSRVDescriptorSize)
		, CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, m_DynamicSkyTexHeapIndex, m_CBVSRVDescriptorSize),
		cubeRtvHandles);

}

void GameApp::BuildCubeDepthStencil()
{
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = CUBEMAP_SIZE;
	depthStencilDesc.Height = CUBEMAP_SIZE;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = m_DepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = m_DepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	HR(m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(m_CubeDepthStencilBuffer.GetAddressOf())));

	m_pd3dDevice->CreateDepthStencilView(m_CubeDepthStencilBuffer.Get(), nullptr, m_CubeDSV);

	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_CubeDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void GameApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO refractDefines[] =
	{
		"REFRACT", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter19\\Default.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["reflectionPS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter19\\Default.hlsl", nullptr, "PS", "ps_5_1");
	m_Shaders["refractionPS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter19\\Default.hlsl", refractDefines, "PS", "ps_5_1");
	m_Shaders["skyVS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter19\\Default.hlsl", nullptr, "Sky_VS", "vs_5_1");
	m_Shaders["skyPS"] = d3dUtil::CompileShader(L"..\\Shader\\Chapter19\\Default.hlsl", nullptr, "Sky_PS", "ps_5_1");

	m_InputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL"  ,0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
}

void GameApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 0);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(10.0f, 10.0f, 20, 20);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.5f, 2.0f, 20, 20);

	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.BaseVertexLocation = boxVertexOffset;
	boxSubmesh.StartIndexLocation = boxIndexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.BaseVertexLocation = gridVertexOffset;
	gridSubmesh.StartIndexLocation = gridIndexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);
	
	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) 
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}
	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}
	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}
	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	HR(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vertices.size());

	HR(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), indices.size());

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
		m_CommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(m_pd3dDevice.Get(),
		m_CommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["column"] = cylinderSubmesh;
	
	m_Geometries[geo->Name] = std::move(geo);

}

void GameApp::BuildSkullGeometry()
{
	std::vector<Vertex> vertices(1);
	std::vector<std::int32_t> indices;
	BoundingBox bounds;

	// read from file 
	ReadDataFromFile(vertices, indices, bounds);

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	submesh.Bounds = bounds;

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
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["skull"] = submesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void GameApp::BuildPSOs()
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
		reinterpret_cast<BYTE*>(m_Shaders["reflectionPS"]->GetBufferPointer()),
		m_Shaders["reflectionPS"]->GetBufferSize()
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

	// refraction
	D3D12_GRAPHICS_PIPELINE_STATE_DESC refractPsoDesc = opaquePsoDesc;
	refractPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["refractionPS"]->GetBufferPointer()),
		m_Shaders["refractionPS"]->GetBufferSize()
	};
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&refractPsoDesc, IID_PPV_ARGS(&m_PSOs["refraction"])));

	//sky
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = m_RootSignature.Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skyVS"]->GetBufferPointer()),
		m_Shaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skyPS"]->GetBufferPointer()),
		m_Shaders["skyPS"]->GetBufferSize()
	};
	HR(m_pd3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&m_PSOs["sky"])));
}

void GameApp::BuildFrameResources()
{
	for(int i = 0;i<g_numFrameResources;++i)
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get(),
			7, (UINT)m_AllRitems.size(), (UINT)m_Materials.size()));
	}
}

void GameApp::BuildMaterials()
{
	auto bricks = std::make_unique<Material>();
	bricks->m_Name = "bricks";
	bricks->m_MatCBIndex = 0;
	bricks->m_DiffuseSrvHeapIndex = 0;
	bricks->m_NormalSrvHeapIndex = 1;
	bricks->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->m_FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	bricks->m_Roughness = 0.3f;

	auto tile = std::make_unique<Material>();
	tile->m_Name = "tile";
	tile->m_MatCBIndex = 1;
	tile->m_DiffuseSrvHeapIndex = 2;
	tile->m_NormalSrvHeapIndex = 3;
	tile->m_DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tile->m_FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	tile->m_Roughness = 0.1f;

	auto mirror = std::make_unique<Material>();
	mirror->m_Name = "mirror";
	mirror->m_MatCBIndex = 2;
	mirror->m_DiffuseSrvHeapIndex = 4;
	mirror->m_NormalSrvHeapIndex = 5;
	mirror->m_DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.1f, 1.0f);
	mirror->m_FresnelR0 = XMFLOAT3(0.98f, 0.98f, 0.98f);
	mirror->m_Roughness = 0.1f;
	mirror->m_Eta = 1.51f;

	auto skullMat = std::make_unique<Material>();
	skullMat->m_Name = "skullMat";
	skullMat->m_MatCBIndex = 3;
	skullMat->m_DiffuseSrvHeapIndex = 4;
	skullMat->m_NormalSrvHeapIndex = 5;
	skullMat->m_DiffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	skullMat->m_FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	skullMat->m_Roughness = 0.2f;

	auto sky = std::make_unique<Material>();
	sky->m_Name = "sky";
	sky->m_MatCBIndex = 4;
	sky->m_DiffuseSrvHeapIndex = 6;
	sky->m_NormalSrvHeapIndex = 5;
	sky->m_DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky->m_FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	sky->m_Roughness = 0.2f;

	m_Materials["bricks"] = std::move(bricks);
	m_Materials["tile"] = std::move(tile);
	m_Materials["mirror"] = std::move(mirror);
	m_Materials["skullMat"] = std::move(skullMat);
	m_Materials["sky"] = std::move(sky);
}

void GameApp::BuildRenderItems()
{
	auto skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(m_SkyBoxScale.x, m_SkyBoxScale.y, m_SkyBoxScale.z));
	skyRitem->TexTransform = MathHelper::Identity4x4();
	skyRitem->ObjCBIndex = 0;
	skyRitem->Mat = m_Materials["sky"].get();
	skyRitem->Geo = m_Geometries["shapeGeo"].get();
	skyRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["sphere"].IndexCount;
	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
	m_AllRitems.push_back(std::move(skyRitem));

	auto columnRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&columnRitem->World, XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	XMStoreFloat4x4(&columnRitem->TexTransform, XMMatrixScaling(1.5f, 2.0f, 1.0f));
	columnRitem->ObjCBIndex = 1;
	columnRitem->Mat = m_Materials["bricks"].get();
	columnRitem->Geo = m_Geometries["shapeGeo"].get();
	columnRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	columnRitem->IndexCount = columnRitem->Geo->DrawArgs["column"].IndexCount;
	columnRitem->StartIndexLocation = columnRitem->Geo->DrawArgs["column"].StartIndexLocation;
	columnRitem->BaseVertexLocation = columnRitem->Geo->DrawArgs["column"].BaseVertexLocation;

	m_RitemLayer[(int)RenderLayer::Opaque].push_back(columnRitem.get());
	m_AllRitems.push_back(std::move(columnRitem));

	auto sphereRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&sphereRitem->World, XMMatrixTranslation(0.0f, 2.5f, 0.0f));
	XMStoreFloat4x4(&sphereRitem->TexTransform, XMMatrixScaling(1.5f, 2.0f, 1.0f));
	sphereRitem->ObjCBIndex = 2;
	sphereRitem->Mat = m_Materials["mirror"].get();
	sphereRitem->Geo = m_Geometries["shapeGeo"].get();
	sphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	sphereRitem->IndexCount = sphereRitem->Geo->DrawArgs["sphere"].IndexCount;
	sphereRitem->StartIndexLocation = sphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	sphereRitem->BaseVertexLocation = sphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
	
	m_RitemLayer[(int)RenderLayer::StaticSky].push_back(sphereRitem.get());
	m_RitemLayer[(int)RenderLayer::DynamicSky].push_back(sphereRitem.get());
	m_AllRitems.push_back(std::move(sphereRitem));

	auto skullRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.4f, 0.4f, 0.4f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjCBIndex = 3;
	skullRitem->Mat = m_Materials["skullMat"].get();
	skullRitem->Geo = m_Geometries["skullGeo"].get();
	skullRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs["skull"].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs["skull"].BaseVertexLocation;

	m_SkullRitem = skullRitem.get();
	m_RitemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());
	m_AllRitems.push_back(std::move(skullRitem));
	
	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->ObjCBIndex = 4;
	gridRitem->Mat = m_Materials["tile"].get();
	gridRitem->Geo = m_Geometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	m_RitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	m_AllRitems.push_back(std::move(gridRitem));

}

void GameApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto objectCB = m_CurrFrameResource->ObjectCB->Resource();

	for (size_t i = 0; i < ritems.size(); ++i) 
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->GetIndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void GameApp::DrawSceneToCubeMap()
{
	m_CommandList->RSSetViewports(1, &m_DynamicCubeMap->Viewport());
	m_CommandList->RSSetScissorRects(1, &m_DynamicCubeMap->ScissorRect());

	// 将立方体图作为RT
	m_CommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_DynamicCubeMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	for (int i = 0; i < 6; ++i) 
	{
		m_CommandList->ClearRenderTargetView(m_DynamicCubeMap->GetRtv(i), Colors::LightSteelBlue, 0, nullptr);
		m_CommandList->ClearDepthStencilView(m_CubeDSV,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		m_CommandList->OMSetRenderTargets(1, &m_DynamicCubeMap->GetRtv(i), true, &m_CubeDSV);

		auto passCB = m_CurrFrameResource->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS passCBAdress = passCB->GetGPUVirtualAddress() + (1 + i) * passCBByteSize;

		m_CommandList->SetGraphicsRootConstantBufferView(1, passCBAdress);

		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Opaque]);
		m_CommandList->SetPipelineState(m_PSOs["sky"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RitemLayer[(int)RenderLayer::Sky]);
		m_CommandList->SetPipelineState(m_PSOs["opaque"].Get());
	}
	m_CommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_DynamicCubeMap->Resource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ));

}

void GameApp::BuildCubeFaceCamera(float x, float y, float z)
{
	// 指定位置
	XMFLOAT3 center(x, y, z);
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);

	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f,y,z),	//+x
		XMFLOAT3(x - 1.0f,y,z),	//-x
		XMFLOAT3(x,y + 1.0f,z),	//+y
		XMFLOAT3(x,y - 1.0f,z),	//-y
		XMFLOAT3(x,y,z + 1.0f),	//+z
		XMFLOAT3(x,y,z - 1.0f) 	//-z
	};

	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f,1.0f,0.0f),	 //+x
		XMFLOAT3(0.0f,1.0f,0.0f),	 //-x
		XMFLOAT3(0.0f,0.0f,-1.0f),	 //+y
		XMFLOAT3(0.0f,0.0f,+1.0f),	 //-y
		XMFLOAT3(0.0f,1.0f,0.0f),	 //+z
		XMFLOAT3(0.0f,1.0f,0.0f) 	 //-z
	};
	
	for (int i = 0; i < 6; ++i) 
	{
		m_CubeCamera = std::make_shared<FirstPersonCamera>();
		m_CubeCamera->SetFrustum(0.5f * MathHelper::Pi, 1.0f, 0.1f, 1000.0f);
		m_CubeCamera->LookAt(center, targets[i], ups[i]);
		m_CubeCamera->UpdateViewMatrix();
	}
}

void GameApp::ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::int32_t>& indices, BoundingBox& bounds)
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

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

		XMFLOAT3 spherePos;
		XMStoreFloat3(&spherePos, XMVector3Normalize(P));

		float theta = atan2f(spherePos.z, spherePos.x);

		if (theta < 0.0f) 
		{
			theta += XM_2PI;
		}
		float phi = acosf(spherePos.y);

		float u = theta / XM_PI * 0.5f;
		float v = phi / XM_PI;

		vertices[i].TexC = { u,v };

		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

	XMStoreFloat3(&bounds.Center, 0.5 * (vMin + vMax));
	XMStoreFloat3(&bounds.Extents, 0.5 * (vMin - vMax));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();
}

void GameApp::ReadDataFromFile(std::vector<Vertex>& vertices, std::vector<std::int32_t>& indices, BoundingSphere& bounds)
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

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

		XMFLOAT3 spherePos;
		XMStoreFloat3(&spherePos, XMVector3Normalize(P));

		float theta = atan2f(spherePos.z, spherePos.x);

		if (theta < 0.0f)
		{
			theta += XM_2PI;
		}
		float phi = acosf(spherePos.y);

		float u = theta / XM_PI * 0.5f;
		float v = phi / XM_PI;

		vertices[i].TexC = { u,v };

		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

	XMFLOAT3 len;
	XMStoreFloat3(&len, XMVector3Length(vMin - vMax));
	XMStoreFloat3(&bounds.Center, 0.5 * (vMin + vMax));
	bounds.Radius = 0.5f * len.x;

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT> GameApp::GetStaticSamplers()
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
			anisotropicWrap ,anisotropicClamp };
}

void GameApp::LoadTexture(std::string name, std::wstring filename)
{
	auto texture = std::make_unique<Texture>();

	texture->m_Name = name;
	texture->m_Filename = filename;
	HR(DirectX::CreateDDSTextureFromFile12(m_pd3dDevice.Get(), m_CommandList.Get(),
		texture->m_Filename.c_str(), texture->m_Resource, texture->m_UploadHeap));
	m_Textures[name] = std::move(texture);
}
