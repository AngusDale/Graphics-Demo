// Lab1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"
#include "DXF.h"

App1::App1()
{

}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	// Initalise scene variables.	
	initVariables(screenWidth, screenHeight);
	initShaders(hwnd);
	initObjects(screenWidth, screenHeight);
	initTextures(screenWidth, screenHeight);
	initLights();
	camera->setPosition(0,10,-60);
}


App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();

	// Release the Direct3D object.
	if (planeSphere)
	{
		delete planeSphere;
		planeSphere = 0;
	}

	if (textureShader)
	{
		delete textureShader;
		textureShader = 0;
	}

	if (manipulationShader) {
		delete manipulationShader;
		manipulationShader = 0;
	}

	if (shadowShader) {
		delete shadowShader;
		shadowShader = 0;
	}

	if (depthShader) {
		delete depthShader;
		depthShader = 0;
	}

	if (manipulationDepthShader)
	{
		delete manipulationDepthShader;
		manipulationDepthShader = 0;
	}

	if (verticalBlurShader)
	{
		delete verticalBlurShader;
		verticalBlurShader = 0;
	}

	if (horizontalBlurShader)
	{
		delete horizontalBlurShader;
		horizontalBlurShader = 0;
	}

	if (thresholdShader)
	{
		delete thresholdShader;
		thresholdShader = 0;
	}

	if (mergeShader)
	{
		delete mergeShader;
		mergeShader = 0;
	}
	
	if (toneMapper)
	{
		delete toneMapper;
		toneMapper = 0;
	}
	
	if (tessShader)
	{
		delete tessShader;
		tessShader = 0;
	}

	if (tessDepthShader)
	{
		delete tessDepthShader;
		tessDepthShader = 0;
	}

	if (manipTessShader)
	{
		delete manipTessShader;
		manipTessShader = 0;
	}

	if (manipTessDepthShader)
	{
		delete manipTessDepthShader;
		manipTessDepthShader = 0;
	}

	if (manipGeometryShader)
	{
		delete manipGeometryShader;
		manipGeometryShader = 0;
	}
}


bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}

	// update scene variables
	update();

	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	return true;
}

bool App1::render()
{
	// generate each light's depth textures
	for (int i = 0; i < 4; i++) {
		// if the light is enabled
		if (lightData[i]->lightEnabled) {
			// if it's a point light
			if (lightType[i] == 0) {
				// generate the 6 depth map cube faces
				for (int j = 0; j < 6; j++) {
					// set the direction to the normalised vectors of the faces
					lights[i]->setDirection(dirs[j].x, dirs[j].y, dirs[j].z);
					depthPass(shadowMap[i][j], lights[i], i);
				}

			}
			else {
				// only generate one depth map
				depthPass(shadowMap[i][0], lights[i], i);
			}
		}
	}

	if (enableBlur || enableBloom) {
		// renders scene to rendertexture
		renderScene(true);
		// blur scene
		doPostProcessing();
	}
	else {
		// render the scene to the back buffer
		renderScene(false);
	}

	return true;
}

void App1::update() {
	// update time
	time += timer->getTime();
	// update map to render
	shadowMapToRender = shadowMap[mapToRender][0];		

	// set post processing modes
	if (enablePP) {
		if (ppMode == 0) {
			enableBlur = true;
			enableBloom = false;
		}
		else {
			enableBlur = false;
			enableBloom = true;
		}
	}
	else {
		enableBlur = false;
		enableBloom = false;
	}

	// update light information
	for (int i = 0; i < 4; i++) {
		lights[i]->setPosition(lightData[i]->lightPosition.x, lightData[i]->lightPosition.y, lightData[i]->lightPosition.z);
		lights[i]->setDirection(lightData[i]->lightDirection.x, lightData[i]->lightDirection.y, lightData[i]->lightDirection.z);
		lights[i]->setAmbientColour(lightData[i]->ambient.x, lightData[i]->ambient.y, lightData[i]->ambient.z, lightData[i]->ambient.w);
		lights[i]->setDiffuseColour(lightData[i]->diffuse.x, lightData[i]->diffuse.y, lightData[i]->diffuse.z, lightData[i]->diffuse.w);
		lights[i]->setSpecularColour(lightData[i]->specularColour.x, lightData[i]->specularColour.y, lightData[i]->specularColour.z, lightData[i]->specularColour.w);
		lights[i]->setSpecularPower(lightData[i]->specularPower);
		
		if(!lightData[i]->lightEnabled){
			// set the colours to black if the light is disabled
			lights[i]->setAmbientColour(0, 0, 0, 0);
			lights[i]->setDiffuseColour(0, 0, 0, 0);
			lights[i]->setSpecularColour(0, 0, 0, 0);
		}
	}
}

#pragma region Post Processing
void App1::horizontalBlur(OrthoMesh* mesh, RenderTexture* target, RenderTexture* texture)
{
	// create matrices
	XMMATRIX worldMatrix, baseViewMatrix, orthoMatrix;

	// set the screen size and render target
	float screenSizeX = (float)target->getTextureWidth();
	target->setRenderTarget(renderer->getDeviceContext());
	target->clearRenderTarget(renderer->getDeviceContext(), 0.05f, 0.05f, 0.05f, 1.0f);

	worldMatrix = renderer->getWorldMatrix();
	baseViewMatrix = camera->getOrthoViewMatrix();
	orthoMatrix = target->getOrthoMatrix();

	// Render for Horizontal Blur
	renderer->setZBuffer(false);
	mesh->sendData(renderer->getDeviceContext());
	horizontalBlurShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, baseViewMatrix, orthoMatrix, texture->getShaderResourceView(), screenSizeX);
	horizontalBlurShader->render(renderer->getDeviceContext(), mesh->getIndexCount());
	renderer->setZBuffer(true);

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	renderer->setBackBufferRenderTarget();
}

