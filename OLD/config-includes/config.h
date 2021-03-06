//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2016.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  This file defines a master configuration option Config, whose values can be loaded
//  at runtime or else set as constant values throughout the code.
//
//  Assuming you have an emp::Config object called config, you can:
//
//  access a setting value:            config.SETTING_NAME()
//  adjust a setting value:            config.SETTING_NAME(new_value)
//  determine if a setting is locked:  config.SETTING_NAME_is_const()
//  lookup a setting dynamically:      config("SETTING_NAME")
//  adjust a setting dynamically:      config("SETTING_NAME", "new_value")
//
//  load settings from a stream:       config.Read(stream);
//  load settings from a file:         config.Read(filename);
//  save settings to a stream:         config.Write(stream);
//  save settings to a file:           config.Write(filename);
//
//  write settings macros to a stream: config.WriteMacros(stream);
//  write settings macros to a file:   config.WriteMacros(filename);
//
//
//  The configuration files generated can use the following keywords in order to
//  configure this object:
//   include OTHER_FILENAME         -- Load in all data from another file.
//   set SETTING_NAME VALUE         -- Set a basic configuration setting.
//   new OBJECT_TYPE OBJECT_NAME    -- Create a new config object of a managed class.
//   use OBJECT_TYPE OBJECT_NAME    -- Use a previouly create configuration object.

#ifndef EMP_CONFIG_H
#define EMP_CONFIG_H


#include <map>
#include <ostream>
#include <fstream>
#include <functional>
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "../tools/errors.h"
#include "../tools/functions.h"
#include "../tools/string_utils.h"
#include "ConfigManager.h"

using namespace std::placeholders;

namespace emp {

  // Master configuration class.
  class Config {
  private:
    class ConfigEntry {
    protected:
      std::string name;
      std::string type;
      std::string default_val;
      std::string desc;

      std::unordered_set<std::string> alias_set;
    public:
      ConfigEntry(const std::string _name, const std::string _type,
                   const std::string _d_val, const std::string _desc)
        : name(_name), type(_type), default_val(_d_val), desc(_desc)
      { ; }
      virtual ~ConfigEntry() { ; }

      const std::string & GetName() const { return name; }
      const std::string & GetType() const { return type; }
      const std::string & GetDefault() const { return default_val; }
      const std::string & GetDescription() const { return desc; }

      ConfigEntry & SetName(const std::string & _in) { name = _in; return *this; }
      ConfigEntry & SetType(const std::string & _in) { type = _in; return *this; }
      ConfigEntry & SetDefault(const std::string & _in) { default_val = _in; return *this; }
      ConfigEntry & SetDescription(const std::string & _in) { desc = _in; return *this; }

      ConfigEntry & AddAlias(const std::string & _in) { alias_set.insert(_in); return *this; }
      bool HasAlias(const std::string & _in) { return alias_set.find(_in) != alias_set.end(); }
      bool IsMatch(const std::string & _in) { return name == _in || HasAlias(_in); }
      const std::unordered_set<std::string> & GetAliases() { return alias_set; }

      virtual std::string GetValue() const = 0;
      virtual std::string GetLiteralValue() const = 0;
      virtual ConfigEntry & SetValue(const std::string & in_val, std::stringstream & warnings) = 0;
      virtual bool IsConst() const = 0;
    };

    // We need type-specific versions on this class to manage variables
    template <class VAR_TYPE> class tConfigEntry : public ConfigEntry {
    private:
      VAR_TYPE & entry_ref;
    public:
      tConfigEntry(const std::string _name, const std::string _type,
                   const std::string _d_val, const std::string _desc,
                   VAR_TYPE & _ref)
        : ConfigEntry(_name, _type, _d_val, _desc), entry_ref(_ref) { ; }
      ~tConfigEntry() { ; }

      std::string GetValue() const { std::stringstream ss; ss << entry_ref; return ss.str(); }
      std::string GetLiteralValue() const { return to_literal(entry_ref); }
      ConfigEntry & SetValue(const std::string & in_val, std::stringstream & warnings) {
        (void) warnings;
        std::stringstream ss; ss << in_val; ss >> entry_ref; return *this;
      }
      bool IsConst() const { return false; }
    };

