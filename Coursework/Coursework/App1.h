// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"
#include "ManipulationShader.h"
#include "ManipulationDepthShader.h"
#include "DepthShader.h"
#include "ShadowShader.h"
#include "TextureShader.h"
#include "VerticalBlurShader.h"
#include "HorizontalBlurShader.h"
#include "BloomThresholdShader.h"
#include "BloomMergeShader.h"
#include "ToneMapShader.h"
#include "TessellationShader.h"
#include "TessellationDepthShader.h"
#include "ManipulationTessShader.h"
#include "ManipulationTessDepthShader.h"
#include "TessellatedPlaneMesh.h"
#include "ManipulationGeometryShader.h"

class App1 : public BaseApplication
{
public:

	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();	

protected:

	bool render();
	void update();
	void gui();

	void depthPass(ShadowMap* map, Light* light, int index); // renders depth data to a shadowmap
	void horizontalBlur(OrthoMesh* mesh, RenderTexture* target, RenderTexture* texture); 
	void verticalBlur(OrthoMesh* mesh, RenderTexture* target, RenderTexture* texture);
	void scaleTexture(OrthoMesh* mesh, RenderTexture* target, RenderTexture* texture); // normal textureshader pass but the render target and mesh will up or down sample the image
	void getBloomThreshold(); // generates an image consisting of the brightest parts of the scene only
	void doToneMapping(RenderTexture* texture); // adjusts the bloom texture based on exposure and gamma
	void doPostProcessing(); // does the multiple downscaling and upscaling passes to generate blur or bloom textures
	void additiveBlend(OrthoMesh* mesh, RenderTexture* target, RenderTexture* texture1, RenderTexture* texture2, float intensity); // combines two render textures
	void renderScene(bool renderToTexture); // renders the objects in a scene
	void finalPass(RenderTexture* texture); // renders the post processed scene

private:
	// used to update light variables
	struct LightData {
		XMFLOAT3 lightPosition;
		XMFLOAT3 lightDirection;
		XMFLOAT4 specularColour;
		ImVec4 diffuse;
		ImVec4 ambient;
		float specularPower;
		bool lightEnabled;
	};

	void initVariables(int screenWidth, int screenHeight);
	void initTextures(int screenWidth, int screenHeight);
	void initObjects(int width, int height);
	void initLights();
	void initShaders(HWND hwnd);
	void lightGui(int i);

	// shaders
	TextureShader* textureShader; // default texture shader
	ManipulationShader* manipulationShader; // carries out vertex manipulation with shadow'd lighting
	ManipulationDepthShader* manipulationDepthShader; // carries out the same vertex manipulation, but outputs a depth texture
	ShadowShader* shadowShader; // shadow shader for calculating lighting and shadows
	DepthShader* depthShader; // calculates a depth texture
	VerticalBlurShader* verticalBlurShader; // blurs and image vertically with a kernel size of 11
	HorizontalBlurShader* horizontalBlurShader; // blurs and image horizontally
	BloomThresholdShader* thresholdShader; // outputs the pixels of the scene that are over a certain threshold
	BloomMergeShader* mergeShader; // combines two blur passes together (in bloom, this is used while upsampling)
	ToneMapShader* toneMapper; // uses exposure and gamma to change a render texture's brightness
	TessellationShader* tessShader; // basic tessellation shader
	TessellationDepthShader* tessDepthShader;
	ManipulationTessShader* manipTessShader; // vertex manipulation on a tessellated plane
	ManipulationTessDepthShader* manipTessDepthShader; // depth pass for vertex manipulation plane
	ManipulationGeometryShader* manipGeometryShader; // applies a geometry shader to the manipulated plane

	// vertex manipulation
	XMFLOAT3 waveSettings[2]; // amplitude, speed, frequency
	XMFLOAT3 windSettings[2]; // amplitude, speed, frequency
	XMFLOAT4 spherePosition; // z value is radius
	float planeToSphere;
	float heightMapAmplitude;

	// meshes
	PlaneMesh* plane;
	TessellatedPlaneMesh* planeSphere;
	SphereMesh* sphere;
	CubeMesh* cube;
	OrthoMesh* ortho;
	OrthoMesh* fullScreenMesh;
	OrthoMesh* sampleMeshes[9]; // contains ortho meshes with scaled aspect ratios used for blur/bloom passes

	// lights
	enum LightType { POINT = 0, DIRECTIONAL, SPOT };

	Light* lights[4]; // two of each light type
	LightData* lightData[4];	

	int lightType[4]; // 1 = POINT, 2 = DIRECTIONAL, 3 = SPOT
	int sceneWidth[4];
	int sceneHeight[4];
	int lightCount;
	float spotOuterAngle[4];
	float spotInnerAngle[4];
	float spotFalloff[4];
	float attenuation[4];
	float nearPlane[4];
	float farPlane[4];
	float mapBias[4];
	XMFLOAT3 dirs[6]; // normalised direction vectors for each face of a depth map cube

	// textures
	ShadowMap* shadowMap[4][6]; // potential for all four lights to be point lights
	ShadowMap* shadowMapToRender; // shadow map to render to the screen's ortho mesh

	RenderTexture* horizontalBlurTexture[9]; // all the horizontal blur passes
	RenderTexture* verticalBlurTexture[9]; // all fully blurred blur passes
	RenderTexture* upscaledPasses[8]; // stores the upscaled versions of the blurred textures
	RenderTexture* combinedPasses[8]; // stores the combined versions of the upscaled textures

	ID3D11RasterizerState* defaultRasterState;
	ID3D11RasterizerState* newRasterState; // used to turn off backface culling for the grass

	ID3D11ShaderResourceView* sphereTextures[4]; // simple blur green red and yellow textures for the spheres

	RenderTexture* blurFilter; // the final blurred texture that should be displayed to the screen
	RenderTexture* bloomFilter; // the final bloom texture
	RenderTexture* toneMappedFilter; // the edited bloom filter
	RenderTexture* renderTexture; // the first texture the scene is rendered to
	RenderTexture* thresholdTexture; // the brightest parts of the scene

	XMINT2 aspectRatios[9]; // array of aspect ratios largest -> smallest

	// variables
	float time;
	float dynamicTessNear;
	float dynamicTessFar;
	float bloomThreshold;
	float bloomIntensity;
	float exposure;

	bool enableBlur;
	bool enableBloom;
	bool enablePP;
	bool enableHDR;
	bool enableGeometryShader;
	bool gammaCorrection;
	bool surfaceLighting;
	bool dynamicTess;
	bool showNormals;
	bool displayMap;

	int mapToRender;
	int ppMode;
	int blurPasses;
	int sWidth;
	int sHeight;
	int tessInsideFactor;
	int tessEdgeFactor;

	XMFLOAT3 cubePos[4];
	XMFLOAT3 spherePos[4];
};

#endif