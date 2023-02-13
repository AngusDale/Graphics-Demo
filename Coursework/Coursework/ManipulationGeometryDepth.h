// Light shader.h
// Basic single light shader setup
#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;


class ManipulationGeometryDepth : public BaseShader
{

public:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
	};

	struct TimeBufferType
	{
		float time;
		float planeSphere;
		float height;
		int tessellationFactor;

		XMFLOAT2 amplitude;
		XMFLOAT2 frequency;
		XMFLOAT2 speed;
		XMFLOAT2 pad;
		XMFLOAT4 spherePosition;

		bool dynamicTess;
		int edgeTessellationFactor;
		float nearBound;
		float farBound;
	};

	struct WindBufferType
	{
		XMFLOAT2 amplitude;
		XMFLOAT2 frequency;
		XMFLOAT2 speed;
		float time;
		bool surfaceLighting;
		float planeToSphere;
		XMFLOAT3 padding;
	};

	struct CameraBufferType {
		XMFLOAT3 camPos;
		float pad;
	};

	ManipulationGeometryDepth(ID3D11Device* device, HWND hwnd);
	~ManipulationGeometryDepth();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &world, const XMMATRIX &view, const XMMATRIX &projection, 
		ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* texture2, float time, XMFLOAT3 waveSettings[2], 
		XMFLOAT3 windSettings[2], float planeToSphere, float mapHeight, XMFLOAT4 spherePos, int tessFactor, int edgeTess, float tessNear, 
		float tessFar, bool dynamicTessellation, FPCamera* cam);

private:
	void initShader(const wchar_t* vsFilename, const wchar_t* psFilename);
	void initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* gsFilename, const wchar_t* psFilename);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11Buffer* camBuffer;
	ID3D11Buffer* timeBuffer;
	ID3D11Buffer* windBuffer;
};
