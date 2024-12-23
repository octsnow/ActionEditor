encode = $(subst $() ,*,$(1))
decode = $(subst *,$() ,$(1))
quotes = $(addprefix ',$(addsuffix ',$(1)))
includes = $(addprefix -I,$(1))
libs = $(addprefix -LIBPATH:,$(1))

RM = cmd.exe /C del
CP = cmd.exe /C copy
MV = cmd.exe /C move

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.obj,$(SRCS))
INC_DIR = includes ..\OctGame\includes C:\glut\freeglut-3.2.1\install\include $(call encode,C:\Program Files (x86)\OpenAL 1.1 SDK\include)
LIB_DIR = ..\..\OctBinary\lib ..\..\Wav\lib ..\OctGame\lib C:\glut\freeglut-3.2.1\install\lib C:\glut\freeglut-3.2.1\install_d\lib $(call encode,C:\Program Files (x86)\OpenAL 1.1 SDK\libs\Win64)

INC_DIR := $(call decode,$(call includes,$(call quotes,$(INC_DIR))))
LIB_DIR := $(call decode,$(call libs,$(call quotes,$(LIB_DIR))))

TARGET = out/main.exe
LIB_TARGET = BlockID.lib
COMPILEOPTIONS = -EHsc -std:c++20 -c
DEBUGFLAGS = -DOCT_DEBUG
CXXFLAGS := $(COMPILEOPTIONS)

.SUFFIXES:.obj .cpp
.PHONY: debug release

all: release

release: CXXFLAGS = $(COMPILEOPTIONS)
release: $(TARGET)
release: $(foreach x,$(LIB_TARGET),lib/$(x))

$(TARGET): $(OBJS)
	LINK $(OBJS) $(LIB_DIR) -OUT:$(TARGET)

$(OBJ_DIR)/%.obj: $(SRC_DIR)/%.cpp
	cl $(CXXFLAGS) $< -Fo$(OBJ_DIR)\$(@F) $(INC_DIR)

lib/%.lib: $(OBJ_DIR)/%.obj
	lib $< $(LIB_DIR) -OUT:$@

run: $(TARGET)
	$(TARGET)

define RMFUNC
	-$(RM) $(1)

endef

clean:
	$(foreach x,$(subst /,\,$(OBJS)) $(foreach x,$(LIB_TARGET),lib\$(x)) $(subst /,\,$(TARGET)),$(call RMFUNC,$(x)))
