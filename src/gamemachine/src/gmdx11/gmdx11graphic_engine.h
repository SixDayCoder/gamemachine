﻿#ifndef __GMDX11GRAPHIC_ENGINE_H__
#define __GMDX11GRAPHIC_ENGINE_H__
#include <gmcommon.h>
#include <gmcom.h>
#include <gmdxincludes.h>
#include <gmmodel.h>
#include <tools.h>
BEGIN_NS

struct GMMVPMatrix;
GM_PRIVATE_OBJECT(GMDx11GraphicEngine)
{
	GMComPtr<ID3D11Device> device;
	GMComPtr<ID3D11DeviceContext> deviceContext;
	GMComPtr<IDXGISwapChain> swapChain;
	GMComPtr<ID3D11DepthStencilView> depthStencilView;
	GMComPtr<ID3D11RenderTargetView> renderTargetView;
	GMScopePtr<IShaderProgram> shaderProgram;

	IShaderLoadCallback* shaderLoadCallback = nullptr;
	bool ready = false;
};

class GMDx11GraphicEngine : public GMObject, public IGraphicEngine
{
	DECLARE_PRIVATE(GMDx11GraphicEngine)

public:
	virtual void init() override;
	virtual void newFrame() override;
	virtual void drawObjects(GMGameObject *objects[], GMuint count, GMBufferMode bufferMode) override;
	virtual void update(GMUpdateDataType type) override;
	virtual void addLight(const GMLight& light) override;
	virtual void removeLights() override;
	virtual void clearStencil() override;
	virtual void beginCreateStencil() override;
	virtual void endCreateStencil() override;
	virtual void beginUseStencil(bool inverse) override;
	virtual void endUseStencil() override;
	virtual void beginBlend(GMS_BlendFunc sfactor, GMS_BlendFunc dfactor) override;
	virtual void endBlend() override;
	virtual IShaderProgram* getShaderProgram(GMShaderProgramType type = GMShaderProgramType::CurrentShaderProgram) override;
	virtual bool event(const GameMachineMessage& e) override;
	virtual void setShaderLoadCallback(IShaderLoadCallback* cb) override
	{
		D(d);
		d->shaderLoadCallback = cb;
	}

public:
	virtual bool setInterface(GameMachineInterfaceID, void*);
	virtual bool getInterface(GameMachineInterfaceID, void**);

public:
	GMComPtr<ID3D11Device> getDevice()
	{
		D(d);
		return d->device;
	}

	GMComPtr<ID3D11DeviceContext> getDeviceContext()
	{
		D(d);
		return d->deviceContext;
	}

	GMComPtr<IDXGISwapChain> getSwapChain()
	{
		D(d);
		return d->swapChain;
	}

	GMComPtr<ID3D11DepthStencilView> getDepthStencilView()
	{
		D(d);
		return d->depthStencilView;
	}

	GMComPtr<ID3D11RenderTargetView> getRenderTargetView()
	{
		D(d);
		return d->renderTargetView;
	}

public:
	IRenderer* getRenderer(GMModelType objectType);

private:
	void initShaders();
	void forwardDraw(GMGameObject *objects[], GMuint count);
	void forwardRender(GMGameObject *objects[], GMuint count);
	void directDraw(GMGameObject *objects[], GMuint count);
};

END_NS
#endif