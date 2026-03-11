CXX := g++
NVCC := nvcc

UNAME_S := $(shell uname -s 2>/dev/null)

THREAD_FLAG :=
NVCC_HOST_FLAGS :=

ifeq ($(UNAME_S),Linux)
	NVCC := $(if $(shell command -v nvcc 2>/dev/null),$(shell command -v nvcc 2>/dev/null),/usr/local/cuda/bin/nvcc)
	THREAD_FLAG := -pthread
	NVCC_HOST_FLAGS := -Xcompiler -pthread
endif

CXXFLAGS := -O3 -std=c++17 -I. -Iinclude $(THREAD_FLAG)
NVCCFLAGS := -O3 -std=c++17 -I. -Iinclude --expt-relaxed-constexpr --use_fast_math
LDFLAGS :=
CUDA_LIBS := -lcudart

OBJDIR := debug/cuda_obj
TARGET := debug/main_cuda

CPP_SRCS := \
	main.cpp \
	berlin_noise.cpp \
	color.cpp \
	stb_impl.cpp

CPP_OBJS := $(CPP_SRCS:%.cpp=$(OBJDIR)/%.o)
CU_OBJS := $(OBJDIR)/cuda_video_renderer.o

OBJS := $(CPP_OBJS) $(CU_OBJS)

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(CPP_OBJS): $(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/cuda_video_renderer.o: cuda_video_renderer.cu | $(OBJDIR)
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(NVCC) -o $@ $(OBJS) $(NVCC_HOST_FLAGS) $(LDFLAGS) $(CUDA_LIBS)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean