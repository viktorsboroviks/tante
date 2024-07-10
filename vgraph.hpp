#include <cassert>
#include <vector>

// Vertex
// _g?
// - get_in_edges()
//   - return <int>?
//   - return pointers?
// - get_out_edges()
// - get_in_nodes()
// - get_out_nodes()
//
// Edge
//
// Graph
// vertices
// - vector
//   - in_edges
//   - out_edges
struct Vertex {
    std::vector<Edge*> p_in_edges;
    std::vector<Edge*> p_out_edges;

    std::vector<Vertex*> p_in_nodes()
    {
        std::vector<Vertex*> ins;
        for (auto p_e : p_in_edges) {
            ins.push_back(p_e->)
        }
    }

    std::vector<Vertex*> p_out_nodes();
};

struct Edge {
    Vertex* p_src;
    Vertex* p_dst;
};

class Graph {
private:
    size_t _get_vertex_i(Vertex* p_v)
    {
        assert(p_v != nullptr);
        bool found = false;
        size_t i;
        for (i = 0; i < vertices.size(); i++) {
            if (&vertices[i] == p_v) {
                found = true;
                break;
            }
        }
        assert(found);
        return(i);
    }

#ifndef NDEBUG
    size_t _vertex_exists(Vertex* p_v)
    {
        assert(p_v != nullptr);
        bool found = false;
        for (size_t i = 0; i < vertices.size(); i++) {
            if (&vertices[i] == p_v) {
                found = true;
                break;
            }
        }
        return(found);
    }
#endif

public:
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;

    Vertex* add_vertex()
    {
        vertices.push_back(Vertex());
        return(&vertices.back());
    }

    Edge* add_edge(Vertex* p_src, Vertex* p_dst)
    {
#ifndef NDEBUG
        _vertex_exists(p_src);
        _vertex_exists(p_dst);
#endif

        edges.push_back(Edge());
        Edge* p_e = &edges.back();
        p_e->p_src = p_src;
        p_e->p_dst = p_dst;
        p_src->p_outs.push_back(p_e);
        p_dst->p_ins.push_back(p_e);
        return(p_e);
    }

    void remove_vertex(Vertex* p_v)
    {
        // update neighbors

        // erase edges
        // erase vertex
        size_t i = _get_vertex_i(p_v);

        vertices.erase(vertices.begin() + i);

#ifndef NDEBUG
        assert(!_vertex_exists(p_v));
#endif
    }

    void remove_edge()
    {

    }
};
