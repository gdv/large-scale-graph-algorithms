use clap::Parser;
use clap_verbosity_flag::Verbosity;
use lsga::common::read_directed_graph;




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
    println!("Number of vertices: {:?}", n);
    println!("Number of edges: {:?}", m);
}
