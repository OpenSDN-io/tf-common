/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * Modifications Copyright (c) Juniper Networks, Inc.
 * Copyright Contributors to the OpenSDN project
 */

#ifndef T_STRUCT_H
#define T_STRUCT_H

#include <string>

#include "t_type.h"
#include "t_struct_common.h"
#include "t_base_type.h"

// Forward declare that puppy
class t_program;

/**
 * A struct is a container for a set of member fields that has a name. Structs
 * are also used to implement exception types.
 *
 */
class t_struct : public t_struct_common {
 public:
  t_struct(t_program* program) :
    t_struct_common(program),
    is_xception_(false),
    xsd_all_(false) {}

  t_struct(t_program* program, const std::string& name) :
    t_struct_common(program, name),
    is_xception_(false),
    xsd_all_(false) {}

  virtual std::string get_struct_type_name() {
    return "Struct";
  }

  void set_xception(bool is_xception) {
    is_xception_ = is_xception;
  }

  void set_union(bool is_union) {
    is_union_ = is_union;
  }

  void set_xsd_all(bool xsd_all) {
    xsd_all_ = xsd_all;
  }

  bool get_xsd_all() const {
    return xsd_all_;
  }

  bool is_struct() const {
    return !is_xception_;
  }

  bool is_xception() const {
    return is_xception_;
  }

  bool is_union() const {
    return is_union_;
  }

  t_field* get_field_by_name(std::string field_name) {
    members_type::const_iterator m_iter;
    for (m_iter = members_in_id_order_.begin(); m_iter != members_in_id_order_.end(); ++m_iter) {
      if ((*m_iter)->get_name() == field_name) {
        return *m_iter;
      }
    }
    return NULL;
  }

#ifdef SANDESH
  virtual bool has_key_annotation() const;
#endif

 private:
  bool is_xception_;
  bool is_union_;
  bool xsd_all_;
};

#endif
