
#include "BaseMesh.h"

class TessellatedPlaneMesh : public BaseMesh
{

public:

	TessellatedPlaneMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int resolution = 100);
	~TessellatedPlaneMesh();

	void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top) override;

protected:
	void initBuffers(ID3D11Device* device);
	int resolution;
};
