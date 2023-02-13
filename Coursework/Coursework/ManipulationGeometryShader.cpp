// tessellation shader.cpp
#include "ManipulationGeometryShader.h"


ManipulationGeometryShader::ManipulationGeometryShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"manipulationGeometry_vs.cso", L"manipulationGeometry_hs.cso", L"manipulationGeometry_ds.cso", 
		L"manipulationGeometry_gs.cso", L"manipulationGeometry_ps.cso");

	// initialise the point light faces
	dirs[0] = XMFLOAT3(0.0f, -1.0f, 0.0f);
	dirs[1] = XMFLOAT3(0.0f, 1.0f, 0.0f);
	dirs[2] = XMFLOAT3(1.0f, 0.0f, 0.0f);
	dirs[3] = XMFLOAT3(-1.0f, 0.0f, 0.0f);
	dirs[4] = XMFLOAT3(0.0f, 0.0f, 1.0f);
	dirs[5] = XMFLOAT3(0.0f, 0.0f, -1.0f);
}


ManipulationGeometryShader::~ManipulationGeometryShader()
{
	if (lightBuffer)
	{
		lightBuffer->Release();
		lightBuffer = 0;
	}
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
	if (sampleStateShadow)
	{
		sampleStateShadow->Release();
		sampleStateShadow = 0;
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

void ManipulationGeometryShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
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

	// Sampler for shadow map sampling.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	renderer->CreateSamplerState(&samplerDesc, &sampleStateShadow);
	
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

	// Setup light buffer
	D3D11_BUFFER_DESC lightBufferDesc;
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBufferType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&lightBufferDesc, NULL, &lightBuffer);

	D3D11_BUFFER_DESC cameraBufferDesc;
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;

	renderer->CreateBuffer(&cameraBufferDesc, NULL, &camBuffer);
}

void ManipulationGeometryShader::initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* gsFilename, const wchar_t* psFilename)
{
	// InitShader must be overwritten and it will load both vertex and pixel shaders + setup buffers
	initShader(vsFilename, psFilename);

	// Load other required shaders.
	loadHullShader(hsFilename);
	loadDomainShader(dsFilename);
	loadGeometryShader(gsFilename);
}


void ManipulationGeometryShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &worldMatrix, const XMMATRIX &viewMatrix, const XMMATRIX &projectionMatrix, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* texture2, ShadowMap* depthMap[4][6], float mapBias[4], Light* light[4],
	float atten[4], int lType[4], float oAngle[4], float iAngle[4], float falloff[4], float time, XMFLOAT3 waveSettings[2], XMFLOAT3 windSettings[2], float planeToSphere, float mapHeight, XMFLOAT4 spherePos, int tessFactor, int edgeTess, float tessNear, float tessFar, bool dynamicTessellation, bool surfaceLight, FPCamera* cam)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	CameraBufferType* camPtr;
	WindBufferType* windPtr;

	// Transpose the matrices to prepare them for the shader.
	XMMATRIX tworld = XMMatrixTranspose(worldMatrix);
	XMMATRIX tview = XMMatrixTranspose(viewMatrix);
	XMMATRIX tproj = XMMatrixTranspose(projectionMatrix);

	// Lock the constant buffer so it can be written to.
	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;// worldMatrix;
	dataPtr->view = tview;
	dataPtr->projection = tproj;

	// loop through all the lights
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			if (lType[i] == 0) {
				// if it's a point light, generate a new view matrix
				light[i]->setDirection(dirs[j].x, dirs[j].y, dirs[j].z);
				light[i]->generateViewMatrix();
			}
			// initialise the view matrices
			dataPtr->lightView[i][j] = XMMatrixTranspose(light[i]->getViewMatrix());
		}

		if (lType[i] == 1) {
			// if it's a directional light, use an ortho matrix
			dataPtr->lightProjection[i] = XMMatrixTranspose(light[i]->getOrthoMatrix());
		}
		else {
			// else create a projection matrix
			dataPtr->lightProjection[i] = XMMatrixTranspose(light[i]->getProjectionMatrix());
		}
	}
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
	deviceContext->HSSetConstantBuffers(2, 1, &camBuffer);

	deviceContext->Map(windBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	windPtr = (WindBufferType*)mappedResource.pData;
	windPtr->amplitude.x = windSettings[0].x;
	windPtr->amplitude.y = windSettings[1].x;
	windPtr->frequency.x = windSettings[0].y;
	windPtr->frequency.y = windSettings[1].y;
	windPtr->speed.x = windSettings[0].z;
	windPtr->speed.y = windSettings[1].z;
	windPtr->time = time;
	windPtr->surfaceLighting = surfaceLight;
	windPtr->planeToSphere = planeToSphere;
	windPtr->padding = XMFLOAT3(1.0f, 1.0f, 1.0f);
	deviceContext->Unmap(windBuffer, 0);
	// send buffer to geometry shader
	deviceContext->GSSetConstantBuffers(2, 1, &windBuffer);

	// Send light data to pixel shader
	LightBufferType* lightPtr;
	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	lightPtr = (LightBufferType*)mappedResource.pData;
	for (int i = 0; i < 4; i++) {
		lightPtr->ambient[i] = light[i]->getAmbientColour();
		lightPtr->diffuse[i] = light[i]->getDiffuseColour();
		lightPtr->position[i] = XMFLOAT4(light[i]->getPosition().x, light[i]->getPosition().y, light[i]->getPosition().z, 1.0f);
		lightPtr->direction[i] = XMFLOAT4(light[i]->getDirection().x, light[i]->getDirection().y, light[i]->getDirection().z, 1.0f);

		lightPtr->specularColour[i] = light[i]->getSpecularColour();
		lightPtr->specularPower[i] = light[i]->getSpecularPower();
		lightPtr->outerAngle[i] = cos(XMConvertToRadians(oAngle[i]));
		lightPtr->innerAngle[i] = cos(XMConvertToRadians(iAngle[i]));
		lightPtr->fall[i] = falloff[i];
		lightPtr->type[i] = lType[i];
		lightPtr->attenuation[i] = 0.0f;
		lightPtr->attenuation[i] = atten[i];
		lightPtr->bias[i] = mapBias[i];
	}
	deviceContext->Unmap(lightBuffer, 0);
	// send buffer to pixel shader
	deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

	// Set shader texture resource in the domain shader.
	deviceContext->DSSetShaderResources(0, 1, &texture);
	deviceContext->DSSetSamplers(0, 1, &sampleState); 

	// Set shader texture resource in the hull shader.
	deviceContext->HSSetShaderResources(0, 1, &texture);
	deviceContext->HSSetSamplers(0, 1, &sampleState);

	// Set shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(0, 1, &texture2);

	// set all the depth maps that the pixel shader will use
	ID3D11ShaderResourceView* map;
	int reg = 1;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			map = depthMap[i][j]->getDepthMapSRV();
			deviceContext->PSSetShaderResources(reg, 1, &map);
			reg++;
		}
	}

	deviceContext->PSSetSamplers(0, 1, &sampleState);
	deviceContext->PSSetSamplers(1, 1, &sampleStateShadow);
}


