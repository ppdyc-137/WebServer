#pragma once

#include <cctype>
#include <memory>
#include <string>

#include <spdlog/spdlog.h>
#include <unordered_map>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include "mutex.h"
#include "util.h"

namespace sylar {

    class ConfigVarBase {
    public:
        ConfigVarBase(std::string name, std::string description)
            : name_(std::move(name)), description_(std::move(description)) {}

        virtual ~ConfigVarBase() = default;

        virtual bool fromString(std::string const&) = 0;
        virtual std::string toString() const = 0;
        virtual std::string typeName() const = 0;

        const std::string& getName() const { return name_; }
        const std::string& getDescription() const { return description_; }

    protected:
        std::string name_;
        std::string description_;
    };

    template <typename T>
    struct Convert {
        static T fromString(std::string const& str) {
            try {
                auto node = YAML::Load(str);
                T val;
                if (YAML::convert<T>::decode(node, val)) {
                    return val;
                }
                throw std::runtime_error("Failed to decode value");
            } catch (const YAML::Exception& e) {
                throw std::runtime_error(std::string("YAML parsing error: ") + e.what());
            }
        }

        static std::string toString(T const& val) {
            try {
                return YAML::Dump(YAML::convert<T>::encode(val));
            } catch (const YAML::Exception& e) {
                throw std::runtime_error(std::string("YAML parsing error: ") + e.what());
            }
        }
    };

    template <typename T>
    class ConfigVar : public ConfigVarBase {
    public:
        using CallbackFunc = std::function<void(T const&, T const&)>;

        ConfigVar(std::string const& name, T const& val, std::string const& description)
            : ConfigVarBase(name, description), val_(val) {}

        void setValue(T const& val) {
            T old_val = value();
            if (old_val == val) {
                return;
            }

            for (const auto& [_, callback] : listeners_) {
                callback(old_val, val);
            }

            WriteLockGuard<RWMutex> lock(mutex_);
            val_ = val;
        }
        T value() const {
            ReadLockGuard<RWMutex> lock(mutex_);
            return val_;
        }

        bool fromString(std::string const& str) override {
            try {
                setValue(Convert<T>::fromString(str));
                return true;
            } catch (std::exception& e) {
                spdlog::error("Config::from_string exception: parse {}: {} from {}", name_, typeName(), str);
            }
            return false;
        }
        std::string toString() const override {
            try {
                return Convert<T>::toString(value());
            } catch (std::exception& e) {
                spdlog::error("Config::to_string exception: convert {}: {} to std::string", name_, typeName());
            }
            return "";
        }

        int addListener(CallbackFunc callback) {
            static int uid = 0;
            WriteLockGuard<RWMutex> lock(mutex_);
            listeners_.emplace(uid++, callback);
            return uid;
        }

        void delListener(int uid) {
            WriteLockGuard<RWMutex> lock(mutex_);
            if (auto iter = listeners_.find(uid); iter != listeners_.end()) {
                listeners_.erase(iter);
            }
        }

        std::string typeName() const override { return typenameDemangle<T>(); }

    private:
        T val_;
        std::unordered_map<int, CallbackFunc> listeners_;
        mutable RWMutex mutex_;
    };

    class Config {
    public:
        using DateType = std::unordered_map<std::string, std::shared_ptr<ConfigVarBase>>;
        using MutexType = RWMutex;

        template <typename T>
        static std::shared_ptr<ConfigVar<T>> lookup(std::string const& name, T const& val,
                                                    std::string const& description) {
            {
                ReadLockGuard lock(mutex());
                if (auto iter = data().find(name); iter != data().end()) {
                    if (auto res = std::dynamic_pointer_cast<ConfigVar<T>>(iter->second); res) {
                        return res;
                    }
                    spdlog::warn("Config::lookup: type dismatch, look for {}: {} but found {}", name,
                                 typenameDemangle<T>(), iter->second->typeName());
                    return nullptr;
                }
            }
            auto config = std::make_shared<ConfigVar<T>>(name, val, description);
            {
                WriteLockGuard lock(mutex());
                data()[name] = config;
            }
            return config;
        }

        template <typename T>
        static std::shared_ptr<ConfigVar<T>> lookup(std::string const& name) {
            ReadLockGuard lock(mutex());
            if (auto iter = data().find(name); iter != data().end()) {
                if (auto res = std::dynamic_pointer_cast<ConfigVar<T>>(iter->second); res) {
                    return res;
                }
                spdlog::warn("Config::lookup: type dismatch, look for {}: {} but found {}", name, typenameDemangle<T>(),
                             iter->second->typeName());
            }
            return nullptr;
        }

        static void loadFromYaml(YAML::Node const&);

    private:
        static DateType& data() {
            static DateType data;
            return data;
        }

        static MutexType& mutex() {
            static MutexType mutex;
            return mutex;
        }
    };

} // namespace sylar