    // We need a special entry type to represent constant values.
    template <class VAR_TYPE> class tConfigConstEntry : public ConfigEntry {
    private:
      const VAR_TYPE literal_val;
    public:
      tConfigConstEntry(const std::string _name, const std::string _type,
                        const std::string _d_val, const std::string _desc,
                        const VAR_TYPE & _literal_val)
        : ConfigEntry(_name, _type, _d_val, _desc), literal_val(_literal_val) { ; }
      ~tConfigConstEntry() { ; }

      std::string GetValue() const { return default_val; }
      std::string GetLiteralValue() const { return to_literal(literal_val); }
      ConfigEntry & SetValue(const std::string & in_val, std::stringstream & warnings) {
        // This is a constant setting.  If we are actually trying to change it, give a warning.
        if (in_val != GetValue()) {
          warnings << "Trying to adjust locked setting '"
                   << name << "' from '" << GetValue()
                   << "' to '" << in_val << "'. Ignoring." << std::endl;
        }
        return *this;
      }
      bool IsConst() const { return true; }
    };

    // A special entry for settings created during the run (only accissibly dynamically)
    class ConfigLiveEntry : public ConfigEntry {
    public:
      ConfigLiveEntry(const std::string _name, const std::string _type,
                       const std::string _d_val, const std::string _desc)
        : ConfigEntry(_name, _type, _d_val, _desc) { ; }
      ~ConfigLiveEntry() { ; }

      std::string GetValue() const { return default_val; }
      std::string GetLiteralValue() const { return to_literal(default_val); }
      ConfigEntry & SetValue(const std::string & in_val, std::stringstream & warnings) {
        (void) warnings;
        default_val = in_val;
        return *this;
      }
      bool IsConst() const { return false; }
    };

    // Entrys should be divided into groups
    class ConfigGroup {
    private:
      std::string m_name;
      std::string m_desc;
      std::vector<ConfigEntry *> entry_set;
    public:
      ConfigGroup(const std::string & _name, const std::string & _desc)
        : m_name(_name), m_desc(_desc)
      { ; }
      ~ConfigGroup() { ; }

      int GetSize() const { return (int) entry_set.size(); }
      ConfigEntry * GetEntry(int id) { return entry_set[id]; }
      ConfigEntry * GetLastEntry() { return entry_set.back(); }

      void Add(ConfigEntry * new_entry) { entry_set.push_back(new_entry); }

      void Write(std::ostream & out) {
        // Print header information with the group name.
        out << "### " << m_name << " ###" << std::endl;
        // Print group description.
        std::vector<std::string> desc_lines;
        slice_string(m_desc, desc_lines);
        for (int comment_line = 0; comment_line < (int) desc_lines.size(); comment_line++) {
          out << "# " << desc_lines[comment_line] << std::endl;
        }
        out << std::endl;

        const int entry_count = entry_set.size();
        std::vector<std::string> setting_info(entry_count);
        int max_length = 0;

        // Loop through once to figure out non-comment output
        for (int i = 0; i < entry_count; i++) {
          setting_info[i] = "set ";
          setting_info[i] += entry_set[i]->GetName();
          setting_info[i] += " ";
          setting_info[i] += entry_set[i]->GetValue();
          if (max_length < (int) setting_info[i].size()) max_length = (int) setting_info[i].size();
        }

        max_length += 2;
        for (int i = 0; i < entry_count; i++) {
          out << setting_info[i];

          // Break the description up over multiple lines.
          std::vector<std::string> desc_lines;
          emp::slice_string(entry_set[i]->GetDescription(), desc_lines);

          int start_col = (int) setting_info[i].size();
          for (int comment_line = 0; comment_line < (int) desc_lines.size(); comment_line++) {
            for (int ws = start_col; ws < max_length; ws++) out << ' ';
            out << "# " << desc_lines[comment_line] << std::endl;
            start_col = 0;
          }
        }

        out << std::endl; // Skip a line after each group.
      }

