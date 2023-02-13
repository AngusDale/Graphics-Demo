#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;

class ManipulationShader : public BaseShader
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

	struct CameraBufferType 
	{
		XMFLOAT3 position;
		float padding;
	};

public:
	ManipulationShader(ID3D11Device* device, HWND hwnd);
	~ManipulationShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX &world, const XMMATRIX &view, const XMMATRIX &projection, 
		ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* texture2, ShadowMap* depthMap[4][6], float mapBias[4], Light* light[4], float atten[4], int lType[4], float oAngle[4],
		float iAngle[4], float falloff[4], float time, XMFLOAT3 waveSettings[2], float planeToSphere, float mapHeight, XMFLOAT4 spherePos, Camera* cam);

private:
	void initShader(const wchar_t* cs, const wchar_t* ps);

private:
	ID3D11Buffer * matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11SamplerState* sampleStateShadow;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* timeBuffer;
	ID3D11Buffer* cameraBuffer;
	XMFLOAT3 dirs[6];
};

