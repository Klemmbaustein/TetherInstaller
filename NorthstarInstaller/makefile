TARGET_EXEC := TetherInstaller

BUILD_DIR := ./Build
SRC_DIRS := ./Source

SRCS := $(shell find $(SRC_DIRS) -name '*.cpp')
SRCS := $(SRCS) ../tinyfiledialogs/tinyfiledialogs.c
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
INC_DIRS := $(SRC_DIRS) ../json/single_include ../tinyfiledialogs
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS := $(INC_FLAGS) -O2 -Wno-unused-result -MMD -MP -fno-char8_t -std=c++2a `sdl2-config --cflags --libs`

ifdef BUILD_ID
CPPFLAGS := -DCI_BUILD=$(BUILD_ID) $(CPPFLAGS)
endif

# Package all C++ object files into a static library
$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ -O2 -lKlemmUI -l:libGLEW.a -lcurl -lGL `sdl2-config --cflags --libs`

# Build C++ object files
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -DGLEW_STATIC -c $< -o $@

# Build C object files
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)