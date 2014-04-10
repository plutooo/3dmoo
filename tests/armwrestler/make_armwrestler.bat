arm-none-eabi-as -march=armv5te -mno-fpu -EL -o armwrestler.o armwrestler.asm
arm-none-eabi-ld -Ttext 0x00100000 -EL -e main armwrestler.o 
cp a.out armwrestler.elf
pause