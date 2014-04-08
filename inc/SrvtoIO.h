#define frameselectoben 0x400478
#define RGBuponeleft 0x400468
#define RGBuptwoleft 0x40046C


u8* IObuffer;
u8* LINEmembuffer;
u8* VRAMbuff;

void initGPU();
void GPUwritereg32(u32 addr, u32 data);
u32 GPUreadreg32(u32 addr);
void GPUTriggerCmdReqQueue();
void GPURegisterInterruptRelayQueue(u32 Flags, u32 Kevent, u32*threadID, u32*outMemHandle);
u8* get_pymembuffer(u32 addr);
u32 get_py_memrestsize(u32 addr);