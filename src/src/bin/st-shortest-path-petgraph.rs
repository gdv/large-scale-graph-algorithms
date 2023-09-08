use clap::Parser;
use clap_verbosity_flag::Verbosity;
use lsga::common::read_directed_graph;
use petgraph::algo::dijkstra;
use petgraph::visit::EdgeRef;

#[derive(Parser, Debug)]
struct Args {
    /// Filename
    #[arg(short, long = "graph", value_name = "FILE")]
    graph_name: String,

    #[arg(short, long, help = "The source vertex")]
    source: u32,
    #[arg(short, long, help = "The target vertex")]
    target: u32,

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
    log::trace!("Reading: {:?}", args.graph_name);
    let g = read_directed_graph(args.graph_name);
    log::trace!("Finished reading file");
    let n = g.node_count() as u32;
    let m = g.edge_count() as u32;
    log::info!("Number of vertices: {:?}", n);
    log::info!("Number of edges: {:?}", m);

    let source = args.source;
    if !g.contains_node(source) {
        panic!("The source vertex {:?} does not exist", source);
    }
    let target = args.target;
    if !g.contains_node(target) {
        panic!("The source vertex {:?} does not exist", target);
    }

    let res = dijkstra(&g, source, Some(target), |e| *(e.weight()));
    log::info!("Total cost {:?}", res[&target]);
}
