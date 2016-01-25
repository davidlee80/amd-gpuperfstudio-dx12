//==============================================================================
// Copyright (c) 2014-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  A LayerManager responsible that acts as the entrypoint for the DX12
///         server plugin.
//==============================================================================

#ifndef DX12LAYERMANAGER_H
#define DX12LAYERMANAGER_H

#include "../Common/ModernAPILayerManager.h"
#include "../Common/defines.h"
#include "DX12Defines.h"

class DX12Interceptor;

extern bool CheckUpdateHooks();

//=============================================================================
/// DX12LayerManager - A LayerManager with the special task of managing
/// layers specific to DX12's runtime stack.
//=============================================================================
class DX12LayerManager : public ModernAPILayerManager
{
public:
    //--------------------------------------------------------------------------
    /// Default constructor, where available layers are determined.
    //--------------------------------------------------------------------------

    DX12LayerManager();
    //--------------------------------------------------------------------------
    /// Virtual destructor, where nothing important happens.
    //--------------------------------------------------------------------------
    virtual ~DX12LayerManager();

    //--------------------------------------------------------------------------
    /// Gets called after a frame has been presented.
    /// \param inSwapchain The swapchain instance used to present to the screen.
    //--------------------------------------------------------------------------
    virtual void OnPresent(void* inSwapchain);

    /// Gets called when a swapchain is created.
    /// \param inSwapchain The swapchain instance that was just created.
    /// \param inDevice The device used to created the swapchain.
    virtual void OnSwapchainCreated(void* inSwapchain, void* inDevice);

    //--------------------------------------------------------------------------
    /// Initialize the LayerManager by pushing available layers and starting the interceptors.
    //--------------------------------------------------------------------------
    bool InitializeLayerManager();

    //--------------------------------------------------------------------------
    /// Shutdown the LayerManager by detaching hooks and destroying instances.
    //--------------------------------------------------------------------------
    bool ShutdownLayerManager();

    //--------------------------------------------------------------------------
    /// Answer the "Has the DX12LayerManager been initialized?" question.
    /// \return True if the DX12LayerManager was initialized successfully.
    //--------------------------------------------------------------------------
    inline bool HasBeenInitialized() const { return mbIsInitialized; }

    //--------------------------------------------------------------------------
    /// Retrieve a pointer to the DX12Interceptor- the logic for call interception lives here.
    //--------------------------------------------------------------------------
    inline DX12Interceptor* GetInterceptor() const { return mInterceptor; }

protected:
    //--------------------------------------------------------------------------
    /// Retrieve basic timing and API usage information by filling in a FrameInfo
    /// instance.
    /// \param outFrameInfo The FrameInfo structure that will be populated with data.
    //--------------------------------------------------------------------------
    virtual void GetFrameInfo(FrameInfo& outFrameInfo);

private:
    //--------------------------------------------------------------------------
    /// An instance of the DX12Interceptor- the piece that helps decide what to do with intercepted calls.
    //--------------------------------------------------------------------------
    DX12Interceptor* mInterceptor;

    //--------------------------------------------------------------------------
    /// A true/false to indicate if the LayerManager was started successfully.
    //--------------------------------------------------------------------------
    bool mbIsInitialized;
};

//--------------------------------------------------------------------------
/// Accessor to the DX12 layer manager
//--------------------------------------------------------------------------
DX12LayerManager* GetDX12LayerManager();

#endif //DX12LAYERMANAGER_H