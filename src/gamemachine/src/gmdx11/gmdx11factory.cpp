﻿#include "stdafx.h"
#include "gmdx11factory.h"
#include "gmdx11graphic_engine.h"
#include "gmdx11modeldataproxy.h"
#include "gmdx11texture.h"
#include "gmdx11glyphmanager.h"
#include "gmdx11framebuffer.h"
#include "gmdx11gbuffer.h"
#include "gmdx11light.h"
#include "gmengine/ui/gmwindow.h"
#include "gmdx11helper.h"
#include <regex>
#include "gmdx11techniques.h"

void GMDx11Factory::createWindow(GMInstance instance, IWindow* parent, OUT IWindow** window)
{
	bool b = GMWindowFactory::createWindowWithDx11(instance, parent, window);
	GM_ASSERT(b);
}

void GMDx11Factory::createTexture(const IRenderContext* context, GMImage* image, REF GMTextureAsset& texture)
{
	GMDx11Texture* t = new GMDx11Texture(context, image);
	t->init();
	texture = GMAsset(GMAssetType::Texture, t);
}

void GMDx11Factory::createModelDataProxy(const IRenderContext* context, GMModel* model, OUT GMModelDataProxy** modelDataProxy)
{
	GM_ASSERT(modelDataProxy);
	(*modelDataProxy) = new GMDx11ModelDataProxy(context, model);
}

void GMDx11Factory::createGlyphManager(const IRenderContext* context, OUT GMGlyphManager** glyphManager)
{
	GM_ASSERT(glyphManager);
	*glyphManager = new GMDx11GlyphManager(context);
}

void GMDx11Factory::createFramebuffer(const IRenderContext* context, OUT IFramebuffer** fb)
{
	*fb = new GMDx11Framebuffer(context);
}

void GMDx11Factory::createFramebuffers(const IRenderContext* context, OUT IFramebuffers** fbs)
{
	*fbs = new GMDx11Framebuffers(context);
}

void GMDx11Factory::createGBuffer(const IRenderContext* context, OUT IGBuffer** g)
{
	*g = new GMDx11GBuffer(context);
}

void GMDx11Factory::createLight(GMLightType type, OUT ILight** out)
{
	if (type == GMLightType::Ambient)
	{
		*out = new GMDx11AmbientLight();
	}
	else
	{
		GM_ASSERT(type == GMLightType::Direct);
		*out = new GMDx11DirectLight();
	}
}

void GMDx11Factory::createWhiteTexture(const IRenderContext* context, REF GMTextureAsset& texture)
{
	GMDx11WhiteTexture* t = new GMDx11WhiteTexture(context);
	t->init();
	texture = GMAsset(GMAssetType::Texture, t);
}

namespace
{
	const GMString s_technique11_template =
		L"technique11 {id}\n"
		L"{\n"
		L"    pass P0\n"
		L"    {\n"
		L"        SetVertexShader({vs});\n"
		L"        SetPixelShader({ps});\n"
		L"        SetGeometryShader({gs});\n"
		L"        SetRasterizerState(GM_RasterizerState);\n"
		L"        SetDepthStencilState(GM_DepthStencilState, 1);\n"
		L"        SetBlendState(GM_BlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);\n"
		L"    }\n"
		L"}";

	GMString getVS(const GMString& code)
	{
		GMString expr = L"VS_OUTPUT (.+)(.*)\\(VS_INPUT (.+)\\)(.*)";
		std::string value;
		std::regex regPair(expr.toStdString());
		std::smatch match;
		std::string c = code.toStdString();
		if (std::regex_search(c, match, regPair))
		{
			GM_ASSERT(match.size() >= 3);
			value = match[1].str();
		}
		return value;
	}

