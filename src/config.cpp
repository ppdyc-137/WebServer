#include "config.h"

namespace {
    void walkNode(std::string const& prefix, YAML::Node const& node,
                  std::list<std::pair<std::string, YAML::Node>>& configs) {
        configs.emplace_back(prefix, node);
        if (node.IsMap()) {
            for (auto const& node : node) {
                walkNode(prefix + (prefix.empty() ? "" : ".") + node.first.Scalar(), node.second, configs);
            }
        }
    }
} // namespace

namespace sylar {

    void Config::loadFromYaml(const YAML::Node& root) {
        std::list<std::pair<std::string, YAML::Node>> configs;
        walkNode("", root, configs);

        ReadLockGuard lock(mutex());
        for (const auto& [name, node] : configs) {
            if (auto iter = data().find(name); iter != data().end()) {
                auto node_str = YAML::Dump(node);
                iter->second->fromString(node_str);
            }
        }
    }

}; // namespace sylar
