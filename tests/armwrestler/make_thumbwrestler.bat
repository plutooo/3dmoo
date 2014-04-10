arm-none-eabi-as -march=armv5te -mno-fpu -mthumb -EL -o thumbwrestler.o thumbwrestler.asm
arm-none-eabi-ld -Ttext 0x00100000  -EL -e main thumbwrestler.o 
arm-none-eabi-objcopy -O binary a.out thumbwrestler.gba
cp a.out thumbwrestler.elf
pause