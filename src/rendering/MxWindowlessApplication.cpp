/*
 * MxWindowlessApplication.cpp
 *
 *  Created on: Mar 27, 2019
 *      Author: andy
 */

#include <rendering/MxWindowlessApplication.h>
#include <rendering/MxWindow.h>
#include <rendering/MxUniverseRenderer.h>

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Image.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Version.h>

#include <iostream>

/**
 *
 */
struct MxWindowlessWindow : MxWindow
{
    MxWindowlessApplication *app;
    
    Magnum::Vector2i windowSize() const override {
        return app->framebuffer().viewport().size();
    };
    
    void redraw() override {
        app->redraw();
    }
    
    Magnum::GL::AbstractFramebuffer &framebuffer() override {
        return app->frameBuffer;
    }
    
    /**
     * attach to an existing GLFW Window
     */
    MxWindowlessWindow(MxWindowlessApplication *_app) : app{_app} {
    };
};

MxWindowlessApplication::~MxWindowlessApplication()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

MxWindowlessApplication::MxWindowlessApplication(const Arguments &args) :
    WindowlessApplication{args, Magnum::NoCreate},
    renderBuffer{Magnum::NoCreate},
    frameBuffer{Magnum::NoCreate},
    depthStencil{Magnum::NoCreate}
{
}

HRESULT MxWindowlessApplication::createContext(const MxSimulator::Config &conf) {

    // default Magnum WindowlessApplication config, does not have any options
    Configuration windowlessConf;
    
    if(!WindowlessApplication::tryCreateContext(windowlessConf)) {
        return c_error(E_FAIL, "could not create windowless context");
    }
    
    Vector2i size = conf.windowSize();

    // create the render buffers here, after we have a context,
    // default ctor makes this with a {Magnum::NoCreate},
    renderBuffer = Magnum::GL::Renderbuffer();
    depthStencil = Magnum::GL::Renderbuffer();
    
    depthStencil.setStorage(GL::RenderbufferFormat::Depth24Stencil8, size);
    
    renderBuffer.setStorage(Magnum::GL::RenderbufferFormat::RGBA8, size);
    
    frameBuffer = Magnum::GL::Framebuffer{{{0,0}, size}};
    
    frameBuffer
        .attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, renderBuffer)
        .attachRenderbuffer(GL::Framebuffer::BufferAttachment::DepthStencil, depthStencil)
        .clear(GL::FramebufferClear::Color)
        .bind();
    
    window = new MxWindowlessWindow(this);

    // renderer accesses the framebuffer from the window handle we pass in.
    renderer = new MxUniverseRenderer(window);
    
    return S_OK;
}


MxUniverseRenderer *MxWindowlessApplication::getRenderer() {
    return renderer;
}

HRESULT MxWindowlessApplication::MxWindowlessApplication::pollEvents()
{
    return E_NOTIMPL;
}

HRESULT MxWindowlessApplication::MxWindowlessApplication::waitEvents()
{
    return E_NOTIMPL;
}

HRESULT MxWindowlessApplication::MxWindowlessApplication::waitEventsTimeout(
        double timeout)
{
    return E_NOTIMPL;
}

HRESULT MxWindowlessApplication::MxWindowlessApplication::postEmptyEvent()
{
    return E_NOTIMPL;
}

HRESULT MxWindowlessApplication::mainLoopIteration(double timeout)
{
    return E_NOTIMPL;
}

struct MxGlfwWindow* MxWindowlessApplication::getWindow()
{
    return NULL;
}

int MxWindowlessApplication::windowAttribute(MxWindowAttributes attr)
{
    return E_NOTIMPL;
}

HRESULT MxWindowlessApplication::setWindowAttribute(MxWindowAttributes attr,
        int val)
{
    return E_NOTIMPL;
}

HRESULT MxWindowlessApplication::redraw()
{
    //frameBuffer.attachRenderbuffer(GL::Framebuffer::ColorAttachment{0}, renderBuffer)
    frameBuffer
        .clear(GL::FramebufferClear::Color)
        .bind();
    
    /* Draw particles */
    renderer->draw();
    
    return S_OK;
}

HRESULT MxWindowlessApplication::close()
{
    return E_NOTIMPL;
}

HRESULT MxWindowlessApplication::destroy()
{
    return E_NOTIMPL;
}

HRESULT MxWindowlessApplication::show()
{
    return redraw();
}

HRESULT MxWindowlessApplication::messageLoop()
{
    return E_NOTIMPL;
}

Magnum::GL::AbstractFramebuffer& MxWindowlessApplication::framebuffer() {
    return frameBuffer;
}
