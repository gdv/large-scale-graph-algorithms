use clap::Parser;
use clap_verbosity_flag::Verbosity;
use float_cmp::approx_eq;
use lsga::common::read_directed_graph;
use petgraph::Direction;
use std::cmp::Ordering;
use std::collections::BinaryHeap;
use std::collections::HashMap;

#[derive(Parser, Debug)]
struct Args {
    #[arg(short, long = "graph", value_name = "FILE")]
    graph_name: String,

    #[command(flatten)]
    verbose: Verbosity,

    #[arg(short, long, help = "The source vertex")]
    source: u32,

    #[clap(short, long)]
    debug: bool,
}

fn main() {
    let args = Args::parse();
    // Initialize the logger
    env_logger::Builder::new()
        .filter_level(args.verbose.log_level_filter())
        .init();

    // Get the filename from the command line argument
    log::debug!("Reading: {:?}", args.graph_name);
    let g = read_directed_graph(args.graph_name);
    let n = g.node_count() as u32;
    let m = g.edge_count() as u32;
    log::info!("Number of vertices: {:?}", n);
    log::info!("Number of edges: {:?}", m);

    let source = args.source;
    if !g.contains_node(source) {
        panic!("The source vertex {:?} does not exist", source);
    }

    // Dijkstra computes a tree of the shortest paths from the source.
    // We store such a tree in the parent map.
    // The parent of the source is the source itself
    let mut parent: HashMap<u32, u32> = HashMap::with_capacity(n as usize);
    let mut distance: HashMap<u32, f32> = HashMap::new();

    let mut q = BinaryHeap::new();

    // We're at `start`, with a zero cost
    distance.insert(source, 0.0);
    q.push(State {
        cost: 0.0,
        next: source,
        prev: source,
    });

    // Examine the frontier with lower cost nodes first (min-heap)
    while let Some(State { cost, next, prev }) = q.pop() {
        // For each node we can reach, see if we can find a way with
        // a lower cost going through this node
        if parent.contains_key(&next) {
            continue;
        }
        parent.insert(next, prev);
        distance.insert(next, cost);
        for w in g.neighbors_directed(next, Direction::Outgoing) {
            if !parent.contains_key(&w) {
                let maybe = State {
                    cost: cost + g.edge_weight(next, w).unwrap(),
                    next: w,
                    prev: next,
                };

                // If so, add it to the frontier and continue
                if !distance.contains_key(&w) || maybe.cost < distance[&w] {
                    q.push(maybe);
                    // Relaxation, we have now found a better way
                    distance.insert(w, maybe.cost);
                }
            }
        }
    }

    for v in g.nodes() {
        let node_name = v;
        let parent_node = parent[&v];
        log::info!("parent of {:?} is {:?}", node_name, parent_node);
    }
}

#[derive(Copy, Clone, PartialEq)]
struct State {
    cost: f32,
    next: u32,
    prev: u32,
}

// The priority queue depends on `Ord`.
// Explicitly implement the trait so the queue becomes a min-heap
// instead of a max-heap.
impl Ord for State {
    fn cmp(&self, other: &Self) -> Ordering {
        // Notice that the we flip the ordering on costs.
        // In case of a tie we compare positions - this step is necessary
        // to make implementations of `PartialEq` and `Ord` consistent.
        if approx_eq!(f32, self.cost, other.cost, epsilon = 0.00000001) {
            self.next.cmp(&other.next)
        } else {
            if self.cost > other.cost {
                Ordering::Less
            } else {
                Ordering::Greater
            }
        }
    }
}

impl Eq for State {}

// `PartialOrd` needs to be implemented as well.
impl PartialOrd for State {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}
