//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  DataManager handles a set of DataNode objects with the same tracking settings.

#ifndef EMP_DATA_MANAGER_H
#define EMP_DATA_MANAGER_H

#include <map>
#include <string>

#include "DataNode.h"

namespace emp {

  template <typename VAL_TYPE, emp::data... MODS>
  class DataManager {
  private:
    using data_t = VAL_TYPE;
    using node_t = DataNode<data_t, MODS...>;

    std::map<std::string, node_t *> node_map;

  public:
    DataManager() = default;
    ~DataManager() {
      for (auto & x : node_map) delete x.second;
    }
  };

}

#endif
