// Template method implementations for DependencyContainer
#pragma once
#include <algorithm>
#include <stdexcept>
#include <typeinfo>

template<typename T>
void DependencyContainer::registerService(const std::string& name, std::function<T*()> factory, 
                    Lifecycle lifecycle,
                    const std::vector<std::string>& dependencies) {
    auto now = std::chrono::steady_clock::now();
    services_[name] = {
        [factory]() -> void* { return static_cast<void*>(factory()); },
        lifecycle,
        nullptr,
        false,
        {
            name,
            typeid(T).name(),
            dependencies,
            now,
            now,
            0,
            true,
            ""
        }
    };
    
    for (const auto& dep : dependencies) {
        dependencyGraph_[name].push_back(dep);
    }
}

template<typename T>
void DependencyContainer::registerInstance(const std::string& name, T* instance) {
    services_[name] = {
        nullptr,
        Lifecycle::Singleton,
        static_cast<void*>(instance),
        true
    };
}

template<typename T>
T* DependencyContainer::resolve(const std::string& name) {
    if (std::find(resolutionStack_.begin(), resolutionStack_.end(), name) != resolutionStack_.end()) {
        std::string cycle = name;
        for (auto it = resolutionStack_.rbegin(); it != resolutionStack_.rend(); ++it) {
            cycle += " -> " + *it;
        }
        throw std::runtime_error("Circular dependency detected: " + cycle);
    }
    
    auto it = services_.find(name);
    if (it == services_.end()) {
        throw std::runtime_error("Service not registered: " + name + 
                               ". Available services: " + getRegisteredServicesList());
    }
    
    auto& registration = it->second;
    auto now = std::chrono::steady_clock::now();
    
    registration.metadata.lastAccessTime = now;
    registration.metadata.accessCount++;
    
    if (registration.lifecycle == Lifecycle::Singleton && registration.isInitialized) {
        return static_cast<T*>(registration.instance);
    }
    
    resolutionStack_.push_back(name);
    T* instance = nullptr;
    
    try {
        validateDependencies(name);
        
        if (registration.factory) {
            instance = static_cast<T*>(registration.factory());
        } else if (registration.instance) {
            instance = static_cast<T*>(registration.instance);
        } else {
            throw std::runtime_error("No factory or instance available for: " + name);
        }
        
        if (registration.lifecycle == Lifecycle::Singleton) {
            registration.instance = static_cast<void*>(instance);
            registration.isInitialized = true;
            registration.metadata.createdTime = now;
        }
        
        registration.metadata.isHealthy = true;
        registration.metadata.lastError = "";
        
    } catch (const std::exception& e) {
        registration.metadata.isHealthy = false;
        registration.metadata.lastError = e.what();
        resolutionStack_.pop_back();
        throw;
    }
    
    resolutionStack_.pop_back();
    return instance;
}

