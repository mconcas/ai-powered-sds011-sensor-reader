#include "plugin_manager.h"
#include <dlfcn.h>
#include <dirent.h>
#include <algorithm>
#include <iostream>
#include <sys/stat.h>
#include <cstring>

#ifdef __APPLE__
    #define PLUGIN_EXTENSION ".dylib"
#else
    #define PLUGIN_EXTENSION ".so"
#endif

PluginManager::PluginManager(const std::string& pluginDir) 
    : pluginDirectory(pluginDir) {
}

PluginManager::~PluginManager() {
    unloadAllPlugins();
}

bool PluginManager::loadPlugin(const std::string& pluginPath) {
    void* handle = loadLibrary(pluginPath);
    if (!handle) {
        std::cerr << "Failed to load plugin library: " << pluginPath << std::endl;
        return false;
    }
    
    // Get plugin factory function
    CreatePluginFunc createFunc = (CreatePluginFunc)dlsym(handle, CREATE_PLUGIN_FUNC);
    if (!createFunc) {
        std::cerr << "Plugin missing createPlugin function: " << pluginPath << std::endl;
        unloadLibrary(handle);
        return false;
    }
    
    // Get plugin info functions
    GetPluginNameFunc getNameFunc = (GetPluginNameFunc)dlsym(handle, GET_PLUGIN_NAME_FUNC);
    GetPluginVersionFunc getVersionFunc = (GetPluginVersionFunc)dlsym(handle, GET_PLUGIN_VERSION_FUNC);
    DestroyPluginFunc destroyFunc = (DestroyPluginFunc)dlsym(handle, DESTROY_PLUGIN_FUNC);
    
    // Create plugin instance
    Plugin* plugin = createFunc();
    if (!plugin) {
        std::cerr << "Failed to create plugin instance: " << pluginPath << std::endl;
        unloadLibrary(handle);
        return false;
    }
    
    // Initialize plugin
    if (!plugin->initialize()) {
        std::cerr << "Failed to initialize plugin: " << pluginPath << std::endl;
        delete plugin;
        unloadLibrary(handle);
        return false;
    }
    
    // Store loaded plugin
    LoadedPlugin loadedPlugin;
    loadedPlugin.handle = handle;
    loadedPlugin.plugin = plugin;  // Store raw pointer
    loadedPlugin.destroyFunc = destroyFunc;
    loadedPlugin.path = pluginPath;
    loadedPlugin.name = getNameFunc ? getNameFunc() : "Unknown";
    loadedPlugin.version = getVersionFunc ? getVersionFunc() : "Unknown";
    
    std::cout << "Loaded plugin: " << loadedPlugin.name 
              << " v" << loadedPlugin.version << std::endl;
    
    loadedPlugins.push_back(std::move(loadedPlugin));
    
    return true;
}

bool PluginManager::loadAllPlugins() {
    auto pluginFiles = findPluginFiles();
    bool allLoaded = true;
    
    for (const auto& file : pluginFiles) {
        if (!loadPlugin(file)) {
            allLoaded = false;
        }
    }
    
    std::cout << "Loaded " << loadedPlugins.size() << " plugin(s)" << std::endl;
    return allLoaded;
}

void PluginManager::unloadAllPlugins() {
    for (auto& loadedPlugin : loadedPlugins) {
        if (loadedPlugin.plugin) {
            loadedPlugin.plugin->cleanup();
            // Use the destroy function if available
            if (loadedPlugin.destroyFunc) {
                loadedPlugin.destroyFunc(loadedPlugin.plugin);
            } else {
                delete loadedPlugin.plugin;
            }
            loadedPlugin.plugin = nullptr;
        }
        if (loadedPlugin.handle) {
            unloadLibrary(loadedPlugin.handle);
            loadedPlugin.handle = nullptr;
        }
    }
    loadedPlugins.clear();
}

std::vector<DeviceInfo> PluginManager::detectAllDevices() const {
    std::vector<DeviceInfo> allDevices;
    
    for (const auto& loadedPlugin : loadedPlugins) {
        auto devices = loadedPlugin.plugin->detectDevices();
        
        // Add devices, but avoid duplicates using DeviceInfo equality
        for (const auto& device : devices) {
            bool isDuplicate = false;
            for (const auto& existingDevice : allDevices) {
                if (existingDevice == device) {
                    isDuplicate = true;
                    break;
                }
            }
            
            if (!isDuplicate) {
                allDevices.push_back(device);
            }
        }
    }
    
    return allDevices;
}

Plugin* PluginManager::findBestPluginForDevice(const DeviceInfo& device) const {
    Plugin* bestPlugin = nullptr;
    double bestScore = 0.0;
    
    for (const auto& loadedPlugin : loadedPlugins) {
        if (loadedPlugin.plugin->canHandleDevice(device)) {
            double score = loadedPlugin.plugin->getDeviceMatchScore(device);
            if (score > bestScore) {
                bestScore = score;
                bestPlugin = loadedPlugin.plugin;
            }
        }
    }
    
    return bestPlugin;
}

std::vector<Plugin*> PluginManager::getLoadedPlugins() const {
    std::vector<Plugin*> plugins;
    for (const auto& loadedPlugin : loadedPlugins) {
        plugins.push_back(loadedPlugin.plugin);
    }
    return plugins;
}

std::vector<std::string> PluginManager::getPluginNames() const {
    std::vector<std::string> names;
    for (const auto& loadedPlugin : loadedPlugins) {
        names.push_back(loadedPlugin.name);
    }
    return names;
}

Plugin* PluginManager::getPluginByName(const std::string& name) const {
    for (const auto& loadedPlugin : loadedPlugins) {
        if (loadedPlugin.name == name) {
            return loadedPlugin.plugin;
        }
    }
    return nullptr;
}

void PluginManager::setPluginDirectory(const std::string& dir) {
    pluginDirectory = dir;
}

std::string PluginManager::getPluginDirectory() const {
    return pluginDirectory;
}

std::vector<std::string> PluginManager::findPluginFiles() const {
    std::vector<std::string> pluginFiles;
    
    DIR* dir = opendir(pluginDirectory.c_str());
    if (!dir) {
        std::cout << "Plugin directory does not exist: " << pluginDirectory << std::endl;
        return pluginFiles;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN) {
            std::string filename = entry->d_name;
            if (isValidPluginFile(filename)) {
                std::string fullPath = pluginDirectory + "/" + filename;
                pluginFiles.push_back(fullPath);
            }
        }
    }
    closedir(dir);
    
    return pluginFiles;
}

bool PluginManager::isValidPluginFile(const std::string& filename) const {
    return filename.find(PLUGIN_EXTENSION) != std::string::npos &&
           filename.find("plugin") != std::string::npos;
}

void* PluginManager::loadLibrary(const std::string& path) const {
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "dlopen error: " << dlerror() << std::endl;
    }
    return handle;
}

void PluginManager::unloadLibrary(void* handle) const {
    if (handle) {
        dlclose(handle);
    }
}
