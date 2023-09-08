use clap::Parser;
use clap_verbosity_flag::Verbosity;
use lsga::common::read_directed_graph;
use petgraph::Direction;
use std::collections::{HashMap, VecDeque};

#[derive(Parser, Debug)]
struct Args {
    #[arg(short, long = "graph", value_name = "FILE")]
    graph_name: String,

    #[command(flatten)]
    verbose: Verbosity,
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

    // Initially the queue contains only the node with id 0, and its parent is dummy
    let mut parent: HashMap<u32, u32> = HashMap::with_capacity(n as usize);
    parent.insert(0, n + 1);
    let mut q = VecDeque::from([0]);

    // BFS
    while !q.is_empty() {
        let v = q.pop_front().unwrap();
        for w in g.neighbors_directed(v, Direction::Outgoing) {
            if !parent.contains_key(&w) {
                parent.insert(w, v);
                q.push_back(w);
            }
        }
    }

    for v in g.nodes() {
        let node_name = v;
        let parent_node = parent[&v];
        log::info!("parent of {:?} is {:?}", node_name, parent_node);
    }
}
