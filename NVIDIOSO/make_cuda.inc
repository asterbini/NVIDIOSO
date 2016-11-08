CUDAOPT += -ccbin /opt/gcc-5.3.0/bin/g++
CUDAOPT += -L/opt/gcc-5.3.0/lib64/gcc/x86_64-fedoraunited-linux-gnu/lib64
CUDAOPT += -Wno-deprecated-gpu-targets
DEBUGFLAG = -g -G -Xcompiler -rdynamic -lineinfo
CCOPT += -arch=sm_52
COMPILE = -x cu -dc
