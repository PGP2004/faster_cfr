CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -Iinclude
SRC_DIR  := src
OBJ_DIR  := build
ISO_DIR  := ../external/hand-isomorphism
ISO_INC  := $(ISO_DIR)/src
ISO_LIB  := $(ISO_DIR)/libhand_index.a
CXXFLAGS += -I$(ISO_INC)
CXXFLAGS += -MMD -MP

PROGRAMS := make_river_strengths make_turn_cdfs make_turn_clusters make_flop_pdfs
PROGRAM_SRCS := $(addsuffix .cpp,$(addprefix $(SRC_DIR)/,$(PROGRAMS)))
ALL_SRCS     := $(wildcard $(SRC_DIR)/*.cpp)
LIB_SRCS     := $(filter-out $(PROGRAM_SRCS),$(ALL_SRCS))
LIB_OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(LIB_SRCS))
ALL_OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(ALL_SRCS))

.PHONY: all clean
all: $(PROGRAMS)

$(PROGRAMS): %: $(OBJ_DIR)/%.o $(LIB_OBJS) $(ISO_LIB)
	$(CXX) $(CXXFLAGS) $^ -lm -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

-include $(ALL_OBJS:.o=.d)

clean:
	rm -rf $(OBJ_DIR) $(PROGRAMS)