﻿#include "stdafx.h"
#include "customshader.h"
#include <linearmath.h>
#include <gmwidget.h>

static gm::GMRenderTechinqueID s_techid;

void Demo_CustomShader::init()
{
	D(d);
	Base::init();

	// 创建对象
	getDemoWorldReference().reset(new gm::GMDemoGameWorld(d->parentDemonstrationWorld->getContext()));

	// 创建一个带纹理的对象
	gm::GMModel* quad = new gm::GMModel();
	quad->setPrimitiveTopologyMode(gm::GMTopologyMode::Triangles);
	quad->setDrawMode(gm::GMModelDrawMode::Index);
	gm::GMMesh* mesh = new gm::GMMesh(quad);

	gm::GMVertex V1 = { { -1.f, -.5f, 0 },{ 0, 0, -1 },{ 0, 1 } };
	gm::GMVertex V2 = { { -1.f, .5f, 0 },{ 0, 0, -1 },{ 0, 0 } };
	gm::GMVertex V3 = { { 1.f, -.5f, 0 },{ 0, 0, -1 },{ 1, 1 } };
	gm::GMVertex V4 = { { 1.f, .5f, 0 },{ 0, 0, -1 },{ 1, 0 } };
	mesh->vertex(V1);
	mesh->vertex(V2);
	mesh->vertex(V3);
	mesh->vertex(V4);
	mesh->index(0);
	mesh->index(1);
	mesh->index(2);

	gm::GMMesh* mesh2 = new gm::GMMesh(quad);
	mesh2->vertex(V3);
	mesh2->vertex(V2);
	mesh2->vertex(V4);
	mesh2->index(0);
	mesh2->index(1);
	mesh2->index(2);

	gm::GMAsset asset = gm::GMAsset(gm::GMAssetType::Model, quad);
	quad->getShader().getMaterial().kd = GMVec3(1, 1, 1);
	quad->getShader().getMaterial().ks = GMVec3(0);

	gm::GMTextureAsset tex;
	gm::GMAsyncResult* result = nullptr;
	tex = gm::GMToolUtil::createTexture(getDemoWorldReference()->getContext(), "gamemachine.png");
	gm::GM_delete(result);

	gm::GMToolUtil::addTextureToShader(quad->getShader(), tex, gm::GMTextureType::Diffuse);
	getDemoWorldReference()->getAssets().addAsset(tex);

	quad->setTechniqueId(s_techid);
	quad->setType(gm::GMModelType::Custom);

	getDemoWorldReference()->getAssets().addAsset(asset);
	gm::GMGameObject* obj = new gm::GMGameObject(asset);
	asDemoGameWorld(getDemoWorldReference())->addObject("texture", obj);

	gm::GMWidget* widget = createDefaultWidget();
	widget->setSize(widget->getSize().width, getClientAreaTop() + 40);
}

void Demo_CustomShader::event(gm::GameMachineHandlerEvent evt)
{
	D_BASE(db, Base);
	D(d);
	Base::event(evt);
	switch (evt)
	{
	case gm::GameMachineHandlerEvent::FrameStart:
		break;
	case gm::GameMachineHandlerEvent::FrameEnd:
		break;
	case gm::GameMachineHandlerEvent::Update:
		getDemoWorldReference()->updateGameWorld(GM.getRunningStates().lastFrameElpased);
		break;
	case gm::GameMachineHandlerEvent::Render:
		getDemoWorldReference()->renderScene();
		break;
	case gm::GameMachineHandlerEvent::Activate:
		break;
	case gm::GameMachineHandlerEvent::Deactivate:
		break;
	case gm::GameMachineHandlerEvent::Terminate:
		break;
	default:
		break;
	}
}

