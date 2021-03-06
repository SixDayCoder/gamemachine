﻿#ifndef __GMWINDOW_H__
#define __GMWINDOW_H__
#include <gmcommon.h>
BEGIN_NS

class GMRenderContext : public IRenderContext
{
public:
	virtual IWindow* getWindow() const override;
	virtual IGraphicEngine* getEngine() const override;

public:
	inline void setWindow(IWindow* window)
	{
		this->window = window;
	}

	inline void setEngine(IGraphicEngine* engine)
	{
		this->engine = engine;
	}

private:
	IWindow* window = nullptr;
	IGraphicEngine* engine = nullptr;
};

GM_PRIVATE_OBJECT(GMWindow)
{
	GMOwnedPtr<IInput> input;
	GMOwnedPtr<IGameHandler> handler;
	GMOwnedPtr<IGraphicEngine> engine;
	GMOwnedPtr<IRenderContext> context;

	GMWindowHandle handle = 0;
	Vector<GMWidget*> widgets;
	GMWindowStates windowStates;
	GMCursorType cursor = GMCursorType::Arrow;
};

class GMWindow : public GMObject, public IWindow
{
	GM_DECLARE_PRIVATE(GMWindow)

public:
	GMWindow();
	~GMWindow();

public:
	virtual IInput* getInputMananger() override;
	virtual void msgProc(const GMMessage& message) override;
	virtual GMRect getWindowRect() override;
	virtual GMRect getRenderRect() override;
	virtual void centerWindow() override;
	virtual bool isWindowActivate() override;
	virtual void setWindowCapture(bool capture) override;
	virtual void showWindow() override;
	virtual GMWindowHandle create(const GMWindowAttributes& wndAttrs) override;
	virtual GMWindowHandle getWindowHandle() const override { D(d); return d->handle; }
	virtual bool addWidget(GMWidget* widget) override;
	virtual void setHandler(AUTORELEASE IGameHandler* handler) override;
	virtual IGameHandler* getHandler() override;
	virtual const GMWindowStates& getWindowStates() override;
	virtual void setCursor(GMCursorType cursorType) override;
	virtual GMWindowProcHandler getProcHandler() override;

public:
	virtual bool getInterface(GameMachineInterfaceID id, void** out) override;
	virtual bool setInterface(GameMachineInterfaceID id, void* in) override;

public:
	inline void setWindowHandle(GMWindowHandle handle)
	{
		D(d);
		d->handle = handle;
	}

	// 以下是由GMWindow子类override的
public:
	virtual bool handleSystemEvent(GMSystemEvent* event, REF GMLResult& result);
	virtual const GMwchar* getWindowClassName() { return L"GameMachine Window"; }
	virtual void onWindowCreated(const GMWindowAttributes& wndAttrs) {}

protected:
	virtual void changeCursor();
};

class GMWindowFactory
{
public:
	static bool createWindowWithOpenGL(GMInstance instance, IWindow* parent, OUT IWindow** window);
	static bool createWindowWithDx11(GMInstance instance, IWindow* parent, OUT IWindow** window);
};
END_NS
#endif