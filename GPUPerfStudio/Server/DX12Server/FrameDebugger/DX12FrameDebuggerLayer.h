//==============================================================================
/// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file DX12FrameDebuggerLayer.h
/// \brief The DX12FrameDebuggerLayer is responsible for operations related to 
/// inspecting individual frames of an instrumented D3D12 application.
//==============================================================================

#ifndef DX12FRAMEDEBUGGERLAYER_H
#define DX12FRAMEDEBUGGERLAYER_H

#include "../../Common/ILayer.h"
#include "../../Common/CommandProcessor.h"
#include "../../Common/TSingleton.h"
#include <unordered_map>

struct IDXGISwapChain;
struct ID3D12CommandQueue;

//--------------------------------------------------------------------------
/// The root layer for all work related to DX12 Frame Debugging.
//--------------------------------------------------------------------------
class DX12FrameDebuggerLayer : public ILayer, public CommandProcessor, public TSingleton< DX12FrameDebuggerLayer >
{
    friend TSingleton < DX12FrameDebuggerLayer >;
public:
    //--------------------------------------------------------------------------
    /// Default destructor for DX12FrameDebuggerLayer.
    //--------------------------------------------------------------------------
    virtual ~DX12FrameDebuggerLayer();

    //--------------------------------------------------------------------------
    /// Called to indicate that a resource is being created
    /// The layer must create its resources and hook functions here
    /// \param type the type of resource that is being created
    /// \param pPtr pointer to the resource that is being created
    /// \return should return false on error; true otherwise
    //--------------------------------------------------------------------------
    virtual bool OnCreate(CREATION_TYPE type, void* pPtr) { (void)type; (void)pPtr; return true; }

    //--------------------------------------------------------------------------
    /// Called to indicate that a resource is being destroyed
    /// detaches from anything that was attached in OnCreate
    /// \param type the type of resource that is being destroyed
    /// \param pPtr pointer to the resource that is being destroyed
    /// \return should return false on error; true otherwise
    //--------------------------------------------------------------------------
    virtual bool OnDestroy(CREATION_TYPE type, void* pPtr) { (void)type; (void)pPtr; return true; }

    //--------------------------------------------------------------------------
    /// Called at the end of a frame
    //--------------------------------------------------------------------------
    virtual void EndFrame();

    //--------------------------------------------------------------------------
    /// Gets called after a frame has been presented.
    /// \param inSwapchain The swapchain instance used to present to the screen.
    //--------------------------------------------------------------------------
    virtual void OnPresent(void* inSwapchain);

    //--------------------------------------------------------------------------
    /// Handler invoked when a new Swapchain is created.
    /// \param inSwapchain The swapchain instance that was just created.
    /// \param inDevice The CommandQueue used to create the swapchain.
    //--------------------------------------------------------------------------
    virtual void OnSwapchainCreated(void* inSwapchain, void* inDevice);

    //--------------------------------------------------------------------------
    /// Retrieve a Derived Settings string. Don't do anything in this case.
    //--------------------------------------------------------------------------
    virtual string GetDerivedSettings() { return ""; }

private:
    //--------------------------------------------------------------------------
    /// Default constructor for DX12FrameDebuggerLayer.
    //--------------------------------------------------------------------------
    DX12FrameDebuggerLayer();

    //--------------------------------------------------------------------------
    /// Capture the current backbuffer image, and return an byte array of PNG-encoded image data.
    /// \param inWidth The requested width of the captured backbuffer image.
    /// \param inHeight The requested height of the captured backbuffer image.
    /// \param ioBackBufferPngData A pointer to the byte array of PNG-encoded image data.
    /// \param outNumBytes The total number of bytes in the array of encoded image data.
    /// \returns True if the back buffer image was captured successfully. False if it failed.
    //--------------------------------------------------------------------------
    bool CaptureBackBuffer(unsigned int inWidth, unsigned int inHeight, unsigned char** ioBackBufferPngData, unsigned int* outNumBytes);

    //--------------------------------------------------------------------------
    /// Handle an incoming image request by sending the image data as a response.
    /// \param inImageCommand The command used to request image data.
    /// \returns True when the backbuffer was suceessfully captured and sent back as a response.
    //--------------------------------------------------------------------------
    bool HandleBackBufferRequest(PictureCommandResponse& inImageCommand);

    //--------------------------------------------------------------------------
    /// A command that allows the client to respond with PNG data of the back buffer image.
    //--------------------------------------------------------------------------
    PictureCommandResponse mGetBackBufferImage;

    //--------------------------------------------------------------------------
    /// The last swapchain used to present an image to the screen.
    //--------------------------------------------------------------------------
    IDXGISwapChain* mLastPresentedSwapchain;

    //--------------------------------------------------------------------------
    /// A type of map used to associate a swapchain with an ID3D12CommandQueue.
    //--------------------------------------------------------------------------
    typedef std::unordered_map<IDXGISwapChain*, ID3D12CommandQueue*> SwapchainToQueueMap;

    //--------------------------------------------------------------------------
    /// A map that associates a swapchain with the CommandQueue used to create it.
    //--------------------------------------------------------------------------
    SwapchainToQueueMap mSwapchainToCommandQueue;

};

#endif // DX12FRAMEDEBUGGERLAYER_H