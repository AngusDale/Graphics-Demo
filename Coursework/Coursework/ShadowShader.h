// Light shader.h
// Basic single light shader setup
#ifndef _SHADOWSHADER_H_
#define _SHADOWSHADER_H_

#include "DXF.h"

using namespace std;
using namespace DirectX;


class ShadowShader : public BaseShader
{
private:
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

	struct CameraBufferType
	{
		XMFLOAT3 position;
		float padding;
	};

public:

	ShadowShader(ID3D11Device* device, HWND hwnd);
	~ShadowShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix,
		const XMMATRIX& projectionMatrix, ID3D11ShaderResourceView* texture, ShadowMap* depthMap[4][6], float mapBias[4], Light* light[4],
		float atten[4], int lType[4], float oAngle[4], float iAngle[4], float falloff[4], Camera* cam);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11SamplerState* sampleStateShadow;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* cameraBuffer;
	// point light depth cube face normals
	XMFLOAT3 dirs[6];
};

#endif