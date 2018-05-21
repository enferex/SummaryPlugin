CFG        = llvm-config-6.0
CLANG      = clang-6.0
CXX        = clang++-6.0
SRCS       = SummaryPlugin.cpp
OBJS      := $(SRCS:.cpp=.o)
PLUGIN     = SummaryPlugin.so
CXXFLAGS  := $(shell $(CFG) --cxxflags --includedir) -Wno-unknown-warning-option 
TESTS     := $(wildcard tests/*.c)
TEST_OBJS := $(TESTS:.c=.to)

all: $(PLUGIN)

$(PLUGIN): $(OBJS)
	$(CXX) -shared $^ -o $@

# Test objects (for testing... duh)
%.to: %.c
	@$(CLANG) -fplugin=./$(PLUGIN) -O \
    -Xclang -add-plugin -Xclang summary -c $< -o $@ \
    -Xclang -plugin-arg-summary \
    -Xclang -fmt="[%F] %L Lines"

.PHONY: test
test: $(TEST_OBJS)

.PHONY: tests
tests: test

clean:
	$(RM) $(OBJS) $(PLUGIN) $(PLUGIN:.so=.dwo) $(TEST_OBJS)