void App1::verticalBlur(OrthoMesh* mesh, RenderTexture* target, RenderTexture* texture)
{
	// create the matrices
	XMMATRIX worldMatrix, baseViewMatrix, orthoMatrix;

	// set the screen size and create a render target
	float screenSizeY = (float)target->getTextureHeight();
	target->setRenderTarget(renderer->getDeviceContext());
	target->clearRenderTarget(renderer->getDeviceContext(), 0.05f, 0.05f, 0.05f, 1.0f);
	

	worldMatrix = renderer->getWorldMatrix();
	baseViewMatrix = camera->getOrthoViewMatrix();
	orthoMatrix = target->getOrthoMatrix();

	// Render for Vertical Blur
	renderer->setZBuffer(false);
	mesh->sendData(renderer->getDeviceContext());
	verticalBlurShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, baseViewMatrix, orthoMatrix, texture->getShaderResourceView(), screenSizeY);
	verticalBlurShader->render(renderer->getDeviceContext(), mesh->getIndexCount());
	renderer->setZBuffer(true);

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	renderer->setBackBufferRenderTarget();
}

void App1::scaleTexture(OrthoMesh* mesh, RenderTexture* target, RenderTexture* texture)
{
	// create the matrices
	XMMATRIX worldMatrix, baseViewMatrix, orthoMatrix;
	// set the render target
	target->setRenderTarget(renderer->getDeviceContext());
	target->clearRenderTarget(renderer->getDeviceContext(), 0.05f, 0.05f, 0.05f, 1.0f);

	worldMatrix = renderer->getWorldMatrix();
	baseViewMatrix = camera->getOrthoViewMatrix();
	orthoMatrix = target->getOrthoMatrix();

	// render the texture to the render target and ortho mesh of the same size
	renderer->setZBuffer(false);
	mesh->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, baseViewMatrix, orthoMatrix, texture->getShaderResourceView());
	textureShader->render(renderer->getDeviceContext(), mesh->getIndexCount());
	renderer->setZBuffer(true);

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	renderer->setBackBufferRenderTarget();
}

void App1::getBloomThreshold()
{
	XMMATRIX worldMatrix, baseViewMatrix, orthoMatrix;
	// set render target
	thresholdTexture->setRenderTarget(renderer->getDeviceContext());
	thresholdTexture->clearRenderTarget(renderer->getDeviceContext(), 0.0f, 0.0f, 0.0f, 1.0f);

	worldMatrix = renderer->getWorldMatrix();
	baseViewMatrix = camera->getOrthoViewMatrix();
	orthoMatrix = thresholdTexture->getOrthoMatrix();

	// get the brightest parts of the scene ready for bloom 
	renderer->setZBuffer(false);
	fullScreenMesh->sendData(renderer->getDeviceContext());
	thresholdShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, baseViewMatrix, orthoMatrix, renderTexture->getShaderResourceView(), bloomThreshold);
	thresholdShader->render(renderer->getDeviceContext(), fullScreenMesh->getIndexCount());
	renderer->setZBuffer(true);

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	renderer->setBackBufferRenderTarget();
}

void App1::doToneMapping(RenderTexture* texture)
{
	XMMATRIX worldMatrix, baseViewMatrix, orthoMatrix;
	// set the render target
	toneMappedFilter->setRenderTarget(renderer->getDeviceContext());
	toneMappedFilter->clearRenderTarget(renderer->getDeviceContext(), 0.05f, 0.05f, 0.05f, 1.0f);

	worldMatrix = renderer->getWorldMatrix();
	baseViewMatrix = camera->getOrthoViewMatrix();
	orthoMatrix = toneMappedFilter->getOrthoMatrix();

	// do tone mapping 
	renderer->setZBuffer(false);
	fullScreenMesh->sendData(renderer->getDeviceContext());
	toneMapper->setShaderParameters(renderer->getDeviceContext(), worldMatrix, baseViewMatrix, orthoMatrix, texture->getShaderResourceView(), exposure, gammaCorrection);
	toneMapper->render(renderer->getDeviceContext(), fullScreenMesh->getIndexCount());
	renderer->setZBuffer(true);

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	renderer->setBackBufferRenderTarget();
}

void App1::additiveBlend(OrthoMesh* mesh, RenderTexture* target, RenderTexture* texture1, RenderTexture* texture2, float intensity)
{
	XMMATRIX worldMatrix, baseViewMatrix, orthoMatrix;
	// set the render target
	target->setRenderTarget(renderer->getDeviceContext());
	target->clearRenderTarget(renderer->getDeviceContext(), 0.05f, 0.05f, 0.05f, 1.0f);

	worldMatrix = renderer->getWorldMatrix();
	baseViewMatrix = camera->getOrthoViewMatrix();
	orthoMatrix = target->getOrthoMatrix();

	// add the two textures
	renderer->setZBuffer(false);
	mesh->sendData(renderer->getDeviceContext());
	mergeShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, baseViewMatrix, orthoMatrix, texture1->getShaderResourceView(), texture2->getShaderResourceView(), intensity);
	mergeShader->render(renderer->getDeviceContext(), mesh->getIndexCount());
	renderer->setZBuffer(true);

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	renderer->setBackBufferRenderTarget();
}

