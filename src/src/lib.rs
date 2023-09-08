pub mod common {
    use flate2::read::GzDecoder;
    use petgraph::graphmap::{DiGraphMap, GraphMap, UnGraphMap};
    use petgraph::EdgeType;
    use std::fs::File;
    use std::io::{BufRead, BufReader};

    fn read_graph<T: EdgeType>(filename: String, graph: &mut GraphMap<u32, f32, T>) {
        let is_gzip = filename.ends_with(".gz");
        let file = File::open(filename).unwrap();
        let reader: Box<dyn BufRead> = if is_gzip {
            let decoder = GzDecoder::new(file);
            Box::new(BufReader::new(decoder))
        } else {
            Box::new(BufReader::new(file))
        };

        for line in reader.lines() {
            let line = line.unwrap();
            /*
             * Comments start with a #
             * Also skip lines with only whitespaces
             */
            if line.starts_with("#") || line.chars().all(|c| c.is_whitespace()) {
                continue;
            }

            let parts: Vec<&str> = line.split_whitespace().collect();

            let from: u32 = parts[0].parse().unwrap();
            let to: u32 = parts[1].parse().unwrap();
            let weight: f32 = if parts.len() >= 3 {
                parts[2].parse().unwrap()
            } else {
                0.0
            };
            graph.add_edge(from, to, weight);
        }
    }

    pub fn read_directed_graph(filename: String) -> DiGraphMap<u32, f32> {
        let mut graph = DiGraphMap::<u32, f32>::new();
        read_graph(filename, &mut graph);
        return graph;
    }
    pub fn read_undirected_graph(filename: String) -> UnGraphMap<u32, f32> {
        let mut graph = UnGraphMap::<u32, f32>::new();
        read_graph(filename, &mut graph);
        return graph;
    }
}
