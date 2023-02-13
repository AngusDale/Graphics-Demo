#pragma once

#include "BaseShader.h"

using namespace std;
using namespace DirectX;

class BloomMergeShader : public BaseShader
{
public:

	struct BloomBufferType {
		float intensity;
		XMFLOAT3 padding;
	};

	BloomMergeShader(ID3D11Device* device, HWND hwnd);
	~BloomMergeShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection, ID3D11ShaderResourceView* sceneTexture, ID3D11ShaderResourceView* bloomTexture, float inten);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* bloomBuffer;
	ID3D11SamplerState* sampleState;
};