void App1::doPostProcessing()
{

	if (enableBloom)
		getBloomThreshold();

	// loop for the amount of blur passes selected
	for (int i = 0; i < blurPasses; i++) {
		// if this is the first pass
		if (i == 0) {
			// blur either the threshold texture, or full scene
			if (enableBloom)
				horizontalBlur(sampleMeshes[i], horizontalBlurTexture[i], thresholdTexture);
			else
				horizontalBlur(sampleMeshes[i], horizontalBlurTexture[i], renderTexture);
		}
		else {
			// if this is not the first loop, blur the last vertical blur texture
			horizontalBlur(sampleMeshes[i], horizontalBlurTexture[i], verticalBlurTexture[i - 1]);
		}

		// apply a vertical blur to the most recent horizontal blur texture
		verticalBlur(sampleMeshes[i], verticalBlurTexture[i], horizontalBlurTexture[i]);

		// no more scaling needs to be done so scale the texture to the screen size
		if (!enableBloom)
			scaleTexture(fullScreenMesh, blurFilter, verticalBlurTexture[i]);
	}

	if (enableBloom) {
		// if there is more than one blur pass
		if (blurPasses != 1) {
			// loop through blur passes - 2. 
			// (There will always be "blurpasses - 1" number of combined textures, and we're accessing i + 1. Hence "-2")
			for (int i = blurPasses - 2; i >= 0; i--) {
				// add all the blooms together
				if (i == blurPasses - 2) {
					// if this is the first loop, upSample the final blurpass texture
					scaleTexture(sampleMeshes[i], upscaledPasses[i], verticalBlurTexture[i + 1]);
				}
				else {
					// upsample the previoulsly combined texture
					scaleTexture(sampleMeshes[i], upscaledPasses[i], combinedPasses[i + 1]); // +1 because we're counting down
				}

				// set combinedPasses as the target, then combine the vertical blur texture and upscaled texture  
				additiveBlend(sampleMeshes[i], combinedPasses[i], verticalBlurTexture[i], upscaledPasses[i], 1.0f);
			}

			// if we're done looping then render the final combined texture to the bloom filter
			additiveBlend(fullScreenMesh, bloomFilter, renderTexture, combinedPasses[0], bloomIntensity);
		}
		else {
			// if there is only one blur pass then set the bloom filter to the scene + blur texture
			additiveBlend(fullScreenMesh, bloomFilter, renderTexture, verticalBlurTexture[0], bloomIntensity);
		}

	}

	// do tone mapping if enabled
	if (enableHDR) {
		if (enableBloom)
			doToneMapping(bloomFilter);
		else
			doToneMapping(blurFilter);
		// render the final filter to the screen
		finalPass(toneMappedFilter);
	}
	else {
		// render the unaltered bloom/blur filter
		if (enableBloom) 
			finalPass(bloomFilter);		
		else
			finalPass(blurFilter);
	}
}
#pragma endregion

void App1::depthPass(ShadowMap* map, Light* light, int index)
{
	// Set the render target to the shadow map
	map->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

	// get the world, view, and projection matrices from the camera and d3d objects.
	XMMATRIX lightProjectionMatrix;

	// set the ortho/projection matrix based on the light type
	switch (lightType[index]) {
	case 0: light->generateProjectionMatrix(nearPlane[index], farPlane[index]);
		lightProjectionMatrix = light->getProjectionMatrix();
		break;
	case 1: light->generateOrthoMatrix(sceneWidth[index], sceneHeight[index], nearPlane[index], farPlane[index]);
		lightProjectionMatrix = light->getOrthoMatrix();
		break;
	case 2: light->generateProjectionMatrix(nearPlane[index], farPlane[index]);
		lightProjectionMatrix = light->getProjectionMatrix();
		break;
	}

	light->generateViewMatrix();
	XMMATRIX lightViewMatrix = light->getViewMatrix();
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	// save the default world matrix
	XMMATRIX temp = worldMatrix;

	for (int i = 0; i < 4; i++) {
		// render the 4 spheres
		worldMatrix *= XMMatrixScaling(5.0f, 5.0f, 5.0f);
		worldMatrix *= XMMatrixTranslation(spherePos[i].x, spherePos[i].y, spherePos[i].z);
		sphere->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
		depthShader->render(renderer->getDeviceContext(), plane->getIndexCount());
		worldMatrix = temp;

		// render the 4 cubes, scale and rotate them
		worldMatrix *= XMMatrixScaling(1.0f, 4.0f, 5.0f);
		if (i > 1)
			worldMatrix *= XMMatrixRotationY(XMConvertToRadians(90.0f));
		worldMatrix *= XMMatrixTranslation(cubePos[i].x, cubePos[i].y, cubePos[i].z);
		cube->sendData(renderer->getDeviceContext());
		depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
		depthShader->render(renderer->getDeviceContext(), cube->getIndexCount());
		worldMatrix = temp;
	}

	// render the vertex manipulation plane
	worldMatrix *= XMMatrixTranslation(-15, -8, -15);
	planeSphere->sendData(renderer->getDeviceContext(), D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	manipTessDepthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix, textureMgr->getTexture(L"height"),
		time, waveSettings, planeToSphere, heightMapAmplitude, spherePosition, tessInsideFactor, tessEdgeFactor, dynamicTessNear, dynamicTessFar, dynamicTess, camera);
	manipTessDepthShader->render(renderer->getDeviceContext(), planeSphere->getIndexCount());	

	// render the sphere-position-sphere
	worldMatrix *= XMMatrixTranslation(spherePosition.x, spherePosition.y, spherePosition.z);
	sphere->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), sphere->getIndexCount());
	worldMatrix = temp;

	// render the floor
	worldMatrix *= XMMatrixTranslation(-50, -10, -50);
	plane->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), plane->getIndexCount());
	worldMatrix = temp;	

}

