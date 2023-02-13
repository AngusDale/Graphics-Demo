// Light shader.h
// Basic single light shader setup
#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;


class TessellationShader : public BaseShader
{

public:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMMATRIX lightView[4][6];
		XMMATRIX lightProjection[4];
	};

	struct LightBufferType
	{
		XMFLOAT4 ambient[4];
		XMFLOAT4 diffuse[4];
		XMFLOAT4 position[4];
		XMFLOAT4 direction[4];
		XMFLOAT4 specularColour[4];
		float specularPower[4];

		int type[4];
		float outerAngle[4];
		float innerAngle[4];
		float fall[4];
		float attenuation[4];
		float bias[4];
	};

	struct CameraBufferType {
		XMFLOAT3 camPos;
		float pad;
	};

	TessellationShader(ID3D11Device* device, HWND hwnd);
	~TessellationShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &world, const XMMATRIX &view, const XMMATRIX &projection, ID3D11ShaderResourceView* texture, ShadowMap* depthMap[4][6], float mapBias[4], Light* light[4],
		float atten[4], int lType[4], float oAngle[4], float iAngle[4], float falloff[4], FPCamera* cam);

private:
	void initShader(const wchar_t* vsFilename, const wchar_t* psFilename);
	void initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11SamplerState* sampleStateShadow;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* camBuffer;
	// point light depth cube face normals
	XMFLOAT3 dirs[6];
};
