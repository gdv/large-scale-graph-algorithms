use clap::Parser;
use clap_verbosity_flag::Verbosity;
use float_cmp::approx_eq;
use lsga::common::read_directed_graph;
use petgraph::graphmap::DiGraphMap;
use petgraph::Direction;
use std::cmp::Ordering;
use std::collections::BinaryHeap;
use std::collections::HashMap;

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

fn extend_frontier(
    g: &DiGraphMap<u32, f32>,
    current: &mut State,
    distance: &mut HashMap<u32, f32>,
    parent: &mut HashMap<u32, u32>,
    queue: &mut BinaryHeap<State>,
    is_reverse: bool,
) -> State {
    /*
     * This is a general version of the standard Dijkstra step, where we can
     * visit the original graph or its reverse.
     *
     * Input:
     * current: the State with the vertex to explore
     * queue: the priority queue
     * distance: a dictionary with the predecessor of each visited node
     * parent: the information needed to build the path
     * is_reverse: the current operation is on the forward search?
     *
     */

    parent.insert(current.next, current.prev);
    distance.insert(current.next, current.cost);
    let neighbors = if is_reverse {
        g.neighbors_directed(current.next, Direction::Incoming)
    } else {
        g.neighbors_directed(current.next, Direction::Outgoing)
    };

    for w in neighbors.into_iter() {
        if !parent.contains_key(&w) {
            /*
             * The vertex w is not in any search tree, therefore consider the
             * arc from the current vertex to w. If it improves over the current
             * distance to w, add it to the priority queue.
             */
            if is_reverse {
                log::debug!(
                    "Extendeding ({:?}->{:?}) cost: {:?}",
                    w,
                    current.next,
                    g.edge_weight(w, current.next)
                );
            } else {
                log::debug!(
                    "Extendeding ({:?}->{:?}) cost: {:?}",
                    current.next,
                    w,
                    g.edge_weight(current.next, w)
                );
            }
            let maybe = State {
                cost: if is_reverse {
                    current.cost + g.edge_weight(w, current.next).unwrap()
                } else {
                    current.cost + g.edge_weight(current.next, w).unwrap()
                },
                next: w,
                prev: current.next,
            };

            // If so, add it to the frontier and continue
            if !distance.contains_key(&w) || maybe.cost < distance[&w] {
                queue.push(maybe);
                // Relaxation, we have now found a better way
                distance.insert(w, maybe.cost);
            }
        }
    }

    /*
     * If the queue is empty, then we have explored the entire connected component.
     * return a sentinel state, where the two vertices are the same.
     *
     * Otherwise we return the min cost element from the queue
     */
    if queue.is_empty() {
        State {
            cost: -99.00,
            next: current.next,
            prev: current.next,
        }
    } else {
        queue.pop().unwrap()
    }
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

    /*
     * This algorithm is essentially the combination of two executions of
     * Dijkstra, one from the source and one from the target (the latter
     * execution on the reverse of the graph, that is we are going to analyze
     * arcs incoming into the current vertex).
     *
     * We need to keep two distinct sets of data structures, one with the suffix
     * _f for forward searches, and one with the _r for reverse searches.
     */

    let mut parent_f: HashMap<u32, u32> = HashMap::new();
    let mut parent_r: HashMap<u32, u32> = HashMap::new();

    /*
     * We start the bidirectional search with the source and the target vertices.
     * current_f and current_r are the best arcs possible for any of the two searches.
     *
     */
    let mut current_f = State {
        cost: 0.0,
        next: source,
        prev: source,
    };
    let mut current_r = State {
        cost: 0.0,
        next: target,
        prev: target,
    };

    /*
     * The algorithm consists of examining the two possible arcs to traverse and
     * choose the one with minimum distance from the starting point of the search.
     *
     * Then we extend the corresponding search tree.
     */
    let mut next: u32;
    {
        /*
         * I need a separate context for the distances and the heaps to minimize
         * the memory usage
         */
        let mut distance_f: HashMap<u32, f32> = HashMap::new();
        let mut q_f = BinaryHeap::new();
        let mut distance_r: HashMap<u32, f32> = HashMap::new();
        let mut q_r = BinaryHeap::new();
        distance_f.insert(source, 0.0);
        distance_r.insert(target, 0.0);
        loop {
            log::debug!(
                "current_f: ({:?}->{:?}) cost: {:?}",
                current_f.prev,
                current_f.next,
                current_f.cost
            );
            log::debug!(
                "current_r: ({:?}->{:?}) cost: {:?}",
                current_r.prev,
                current_r.next,
                current_r.cost
            );

            /*
             * Choose the next arc to traverse and extend the corresponding search tree.
             */
            if current_r.cost < current_f.cost {
                /*
                 * extend the reverse search tree
                 */
                log::trace!("Extending the reverse tree");
                next = current_r.next;
                current_r = extend_frontier(
                    &g,
                    &mut current_r,
                    &mut distance_r,
                    &mut parent_r,
                    &mut q_r,
                    true,
                );
                log::debug!(
                    "extended current_r: ({:?}->{:?}) cost: {:?}",
                    current_r.prev,
                    current_r.next,
                    current_r.cost
                );
                /*
                 * The queues might contain a vertex that has already been
                 * included in the corresponding search tree. In this case we discard the entry
                 * and we move to the next element in the queue.
                 */
                while !q_r.is_empty() && parent_r.contains_key(&(current_r.next)) {
                    current_r = q_r.pop().unwrap();
                }
            } else {
                /*
                 * extend the forward search tree
                 */
                log::trace!("Extending the forward tree");
                next = current_f.next;
                current_f = extend_frontier(
                    &g,
                    &mut current_f,
                    &mut distance_f,
                    &mut parent_f,
                    &mut q_f,
                    false,
                );
                log::debug!(
                    "extended current_f: ({:?}->{:?}) cost: {:?}",
                    current_f.prev,
                    current_f.next,
                    current_f.cost
                );
                while !q_f.is_empty() && parent_f.contains_key(&(current_f.next)) {
                    current_f = q_f.pop().unwrap();
                }
            }

            log::trace!("Distance_f: {:?}", distance_f);
            log::trace!("Parent_f: {:?}", parent_f);
            log::trace!("Q_f: {:?}", q_f);
            log::trace!("Distance_r: {:?}", distance_r);
            log::trace!("Parent_r: {:?}", parent_r);
            log::trace!("Q_r: {:?}", q_r);

            /*
             * if the new vertex we have just reached (next) has been visited from both
             * the source and the target, the two search trees have reached a common
             * vertex and our search for a shortest path has completed
             */
            if parent_f.contains_key(&next) && parent_r.contains_key(&next) {
                break;
            }

            /*
             * check if we have explored the entire graph, without finding a common
             * vertex in the two search trees.
             * In this case, there is no path from the source to the target.
             */
            if current_f.next == current_f.prev && current_r.next == current_r.prev {
                panic!("Unreachable vertices");
            }
        }
    }
    /*
     * reconstruct the best source-target path, using a opt stack.
     *
     * We need to reconstruct separately the portion of the path from source to
     * next (on the forward search tree), and the portion from target to next (on
     * the reverse search tree). The latter portion must be reversed.
     */
    log::trace!("Path computed. Middle vertex: {:?}", next);
    log::trace!("Parent_f: {:?}", parent_f);
    log::trace!("Parent_r: {:?}", parent_r);
    let mut opt = Vec::new();
    let mut v = next;
    while v != parent_f[&v] {
        let cost = g.edge_weight(parent_f[&v], v).unwrap();
        opt.push(State {
            cost: *cost,
            next: v,
            prev: parent_f[&v],
        });
        v = parent_f[&v];
        log::trace!("Vertex forward:: {:?}", v);
    }
    /*
     * reverse search tree
     */
    v = next;
    let mut opt_r = Vec::new();
    while v != parent_r[&v] {
        let cost = g.edge_weight(v, parent_r[&v]).unwrap();
        opt_r.push(State {
            cost: *cost,
            next: parent_r[&v],
            prev: v,
        });
        v = parent_r[&v];
        log::trace!("Vertex reverse:: {:?}", v);
    }

    /*
    print the optimal path
     */
    let mut path: Vec<_> = opt.iter().rev().cloned().collect();
    path.extend(opt_r);
    let mut tot_cost: f32 = 0.0;
    log::trace!(
        "The shortest path from {:?} to {:?} is:",
        args.source,
        args.target
    );
    for v in path.iter() {
        tot_cost += v.cost;
        log::info!(
            "{:?} -> {:?} with cost {:?} at distance {:?}",
            v.prev,
            v.next,
            v.cost,
            tot_cost
        );
    }
    println!("Total cost: {:?}", tot_cost);
}

#[derive(Copy, Clone, PartialEq, Debug)]
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
