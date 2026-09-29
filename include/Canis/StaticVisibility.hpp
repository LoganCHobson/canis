#pragma once
#include <Canis/Frustum.hpp>
#include <algorithm>
#include <numeric>
#include <vector>

namespace Canis {
// Conservative broad phase. Leaves still use the renderer's exact model bounds.
// Rebuild only when static membership or world bounds change; eye/light queries
// never change the tree or impose camera-dependent instance ordering.
class StaticVisibility {
public:
    struct Bounds {
        size_t candidate;
        Vector3 minimum, maximum;
        bool operator==(const Bounds& b) const {
            return candidate==b.candidate && minimum==b.minimum && maximum==b.maximum;
        }
    };
    void Update(const std::vector<Bounds>& bounds) {
        if (m_bounds==bounds) return;
        m_bounds=bounds; m_nodes.clear();m_order.resize(bounds.size());
        std::iota(m_order.begin(),m_order.end(),size_t(0));
        if(!bounds.empty())Build(0,bounds.size());
    }
    void Query(const Matrix4& clip,size_t count,std::vector<unsigned char>& hidden) const {
        hidden.assign(count,0);
        if(!m_nodes.empty())QueryNode(0,clip,hidden);
    }
private:
    struct Node {Vector3 minimum,maximum;size_t begin,end,left=0,right=0;};
    std::vector<Bounds> m_bounds;
    std::vector<size_t> m_order;
    std::vector<Node> m_nodes;
    size_t Build(size_t begin,size_t end) {
        Node node{m_bounds[m_order[begin]].minimum,m_bounds[m_order[begin]].maximum,begin,end};
        for(size_t i=begin+1;i<end;++i) {
            node.minimum=glm::min(node.minimum,m_bounds[m_order[i]].minimum);
            node.maximum=glm::max(node.maximum,m_bounds[m_order[i]].maximum);
        }
        size_t index=m_nodes.size();m_nodes.push_back(node);
        if(end-begin>8) {
            auto extent=node.maximum-node.minimum;
            int axis=extent.y>extent.x?1:0;if(extent.z>extent[axis])axis=2;
            size_t middle=begin+(end-begin)/2;
            std::nth_element(m_order.begin()+begin,m_order.begin()+middle,m_order.begin()+end,
                [&](size_t a,size_t b) {return m_bounds[a].minimum[axis]+m_bounds[a].maximum[axis]<m_bounds[b].minimum[axis]+m_bounds[b].maximum[axis];});
            size_t left=Build(begin,middle),right=Build(middle,end);
            m_nodes[index].left=left;m_nodes[index].right=right;
        }
        return index;
    }
    void QueryNode(size_t index,const Matrix4& clip,std::vector<unsigned char>& hidden) const {
        const auto& node=m_nodes[index];
        if(BoundsOutsideFrustum(clip,node.minimum,node.maximum)) {
            for(size_t i=node.begin;i<node.end;++i)hidden[m_bounds[m_order[i]].candidate]=1;
        } else if(node.left) {
            QueryNode(node.left,clip,hidden);QueryNode(node.right,clip,hidden);
        }
    }
};
}
