#pragma once

#include "plugin_interface.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

struct LoadedPlugin {
    void* handle;
    Plugin* plugin;  // Use raw pointer instead of unique_ptr
    DestroyPluginFunc destroyFunc;
    std::string path;
    std::string name;
    std::string version;
};

/**
 * @brief Dynamic plugin manager
 */
class PluginManager {
private:
    std::vector<LoadedPlugin> loadedPlugins;
    std::string pluginDirectory;
    
public:
    PluginManager(const std::string& pluginDir = "plugins");
    ~PluginManager();
    
    // Plugin Management
    bool loadPlugin(const std::string& pluginPath);
    bool loadAllPlugins();
    void unloadAllPlugins();
    
    // Device Detection and Matching
    std::vector<DeviceInfo> detectAllDevices() const;
    Plugin* findBestPluginForDevice(const DeviceInfo& device) const;
    std::vector<Plugin*> getLoadedPlugins() const;
    
    // Plugin Info
    std::vector<std::string> getPluginNames() const;
    Plugin* getPluginByName(const std::string& name) const;
    
    // Utility
    void setPluginDirectory(const std::string& dir);
    std::string getPluginDirectory() const;
    
private:
    std::vector<std::string> findPluginFiles() const;
    bool isValidPluginFile(const std::string& filename) const;
    void* loadLibrary(const std::string& path) const;
    void unloadLibrary(void* handle) const;
};