void App1::renderScene(bool renderToTexture)
{
	// if we're not blurring the scene, set the render target to the back buffer
	if (!renderToTexture) {
		renderer->setBackBufferRenderTarget();
		renderer->resetViewport();
		// Clear the scene. (default blue colour)
		renderer->beginScene(0.05f, 0.05f, 0.05f, 1.0f);
	}
	else {
		 // Set the render target to be the render texture and clear it
		renderTexture->setRenderTarget(renderer->getDeviceContext());
		renderTexture->clearRenderTarget(renderer->getDeviceContext(), 0.05f, 0.05f, 0.05f, 1.0f);
	}

	camera->update();

	// Get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	// Generate the view matrix based on the camera's position.
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();
	XMMATRIX temp = worldMatrix;


	for (int i = 0; i < 4; i++) {
		// render light gizmos
		worldMatrix *= XMMatrixTranslation(lightData[i]->lightPosition.x, lightData[i]->lightPosition.y, lightData[i]->lightPosition.z);
		sphere->sendData(renderer->getDeviceContext());
		textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L""));
		textureShader->render(renderer->getDeviceContext(), plane->getIndexCount());
		worldMatrix = temp;		

		// render the 4 spheres
		worldMatrix *= XMMatrixScaling(5.0f, 5.0f, 5.0f);
		worldMatrix *= XMMatrixTranslation(spherePos[i].x, spherePos[i].y, spherePos[i].z);
		sphere->sendData(renderer->getDeviceContext());
		textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, sphereTextures[i]);
		textureShader->render(renderer->getDeviceContext(), plane->getIndexCount());
		worldMatrix = temp;

		// render the 4 cubes
		worldMatrix *= XMMatrixScaling(1.0f, 4.0f, 5.0f);
		if (i > 1)
			worldMatrix *= XMMatrixRotationY(XMConvertToRadians(90.0f));
		worldMatrix *= XMMatrixTranslation(cubePos[i].x, cubePos[i].y, cubePos[i].z);
		cube->sendData(renderer->getDeviceContext());
		shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix,
			textureMgr->getTexture(L"brick"), shadowMap, mapBias, lights, attenuation, lightType, spotOuterAngle, spotInnerAngle, spotFalloff, camera);
		shadowShader->render(renderer->getDeviceContext(), cube->getIndexCount());
		worldMatrix = temp;

	}

	// render vertex manipulation plane
	worldMatrix *= XMMatrixTranslation(-15, -8, -15);
	planeSphere->sendData(renderer->getDeviceContext(), D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	manipTessShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"height"),
		textureMgr->getTexture(L"mars"), shadowMap, mapBias, lights, attenuation, lightType, spotOuterAngle, spotInnerAngle, spotFalloff, time,
		waveSettings, planeToSphere, heightMapAmplitude, spherePosition, tessInsideFactor, tessEdgeFactor, dynamicTessNear, dynamicTessFar, dynamicTess, showNormals, camera);
	manipTessShader->render(renderer->getDeviceContext(), planeSphere->getIndexCount());

	// if the geometry shader is enabled, render the grass
	if (enableGeometryShader) {
		// if we're not using wireframe mode, change the raster state to disable backface culling
		if (!wireframeToggle)
			renderer->getDeviceContext()->RSSetState(newRasterState);

		planeSphere->sendData(renderer->getDeviceContext(), D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
		manipGeometryShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"height"),
			textureMgr->getTexture(L"mars"), shadowMap, mapBias, lights, attenuation, lightType, spotOuterAngle, spotInnerAngle, spotFalloff, time,
			waveSettings, windSettings, planeToSphere, heightMapAmplitude, spherePosition, tessInsideFactor, tessEdgeFactor, dynamicTessNear, dynamicTessFar, dynamicTess, surfaceLighting, camera);
		manipGeometryShader->render(renderer->getDeviceContext(), planeSphere->getIndexCount());

		if (!wireframeToggle)
			renderer->getDeviceContext()->RSSetState(defaultRasterState);
	}

	// render the center of the sphere
	worldMatrix *= XMMatrixTranslation(spherePosition.x, spherePosition.y, spherePosition.z);
	sphere->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix,
		textureMgr->getTexture(L""), shadowMap, mapBias, lights, attenuation, lightType, spotOuterAngle, spotInnerAngle, spotFalloff, camera);
	shadowShader->render(renderer->getDeviceContext(), sphere->getIndexCount());	
	worldMatrix = temp;	

	// render the floor if we're not in wireframe mode
	if (!wireframeToggle) {
		worldMatrix *= XMMatrixTranslation(-50, -10, -50);
		plane->sendData(renderer->getDeviceContext());
		if (enablePP)
			shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"dwood"), shadowMap, mapBias, lights, attenuation, lightType, spotOuterAngle, spotInnerAngle, spotFalloff, camera);
		else
			shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"lwood"), shadowMap, mapBias, lights, attenuation, lightType, spotOuterAngle, spotInnerAngle, spotFalloff, camera);

		shadowShader->render(renderer->getDeviceContext(), plane->getIndexCount());
		worldMatrix = temp;
	}

	// render the depth map if we're not doing post processing
	if (!renderToTexture) {
		if (displayMap) {
			renderer->setZBuffer(false);
			XMMATRIX orthoMatrix = renderer->getOrthoMatrix();  // ortho matrix for 2D rendering
			XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();	// Default camera position for orthographic rendering

			ortho->sendData(renderer->getDeviceContext());
			textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, shadowMapToRender->getDepthMapSRV());
			textureShader->render(renderer->getDeviceContext(), ortho->getIndexCount());
			renderer->setZBuffer(true);
		}
		// Render GUI
		gui();

		// Present the rendered scene to the screen.
		renderer->endScene();
	}
	else {
		// Reset the render target back to the original back buffer and not the render to texture anymore.
		renderer->setBackBufferRenderTarget();
	}
}

