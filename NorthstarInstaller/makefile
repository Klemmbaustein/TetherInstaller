TARGET_EXEC := TetherInstaller

BUILD_DIR := ./Build
SRC_DIRS := ./Source /home/jan/code/UISystem/Headers

SRCS := $(shell find $(SRC_DIRS) -name '*.cpp')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
INC_DIRS := $(SRC_DIRS)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS := $(INC_FLAGS) -MMD -MP -Wdelete-incomplete -std=c++2a `sdl2-config --cflags --libs`

# Package all C++ object files into a static library
$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ -lUISystem -l:libGLEW.a -lcurl -lzip -lGL -L/home/jan/code/UISystem/Build `sdl2-config --cflags --libs`

# Build C++ object files
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGLEW_STATIC -c $< -o $@


.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)