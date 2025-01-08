#include <format>
#include <iostream>
#include <spdlog/spdlog.h>

#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

#include "config.h"

struct Person {
    std::string name_;
    int age_{};

    bool operator==(Person const& that) const { return name_ == that.name_ && age_ == that.age_; }

    std::string toString() const { return std::format("{{ name: {}, age: {} }}", name_, age_); }
};

namespace sylar {

    template <>
    struct Convert<Person> {
        static Person fromString(std::string const& str) {
            auto node = YAML::Load(str);
            Person rhs;
            rhs.name_ = node["name"].as<std::string>();
            rhs.age_ = node["age"].as<int>();
            return rhs;
        }
        static std::string toString(Person const& val) { return val.toString(); }
    };

} // namespace sylar

int main() {
    spdlog::set_level(spdlog::level::debug);
    auto config_name = sylar::Config::lookup<std::string>("name", "None", "");
    auto config_age = sylar::Config::lookup<int>("age", 0, "");
    auto config_person = sylar::Config::lookup<Person>("person", {}, "");
    config_person->addListener(0, [](auto const& old_val, auto const& new_val) {
        std::cout << std::format("update: {} to {}", old_val.toString(), new_val.toString()) << '\n';
    });
    auto config_vec = sylar::Config::lookup<std::vector<int>>("vec", {}, "");

    const auto* yaml = R"(
    name: "ppdy"
    age: 12
    person:
        name: "ppdy"
        age: 12
    vec: [1, 2, 3]
    )";
    auto node = YAML::Load(yaml);

    sylar::Config::loadFromYaml(node);

    std::cout << config_name->value() << "\n";
    std::cout << config_age->value() << "\n";
    std::cout << config_person->toString() << "\n";
    std::cout << config_vec->toString() << "\n";

    auto config_vec_err = sylar::Config::lookup<std::vector<float>>("vec");
}
