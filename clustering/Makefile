CXX      := $(shell ls /opt/homebrew/bin/g++-* 2>/dev/null | sort -V | tail -n1)
ifeq ($(CXX),)
CXX    := g++
endif
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -fopenmp -Iinclude
SRC_DIR  := src
BUILD_DIR := build
OBJ_DIR  := $(BUILD_DIR)/obj
BIN_DIR  := $(BUILD_DIR)/bin
ISO_DIR  := ../external/hand-isomorphism
ISO_INC  := $(ISO_DIR)/src
ISO_LIB  := $(ISO_DIR)/libhand_index.a
CXXFLAGS += -I$(ISO_INC)
CXXFLAGS += -MMD -MP

PROGRAMS := make_river_strengths make_turn_cdfs make_turn_clusters make_flop_pdfs make_flop_clusters
PROGRAM_BINS := $(addprefix $(BIN_DIR)/,$(PROGRAMS))
PROGRAM_SRCS := $(addsuffix .cpp,$(addprefix $(SRC_DIR)/,$(PROGRAMS)))
ALL_SRCS     := $(wildcard $(SRC_DIR)/*.cpp)
LIB_SRCS     := $(filter-out $(PROGRAM_SRCS),$(ALL_SRCS))
LIB_OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(LIB_SRCS))
ALL_OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(ALL_SRCS))

.PHONY: all clean
all: $(PROGRAM_BINS)

$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(LIB_OBJS) $(ISO_LIB) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -lm -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

-include $(ALL_OBJS:.o=.d)

clean:
	rm -rf $(BUILD_DIR)