      void WriteMacros(std::ostream & out) {
        // Print header information to register group.
        out << "EMP_CONFIG_GROUP(" << m_name << ", \"" << m_desc << "\")" << std::endl;

        // Loop through once to figure out non-comment output
        for (ConfigEntry * cur_entry : entry_set) {
          if (cur_entry->IsConst()) { out << "EMP_CONFIG_CONST("; }
          else { out << "EMP_CONFIG_VAR("; }

          out << cur_entry->GetName() << ", "
              << cur_entry->GetType() << ", "
              << cur_entry->GetLiteralValue() << ", "
              << to_literal( cur_entry->GetDescription() )
              << std::endl;

          // Output aliases.
          const std::unordered_set<std::string> & alias_set = cur_entry->GetAliases();
          for (const std::string & cur_alias : alias_set) {
            out << "EMP_CONFIG_ALIAS(" << cur_alias << ")" << std::endl;
          }
        }

        out << std::endl; // Skip a line after each group.
      }
    };

    // Private member variables
    std::map<std::string, ConfigEntry *> m_var_map; // All variables across groups.
    std::string m_version_id;                       // Unique version ID to ensure synced config.
    std::vector<ConfigGroup *> m_group_set;         // All of the config groups.
    std::stringstream m_warnings;                   // Aggrigate warnings for combined display.
    int m_delay_warnings;                           // Count of delays to collect warnings for printing.

    // Map new type names to the manager that handles them.
    std::map<std::string, ConfigManager_Base *> m_type_manager_map;

    // Build a map of extra input commands to the function that they should call if triggered.
    std::map<std::string, std::function<bool(std::string)> > m_command_map;
    std::map<std::string, std::function<bool(std::string)> > m_new_map;
    std::map<std::string, std::function<bool(std::string)> > m_use_map;

    // Place all of the config private member variables here.
#define EMP_CONFIG_VAR(NAME, TYPE, DEFAULT, DESC) TYPE m_ ## NAME;
#include "config_include.h"

