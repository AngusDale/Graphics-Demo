#pragma once

#include "BaseShader.h"

using namespace std;
using namespace DirectX;

class ToneMapShader : public BaseShader
{
public:
	struct ToneBufferType {
		float exposure;
		bool gammaCorrection;
		bool pad;
		XMFLOAT2 padding;
	};

	ToneMapShader(ID3D11Device* device, HWND hwnd);
	~ToneMapShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* texture, float exp, bool gamma);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* toneBuffer;
	ID3D11SamplerState* sampleState;
};

