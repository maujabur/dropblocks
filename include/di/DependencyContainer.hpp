#pragma once
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

class DependencyContainer {
public:
    enum class Lifecycle {
        Singleton,
        Transient
    };

private:
    struct ServiceMetadata {
        std::string name;
        std::string typeName;
        std::vector<std::string> dependencies;
        std::chrono::steady_clock::time_point createdTime;
        std::chrono::steady_clock::time_point lastAccessTime;
        size_t accessCount = 0;
        bool isHealthy = true;
        std::string lastError;
    };
    
    struct ServiceRegistration {
        std::function<void*()> factory;
        Lifecycle lifecycle;
        void* instance = nullptr;
        bool isInitialized = false;
        ServiceMetadata metadata;
    };
    
    std::map<std::string, ServiceRegistration> services_;
    std::vector<std::string> resolutionStack_;
    std::map<std::string, std::vector<std::string>> dependencyGraph_;
    
    void validateDependencies(const std::string& serviceName) const;
    std::string join(const std::vector<std::string>& vec, const std::string& sep) const;

public:
    DependencyContainer() = default;
    ~DependencyContainer();
    
    DependencyContainer(const DependencyContainer&) = delete;
    DependencyContainer& operator=(const DependencyContainer&) = delete;
    DependencyContainer(DependencyContainer&&) = delete;
    DependencyContainer& operator=(DependencyContainer&&) = delete;
    
    template<typename T>
    void registerService(const std::string& name, std::function<T*()> factory, 
                        Lifecycle lifecycle = Lifecycle::Singleton,
                        const std::vector<std::string>& dependencies = {});
    
    template<typename T>
    void registerInstance(const std::string& name, T* instance);
    
    template<typename T>
    T* resolve(const std::string& name);
    
    bool isRegistered(const std::string& name) const;
    std::vector<std::string> getRegisteredServices() const;
    void clear();
    std::string getStats() const;
    std::string getServiceInfo(const std::string& name) const;
    std::string getDependencyGraph() const;
    bool validateAllServices() const;
    std::string getRegisteredServicesList() const;
};

// Template implementations (must be in header)
#include "di/DependencyContainer.inl"

