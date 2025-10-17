#include "di/DependencyContainer.hpp"

DependencyContainer::~DependencyContainer() {
    clear();
}

bool DependencyContainer::isRegistered(const std::string& name) const {
    return services_.find(name) != services_.end();
}

std::vector<std::string> DependencyContainer::getRegisteredServices() const {
    std::vector<std::string> names;
    for (const auto& pair : services_) {
        names.push_back(pair.first);
    }
    return names;
}

void DependencyContainer::clear() {
    services_.clear();
    resolutionStack_.clear();
}

std::string DependencyContainer::getStats() const {
    int singletonCount = 0;
    int transientCount = 0;
    int initializedCount = 0;
    int healthyCount = 0;
    size_t totalAccessCount = 0;
    
    for (const auto& pair : services_) {
        if (pair.second.lifecycle == Lifecycle::Singleton) {
            singletonCount++;
        } else {
            transientCount++;
        }
        if (pair.second.isInitialized) {
            initializedCount++;
        }
        if (pair.second.metadata.isHealthy) {
            healthyCount++;
        }
        totalAccessCount += pair.second.metadata.accessCount;
    }
    
    return "Services: " + std::to_string(services_.size()) + 
           " (Singletons: " + std::to_string(singletonCount) + 
           ", Transients: " + std::to_string(transientCount) + 
           ", Initialized: " + std::to_string(initializedCount) + 
           ", Healthy: " + std::to_string(healthyCount) + 
           ", Total Access: " + std::to_string(totalAccessCount) + ")";
}

std::string DependencyContainer::getServiceInfo(const std::string& name) const {
    auto it = services_.find(name);
    if (it == services_.end()) {
        return "Service not found: " + name;
    }
    
    const auto& reg = it->second;
    const auto& meta = reg.metadata;
    
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - meta.createdTime).count();
    auto lastAccess = std::chrono::duration_cast<std::chrono::milliseconds>(now - meta.lastAccessTime).count();
    
    return "Service: " + name + 
           "\n  Type: " + meta.typeName +
           "\n  Lifecycle: " + (reg.lifecycle == Lifecycle::Singleton ? "Singleton" : "Transient") +
           "\n  Initialized: " + (reg.isInitialized ? "Yes" : "No") +
           "\n  Healthy: " + (meta.isHealthy ? "Yes" : "No") +
           "\n  Access Count: " + std::to_string(meta.accessCount) +
           "\n  Age: " + std::to_string(age) + "ms" +
           "\n  Last Access: " + std::to_string(lastAccess) + "ms ago" +
           "\n  Dependencies: " + (meta.dependencies.empty() ? "None" : join(meta.dependencies, ", ")) +
           (meta.lastError.empty() ? "" : "\n  Last Error: " + meta.lastError);
}

std::string DependencyContainer::getDependencyGraph() const {
    std::string result = "Dependency Graph:\n";
    for (const auto& pair : dependencyGraph_) {
        result += "  " + pair.first + " -> ";
        if (pair.second.empty()) {
            result += "[]\n";
        } else {
            result += "[" + join(pair.second, ", ") + "]\n";
        }
    }
    return result;
}

bool DependencyContainer::validateAllServices() const {
    bool allValid = true;
    for (const auto& pair : services_) {
        if (!pair.second.metadata.isHealthy) {
            allValid = false;
        }
    }
    return allValid;
}

std::string DependencyContainer::getRegisteredServicesList() const {
    std::vector<std::string> names;
    for (const auto& pair : services_) {
        names.push_back(pair.first);
    }
    return join(names, ", ");
}

void DependencyContainer::validateDependencies(const std::string& serviceName) const {
    auto it = services_.find(serviceName);
    if (it == services_.end()) return;
    
    const auto& deps = it->second.metadata.dependencies;
    for (const auto& dep : deps) {
        if (services_.find(dep) == services_.end()) {
            throw std::runtime_error("Dependency not found: " + dep + " (required by " + serviceName + ")");
        }
    }
}

std::string DependencyContainer::join(const std::vector<std::string>& vec, const std::string& sep) const {
    if (vec.empty()) return "";
    
    std::string result = vec[0];
    for (size_t i = 1; i < vec.size(); ++i) {
        result += sep + vec[i];
    }
    return result;
}