  public:
    Config(const std::string & in_version = "")
      : m_version_id(in_version)
      , m_delay_warnings(0)
        // Setup inital values for all variables.
#define EMP_CONFIG_VAR(NAME, TYPE, DEFAULT, DESC) , m_ ## NAME(DEFAULT)
#include "config_include.h"
    {
      // Build a map to information about each variable.
#define EMP_CONFIG_VAR(NAME, TYPE, DEFAULT, DESC)                                          \
      m_var_map[#NAME] = new tConfigEntry<TYPE>(#NAME, #TYPE, #DEFAULT, DESC, m_ ## NAME); \
      m_group_set.back()->Add(m_var_map[#NAME]);
#define EMP_CONFIG_CONST(NAME, TYPE, VALUE, DESC)                                          \
      m_var_map[#NAME] = new tConfigConstEntry<TYPE>(#NAME, #TYPE, #VALUE, DESC, VALUE); \
      m_group_set.back()->Add(m_var_map[#NAME]);
#define EMP_CONFIG_GROUP(NAME, DESC) \
      m_group_set.push_back(new ConfigGroup(#NAME, DESC));
#include "config_include.h"
    }

    ~Config() {
      // Delete all entries in the var_map
      for (auto it = m_var_map.begin(); it != m_var_map.end(); it++) {
        delete it->second;
      }
      for (auto it = m_type_manager_map.begin(); it != m_type_manager_map.end(); it++) {
        delete it->second;
      }
    }

    std::string Get(const std::string & setting_name) {
      if (m_var_map.find(setting_name) == m_var_map.end()) {
        // This setting is not currently in the map!
        // @CAO Print warning?
        return "";
      }
      return m_var_map[setting_name]->GetValue();
    }

    Config & Set(const std::string & setting_name, const std::string & new_value,
                  const std::string & in_desc="") {
      if (m_var_map.find(setting_name) == m_var_map.end()) {
        // This setting is not currently in the map!  We should put it in, but let user know.
        m_warnings << "Unknown setting '" << setting_name << "'.  Creating." << std::endl;
        m_var_map[setting_name] = new ConfigLiveEntry(setting_name, "std::string", new_value, in_desc);
        m_group_set.back()->Add(m_var_map[setting_name]);
      }
      m_var_map[setting_name]->SetValue(new_value, m_warnings);
      if (!m_delay_warnings && m_warnings.rdbuf()->in_avail()) {
        emp::NotifyWarning(m_warnings.str());
        m_warnings.str(std::string()); // Clear the warnings.
      }
      return *this;
    }

    std::string operator()(const std::string & setting_name) { return Get(setting_name); }

    Config & operator()(const std::string & setting_name, const std::string & new_value) {
      return Set(setting_name, new_value);
    }

    // Generate a text representation (typically a file) for the state of Config
    void Write(std::ostream & out) {
      // @CAO Start by printing some file header information?

      // Next print each group and it's information.
      for (auto it = m_group_set.begin(); it != m_group_set.end(); it++) {
        (*it)->Write(out);
      }
    }

    // If a string is passed into Write, treat it as a filename.
    void Write(std::string filename) {
      std::ofstream out(filename);
      Write(out);
      out.close();
    }

    // Generate a text representation (typically a file) for the state of Config
    void WriteMacros(std::ostream & out) {
      out << "/////////////////////////////////////////////////////////////////////////////////\n"
          << "//  This is an auto-generated file that defines a set of configuration options.\n"
          << "//  This file is read in mulitple times from config.h, each with different macro\n"
          << "//  definitions to generate correct, effecient code for the command below.\n"
          << "//\n"
          << "//  The available commands are:\n"
          << "//\n"
          << "//  EMP_CONFIG_GROUP(group name, group description string)\n"
          << "//   Start a new group of configuration options.  Group structure is preserved\n"
          << "//   when user-accessible configuration options are generated.\n"
          << "//\n"
          << "//  EMP_CONFIG_VAR(variable name, type, default value, description string)\n"
          << "//   Create a new setting in the emp::Config object that can be easily accessed.\n"
          << "//\n"
          << "//  EMP_CONFIG_ALIAS(alias name)\n"
          << "//   Include an alias for the previous setting.  This command is useful to\n"
          << "//   maintain backward compatibility if names change in newer software versions.\n"
          << "//\n"
          << "//  EMP_CONFIG_CONST(variable name, type, fixed value, description string)\n"
          << "//   Create a new configuration constant that cannot be changed.  In practice,\n"
          << "//   allows broader optimizations in the code.\n"
          << std::endl;

      // Next print each group and it's information.
      for (auto it = m_group_set.begin(); it != m_group_set.end(); it++) {
        (*it)->WriteMacros(out);
      }
    }

    // If a string is passed into Write, treat it as a filename.
    void WriteMacros(std::string filename) {
      std::ofstream out(filename);
      WriteMacros(out);
      out.close();
    }

    // Read in from a text representation (typically a file) to set the state of Config.
    // Return success state.
    bool Read(std::istream & input) {
      // Load in the file one line at a time and process each line.
      std::string cur_line;
      m_delay_warnings++;

      // Loop through the file until eof is hit (does this work for other streams?)
      while (!input.eof()) {
        std::getline(input, cur_line);             // Get the current input line.
        cur_line = emp::string_pop(cur_line, '#'); // Deal with commments.
        emp::left_justify(cur_line);               // Clear out leading whitespace.
        if (cur_line == "") continue;              // Skip empty lines.

        std::string command = emp::string_pop_word(cur_line);
        emp::right_justify(cur_line);

        if (command == "include") {
          // Recursively include another configuration file.
          std::string filename = emp::string_pop_word(cur_line);
          Read(filename);
        }
        else if (command == "new") {
          std::string type_name = emp::string_pop_word(cur_line);
          // @CAO Make sure type exists!
          m_new_map[type_name](cur_line);
        }
        else if (command == "set") {
          // Set a specific value.
          std::string setting_name = emp::string_pop_word(cur_line);
          Set(setting_name, cur_line);
        }
        else if (command == "use") {
          std::string type_name = emp::string_pop_word(cur_line);
          // @CAO Make sure type exists!
          m_use_map[type_name](cur_line);
        }
        else if (m_command_map.find(command) != m_command_map.end()) {
          // Run this custom command.
          m_command_map[command](cur_line);
        }
        else {
          // We don't know this command... give an error and move on.
          std::stringstream ss;
          ss << "Unknown configuration command '" << command << "'. Ignoring." << std::endl;
          emp::NotifyError(ss.str());
        }
      }

      // Print out all accumulated warnings (if any).
      if (m_warnings.rdbuf()->in_avail()) {
        emp::NotifyWarning(m_warnings.str());
        m_warnings.str(std::string()); // Clear the warnings.
      }
      m_delay_warnings--;

      return true;
    }

    bool Read(std::string filename) {
      std::ifstream in_file(filename);
      if (in_file.fail()) {
        std::stringstream ss;
        ss << "Unable to open config file '" << filename << "'. Ignoring." << std::endl;
        emp::NotifyError(ss.str());
        return false;
      }
      bool success = Read(in_file);
      in_file.close();
      return success;
    }


    void AddCommand(const std::string & command_name, std::function<bool(std::string)> command_fun) {
      // Give a warning if we are re-defining an existing command.
      if (m_command_map.find(command_name) != m_command_map.end()) {
        m_warnings << "Re-defining command '" << command_name << "'. Allowing." << std::endl;
        if (!m_delay_warnings) {
          emp::NotifyWarning(m_warnings.str());
          m_warnings.str(std::string()); // Clear the warnings.
        }
      }
      m_command_map[command_name] = command_fun;
    }

    void AddNewCallback(const std::string & type_name, std::function<bool(std::string)> new_fun) {
      // Give a warning if we are re-defining an existing command.
      if (m_new_map.find(type_name) != m_new_map.end()) {
        m_warnings << "Re-defining config type '" << type_name << "'. Allowing." << std::endl;
        if (!m_delay_warnings) {
          emp::NotifyWarning(m_warnings.str());
          m_warnings.str(std::string()); // Clear the warnings.
        }
      }
      m_new_map[type_name] = new_fun;
    }

    void AddUseCallback(const std::string & type_name, std::function<bool(std::string)> use_fun) {
      // Give a warning if we are re-defining an existing command.
      if (m_use_map.find(type_name) != m_use_map.end()) {
        m_warnings << "Re-defining config type '" << type_name << "'. Allowing." << std::endl;
        if (!m_delay_warnings) {
          emp::NotifyWarning(m_warnings.str());
          m_warnings.str(std::string()); // Clear the warnings.
        }
      }
      m_use_map[type_name] = use_fun;
    }


    template <class MANAGED_TYPE>
    void AddManagedType(const std::string & type_keyword, const std::string & command_keyword,
                        std::function<bool(MANAGED_TYPE &, std::string)> fun_callback)
    {
      ConfigManager<MANAGED_TYPE> * new_manager = new ConfigManager<MANAGED_TYPE>(type_keyword, command_keyword, fun_callback);
      m_type_manager_map[type_keyword] = new_manager;

      AddCommand(command_keyword,
                 std::bind(&ConfigManager<MANAGED_TYPE>::CommandCallback, new_manager, _1) );
      AddNewCallback(type_keyword,
                     std::bind(&ConfigManager<MANAGED_TYPE>::NewObject, new_manager, _1) );
      AddUseCallback(type_keyword,
                     std::bind(&ConfigManager<MANAGED_TYPE>::UseObject, new_manager, _1) );

    }

    // Build Get and Set Accessors, as well as const check
#define EMP_CONFIG_VAR(NAME, TYPE, DEFAULT, DESC)                       \
    inline const TYPE & NAME() const { return m_ ## NAME; }             \
    const TYPE & NAME(const TYPE & _in) { m_ ## NAME = _in; return m_ ## NAME; } \
    bool NAME ## _is_const() const { return false; }
#define EMP_CONFIG_CONST(NAME, TYPE, VALUE, DESC)                       \
    constexpr static TYPE NAME() { return VALUE; }                      \
    TYPE NAME(const TYPE & _in) {                                       \
      std::stringstream ss;                                             \
      ss << "Trying to set const '" << #NAME << "'. Ignoring." << std::endl; \
      emp::NotifyWarning(ss.str());                                     \
      return VALUE;                                                     \
    }                                                                   \
    bool NAME ## _is_const() const { return true; }
#include "config_include.h"

  };
};

#endif
