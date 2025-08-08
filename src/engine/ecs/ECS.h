#pragma once
#include <unordered_map>
#include <cstdint>
#include <typeindex>
#include <memory>

namespace nova {
using Entity = uint32_t;

class Registry {
    Entity next{1};
    std::unordered_map<std::type_index, std::unordered_map<Entity, std::shared_ptr<void>>> pools;
public:
    Entity create(){ return next++; }
    template<typename C, typename...Args>
    C& emplace(Entity e, Args&&...args) {
        auto& pool = pools[std::type_index(typeid(C))];
        auto p = std::make_shared<C>(C{std::forward<Args>(args)...});
        pool[e] = p; return *static_cast<C*>(p.get());
    }
    template<typename C> bool has(Entity e) const {
        auto it = pools.find(std::type_index(typeid(C)));
        return it != pools.end() && it->second.count(e);
    }
    template<typename C> C& get(Entity e){
        auto& pool = pools[std::type_index(typeid(C))];
        return *static_cast<C*>(pool.at(e).get());
    }
    template<typename A, typename B, typename Func>
    void view(Func&& f){
        auto& pa = pools[std::type_index(typeid(A))];
        auto& pb = pools[std::type_index(typeid(B))];
        for (auto& [e, ca] : pa){
            if (pb.count(e)) f(e, *static_cast<A*>(ca.get()), *static_cast<B*>(pb[e].get()));
        }
    }
};
}