void App1::finalPass(RenderTexture* texture)
{
	renderer->beginScene(0.05f, 0.05f, 0.05f, 1.0f);

	// RENDER THE RENDER TEXTURE SCENE
	renderer->setZBuffer(false);
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX orthoMatrix = renderer->getOrthoMatrix();  // ortho matrix for 2D rendering
	XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();	// Default camera position for orthographic rendering

	// render the post processed scene to a screen sized ortho mesh
	fullScreenMesh->sendData(renderer->getDeviceContext());
	textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, texture->getShaderResourceView());
	textureShader->render(renderer->getDeviceContext(), fullScreenMesh->getIndexCount());
	renderer->setZBuffer(true);

	// render the depth map if enabled
	if (displayMap) {
		renderer->setZBuffer(false);
		XMMATRIX orthoMatrix = renderer->getOrthoMatrix();  // ortho matrix for 2D rendering
		XMMATRIX orthoViewMatrix = camera->getOrthoViewMatrix();	// Default camera position for orthographic rendering

		ortho->sendData(renderer->getDeviceContext());
		textureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, orthoViewMatrix, orthoMatrix, shadowMapToRender->getDepthMapSRV());
		textureShader->render(renderer->getDeviceContext(), ortho->getIndexCount());
		renderer->setZBuffer(true);
	}

	// Render GUI
	gui();

	// Present the rendered scene to the screen.
	renderer->endScene();
}

