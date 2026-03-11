TARGET := main.exe
EXTERN := cpp
COMPILER := clang++

COMPILE_OPTION := -Wall -O2
# to generate dependent files #
COMPILE_OPTION_DES := -MMD -MP

OPT ?= -o test.mp4

# store .o and .d files #
TMPDIR := tmp
# store the target file #
DEBUGDIR := debug

# sources, objects and dependencies #
SRCS := $(wildcard *.$(EXTERN))
OBJS := $(patsubst %.$(EXTERN), $(TMPDIR)/%.o, $(SRCS))
DEPS := $(patsubst %.$(EXTERN), $(TMPDIR)/%.d, $(SRCS))

# link #
$(DEBUGDIR)/$(TARGET) : $(OBJS) | $(DEBUGDIR)
	@ echo linking...
	$(COMPILER) $(OBJS) -o $(DEBUGDIR)/$(TARGET)
	@ echo completed!

# compile #
$(TMPDIR)/%.o : %.$(EXTERN) | $(TMPDIR)
	@ echo compiling $<...
	$(COMPILER) $< -o $@ -c $(COMPILE_OPTION) $(COMPILE_OPTION_DES)

# create TMPDIR when it does not exist #
$(TMPDIR) :
	@ mkdir $(patsubst ./%, %, $(TMPDIR))

$(DEBUGDIR) :
	@ mkdir $(DEBUGDIR)

# files dependecies #
-include $(DEPS)

# run command #
.PHONY : run
run : $(DEBUGDIR)/$(TARGET)
	./$(DEBUGDIR)/$(TARGET) $(OPT)

# clean command #
.PHONY : clean
clean :
	@ echo try to clean...
	rm -r $(DEBUGDIR)/$(TARGET) $(OBJS) $(DEPS)
	@ echo completed!