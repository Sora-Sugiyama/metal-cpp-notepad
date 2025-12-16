//
//  main.cpp
//  metalPractice
//
//  Created by Sora Sugiyama on 12/16/25.
//

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <iostream>
#include <exception>

class MetalAdder
{
    MTL::Device* device;
    MTL::ComputePipelineState* addFunctionPSO;
    MTL::CommandQueue* commandQueue;
    NS::Error* error;
    
public:
    MetalAdder(MTL::Device* device):device(device)
    {
        error=nil;
        
        // Load the shader files with a .metal file extension in the project
        
        MTL::Library* defaultLibrary = device->newDefaultLibrary();
        if(defaultLibrary == nil)
        {
            throw std::runtime_error("Failed to find the deault library.\n");
        }
        
        MTL::Function* addFunction = defaultLibrary->newFunction(NS::String::string("add_arrays", NS::UTF8StringEncoding));
        if(addFunction == nil)
        {
            throw std::runtime_error("Failed to find the adder function.\n");
        }
        
        addFunctionPSO = device->newComputePipelineState(addFunction,&error);
        commandQueue = device->newCommandQueue();
        
        defaultLibrary->release();
        addFunction->release();
    }
    
    ~MetalAdder()
    {
        if(addFunctionPSO)addFunctionPSO->release();
        if(commandQueue)commandQueue->release();
    }
    
    void add(float A[], float B[], float result[], const size_t length){
        size_t bufferSize = length * sizeof(float);
        MTL::Buffer* bufferA = device->newBuffer(bufferSize, MTL::ResourceStorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(bufferSize, MTL::ResourceStorageModeShared);
        MTL::Buffer* bufferResult = device->newBuffer(bufferSize, MTL::ResourceStorageModeShared);
        
        float* bufferAContents = (float*)bufferA->contents();
        float* bufferBContents = (float*)bufferB->contents();
        float* bufferResultContents = (float*)bufferResult->contents();
        
        memcpy(bufferAContents, A, bufferSize);
        memcpy(bufferBContents, B, bufferSize);
        memset(bufferResultContents, 0, bufferSize);
        
        MTL::CommandBuffer* commandBuffer = commandQueue->commandBuffer();
        MTL::ComputeCommandEncoder* computeEncoder = commandBuffer->computeCommandEncoder();
        
        computeEncoder->setComputePipelineState(addFunctionPSO);
        computeEncoder->setBuffer(bufferA, 0, 0);
        computeEncoder->setBuffer(bufferB, 0, 1);
        computeEncoder->setBuffer(bufferResult, 0, 2);
        
        MTL::Size gridSize = MTL::Size::Make(length, 1, 1);
        
        NS::UInteger threadGroupSize_tmp = addFunctionPSO->maxTotalThreadsPerThreadgroup();
        if(threadGroupSize_tmp > length)
        {
            threadGroupSize_tmp = length;
        }
        MTL::Size threadGroupSize = MTL::Size::Make(threadGroupSize_tmp, 1, 1);
        
        computeEncoder->dispatchThreads(gridSize, threadGroupSize);
        computeEncoder->endEncoding();
        
        commandBuffer->commit();
        commandBuffer->waitUntilCompleted();
        
        for(size_t i=0; i < length; i++)
        {
            result[i] = bufferResultContents[i];
        }
        
        bufferA->release();
        bufferB->release();
        bufferResult->release();
        
    }
};

float arr1[10] = {1, 2, 3, 4, 5}, arr2[10]= {1, 2, 3, 4, 5}, res[10];
int main()
{
    NS::AutoreleasePool* pool=NS::AutoreleasePool::alloc()->init();
    MTL::Device* device=MTL::CreateSystemDefaultDevice();
    MetalAdder* adder=new MetalAdder(device);
    
    adder->add(arr1, arr2, res, 5);
    delete adder;
    
    for(size_t i=0; i < 5; i++)
    {
        std::cout<<res[i]<<" ";
    }
    std::cout<<std::endl;
    
    return 0;
}
