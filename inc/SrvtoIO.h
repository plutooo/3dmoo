void initGPU();
void GPUwritereg32(u32 addr, u32 data);
u32 GPUreadreg32(u32 addr);
void GPUTriggerCmdReqQueue();
void GPURegisterInterruptRelayQueue(u32 Flags, u32 Kevent, u32*threadID, u32*outMemHandle);