CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -g -O0 -Iinclude -I../external/hand-isomorphism/src
LDFLAGS  := -L../external/hand-isomorphism -lhand_index
TARGET   := main
SRCDIR   := src
BUILDDIR := build
OBJDIR   := $(BUILDDIR)/obj
BINDIR   := $(BUILDDIR)/bin

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

$(BINDIR)/$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR)

.PHONY: clean