void App1::gui()
{
	// Build UI
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Checkbox("Wireframe", &wireframeToggle); ImGui::SameLine();
	ImGui::Checkbox("Enable Post Processing", &enablePP);

	// POST PROCESSING
	if (enablePP) {
		ImGui::Dummy(ImVec2(0, 5));
		ImGui::Checkbox("HDR Tone Mapping", &enableHDR); 
		if (enableHDR) {
			ImGui::SameLine();
			ImGui::Checkbox("Gamma Correction", &gammaCorrection);
		}
		ImGui::RadioButton("Blur", &ppMode, 0); ImGui::SameLine();
		ImGui::RadioButton("Bloom", &ppMode, 1);
		ImGui::SliderInt("Blur Passes", &blurPasses, 1, 9);
		ImGui::SliderFloat("Bloom Threshold", &bloomThreshold, 0.0f, 3.0f);
		ImGui::SliderFloat("Bloom Intensity", &bloomIntensity, 0.0f, 1.0f);

		if(enableHDR)
			ImGui::SliderFloat("Tone Map Exposure", &exposure, 0.0f, 3.0f);
	}

	ImGui::Dummy(ImVec2(0, 10));	

	// VERTEX MANIPULATION
	if (ImGui::CollapsingHeader("Plane/Sphere Settings"))
	{
		ImGui::Indent(20);
		ImGui::Dummy(ImVec2(0, 5));
		ImGui::Checkbox("Show Normals", &showNormals);
		ImGui::Dummy(ImVec2(0, 5));
		ImGui::SliderFloat("Plane to Sphere", &planeToSphere, 0.0f, 1.0f);
		ImGui::Dummy(ImVec2(0, 5));
		ImGui::DragFloat3("Sphere Position", &spherePosition.x, 0.1f, -100.0f, 100.0f);
		ImGui::SliderFloat("Sphere Radius", &spherePosition.w, 1.0f, 50.0f);
		ImGui::SliderFloat("Map Height", &heightMapAmplitude, 0.0f, 100.0f);
		ImGui::Dummy(ImVec2(0, 5));
		ImGui::Text("Wave Settings");
		ImGui::PushItemWidth(ImGui::GetWindowWidth()/2.0f - 100);		
		ImGui::DragFloat("Amplitude X", &waveSettings[0].x, 0.01f, 0.0f, 30.0f); ImGui::SameLine();
		ImGui::DragFloat("Amplitude Z", &waveSettings[1].x, 0.01f, 0.0f, 30.0f);
		ImGui::DragFloat("Frequency X", &waveSettings[0].y, 0.01f, 0.0f, 4.0f); ImGui::SameLine();
		ImGui::DragFloat("Frequency Z", &waveSettings[1].y, 0.01f, 0.0f, 4.0f);
		ImGui::DragFloat("Speed X    ", &waveSettings[0].z, 0.01f, -10.0f, 10.0f); ImGui::SameLine();
		ImGui::DragFloat("Speed Z    ", &waveSettings[1].z, 0.01f, -10.0f, 10.0f); 
		ImGui::PopItemWidth();
		ImGui::Indent(-20);
	}

	ImGui::Dummy(ImVec2(0, 10));

	// TESSELLATION
	if (ImGui::CollapsingHeader("Tessellation Settings"))
	{
		ImGui::Checkbox("Dynamically Tessellate", &dynamicTess);
		if (dynamicTess) {
			ImGui::SliderFloat("Near Bound", &dynamicTessNear, 0, 100);
			ImGui::SliderFloat("Far Bound", &dynamicTessFar, 0, 100);
		}
		else {
			ImGui::SliderInt("Inside Tessellation", &tessInsideFactor, 1, 16);
			ImGui::SliderInt("Edge Tessellation", &tessEdgeFactor, 1, 16);
		}
	}

	ImGui::PushID(10);
	ImGui::Dummy(ImVec2(0, 10));

	// GEOMETRY SHADER
	ImGui::Checkbox("Enable Geometry Shader", &enableGeometryShader);
	if (ImGui::CollapsingHeader("Geometry Shader Settings"))
	{
		ImGui::Indent(20);
		ImGui::Text("Wind Settings");
		ImGui::PushItemWidth(ImGui::GetWindowWidth() / 2.0f - 100);
		ImGui::DragFloat("Amplitude X", &windSettings[0].x, 0.01f, 0.0f, 30.0f); ImGui::SameLine();
		ImGui::DragFloat("Amplitude Z", &windSettings[1].x, 0.01f, 0.0f, 30.0f);
		ImGui::DragFloat("Frequency X", &windSettings[0].y, 0.01f, 0.0f, 4.0f); ImGui::SameLine();
		ImGui::DragFloat("Frequency Z", &windSettings[1].y, 0.01f, 0.0f, 4.0f);
		ImGui::DragFloat("Speed X    ", &windSettings[0].z, 0.01f, -10.0f, 10.0f); ImGui::SameLine();
		ImGui::DragFloat("Speed Z    ", &windSettings[1].z, 0.01f, -10.0f, 10.0f);
		ImGui::PopItemWidth();
		ImGui::Indent(-20);
	}
	ImGui::PopID();

	ImGui::Dummy(ImVec2(0, 10));

	// DEPTH MAP
	ImGui::Checkbox("Display Depth Map", &displayMap);
	if (displayMap) {
		ImGui::Text("Depth Map to Render (Point lights show bottom face)");
		ImGui::RadioButton("Light 1", &mapToRender, 0); ImGui::SameLine();
		ImGui::RadioButton("Light 2", &mapToRender, 1); ImGui::SameLine();
		ImGui::RadioButton("Light 3", &mapToRender, 2); ImGui::SameLine();
		ImGui::RadioButton("Light 4", &mapToRender, 3); ImGui::SameLine();
		ImGui::Dummy(ImVec2(0, 5));
	}

	// LIGHTS
	if (ImGui::CollapsingHeader("Lights"))
	{		
		ImGui::Indent(20);
		// create the headers for the lights
		for (int i = 0; i < 4; i++)
		{

			ImGui::Dummy(ImVec2(0, 5));
			ImGui::PushID(i);
			ImGui::Checkbox("Enabled", &lightData[i]->lightEnabled); ImGui::SameLine();
			ImGui::RadioButton("Point", &lightType[i], 0); ImGui::SameLine();
			ImGui::RadioButton("Directional", &lightType[i], 1); ImGui::SameLine();
			ImGui::RadioButton("Spot", &lightType[i], 2);

			// Change label based on light type
			string label;
			if (lightType[i] == POINT)
			{
				label += "Point Light";
			}
			else if (lightType[i] == DIRECTIONAL)
			{
				label += "Directional Light ";
			}
			else if (lightType[i] == SPOT)
			{
				label += "Spot Light";
			}

			if (ImGui::CollapsingHeader(label.c_str()))
			{
				lightGui(i);
			}

			ImGui::PopID();
		}
	}

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void App1::lightGui(int i)
{	
	ImGui::Indent(20);

	ImGui::Dummy(ImVec2(0, 5));
	ImGui::DragFloat3("Position", &lightData[i]->lightPosition.x, 0.1f, -75.0f, 75.0f);

	if (lightType[i] != POINT)
		ImGui::SliderFloat3("Direction", &lightData[i]->lightDirection.x, -1.0f, 1.0f);

	ImGui::ColorEdit3("Diffuse Colour", &lightData[i]->diffuse.x, ImGuiColorEditFlags_Float);
	ImGui::ColorEdit3("Ambient Colour", &lightData[i]->ambient.x, ImGuiColorEditFlags_Float);
	ImGui::ColorEdit3("Specular Colour", &lightData[i]->specularColour.x, ImGuiColorEditFlags_Float);
	ImGui::SliderFloat("Specular Power", &lightData[i]->specularPower, 0.0f, 50.0f);
	ImGui::SliderFloat("Attenuation", &attenuation[i], 0.0f, 1.0f);

	ImGui::Dummy(ImVec2(0, 5));
	ImGui::Text("Shadow");
	ImGui::DragFloat("Shadow Map Bias", &mapBias[i], 0.0001f, 0.0002, 0.1f);
	ImGui::SliderFloat("Near Plane", &nearPlane[i], 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &farPlane[i], 1.0f, 250.0f);

	ImGui::Dummy(ImVec2(0, 5));

	// SPOTLIGHT
	if (lightType[i] == SPOT) {
		ImGui::SliderFloat("Outer Angle", &spotOuterAngle[i], 0, 180);
		ImGui::SliderFloat("Inner Angle", &spotInnerAngle[i], 0, 180);
		ImGui::SliderFloat("Falloff", &spotFalloff[i], 0, 20);
	}
	ImGui::Indent(-20);
}

#pragma region Inits
void App1::initShaders(HWND hwnd)
{
	textureShader = new TextureShader(renderer->getDevice(), hwnd);

	// vertex manipulation and tessellation shaders
	tessShader = new TessellationShader(renderer->getDevice(), hwnd);
	manipTessShader = new ManipulationTessShader(renderer->getDevice(), hwnd);
	manipulationShader = new ManipulationShader(renderer->getDevice(), hwnd);
	manipGeometryShader = new ManipulationGeometryShader(renderer->getDevice(), hwnd);

	// depth and shadow shaders
	manipTessDepthShader = new ManipulationTessDepthShader(renderer->getDevice(), hwnd);
	manipulationDepthShader = new ManipulationDepthShader(renderer->getDevice(), hwnd);
	tessDepthShader = new TessellationDepthShader(renderer->getDevice(), hwnd);
	depthShader = new DepthShader(renderer->getDevice(), hwnd);
	shadowShader = new ShadowShader(renderer->getDevice(), hwnd);

	// post processing shaders
	verticalBlurShader = new VerticalBlurShader(renderer->getDevice(), hwnd);
	horizontalBlurShader = new HorizontalBlurShader(renderer->getDevice(), hwnd);
	thresholdShader = new BloomThresholdShader(renderer->getDevice(), hwnd);
	mergeShader = new BloomMergeShader(renderer->getDevice(), hwnd);
	toneMapper = new ToneMapShader(renderer->getDevice(), hwnd);
}

void App1::initObjects(int width, int height)
{
	plane = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), 100);
	planeSphere = new TessellatedPlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), 30);
	sphere = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	cube = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
	ortho = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(),
		height / 3, height / 3, -width / 2.7, height / 2.7);
	fullScreenMesh = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(),
		width, height, 0.0, 0.0);

	for (int i = 0; i < 9; i++) {
		sampleMeshes[i] = new OrthoMesh(renderer->getDevice(), renderer->getDeviceContext(),
			aspectRatios[i].x, aspectRatios[i].y, 0, 0);
	}
}

