#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/visitors.hpp>

// TODO: move to tools?
struct dfs_visitor : public boost::default_dfs_visitor {
    dfs_visitor(Vertex target, bool& found) : target(target), found(found) {}

    template <typename TGraph>
    void examine_vertex(TGraph::vertex_descriptor v, const TGraph& g) {
        if (v == target) {
            found = true;
            throw boost::graph_exception();
        }
    }

    Vertex target;
    bool& found;
};

bool path_exists_dfs(const Graph& g, Vertex start, Vertex target) {
    bool found = false;
    dfs_visitor vis(target, found);

    try {
        boost::depth_first_search(g, boost::visitor(vis).root_vertex(start));
    } catch (const boost::graph_exception&) {
        // Target found, exception thrown to terminate search early
    }

    return found;
}
