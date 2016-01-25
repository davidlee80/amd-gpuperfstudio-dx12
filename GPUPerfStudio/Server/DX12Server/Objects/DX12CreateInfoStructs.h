//==============================================================================
// Copyright (c) 2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Declarations of structures designed to hold all the parameters used to create interfaces.
//==============================================================================

#ifndef DX12CREATEINFOSTRUCTS_H
#define DX12CREATEINFOSTRUCTS_H

#include "DX12Defines.h"

class GPS_ID3D12Heap;
class GPS_ID3D12RootSignature;
class GPS_ID3D12CommandAllocator;
class GPS_ID3D12PipelineState;

//--------------------------------------------------------------------------
/// A baseclass for all CreateInfo types.
//--------------------------------------------------------------------------
class GPS_DX12CreateInfoBase
{
public:
    GPS_DX12CreateInfoBase() {}
    virtual ~GPS_DX12CreateInfoBase() {}

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const = 0;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new RootSignature instance.
//--------------------------------------------------------------------------
class GPS_ID3D12RootSignatureCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12RootSignatureCreateInfo(UINT inNodeMask, const void* inBlobWithRootSignature, SIZE_T inBlobLengthInBytes);
    virtual ~GPS_ID3D12RootSignatureCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    UINT mNodeMask;
    char* mBlobWithRootSignature;
    SIZE_T mBlobLengthInBytes;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new RootSignature instance.
//--------------------------------------------------------------------------
class GPS_ID3D12RootSignatureDeserializerCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12RootSignatureDeserializerCreateInfo(LPCVOID pSrcData, SIZE_T SrcDataSizeInBytes);
    virtual ~GPS_ID3D12RootSignatureDeserializerCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    char* mSrcData;
    SIZE_T mSrcDataSizeInBytes;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Heap instance.
//--------------------------------------------------------------------------
class GPS_ID3D12HeapCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12HeapCreateInfo(const D3D12_HEAP_DESC* inDescription);
    virtual ~GPS_ID3D12HeapCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    D3D12_HEAP_DESC* mDescription;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Resource instance.
//--------------------------------------------------------------------------
class GPS_ID3D12ResourceCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12ResourceCreateInfo(const D3D12_RESOURCE_DESC* inResourceDescription, D3D12_RESOURCE_STATES inInitialState, const D3D12_CLEAR_VALUE* inOptimizedClearValue);
    virtual ~GPS_ID3D12ResourceCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

protected:
    D3D12_RESOURCE_DESC* mResourceDescription;
    D3D12_RESOURCE_STATES mInitialState;
    D3D12_CLEAR_VALUE* mOptimizedClearValue;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Resource instance.
//--------------------------------------------------------------------------
class GPS_ID3D12ComittedResourceCreateInfo : public GPS_ID3D12ResourceCreateInfo
{
public:
    GPS_ID3D12ComittedResourceCreateInfo(const D3D12_HEAP_PROPERTIES* pHeapProperties, D3D12_HEAP_FLAGS inHeapFlags, const D3D12_RESOURCE_DESC* inResourceDescription, D3D12_RESOURCE_STATES inInitialState, const D3D12_CLEAR_VALUE* inOptimizedClearValue);
    virtual ~GPS_ID3D12ComittedResourceCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    D3D12_HEAP_PROPERTIES* mHeapProperties;
    D3D12_HEAP_FLAGS mHeapFlags;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Resource instance.
//--------------------------------------------------------------------------
class GPS_ID3D12PlacedResourceCreateInfo : public GPS_ID3D12ResourceCreateInfo
{
public:
    GPS_ID3D12PlacedResourceCreateInfo(GPS_ID3D12Heap* inHeap, UINT64 inHeapOffset, const D3D12_RESOURCE_DESC* inResourceDescription, D3D12_RESOURCE_STATES inInitialState, const D3D12_CLEAR_VALUE* inOptimizedClearValue);
    virtual ~GPS_ID3D12PlacedResourceCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    GPS_ID3D12Heap* mHeap;
    UINT64 mHeapOffset;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Resource instance.
//--------------------------------------------------------------------------
class GPS_ID3D12ReservedResourceCreateInfo : public GPS_ID3D12ResourceCreateInfo
{
public:
    GPS_ID3D12ReservedResourceCreateInfo(const D3D12_RESOURCE_DESC* inResourceDescription, D3D12_RESOURCE_STATES inInitialState, const D3D12_CLEAR_VALUE* inOptimizedClearValue);
    virtual ~GPS_ID3D12ReservedResourceCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new CommandAllocator instance.
//--------------------------------------------------------------------------
class GPS_ID3D12CommandAllocatorCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12CommandAllocatorCreateInfo(D3D12_COMMAND_LIST_TYPE inCommandListType);
    virtual ~GPS_ID3D12CommandAllocatorCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    D3D12_COMMAND_LIST_TYPE mCommandListType;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Fence instance.
//--------------------------------------------------------------------------
class GPS_ID3D12FenceCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12FenceCreateInfo(UINT64 inInitialValue, D3D12_FENCE_FLAGS inFlags);
    virtual ~GPS_ID3D12FenceCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    UINT64 mInitialValue;
    D3D12_FENCE_FLAGS mFlags;
};

//--------------------------------------------------------------------------
/// A baseclass designed of the type used to hold info used to create a PipelineState instance.
//--------------------------------------------------------------------------
class GPS_ID3D12PipelineStateCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12PipelineStateCreateInfo() {}
    virtual ~GPS_ID3D12PipelineStateCreateInfo() {}
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Graphics PipelineState instance.
//--------------------------------------------------------------------------
class GPS_ID3D12GraphicsPipelineStateCreateInfo : public GPS_ID3D12PipelineStateCreateInfo
{
public:
    GPS_ID3D12GraphicsPipelineStateCreateInfo(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* inGraphicsPipelineDescription);
    virtual ~GPS_ID3D12GraphicsPipelineStateCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC* mGraphicsPipelineDescription;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Compute PipelineState instance.
//--------------------------------------------------------------------------
class GPS_ID3D12ComputePipelineStateCreateInfo : public GPS_ID3D12PipelineStateCreateInfo
{
public:
    GPS_ID3D12ComputePipelineStateCreateInfo(const D3D12_COMPUTE_PIPELINE_STATE_DESC* inComputePipelineDescription);
    virtual ~GPS_ID3D12ComputePipelineStateCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    D3D12_COMPUTE_PIPELINE_STATE_DESC* mComputePipelineDescription;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new DescriptorHeap instance.
//--------------------------------------------------------------------------
class GPS_ID3D12DescriptorHeapCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12DescriptorHeapCreateInfo(const D3D12_DESCRIPTOR_HEAP_DESC* inDescription);
    virtual ~GPS_ID3D12DescriptorHeapCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    D3D12_DESCRIPTOR_HEAP_DESC* mDescription;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new QueryHeap instance.
//--------------------------------------------------------------------------
class GPS_ID3D12QueryHeapCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12QueryHeapCreateInfo(const D3D12_QUERY_HEAP_DESC* inDescription);
    virtual ~GPS_ID3D12QueryHeapCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    D3D12_QUERY_HEAP_DESC* mDescription;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new CommandSignature instance.
//--------------------------------------------------------------------------
class GPS_ID3D12CommandSignatureCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12CommandSignatureCreateInfo(const D3D12_COMMAND_SIGNATURE_DESC* inDescription, GPS_ID3D12RootSignature* inRootSignature);
    virtual ~GPS_ID3D12CommandSignatureCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    D3D12_COMMAND_SIGNATURE_DESC* mDescription;
    GPS_ID3D12RootSignature* mRootSignature;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new CommandList instance.
//--------------------------------------------------------------------------
class GPS_ID3D12CommandListCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12CommandListCreateInfo(UINT inNodeMask, D3D12_COMMAND_LIST_TYPE inType, GPS_ID3D12CommandAllocator* inCommandAllocator, GPS_ID3D12PipelineState* inInitialState);
    virtual ~GPS_ID3D12CommandListCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    UINT mNodeMask;
    D3D12_COMMAND_LIST_TYPE mType;
    GPS_ID3D12CommandAllocator* mCommandAllocator;
    GPS_ID3D12PipelineState* mInitialState;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new CommandQueue instance.
//--------------------------------------------------------------------------
class GPS_ID3D12CommandQueueCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12CommandQueueCreateInfo(const D3D12_COMMAND_QUEUE_DESC* inCommandQueueDesc);
    virtual ~GPS_ID3D12CommandQueueCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

    D3D12_COMMAND_QUEUE_DESC* GetDescription() const { return mQueueDescription; }

private:
    D3D12_COMMAND_QUEUE_DESC* mQueueDescription;
};

//--------------------------------------------------------------------------
/// A class designed to hold all info used to create a new Device instance.
//--------------------------------------------------------------------------
class GPS_ID3D12DeviceCreateInfo : public GPS_DX12CreateInfoBase
{
public:
    GPS_ID3D12DeviceCreateInfo(IUnknown* inAdapater, D3D_FEATURE_LEVEL inFeatureLevel);
    virtual ~GPS_ID3D12DeviceCreateInfo();

    virtual void AppendCreateInfoXML(gtASCIIString& outCreateInfoXML) const;

private:
    IUnknown* mAdapater;
    D3D_FEATURE_LEVEL mMinimumFeatureLevel;
};

#endif // DX12CREATEINFOSTRUCTS_H
