CXX      := g++
CXXFLAGS := -O3 -march=alderlake -flto -std=c++20 -Wall -Wextra
INCDIR   := include
CXXFLAGS += -I$(INCDIR)

TARGET   := src/main.exe
SRCDIR   := src
SRCS     := $(SRCDIR)/main.c++
OBJS     := $(SRCS:.c++=.o)

RM := C:/msys64/usr/bin/rm.exe -f

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c++
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)

.PHONY: all clean