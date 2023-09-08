use clap::Parser;
use clap_verbosity_flag::Verbosity;
use lsga::common::read_directed_graph;
use petgraph::algo::dijkstra;
use petgraph::prelude::*;

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

    let res = dijkstra(&g, source, None, |e| *e.weight());
    log::info!("{:?}", res);
}