	GMString getPS(const GMString& code)
	{
		GMString expr = L"float4 (.+)(.*)\\(PS_INPUT (.+)\\)(.*)SV_TARGET(.*)";
		std::string value;
		std::regex regPair(expr.toStdString());
		std::smatch match;
		std::string c = code.toStdString();
		if (std::regex_search(c, match, regPair))
		{
			GM_ASSERT(match.size() >= 3);
			value = match[1].str();
		}
		return value;
	}

	GMString getGS(const GMString& code)
	{
		GMString expr = L"void (.+)(.*)\\((.*)(point|line|triangle|lineadj|triangleadj) (.+)\\)(.*)";
		std::string value;
		std::regex regPair(expr.toStdString());
		std::smatch match;
		std::string c = code.toStdString();
		if (std::regex_search(c, match, regPair))
		{
			GM_ASSERT(match.size() >= 3);
			value = match[1].str();
		}
		return value;
	}
}

void GMDx11Factory::createShaderPrograms(const IRenderContext* context, const GMRenderTechniqueManager& manager, REF Vector<IShaderProgram*>* out)
{
	IShaderProgram* shaderProgram = context->getEngine()->getShaderProgram();
	if (shaderProgram)
	{
		gm_warning(gm_dbg_wrap("Shader program is inited. So this function won't work."));
	}
	else
	{
		GMString fxCode;
		GMString fxPath;
		// 先找到原始的Effect代码
		bool foundFx = false;
		for (auto& renderTechniques : manager.getRenderTechniques())
		{
			for (auto& technique : renderTechniques.getTechniques())
			{
				if (technique.getShaderType() == GMShaderType::Effect)
				{
					fxCode = technique.getCode(GMRenderEnvironment::DirectX11);
					fxPath = technique.getPath(GMRenderEnvironment::DirectX11);
					foundFx = true;
					break;
				}
			}
			if (foundFx)
				break;
		}

		if (!foundFx)
		{
			gm_error(gm_dbg_wrap("An effect file must be identified."));
			return;
		}

		if (fxPath.isEmpty())
		{
			gm_error(gm_dbg_wrap("The Effect file path must be identified."));
			return;
		}

		// 注入effect代码
		fxCode += L"\n";
		for (auto& renderTechniques : manager.getRenderTechniques())
		{
			bool skip = false;
			GMString vs, ps, gs;
			// 先通过匹配，获取VS, PS入口
			for (auto& technique : renderTechniques.getTechniques())
			{
				if (technique.getShaderType() == GMShaderType::Effect)
				{
					skip = true;
					break;
				}

				const GMString& code = technique.getCode(GMRenderEnvironment::DirectX11);

				if (technique.getShaderType() == GMShaderType::Vertex)
					vs = getVS(code);
				else if (technique.getShaderType() == GMShaderType::Pixel)
					ps = getPS(code);
				else if (technique.getShaderType() == GMShaderType::Geometry)
					gs = getGS(code);

				fxCode += code;
				fxCode += L"\n";
			}

			// 跳过Effect着色器
			if (skip)
				continue;

			// 拿到PS, VS后，通过s_technique11_template插入fx底部
			fxCode += s_technique11_template
				.replace(L"{vs}", vs.isEmpty() ? L"NULL" : L"CompileShader(vs_5_0," + vs + L"())")
				.replace(L"{ps}", vs.isEmpty() ? L"NULL" : L"CompileShader(ps_5_0," + ps + L"())")
				.replace(L"{gs}", gs.isEmpty() ? L"NULL" : L"CompileShader(gs_5_0," + gs + L"())")
				.replace(L"{id}", GMDx11Technique::getTechniqueNameByTechniqueId(renderTechniques.getId()) );
			fxCode += L"\n";
		}

		// 生成
		GMDx11Helper::loadEffectShader(context->getEngine(), fxCode, fxPath);

		// 填充，当然Dx11下只有1个Shader
		for (auto& renderTechniques : manager.getRenderTechniques())
		{
			if (out)
				out->push_back(shaderProgram);
		}
	}
}