void App1::initLights()
{
	for (int i = 0; i < 4; i++) {
		lightData[i] = new LightData;
	}

	// set light types
	lightType[0] = 0;
	lightType[1] = 0;
	lightType[2] = 1;
	lightType[3] = 2;

	// set defaults
	for (int i = 0; i < 4; i++) {
		lights[i] = new Light;
		lightData[i]->ambient = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		lightData[i]->diffuse = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

		lightData[i]->specularPower = 30.0f;
		lightData[i]->specularColour = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		lightData[i]->lightEnabled = true;
		lightData[i]->lightDirection = XMFLOAT3(0, -1, 0);
		spotOuterAngle[i] = 25.0f;
		spotInnerAngle[i] = 20.0f;
		spotFalloff[i] = 5.0f;
		nearPlane[i] = 1.0f;
		farPlane[i] = 100.0f;
		sceneHeight[i] = 100.0f;
		sceneWidth[i] = 100.0f;
		lights[i]->generateOrthoMatrix(sceneWidth[i], sceneHeight[i], nearPlane[i], farPlane[i]);
		mapBias[i] = 0.005f;
	}
	lightData[1]->lightEnabled = false;

	// change specific defaults
	nearPlane[0] = 18.0f;
	nearPlane[1] = 25.0f;
	nearPlane[2] = 8.8f;
	nearPlane[3] = 20.0f;

	farPlane[2] = 95.5f;

	mapBias[0] = 0.007f;
	mapBias[1] = 0.007f;
	mapBias[2] = 0.007f;
	mapBias[3] = 0.005f;

	lightData[0]->lightPosition = XMFLOAT3(-31.0f, 5.0f, -34.0f);
	lightData[1]->lightPosition = XMFLOAT3(-45.0f, 30.0f, 18.0f);
	lightData[2]->lightPosition = XMFLOAT3(30.0f, 20.0f, -0.5f);
	lightData[3]->lightPosition = XMFLOAT3(32.0f, 5.0f, -30.0f);

	lightData[0]->ambient = ImVec4(0.0f, 0.1f, 0.15f, 1.0f);

	lightData[0]->diffuse = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
	lightData[1]->diffuse = ImVec4(0.13f, 0.5f, 0.13f, 1.0f);
	lightData[2]->diffuse = ImVec4(0.4f, 0.12f, 0.12f, 1.0f);
	lightData[3]->diffuse = ImVec4(0.7f, 0.6f, 0.0f, 1.0f);

	lightData[3]->specularColour = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);

	lightData[1]->lightDirection = XMFLOAT3(1.0f, -0.6f, 0.0f); 
	lightData[2]->lightDirection = XMFLOAT3(-1.0f, -0.6f, 0.0f);
	lightData[3]->lightDirection = XMFLOAT3(0.0f, -0.6f, 1.0f);

}

