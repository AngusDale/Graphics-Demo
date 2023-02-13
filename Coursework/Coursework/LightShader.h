#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class LightShader : public BaseShader
{
private:
	struct LightBufferType
	{
		XMFLOAT4 ambient[6];
		XMFLOAT4 diffuse[6];
		XMFLOAT4 position[6];
		XMFLOAT4 direction[6];
		XMFLOAT4 specularColour[6];

		float specularPower[6];
		float type[6];
		float outerAngle[6];
		float innerAngle[6];
		float falloff[6];
		XMFLOAT2 padding;
	};

	struct CameraBufferType 
	{
		XMFLOAT3 position;
		FLOAT padding;
	};

public:
	LightShader(ID3D11Device* device, HWND hwnd);
	~LightShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &world, const XMMATRIX &view, const XMMATRIX &projection, ID3D11ShaderResourceView* texture, Light* light[6], Camera* cam);

private:
	void initShader(const wchar_t* cs, const wchar_t* ps);

private:
	ID3D11Buffer * matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;
};

