#pragma once

#include "BaseShader.h"

using namespace std;
using namespace DirectX;

class BloomThresholdShader : public BaseShader
{
private:
	struct ThresholdBufferType
	{
		float threshold;
		XMFLOAT3 padding;
	};

public:

	BloomThresholdShader(ID3D11Device* device, HWND hwnd);
	~BloomThresholdShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* texture, float thresh);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11Buffer* thresholdBuffer;
};

