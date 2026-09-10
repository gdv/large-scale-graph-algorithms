TARGET_EXEC    := bin/coloring
TST_EXEC       := bin/test_coloring
TARGET_DIJKSTRA_IGRAPH := bin/dijkstra-igraph
TARGET_DIJKSTRA := bin/dijkstra
TST_DIJKSTRA   := bin/test_dijkstra
TST_CSR        := bin/test_csr
BUILD_DIR      := ./build
SRC_DIRS       := ./src
TST_DIRS       := ./tests

BINS := $(TARGET_EXEC) $(TARGET_DIJKSTRA_IGRAPH) $(TARGET_DIJKSTRA)



BIN_COLORING_OBJS := $(addprefix $(BUILD_DIR)/, coloring.o greedy.o welsh_powell.o \
	dsatur.o rlf.o iterated_greedy.o sa.o tabucol.o antcolony.o output.o)

BIN_DIJKSTRA_IGRAPH_OBJS := $(addprefix $(BUILD_DIR)/, dijkstra_main.o dijkstra.o)

BIN_DIJKSTRA_OBJS := $(addprefix $(BUILD_DIR)/, dijkstra_main2.o graph_io.o \
	adjacency.o adj_array.o adj_sorted.o adj_linked.o adj_hash.o \
	priority_queue.o pq_binary.o pq_unsorted.o pq_dary.o dijkstra_core.o)

ALGO_OBJS := $(filter-out $(BUILD_DIR)/coloring.o, $(BIN_COLORING_OBJS))
TST_OBJ   := $(BUILD_DIR)/test_coloring.o
BIN_TEST_OBJS := $(TST_OBJ) $(ALGO_OBJS)

TST_DIJKSTRA_OBJS := $(BUILD_DIR)/test_dijkstra.o $(BUILD_DIR)/dijkstra.o \
	$(BUILD_DIR)/graph_io.o $(BUILD_DIR)/adjacency.o \
	$(BUILD_DIR)/adj_array.o $(BUILD_DIR)/adj_sorted.o \
	$(BUILD_DIR)/adj_linked.o $(BUILD_DIR)/adj_hash.o \
	$(BUILD_DIR)/priority_queue.o $(BUILD_DIR)/pq_binary.o \
	$(BUILD_DIR)/pq_unsorted.o $(BUILD_DIR)/pq_dary.o \
	$(BUILD_DIR)/dijkstra_core.o

ALL_OBJS := $(sort $(BIN_COLORING_OBJS) $(BIN_DIJKSTRA_IGRAPH_OBJS) $(BIN_DIJKSTRA_OBJS) $(TST_OBJ) $(TST_DIJKSTRA_OBJS) $(TST_CSR_OBJS))
DEPS := $(ALL_OBJS:.o=.d)

vpath %.c $(SRC_DIRS) $(TST_DIRS)

bin: $(BINS)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS := $(INC_FLAGS) -MMD -MP
LDFLAGS  :=

$(TARGET_EXEC): $(BIN_COLORING_OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_DIJKSTRA_IGRAPH): $(BIN_DIJKSTRA_IGRAPH_OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_DIJKSTRA): $(BIN_DIJKSTRA_OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TST_EXEC): $(BIN_TEST_OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

$(TST_DIJKSTRA): $(TST_DIJKSTRA_OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

TST_CSR_OBJS := $(BUILD_DIR)/test_csr.o $(BUILD_DIR)/util.o $(BUILD_DIR)/csr.o

$(TST_CSR): $(TST_CSR_OBJS)
	mkdir -p $(dir $@)
	$(CC) $^ -o $@ $(LDFLAGS)

.PHONY: test tests
test tests: $(TST_EXEC) $(TST_DIJKSTRA) $(TST_CSR)
	./$(TST_EXEC)
	./$(TST_DIJKSTRA)
	./$(TST_CSR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BINS) $(TST_EXEC) $(TST_DIJKSTRA) $(TST_CSR)

-include $(DEPS)
# Dep: igraph (system, located via pkg-config)
CFLAGS  += $(shell pkg-config --cflags igraph)
LDFLAGS += $(shell pkg-config --libs igraph) -lm
