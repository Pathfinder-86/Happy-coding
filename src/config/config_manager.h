#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <variant>
namespace config {

enum class ConfigType { String, Bool, Int, Double };
using ConfigValue = std::variant<std::string, bool, int, double>;

class Config {
public:
    ConfigType type;
    ConfigValue value;
};


class ConfigManager {
private:
    std::map<std::string, Config> config_map;
    ConfigManager() {
        init_config_value();
    }
public:
    static ConfigManager& get_instance() {
        static ConfigManager instance;
        return instance;
    }
    void parse_config(int argc, char *argv[]);
    void add_config(const std::string &key, ConfigType type, const ConfigValue &value);
    void init_config_value();
    void set_config(const std::string& key, const ConfigValue& new_value);
    ConfigValue get_config_value(const std::string& key) const;
};
}
#endif // CONFIG_MANAGER_H
