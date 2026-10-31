/*
 * Copyright (c) 2019 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */

#ifndef T_STRUCT_COMMON_H
#define T_STRUCT_COMMON_H

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "t_type.h"
#include "t_field.h"

class t_program;

/**
 * A struct_common is a container for a set of member fields that has a name.
 * It is a base class for struct and sandesh containers.
 *
 */
class t_struct_common : public t_type {
 public:
  typedef std::vector<t_field*> members_type;

  virtual ~t_struct_common() {}

  virtual std::string get_struct_type_name() = 0;

  const members_type& get_members() {
    return members_;
  }

  const members_type& get_sorted_members() {
    return members_in_id_order_;
  }

  bool append(t_field* elem) {
    members_.push_back(elem);

    typedef members_type::iterator iter_type;
    std::pair<iter_type, iter_type> bounds = std::equal_range(
      members_in_id_order_.begin(), members_in_id_order_.end(),
      elem, t_field::key_compare()
    );

    if (bounds.first != bounds.second) {
      return false;
    }

    members_in_id_order_.insert(bounds.second, elem);
    return true;
  }

  virtual std::string get_fingerprint_material() const {
    std::string rv = "{";
    members_type::const_iterator m_iter;
    for (m_iter = members_in_id_order_.begin(); m_iter != members_in_id_order_.end(); ++m_iter) {
      rv += (*m_iter)->get_fingerprint_material();
      rv += ";";
    }
    rv += "}";
    return rv;
  }

  virtual void generate_fingerprint() {
    t_type::generate_fingerprint();
    members_type::const_iterator m_iter;
    for (m_iter = members_in_id_order_.begin(); m_iter != members_in_id_order_.end(); ++m_iter) {
      (*m_iter)->get_type()->generate_fingerprint();
    }
  }

 protected:
  t_struct_common(t_program* program) :
    t_type(program) {}

  t_struct_common(t_program* program, const std::string& name) :
    t_type(program, name) {}

  members_type members_;
  members_type members_in_id_order_;
};

#endif // T_STRUCT_COMMON_H
