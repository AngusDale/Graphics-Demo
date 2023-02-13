// tessellation shader.cpp
#include "ManipulationGeometryDepth.h"


ManipulationGeometryDepth::ManipulationGeometryDepth(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"geometryDepth_vs.cso", L"geometryDepth_hs.cso", L"geometryDepth_ds.cso", 
		L"geometryDepth_gs.cso", L"geometryDepth_ps.cso");
}


ManipulationGeometryDepth::~ManipulationGeometryDepth()
{
	if (windBuffer)
	{
		windBuffer->Release();
		windBuffer = 0;
	}
	if (timeBuffer)
	{
		timeBuffer->Release();
		timeBuffer = 0;
	}
	if (camBuffer)
	{
		camBuffer->Release();
		camBuffer = 0;
	}
	if (sampleState)
	{
		sampleState->Release();
		sampleState = 0;
	}
	if (matrixBuffer)
	{
		matrixBuffer->Release();
		matrixBuffer = 0;
	}
	if (layout)
	{
		layout->Release();
		layout = 0;
	}
	
	//Release base shader components
	BaseShader::~BaseShader();
}

void ManipulationGeometryDepth::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);
	
	D3D11_BUFFER_DESC timeBufferDesc;
	timeBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	timeBufferDesc.ByteWidth = sizeof(TimeBufferType);
	timeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	timeBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	timeBufferDesc.MiscFlags = 0;
	timeBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&timeBufferDesc, NULL, &timeBuffer);

	D3D11_BUFFER_DESC windBufferDesc;
	windBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	windBufferDesc.ByteWidth = sizeof(TimeBufferType);
	windBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	windBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	windBufferDesc.MiscFlags = 0;
	windBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&windBufferDesc, NULL, &windBuffer);

	D3D11_BUFFER_DESC cameraBufferDesc;
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;

	renderer->CreateBuffer(&cameraBufferDesc, NULL, &camBuffer);
}

void ManipulationGeometryDepth::initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* gsFilename, const wchar_t* psFilename)
{
	// InitShader must be overwritten and it will load both vertex and pixel shaders + setup buffers
	initShader(vsFilename, psFilename);

	// Load other required shaders.
	loadHullShader(hsFilename);
	loadDomainShader(dsFilename);
	loadGeometryShader(gsFilename);
}


void ManipulationGeometryDepth::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection,
	ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* texture2, float time, XMFLOAT3 waveSettings[2],
	XMFLOAT3 windSettings[2], float planeToSphere, float mapHeight, XMFLOAT4 spherePos, int tessFactor, int edgeTess, float tessNear,
	float tessFar, bool dynamicTessellation, FPCamera* cam)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	CameraBufferType* camPtr;
	WindBufferType* windPtr;

	// Transpose the matrices to prepare them for the shader.
	XMMATRIX tworld = XMMatrixTranspose(world);
	XMMATRIX tview = XMMatrixTranspose(view);
	XMMATRIX tproj = XMMatrixTranspose(projection);

	// Lock the constant buffer so it can be written to.
	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;// worldMatrix;
	dataPtr->view = tview;
	dataPtr->projection = tproj;

	// give the matrix buffer to the geometry and domain shader
	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->GSSetConstantBuffers(0, 1, &matrixBuffer);
	deviceContext->DSSetConstantBuffers(0, 1, &matrixBuffer);

	TimeBufferType* timePtr;
	deviceContext->Map(timeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	timePtr = (TimeBufferType*)mappedResource.pData;
	timePtr->time = time;
	timePtr->planeSphere = planeToSphere;
	timePtr->height = mapHeight;
	timePtr->tessellationFactor = tessFactor;
	timePtr->dynamicTess = dynamicTessellation;
	timePtr->amplitude.x = waveSettings[0].x;
	timePtr->amplitude.y = waveSettings[1].x;
	timePtr->frequency.x = waveSettings[0].y;
	timePtr->frequency.y = waveSettings[1].y;
	timePtr->speed.x = waveSettings[0].z;
	timePtr->speed.y = waveSettings[1].z;
	timePtr->pad = XMFLOAT2(1.0f, 1.0f);
	timePtr->spherePosition = spherePos; 
	timePtr->nearBound = tessNear;
	timePtr->farBound = tessFar;
	timePtr->edgeTessellationFactor = edgeTess;
	deviceContext->Unmap(timeBuffer, 0);
	// send buffer to domain and hull shader
	deviceContext->DSSetConstantBuffers(1, 1, &timeBuffer);
	deviceContext->HSSetConstantBuffers(0, 1, &timeBuffer);

	deviceContext->Map(camBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	camPtr = (CameraBufferType*)mappedResource.pData;
	camPtr->camPos = cam->getPosition();
	camPtr->pad = 0.0f;
	deviceContext->Unmap(camBuffer, 0);
	// send buffer to geometry and hull shader
	deviceContext->GSSetConstantBuffers(1, 1, &camBuffer);
	deviceContext->HSSetConstantBuffers(1, 1, &camBuffer);

	deviceContext->Map(windBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	windPtr = (WindBufferType*)mappedResource.pData;
	windPtr->amplitude.x = windSettings[0].x;
	windPtr->amplitude.y = windSettings[1].x;
	windPtr->frequency.x = windSettings[0].y;
	windPtr->frequency.y = windSettings[1].y;
	windPtr->speed.x = windSettings[0].z;
	windPtr->speed.y = windSettings[1].z;
	windPtr->time = time;
	windPtr->planeToSphere = planeToSphere;
	windPtr->padding = XMFLOAT3(1.0f, 1.0f, 1.0f);
	deviceContext->Unmap(windBuffer, 0);
	// send buffer to geometry shader
	deviceContext->GSSetConstantBuffers(2, 1, &windBuffer);	

	// Set shader texture resource in the domain shader.
	deviceContext->DSSetShaderResources(0, 1, &texture);
	deviceContext->DSSetSamplers(0, 1, &sampleState); 

	// Set shader texture resource in the hull shader.
	deviceContext->HSSetShaderResources(0, 1, &texture);
	deviceContext->HSSetSamplers(0, 1, &sampleState);
}