void Demo_CustomShader::initCustomShader(const gm::IRenderContext* context)
{
	gm::IGraphicEngine* engine = context->getEngine();
	gm::GMRenderTechniques techs;
	gm::GMRenderTechnique vertexTech(gm::GMShaderType::Vertex);
	vertexTech.setCode(
		gm::GMRenderEnvironment::OpenGL,
		L"#version 330 core\n"
		L"layout(location = 0) in vec3 position;"
		L"layout(location = 1) in vec3 normal;"
		L"layout(location = 2) in vec2 uv;"
		L"layout(location = 3) in vec3 tangent;"
		L"layout(location = 4) in vec3 bitangent;"
		L"layout(location = 5) in vec2 lightmapuv;"
		L"layout(location = 6) in vec4 color;"
		L"out vec2 vs_uv;"
		L"uniform mat4 GM_ViewMatrix;"
		L"uniform mat4 GM_WorldMatrix;"
		L"uniform mat4 GM_ProjectionMatrix;"
		L"void main(void)"
		L"{"
		L" vs_uv = uv;"
		L" gl_Position = GM_ProjectionMatrix * GM_ViewMatrix * GM_WorldMatrix * vec4(position, 1);"
		L"}"
	);
	vertexTech.setCode(
		gm::GMRenderEnvironment::DirectX11,
		L"VS_OUTPUT VS_Custom(VS_INPUT input)\n"
		L"{\n"
		L"	return VS_3D(input);\n"
		L"}\n"
	);
	techs.addRenderTechnique(vertexTech);

	gm::GMRenderTechnique geometryTech(gm::GMShaderType::Geometry);
	geometryTech.setCode(
		gm::GMRenderEnvironment::OpenGL,
		L"#version 330 core\n														"
		L"layout(triangles) in;														"
		L"layout(triangle_strip, max_vertices = 3) out;								"
		L"in vec2 vs_uv[];															"
		L"out vec2 gs_uv;															"
		L"																			"
		L"void main() {																"
		L"	gs_uv = vs_uv[0];														"
		L"	gl_Position = gl_in[0].gl_Position + vec4(-0.1, 0.0, 0.0, 0.0);			"
		L"	EmitVertex();															"
		L"																			"
		L"	gs_uv = vs_uv[1];														"
		L"	gl_Position = gl_in[1].gl_Position + vec4(0.0, 0.5, 0.0, 0.0);			"
		L"	EmitVertex();															"
		L"																			"
		L"	gs_uv = vs_uv[2];														"
		L"	gl_Position = gl_in[2].gl_Position + vec4(-0.1, 0.0, 0.1, 0.0);			"
		L"	EmitVertex();															"
		L"																			"
		L"	EndPrimitive();															"
		L"}																			"
	);
	geometryTech.setCode(
		gm::GMRenderEnvironment::DirectX11,
		L"[maxvertexcount(3)]\n"
		L"void GS_Custom(triangle VS_OUTPUT input[3], inout TriangleStream<GS_OUTPUT> output) {\n"
		L"	GS_OUTPUT g[3];\n"
		L"	g[0] = input[0];\n"
		L"	g[1] = input[1];\n"
		L"	g[2] = input[2];\n"
		L"	g[0].Position = g[0].Position + float4(-0.1, 0.0, 0.0, 0.0);\n"
		L"	g[1].Position = g[1].Position + float4(0.0, 0.5, 0.0, 0.0);\n"
		L"	g[2].Position = g[2].Position + float4(-0.1, 0.0, 0.1, 0.0);\n"
		L"	output.Append(g[0]);\n"
		L"	output.Append(g[1]);\n"
		L"	output.Append(g[2]);\n"
		L"}\n"
	);
	techs.addRenderTechnique(geometryTech);

	gm::GMRenderTechnique pixelTech(gm::GMShaderType::Pixel);
	pixelTech.setCode(
		gm::GMRenderEnvironment::OpenGL,
		L"#version 330 core\n"
		L"in vec2 gs_uv;"
		L"struct GMTexture"
		L"{"
		L"	sampler2D Texture;"
		L"	float OffsetX;"
		L"	float OffsetY;"
		L"	float ScaleX;"
		L"	float ScaleY;"
		L"	int Enabled;"
		L"};"
		L"uniform GMTexture GM_DiffuseTextureAttribute;"
		L"void main(void) { gl_FragColor = texture(GM_DiffuseTextureAttribute.Texture, gs_uv); }"
	);
	pixelTech.setCode(
		gm::GMRenderEnvironment::DirectX11,
		L"float4 PS_Custom(PS_INPUT input) : SV_TARGET\n"
		L"{\n"
		L"	float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);\n"
		L"	color += GM_DiffuseTextureAttribute.Sample(GM_DiffuseTexture, GM_DiffuseSampler, input.Texcoord);\n"
		L"	return color;\n"
		L"}"
	);
	techs.addRenderTechnique(pixelTech);

	s_techid = engine->getRenderTechniqueManager()->addRenderTechniques(techs);
}