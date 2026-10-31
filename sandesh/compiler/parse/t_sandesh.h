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
 * Copyright 2006-2017 The Apache Software Foundation.
 * https://github.com/apache/thrift
 *
 * Modifications Copyright (c) Juniper Networks, Inc.
 * Copyright Contributors to the OpenSDN project
 */

#ifndef T_SANDESH_H
#define T_SANDESH_H

#include <string>

#include "t_type.h"
#include "t_struct_common.h"
#include "t_base_type.h"

// Forward declare that puppy
class t_program;

/**
 * A sandesh is a container for a set of member fields that has a name.
 *
 */
class t_sandesh : public t_struct_common {
 public:
  t_sandesh(t_program* program) :
    t_struct_common(program),
    type_(NULL) {}

  t_sandesh(t_program* program, const std::string& name) :
    t_struct_common(program, name),
    type_(NULL) {}

  virtual std::string get_struct_type_name() {
    return "Sandesh";
  }

  virtual bool is_sandesh() const {
    return true;
  }

  void set_type(t_type* type) {
    type_ = type;
  }

  bool exist_opt_field() {
    members_type::const_iterator m_iter;
    for (m_iter = members_in_id_order_.begin(); m_iter != members_in_id_order_.end(); ++m_iter) {
      if ((*m_iter)->get_req() == t_field::T_OPTIONAL) {
        return true;
      }
    }
    return false;
  }

  const t_type* get_type() {
    return type_;
  }

  bool is_level_category_supported() const {
    t_base_type *btype = (t_base_type *) type_;
    return btype->is_sandesh_system() || btype->is_sandesh_object() ||
      btype->is_sandesh_flow() || btype->is_sandesh_uve();
  }

  virtual bool has_key_annotation() const;

 private:
  t_type*      type_;
};

#endif
