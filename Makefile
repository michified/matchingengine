CXX      := g++
CXXFLAGS := -O3 -march=alderlake -flto -std=c++20 -Wall -Wextra
INCDIR   := include
CXXFLAGS += -I$(INCDIR)

TARGET   := src/main.exe
SRCDIR   := src
SRCS     := $(SRCDIR)/main.c++
OBJS     := $(SRCS:.c++=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c++
	$(CXX) $(CXXFLAGS) -c $< -o $@

WIN_OBJS := $(subst /,\\,$(OBJS))
WIN_TARGET := $(subst /,\\,$(TARGET))

clean:
	cmd /c "if exist $(WIN_OBJS) del /Q /F $(WIN_OBJS)"
	cmd /c "if exist $(WIN_TARGET) del /Q /F $(WIN_TARGET)"

.PHONY: all clean