// Colour shader.h
// Simple shader example.
#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;


class ManipulationDepthShader : public BaseShader
{

public:

	struct TimeBufferType
	{
		float time;
		float planeSphere;
		float height;
		float padding;

		XMFLOAT2 amplitude;
		XMFLOAT2 frequency;
		XMFLOAT2 speed;
		XMFLOAT2 pad;
		XMFLOAT4 spherePosition;
	};

	ManipulationDepthShader(ID3D11Device* device, HWND hwnd);
	~ManipulationDepthShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, 
		ID3D11ShaderResourceView* texture, XMFLOAT3 waveSettings[2], float time, float planeToSphere, float mapHeight, XMFLOAT4 spherePos);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* timeBuffer;
	ID3D11SamplerState* sampleState;
};