void App1::initTextures(int screenWidth, int screenHeight)
{
	textureMgr->loadTexture(L"height", L"res/marsHeight3.png");
	textureMgr->loadTexture(L"mars", L"res/marsTexture.jpg");
	textureMgr->loadTexture(L"dwood", L"res/darkWood.jpg");
	textureMgr->loadTexture(L"lwood", L"res/wood.png");
	textureMgr->loadTexture(L"brick", L"res/brick1.dds");
	textureMgr->loadTexture(L"colour0", L"res/colour1.png");
	textureMgr->loadTexture(L"colour1", L"res/colour2.png");
	textureMgr->loadTexture(L"colour2", L"res/colour3.png");
	textureMgr->loadTexture(L"colour3", L"res/colour4.png");
	
	sphereTextures[0] = textureMgr->getTexture(L"colour0");
	sphereTextures[1] = textureMgr->getTexture(L"colour1");
	sphereTextures[2] = textureMgr->getTexture(L"colour2");
	sphereTextures[3] = textureMgr->getTexture(L"colour3");

	int shadowmapWidth = 2048;
	int shadowmapHeight = 2048;

	// create 24 shadow maps
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			shadowMap[i][j] = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);
		}
	}

	renderTexture = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	bloomFilter = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	blurFilter = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	toneMappedFilter = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	thresholdTexture = new RenderTexture(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);

	for (int i = 0; i < 9; i++) {
		horizontalBlurTexture[i] = new RenderTexture(renderer->getDevice(), aspectRatios[i].x, aspectRatios[i].y, SCREEN_NEAR, SCREEN_DEPTH);
		verticalBlurTexture[i] = new RenderTexture(renderer->getDevice(), aspectRatios[i].x, aspectRatios[i].y, SCREEN_NEAR, SCREEN_DEPTH);
	}

	for (int i = 0; i < 8; i++) {
		combinedPasses[i] = new RenderTexture(renderer->getDevice(), aspectRatios[i].x, aspectRatios[i].y, SCREEN_NEAR, SCREEN_DEPTH);
		upscaledPasses[i] = new RenderTexture(renderer->getDevice(), aspectRatios[i].x, aspectRatios[i].y, SCREEN_NEAR, SCREEN_DEPTH);
	}
}

void App1::initVariables(int screenWidth, int screenHeight)
{
	time = 0.0f;
	heightMapAmplitude = 14.0f;
	waveSettings[0] = XMFLOAT3(0.0f, 0.65f, 1.0f);
	waveSettings[1] = XMFLOAT3(0.0f, 0.5f, 1.0f);

	windSettings[0] = XMFLOAT3(0.83f, 0.22f, 1.0f);
	windSettings[1] = XMFLOAT3(0.33f, 1.5f, 1.0f);

	planeToSphere = 0.0f;
	spherePosition = XMFLOAT4(15.1f, 11.0f, 15.0f, 8.0f); // z value is radius
	mapToRender = 0;

	dirs[0] = XMFLOAT3(0.0f, -1.0f, 0.0f);
	dirs[1] = XMFLOAT3(0.0f,  1.0f, 0.0f);
	dirs[2] = XMFLOAT3(1.0f,  0.0f, 0.0f);
	dirs[3] = XMFLOAT3(-1.0f, 0.0f, 0.0f);
	dirs[4] = XMFLOAT3(0.0f,  0.0f, 1.0f);
	dirs[5] = XMFLOAT3(0.0f,  0.0f, -1.0f);

	cubePos[0] = XMFLOAT3(-30, -6.0f, 0);
	cubePos[1] = XMFLOAT3(30, -6.0f, 0);
	cubePos[2] = XMFLOAT3(0, -6.0f, 30);
	cubePos[3] = XMFLOAT3(0, -6.0f, -30);

	spherePos[0] = XMFLOAT3(-50, -4.0f, 50);
	spherePos[1] = XMFLOAT3(20, -4.0f, 50);
	spherePos[2] = XMFLOAT3(-20, -4.0f, 50);
	spherePos[3] = XMFLOAT3(50, -4.0f, 50);

	// init bools
	enableBlur = false;
	enableBloom = false;
	enablePP = false;
	displayMap = false;
	enableHDR = false;
	gammaCorrection = false;
	dynamicTess = false;
	surfaceLighting = false;
	enableGeometryShader = true;

	tessInsideFactor = 5;
	tessEdgeFactor = 5;
	bloomThreshold = 1.4f;
	blurPasses = 9;
	ppMode = 1;
	exposure = 1.0f;
	bloomIntensity = 1.0f;
	dynamicTessFar = 32.0f;
	dynamicTessNear = 10.0f;

	// get every 8th 16:9 aspect ratio to 128x72
	aspectRatios[0].x = 1200 - 16;
	aspectRatios[0].y = 675 - 9;

	aspectRatios[1].x = 1024;
	aspectRatios[1].y = 576;

	aspectRatios[2].x = 896;
	aspectRatios[2].y = 504;

	aspectRatios[3].x = 512;
	aspectRatios[3].y = 288;

	aspectRatios[4].x = 256;
	aspectRatios[4].y = 144;

	aspectRatios[5].x = 128;
	aspectRatios[5].y = 72;

	aspectRatios[6].x = 64;
	aspectRatios[6].y = 36;

	aspectRatios[7].x = 32;
	aspectRatios[7].y = 18;

	aspectRatios[8].x = 16;
	aspectRatios[8].y = 9;

	D3D11_RASTERIZER_DESC rasterDesc;
	// Recreate the default raster state
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = true;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	renderer->getDevice()->CreateRasterizerState(&rasterDesc, &defaultRasterState);

	// create a new raster state with backface culling off
	rasterDesc.CullMode = D3D11_CULL_NONE;
	renderer->getDevice()->CreateRasterizerState(&rasterDesc, &newRasterState);
}
#pragma endregion
