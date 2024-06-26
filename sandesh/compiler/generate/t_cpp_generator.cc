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
 * Contains some contributions under the Thrift Software License.
 * Please see doc/old-thrift-license.txt in the Thrift distribution for
 * details.
 */

#include <cassert>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifdef SANDESH
#include <algorithm>
#endif

#include <sys/stat.h>
#include <boost/tuple/tuple.hpp>
#include "platform.h"
#include "t_oop_generator.h"
using namespace std;
using boost::tuple;
using boost::tuples::make_tuple;

/**
 * C++ code generator. This is legitimacy incarnate.
 *
 */
class t_cpp_generator : public t_oop_generator {
 public:
  t_cpp_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
    : t_oop_generator(program)
  {
    (void) option_string;
    std::map<std::string, std::string>::const_iterator iter;

    iter = parsed_options.find("pure_enums");
    gen_pure_enums_ = (iter != parsed_options.end());

    iter = parsed_options.find("dense");
    gen_dense_ = (iter != parsed_options.end());

    iter = parsed_options.find("include_prefix");
    use_include_prefix_ = (iter != parsed_options.end());

    iter = parsed_options.find("cob_style");
    gen_cob_style_ = (iter != parsed_options.end());

    iter = parsed_options.find("no_client_completion");
    gen_no_client_completion_ = (iter != parsed_options.end());

    iter = parsed_options.find("templates");
    gen_templates_ = (iter != parsed_options.end());

    out_dir_base_ = "gen-cpp";
  }

  /**
   * Init and close methods
   */

  void init_generator();
  void close_generator();

  void generate_consts(std::vector<t_const*> consts);

  /**
   * Program-level generation functions
   */

  void generate_typedef(t_typedef* ttypedef);
  void generate_enum(t_enum* tenum);
  void generate_struct(t_struct* tstruct) {
    generate_cpp_struct(tstruct, false);
  }
  void generate_xception(t_struct* txception) {
    generate_cpp_struct(txception, true);
  }
#ifdef SANDESH
  void generate_sandesh(t_sandesh* tsandesh) {
    generate_cpp_sandesh(tsandesh);
  }
  void generate_cpp_sandesh(t_sandesh* tsandesh);
  void generate_sandesh_definition   (std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_fingerprint   (std::ofstream& out, t_sandesh* tsandesh, bool is_definition);
  void generate_sandesh_http_reader  (std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_reader       (std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_writer       (std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_creator      (std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_member_init_list(std::ofstream& out, t_sandesh* tsandesh, bool init_dval = false);
  void generate_sandesh_base_init    (ofstream& out, t_sandesh* tsandesh, bool init_dval);
  void generate_sandesh_hints        (std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_request      (std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_default_ctor (std::ofstream& out, t_sandesh* tsandesh, bool is_request);
  void generate_sandesh_loggers      (std::ofstream& out, t_sandesh* tsandesh);
  void generate_logger_field         (std::ofstream& out, t_field *tfield, string prefix, bool log_value_only, bool no_name_log, bool for_sandesh = false);
  void generate_logger_struct        (std::ofstream& out, t_struct *tstruct, string prefix, string name);
  void generate_logger_container     (std::ofstream& out, t_type* ttype, string name, bool log_value_only);
  void generate_logger_map_element   (std::ofstream& out, t_map* tmap, string iter, bool log_value_only);
  void generate_logger_set_element   (std::ofstream& out, t_set* tset, string iter, bool log_value_only);
  void generate_logger_list_element   (std::ofstream& out, t_list* tlist, string iter, bool log_value_only);
  void generate_sandesh_get_size     (std::ofstream& out, t_sandesh* tsandesh);
  void generate_get_size_field       (std::ofstream& out, t_field *tfield);
  void generate_get_size_struct      (std::ofstream& out, t_struct *tstruct, string name);
  void generate_get_size_container   (std::ofstream& out, t_type* ttype, string name);
  void generate_get_size_map_element (std::ofstream& out, t_map* tmap, string name);
  void generate_get_size_list_element(std::ofstream& out, t_list* ttype, string name);
  void generate_get_size_set_element (std::ofstream& out, t_set* ttype, string name);
  void generate_sandesh_trace        (std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_context      (std::ofstream& out, t_sandesh* tsandesh, string val);
  void generate_sandesh_seqnum(std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_versionsig(std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_static_seqnum_def(std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_static_versionsig_def(std::ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_trace_seqnum_ctor(std::ofstream& out, t_sandesh* tsandesh);
  void generate_static_const_string_definition(std::ofstream& out, t_sandesh* tsandesh);
  std::string generate_sandesh_no_static_const_string_function(t_sandesh *tsandesh, bool signature, bool autogen_darg, bool trace = false, bool request = false, bool ctorcall = false);
  void generate_static_const_string_definition(std::ofstream& out, std::string name,
                                               const vector<t_field*>& fields);
  std::string generate_sandesh_async_creator(t_sandesh *tsandesh, bool signature, bool expand_autogen, bool skip_autogen,
                                             std::string prefix, std::string suffix, bool category_level_file_line_first,
                                             bool autogen_category_level, bool drop_log_reason, bool use_sandesh_object = false);
  void generate_sandesh_rate_limit_fn(ofstream &out, t_sandesh *tsandesh);
  void generate_sandesh_async_send_fn(ofstream &out, t_sandesh *tsandesh,
      bool generate_sandesh_object, bool generate_rate_limit, bool generate_system_log);
  void generate_sandesh_async_send_macros(ofstream &out, t_sandesh *tsandesh,
      bool generate_sandesh_object);
  void generate_sandesh_static_log_fn(ofstream &out, t_sandesh *tsandesh,
      bool generate_sandesh_object);
  void generate_sandesh_async_create_fn(ofstream &out, t_sandesh *tsandesh);
  void generate_sandesh_async_create_macro(ofstream &out, t_sandesh *tsandesh);
  void generate_sandesh_session_log_unrolled_fn(ofstream &out, t_sandesh *tsandesh);
  void generate_sandesh_session_adjust_session_end_point_objects_fn(ofstream &out,
                                                      t_sandesh *tsandesh);
  void generate_sandesh_flow_send_fn(ofstream &out, t_sandesh *tsandesh);
  void generate_sandesh_systemlog_creators(ofstream &out, t_sandesh *tsandesh);
  void generate_sandesh_objectlog_creators(ofstream &out, t_sandesh *tsandesh);
  void generate_sandesh_flow_creators(ofstream &out, t_sandesh *tsandesh);
  void generate_sandesh_uve_creator(std::ofstream& out, t_sandesh* tsandesh);
  std::string generate_sandesh_trace_creator(t_sandesh *tsandesh, bool signature, bool expand_autogen, bool skip_autogen,
                                             std::string prefix, std::string suffix);
  void generate_sandesh_updater(ofstream& out, t_sandesh* tsandesh);
  void generate_isRatelimitPass(ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_static_rate_limit_log_def(ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_static_rate_limit_mutex_def(ofstream& out, t_sandesh* tsandesh);
  void generate_sandesh_static_rate_limit_buffer_def(ofstream& out, t_sandesh* tsandesh);

  typedef enum {
      MANDATORY = 0,
      INLINE = 1,
      HIDDEN = 2,
      PERIODIC = 3,
      HIDDEN_PER = 4,
      CA_MAX = 5
  } CacheAttribute;

  typedef enum {
      RM_DIAL = 0,
      RM_AGG = 1,
      RM_DIFF = 2,
      RM_MAX = 3
  } RawMetric;

  struct DSInfo {
    DSInfo(bool is_map, size_t period, CacheAttribute cat,
          string rawtype, RawMetric rmtype,
          string resulttype, string algo, string annotation,
          string compattr, string subcompattr, string prealgo) :
        is_map_(is_map), period_(period), cat_(cat),
        rawtype_(rawtype), rmtype_(rmtype),
        resulttype_(resulttype), algo_(algo),
        annotation_(annotation),
        compattr_(compattr) , subcompattr_(subcompattr), prealgo_(prealgo) {}
    bool is_map_;
    size_t period_;
    CacheAttribute cat_;
    string rawtype_;
    RawMetric rmtype_;
    string resulttype_;
    string algo_;
    string annotation_;
    string compattr_;
    string subcompattr_;
    string prealgo_;
  };
  void derived_stats_info(t_struct* tstruct,
    map<string,DSInfo>& dsmap,
    // val is set of ds attributes
    map<string,set<string> >& rawmap);

  void cache_attr_info(t_struct* tstruct, std::map<string, CacheAttribute>& attrs);

  void freq_info(t_struct* tstruct,
    set<string>& inl, set<string>& never, set<string>& periodic);
#endif

  void generate_cpp_struct(t_struct* tstruct, bool is_exception);

  void generate_service(t_service* tservice);

  void print_const_value(std::ofstream& out, std::string name, t_type* type, t_const_value* value);
  std::string render_const_value(std::ofstream& out, std::string name, t_type* type, t_const_value* value);

  void generate_struct_definition    (std::ofstream& out, t_struct* tstruct, bool is_exception=false, bool pointers=false, bool read=true, bool write=true);
  void generate_struct_fingerprint   (std::ofstream& out, t_struct* tstruct, bool is_definition);
  void generate_struct_reader        (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_writer        (std::ofstream& out, t_struct* tstruct, bool pointers=false);
  void generate_struct_result_writer (std::ofstream& out, t_struct* tstruct, bool pointers=false);
#ifdef SANDESH
  void generate_static_const_string_definition(std::ofstream& out, t_struct* tstruct);
  void generate_struct_logger        (ofstream& out, const string& name,
                                             const vector<t_field*>& fields);
  void generate_struct_get_size      (ofstream& out, const string& name,
                                             const vector<t_field*>& fields);
#endif

  /**
   * Service-level generation functions
   */

  void generate_service_interface (t_service* tservice, string style);
  void generate_service_interface_factory (t_service* tservice, string style);
  void generate_service_null      (t_service* tservice, string style);
  void generate_service_multiface (t_service* tservice);
  void generate_service_helpers   (t_service* tservice);
  void generate_service_client    (t_service* tservice, string style);
  void generate_service_processor (t_service* tservice, string style);
  void generate_service_skeleton  (t_service* tservice);
  void generate_process_function  (t_service* tservice, t_function* tfunction,
                                   string style, bool specialized=false);
  void generate_function_helpers  (t_service* tservice, t_function* tfunction);
  void generate_service_async_skeleton (t_service* tservice);

  /**
   * Serialization constructs
   */

  void generate_deserialize_field        (std::ofstream& out,
                                          t_field*    tfield,
                                          std::string prefix="",
                                          std::string suffix="");

  void generate_deserialize_struct       (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_deserialize_container    (std::ofstream& out,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_deserialize_set_element  (std::ofstream& out,
                                          t_set*      tset,
                                          std::string prefix="");

  void generate_deserialize_map_element  (std::ofstream& out,
                                          t_map*      tmap,
                                          std::string prefix="");

  void generate_deserialize_list_element (std::ofstream& out,
                                          t_list*     tlist,
                                          std::string prefix,
                                          bool push_back,
                                          std::string index);

  void generate_serialize_field          (std::ofstream& out,
                                          t_field*    tfield,
                                          std::string prefix="",
                                          std::string suffix="");

  void generate_serialize_struct         (std::ofstream& out,
                                          t_struct*   tstruct,
                                          std::string prefix="");

  void generate_serialize_container      (std::ofstream& out,
                                          t_type*     ttype,
                                          std::string prefix="");

  void generate_serialize_map_element    (std::ofstream& out,
                                          t_map*      tmap,
                                          std::string iter);

  void generate_serialize_set_element    (std::ofstream& out,
                                          t_set*      tmap,
                                          std::string iter);

  void generate_serialize_list_element   (std::ofstream& out,
                                          t_list*     tlist,
                                          std::string iter);

  void generate_function_call            (ostream& out,
                                          t_function* tfunction,
                                          string target,
                                          string iface,
                                          string arg_prefix);

#ifdef SANDESH
  void generate_serialize_sandesh        (std::ofstream& out,
                                          t_sandesh*   tsandesh,
                                          std::string prefix="");
#endif

  /*
   * Helper rendering functions
   */

  std::string namespace_prefix(std::string ns);
  std::string namespace_open(std::string ns);
  std::string namespace_close(std::string ns);
  std::string type_name(t_type* ttype, bool in_typedef=false, bool arg=false);
  std::string base_type_name(t_base_type::t_base tbase);
#ifdef SANDESH
  std::string declare_field(t_field* tfield, bool init=false, bool pointer=false, bool constant=false, bool reference=false, bool constructor=false);
#else
  std::string declare_field(t_field* tfield, bool init=false, bool pointer=false, bool constant=false, bool reference=false);
#endif
  std::string function_signature(t_function* tfunction, std::string style, std::string prefix="", bool name_params=true);
  std::string cob_function_signature(t_function* tfunction, std::string prefix="", bool name_params=true);
  std::string argument_list(t_struct* tstruct, bool name_params=true, bool start_comma=false);
  std::string type_to_enum(t_type* ttype);
  std::string local_reflection_name(const char*, t_type* ttype, bool external=false);

  void generate_enum_constant_list(std::ofstream& f,
                                   const vector<t_enum_value*>& constants,
                                   const char* prefix,
                                   const char* suffix,
                                   bool include_values);

  // These handles checking gen_dense_ and checking for duplicates.
  void generate_local_reflection(std::ofstream& out, t_type* ttype, bool is_definition);
  void generate_local_reflection_pointer(std::ofstream& out, t_type* ttype);

  bool is_complex_type(t_type* ttype) {
    ttype = get_true_type(ttype);

    return
      ttype->is_container() ||
      ttype->is_struct() ||
      ttype->is_xception() ||
#ifdef SANDESH
      ttype->is_sandesh() ||
      (ttype->is_base_type() && (((t_base_type *)ttype)->get_base() == t_base_type::TYPE_XML)) ||
#endif
      (ttype->is_base_type() && (((t_base_type*)ttype)->get_base() == t_base_type::TYPE_STRING));
  }

  void set_use_include_prefix(bool use_include_prefix) {
    use_include_prefix_ = use_include_prefix;
  }

 private:
  /**
   * Returns the include prefix to use for a file generated by program, or the
   * empty string if no include prefix should be used.
   */
  std::string get_include_prefix(const t_program& program) const;

  /**
   * Makes a helper function to gen a generic body of
   * sandesh and struct readers.
   */
  void generate_common_struct_reader_body(std::ofstream& out,
                                          t_struct_common* tstruct,
                                          bool pointers = false);

  /**
   * Makes a helper function to gen a generic body of
   * sandesh and struct writers.
   */
  void generate_common_struct_writer_body(std::ofstream& out,
                                          t_struct_common* tstruct,
                                          bool pointers = false);

  /**
   * True if we should generate pure enums for Thrift enums, instead of wrapper classes.
   */
  bool gen_pure_enums_;

  /**
   * True if we should generate local reflection metadata for TDenseProtocol.
   */
  bool gen_dense_;

  /**
   * True if we should generate templatized reader/writer methods.
   */
  bool gen_templates_;

  /**
   * True if we should use a path prefix in our #include statements for other
   * thrift-generated header files.
   */
  bool use_include_prefix_;

  /**
   * True if we should generate "Continuation OBject"-style classes as well.
   */
  bool gen_cob_style_;

  /**
   * True if we should omit calls to completion__() in CobClient class.
   */
  bool gen_no_client_completion_;

  /**
   * Strings for namespace, computed once up front then used directly
   */

  std::string ns_open_;
  std::string ns_close_;

  /**
   * File streams, stored here to avoid passing them as parameters to every
   * function.
   */

  std::ofstream f_types_;
  std::ofstream f_types_impl_;
  std::ofstream f_types_tcc_;
  std::ofstream f_header_;
  std::ofstream f_service_;
  std::ofstream f_service_tcc_;
#ifdef SANDESH
  std::ofstream f_request_impl_;
  std::ofstream f_html_template_;
  struct sandesh_logger {
    enum type {
      BUFFER,
      LOG,
      FORCED_LOG,
    };
  };
  void generate_sandesh_logger(std::ofstream& out, t_sandesh* tsandesh,
    sandesh_logger::type ltype);
  void generate_sandesh_static_logger(ofstream &out, t_sandesh *tsandesh,
      bool generate_sandesh_object);
  void generate_sandesh_static_drop_logger(ofstream &out, t_sandesh *tsandesh,
      bool generate_sandesh_object);
#endif

  /**
   * When generating local reflections, make sure we don't generate duplicates.
   */
  std::set<std::string> reflected_fingerprints_;

  // The ProcessorGenerator is used to generate parts of the code,
  // so it needs access to many of our protected members and methods.
  //
  // TODO: The code really should be cleaned up so that helper methods for
  // writing to the output files are separate from the generator classes
  // themselves.
  friend class ProcessorGenerator;
};

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 */
void t_cpp_generator::init_generator() {
  // Make output directory
  MKDIR(get_out_dir().c_str());

  // Make output file
  string f_types_name = get_out_dir()+program_name_+"_types.h";
  f_types_.open(f_types_name.c_str());

  string f_types_impl_name = get_out_dir()+program_name_+"_types.cpp";
  f_types_impl_.open(f_types_impl_name.c_str());

#ifdef SANDESH
  string f_request_impl_name = get_out_dir()+program_name_+"_request_skeleton.cpp";
  f_request_impl_.open(f_request_impl_name.c_str());

  string f_html_template_name = get_out_dir()+program_name_+"_html_template.cpp";
  f_html_template_.open(f_html_template_name.c_str());
#endif

  if (gen_templates_) {
    // If we don't open the stream, it appears to just discard data,
    // which is fine.
    string f_types_tcc_name = get_out_dir()+program_name_+"_types.tcc";
    f_types_tcc_.open(f_types_tcc_name.c_str());
  }

  // Print header
  f_types_ <<
    autogen_comment();
  f_types_impl_ <<
    autogen_comment();
  f_types_tcc_ <<
    autogen_comment();
#ifdef SANDESH
  f_request_impl_ <<
    autogen_comment();
  f_html_template_ <<
    autogen_comment();
#endif

  // Start ifndef
  f_types_ <<
    "#ifndef " << program_name_ << "_TYPES_H" << endl <<
    "#define " << program_name_ << "_TYPES_H" << endl <<
    endl;
  f_types_tcc_ <<
    "#ifndef " << program_name_ << "_TYPES_TCC" << endl <<
    "#define " << program_name_ << "_TYPES_TCC" << endl <<
    endl;

  // Include base types
  f_types_ <<
    "#include <sandesh/Thrift.h>" << endl <<
#ifndef SANDESH
    "#include <TApplicationException.h>" << endl <<
#else
    "#include <tbb/atomic.h>" << endl <<
    "#include <boost/shared_ptr.hpp>" << endl <<
    "#include <sandesh/derived_stats.h>" << endl <<
    "#include <sandesh/derived_stats_algo.h>" << endl <<
    "#include <boost/pointer_cast.hpp>" << endl <<
#endif
    "#include <base/trace.h>" << endl;
  if (program_name_ != "sandesh") {
    f_types_ << "#include <sandesh/sandesh_types.h>" << endl;
    f_types_ << "#include <sandesh/sandesh_constants.h>" << endl;
    if (program_name_ != "derived_stats_results" && program_name_ != "vns") {
      f_types_ << "#include <sandesh/sandesh.h>" << endl;
    }
  }
  f_types_ <<
    "#include <sandesh/protocol/TProtocol.h>" << endl <<
    "#include <sandesh/transport/TTransport.h>" << endl <<
    endl;

  // Include other Thrift includes
  const vector<t_program*>& includes = program_->get_includes();
  for (size_t i = 0; i < includes.size(); ++i) {
    f_types_ <<
      "#include \"" << get_include_prefix(*(includes[i])) <<
      includes[i]->get_name() << "_types.h\"" << endl;

    // XXX(simpkins): If gen_templates_ is enabled, we currently assume all
    // included files were also generated with templates enabled.
    f_types_tcc_ <<
      "#include \"" << get_include_prefix(*(includes[i])) <<
      includes[i]->get_name() << "_types.tcc\"" << endl;
  }
  f_types_ << endl;

  // Include custom headers
  const vector<string>& cpp_includes = program_->get_cpp_includes();
  for (size_t i = 0; i < cpp_includes.size(); ++i) {
    if (cpp_includes[i][0] == '<') {
      f_types_ <<
        "#include " << cpp_includes[i] << endl;
    } else {
      f_types_ <<
        "#include \"" << cpp_includes[i] << "\"" << endl;
    }
  }
  f_types_ <<
    endl;

  // Include the types file
#ifdef SANDESH
  f_types_impl_ <<
    "#include <boost/date_time/posix_time/posix_time.hpp>" << endl << endl <<
    "#include <base/logging.h>" << endl << endl <<
    "#include <sandesh/sandesh_uve.h>" << endl <<
    "#include <sandesh/sandesh_http.h>" << endl <<
    "#include <sandesh/sandesh_trace.h>" << endl <<
    "#include <curl/curl.h>" << endl <<
    "#include <boost/foreach.hpp>" << endl <<
    "#include <boost/assign/list_of.hpp>" << endl <<
    "#include <boost/make_shared.hpp>" << endl << endl;
#endif
  f_types_impl_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl <<
    endl;
  f_types_tcc_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl <<
    endl;
#ifdef SANDESH
  f_request_impl_ <<
    "#include <sandesh/sandesh_types.h>" << endl <<
    "#include <sandesh/sandesh_constants.h>" << endl <<
    "#include <sandesh/sandesh.h>" << endl <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl <<
    endl;
  f_html_template_ <<
    "#include <sandesh/sandesh_http.h>" << endl;
  f_html_template_ <<
    "static SandeshHttp::HtmlInfo h_info(" <<
    program_name_ << "_xml," << program_name_ << "_xml_len);" << endl;
  f_html_template_ <<
    "static SandeshHttp sh(\"" << program_name_ << ".xml\", h_info);" << endl;
  f_html_template_ << "int " << program_name_ << "_marker = 0;" << endl;

#endif


  // If we are generating local reflection metadata, we need to include
  // the definition of TypeSpec.
  if (gen_dense_) {
    f_types_impl_ <<
      "#include <TReflectionLocal.h>" << endl <<
      endl;
  }

  // Open namespace
  ns_open_ = namespace_open(program_->get_namespace("cpp"));
  ns_close_ = namespace_close(program_->get_namespace("cpp"));

  f_types_ <<
    ns_open_ << endl <<
    endl;

  f_types_impl_ <<
    ns_open_ << endl <<
    endl;

  f_types_tcc_ <<
    ns_open_ << endl <<
    endl;

#ifdef SANDESH
  f_request_impl_ <<
    ns_open_ << endl <<
    endl;
#endif
}

/**
 * Closes the output files.
 */
void t_cpp_generator::close_generator() {
  // Close namespace
  f_types_ <<
    ns_close_ << endl <<
    endl;
  f_types_impl_ <<
    ns_close_ << endl;
  f_types_tcc_ <<
    ns_close_ << endl <<
    endl;
#ifdef SANDESH
  f_request_impl_ <<
    ns_close_ << endl;
#endif

  // Include the types.tcc file from the types header file,
  // so clients don't have to explicitly include the tcc file.
  // TODO(simpkins): Make this a separate option.
  if (gen_templates_) {
    f_types_ <<
      "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
      "_types.tcc\"" << endl <<
      endl;
  }

  // Close ifndef
  f_types_ <<
    "#endif" << endl;
  f_types_tcc_ <<
    "#endif" << endl;

  // Close output file
  f_types_.close();
  f_types_impl_.close();
  f_types_tcc_.close();
#ifdef SANDESH
  f_request_impl_.close();
  f_html_template_.close();
#endif
}

/**
 * Generates a typedef. This is just a simple 1-liner in C++
 *
 * @param ttypedef The type definition
 */
void t_cpp_generator::generate_typedef(t_typedef* ttypedef) {
  f_types_ <<
    indent() << "typedef " << type_name(ttypedef->get_type(), true) << " " << ttypedef->get_symbolic() << ";" << endl <<
    endl;
}


void t_cpp_generator::generate_enum_constant_list(std::ofstream& f,
                                                  const vector<t_enum_value*>& constants,
                                                  const char* prefix,
                                                  const char* suffix,
                                                  bool include_values) {
  f << " {" << endl;
  indent_up();

  vector<t_enum_value*>::const_iterator c_iter;
  bool first = true;
  for (c_iter = constants.begin(); c_iter != constants.end(); ++c_iter) {
    if (first) {
      first = false;
    } else {
      f << "," << endl;
    }
    indent(f)
      << prefix << (*c_iter)->get_name() << suffix;
    if (include_values && (*c_iter)->has_value()) {
      f << " = " << (*c_iter)->get_value();
    }
  }

  f << endl;
  indent_down();
  indent(f) << "};" << endl;
}

/**
 * Generates code for an enumerated type. In C++, this is essentially the same
 * as the thrift definition itself, using the enum keyword in C++.
 *
 * @param tenum The enumeration
 */
void t_cpp_generator::generate_enum(t_enum* tenum) {
  vector<t_enum_value*> constants = tenum->get_constants();

  std::string enum_name = tenum->get_name();
  if (!gen_pure_enums_) {
    enum_name = "type";
    f_types_ <<
      indent() << "struct " << tenum->get_name() << " {" << endl;
    indent_up();
  }
  f_types_ <<
    indent() << "enum " << enum_name;

  generate_enum_constant_list(f_types_, constants, "", "", true);

  if (!gen_pure_enums_) {
    indent_down();
    f_types_ << "};" << endl;
  }

  f_types_ << endl;

  /**
     Generate a character array of enum names for debugging purposes.
  */
  std::string prefix = "";
  if (!gen_pure_enums_) {
    prefix = tenum->get_name() + "::";
  }

  f_types_impl_ <<
    indent() << "int _k" << tenum->get_name() << "Values[] =";
  generate_enum_constant_list(f_types_impl_, constants, prefix.c_str(), "", false);

  f_types_impl_ <<
    indent() << "const char* _k" << tenum->get_name() << "Names[] =";
  generate_enum_constant_list(f_types_impl_, constants, "\"", "\"", false);

  f_types_ <<
    indent() << "extern const std::map<int, const char*> _" <<
    tenum->get_name() << "_VALUES_TO_NAMES;" << endl << endl;

  f_types_impl_ <<
    indent() << "const std::map<int, const char*> _" << tenum->get_name() <<
    "_VALUES_TO_NAMES(::contrail::sandesh::TEnumIterator(" << constants.size() <<
    ", _k" << tenum->get_name() << "Values" <<
    ", _k" << tenum->get_name() << "Names), " <<
    "::contrail::sandesh::TEnumIterator(-1, NULL, NULL));" << endl << endl;

  generate_local_reflection(f_types_, tenum, false);
  generate_local_reflection(f_types_impl_, tenum, true);
}

/**
 * Generates a class that holds all the constants.
 */
void t_cpp_generator::generate_consts(std::vector<t_const*> consts) {
  string f_consts_name = get_out_dir()+program_name_+"_constants.h";
  ofstream f_consts;
  f_consts.open(f_consts_name.c_str());

  string f_consts_impl_name = get_out_dir()+program_name_+"_constants.cpp";
  ofstream f_consts_impl;
  f_consts_impl.open(f_consts_impl_name.c_str());

  // Print header
  f_consts <<
    autogen_comment();
  f_consts_impl <<
    autogen_comment();

  // Start ifndef
  f_consts <<
    "#ifndef " << program_name_ << "_CONSTANTS_H" << endl <<
    "#define " << program_name_ << "_CONSTANTS_H" << endl <<
    endl <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl <<
    endl <<
    ns_open_ << endl <<
    endl;

#ifdef SANDESH
  f_consts_impl <<
    "#include <base/trace.h>" << endl <<
    "#include <sandesh/sandesh_types.h>" << endl <<
    "#include <sandesh/sandesh_constants.h>" << endl <<
    "#include <sandesh/sandesh.h>" << endl <<
    "#include <sandesh/sandesh_trace.h>" << endl << endl;
#endif
  f_consts_impl <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_constants.h\"" << endl <<
    endl <<
    ns_open_ << endl <<
    endl;

  f_consts <<
    "class " << program_name_ << "Constants {" << endl <<
    " public:" << endl <<
    "  " << program_name_ << "Constants();" << endl <<
    endl;
  indent_up();
  vector<t_const*>::iterator c_iter;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    string name = (*c_iter)->get_name();
    t_type* type = (*c_iter)->get_type();
    f_consts <<
      indent() << type_name(type) << " " << name << ";" << endl;
  }
  indent_down();
  f_consts <<
    "};" << endl;

  f_consts_impl <<
    "const " << program_name_ << "Constants g_" << program_name_ << "_constants;" << endl <<
    endl <<
    program_name_ << "Constants::" << program_name_ << "Constants() {" << endl;
  indent_up();
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    print_const_value(f_consts_impl,
                      (*c_iter)->get_name(),
                      (*c_iter)->get_type(),
                      (*c_iter)->get_value());
  }
  indent_down();
  indent(f_consts_impl) <<
    "}" << endl;

  f_consts <<
    endl <<
    "extern const " << program_name_ << "Constants g_" << program_name_ << "_constants;" << endl <<
    endl <<
    ns_close_ << endl <<
    endl <<
    "#endif" << endl;
  f_consts.close();

  f_consts_impl <<
    endl <<
    ns_close_ << endl <<
    endl;
}

/**
 * Prints the value of a constant with the given type. Note that type checking
 * is NOT performed in this function as it is always run beforehand using the
 * validate_types method in main.cc
 */
void t_cpp_generator::print_const_value(ofstream& out, string name, t_type* type, t_const_value* value) {
  type = get_true_type(type);
  if (type->is_base_type()) {
    string v2 = render_const_value(out, name, type, value);
    indent(out) << name << " = " << v2 << ";" << endl <<
      endl;
  } else if (type->is_enum()) {
    indent(out) << name << " = (" << type_name(type) << ")" << value->get_integer() << ";" << endl <<
      endl;
  } else if (type->is_struct() || type->is_xception()) {
    const vector<t_field*>& fields = ((t_struct*)type)->get_members();
    vector<t_field*>::const_iterator f_iter;
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      t_type* field_type = NULL;
      for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if ((*f_iter)->get_name() == v_iter->first->get_string()) {
          field_type = (*f_iter)->get_type();
        }
      }
      if (field_type == NULL) {
        throw "type error: " + type->get_name() + " has no field " + v_iter->first->get_string();
      }
      string val = render_const_value(out, name, field_type, v_iter->second);
      indent(out) << name << "." << v_iter->first->get_string() << " = " << val << ";" << endl;
      indent(out) << name << ".__isset." << v_iter->first->get_string() << " = true;" << endl;
    }
    out << endl;
  } else if (type->is_map()) {
    t_type* ktype = ((t_map*)type)->get_key_type();
    t_type* vtype = ((t_map*)type)->get_val_type();
    const map<t_const_value*, t_const_value*>& val = value->get_map();
    map<t_const_value*, t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string key = render_const_value(out, name, ktype, v_iter->first);
      string val = render_const_value(out, name, vtype, v_iter->second);
      indent(out) << name << ".insert(std::make_pair(" << key << ", " << val << "));" << endl;
    }
    out << endl;
  } else if (type->is_list()) {
    t_type* etype = ((t_list*)type)->get_elem_type();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string val = render_const_value(out, name, etype, *v_iter);
      indent(out) << name << ".push_back(" << val << ");" << endl;
    }
    out << endl;
  } else if (type->is_set()) {
    t_type* etype = ((t_set*)type)->get_elem_type();
    const vector<t_const_value*>& val = value->get_list();
    vector<t_const_value*>::const_iterator v_iter;
    for (v_iter = val.begin(); v_iter != val.end(); ++v_iter) {
      string val = render_const_value(out, name, etype, *v_iter);
      indent(out) << name << ".insert(" << val << ");" << endl;
    }
    out << endl;
  } else {
    throw "INVALID TYPE IN print_const_value: " + type->get_name();
  }
}

/**
 *
 */
string t_cpp_generator::render_const_value(ofstream& out, string name, t_type* type, t_const_value* value) {
  (void) name;
  std::ostringstream render;

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_STRING:
#ifdef SANDESH
    case t_base_type::TYPE_STATIC_CONST_STRING:
    case t_base_type::TYPE_XML:
#endif
      render << '"' << get_escaped_string(value) << '"';
      break;
    case t_base_type::TYPE_BOOL:
      render << ((value->get_integer() > 0) ? "true" : "false");
      break;
    case t_base_type::TYPE_BYTE:
    case t_base_type::TYPE_I16:
    case t_base_type::TYPE_I32:
#ifdef SANDESH
    case t_base_type::TYPE_U16:
    case t_base_type::TYPE_U32:
#endif
      render << value->get_integer();
      break;
    case t_base_type::TYPE_I64:
      render << value->get_integer() << "LL";
      break;
#ifdef SANDESH
    case t_base_type::TYPE_U64:
      render << value->get_integer() << "ULL";
      break;
    case t_base_type::TYPE_UUID:
      render << "boost::uuids::string_generator()(\"" << value->get_uuid() << "\")";
      break;
#endif
    case t_base_type::TYPE_DOUBLE:
      if (value->get_type() == t_const_value::CV_INTEGER) {
        render << value->get_integer();
      } else {
        render << value->get_double();
      }
      break;
#ifdef SANDESH
    case t_base_type::TYPE_IPV4:
      render << value->get_integer();
      break;
    case t_base_type::TYPE_IPADDR:
      render << "boost::asio::ip::address::from_string(\"" <<
        value->get_string() << "\")";
      break;
#endif
    default:
      throw "compiler error: no const of base type " + t_base_type::t_base_name(tbase);
    }
  } else if (type->is_enum()) {
    render << "(" << type_name(type) << ")" << value->get_integer();
  } else {
    string t = tmp("tmp");
    indent(out) << type_name(type) << " " << t << ";" << endl;
    print_const_value(out, t, type, value);
    render << t;
  }

  return render.str();
}

/**
 * Generates a struct definition for a thrift data type. This is a class
 * with data members and a read/write() function, plus a mirroring isset
 * inner class.
 *
 * @param tstruct The struct definition
 */
void t_cpp_generator::generate_cpp_struct(t_struct* tstruct, bool is_exception) {
  generate_struct_definition(f_types_, tstruct, is_exception);
  generate_struct_fingerprint(f_types_impl_, tstruct, true);
  generate_local_reflection(f_types_, tstruct, false);
  generate_local_reflection(f_types_impl_, tstruct, true);
  generate_local_reflection_pointer(f_types_impl_, tstruct);
#ifdef SANDESH
  generate_static_const_string_definition(f_types_impl_, tstruct);
#endif

  std::ofstream& out = (gen_templates_ ? f_types_tcc_ : f_types_impl_);
  generate_struct_reader(out, tstruct);
  generate_struct_writer(out, tstruct);
#ifdef SANDESH
  generate_struct_logger(out, tstruct->get_name(), tstruct->get_members());
  generate_struct_get_size(out, tstruct->get_name(), tstruct->get_members());
#endif
}

#ifdef SANDESH
/**
 * Generates a sandesh definition for a thrift data type. This is a class
 * with data members and a read/write() function, plus a mirroring isset
 * inner class.
 *
 * @param tsandesh The sandesh definition
 */
void t_cpp_generator::generate_cpp_sandesh(t_sandesh* tsandesh) {
    bool is_request =
            ((t_base_type *)tsandesh->get_type())->is_sandesh_request();
    bool is_trace =
            ((t_base_type *)tsandesh->get_type())->is_sandesh_trace() ||
            ((t_base_type *)tsandesh->get_type())->is_sandesh_trace_object();
    bool is_uve =
            ((t_base_type *)tsandesh->get_type())->is_sandesh_uve();
    bool is_alarm =
            ((t_base_type *)tsandesh->get_type())->is_sandesh_alarm();
    bool is_system =
            ((t_base_type *)tsandesh->get_type())->is_sandesh_system();

    generate_sandesh_definition(f_types_, tsandesh);
    generate_sandesh_fingerprint(f_types_impl_, tsandesh, true);
    std::ofstream& out = f_types_impl_;
    generate_static_const_string_definition(out, tsandesh);
    generate_sandesh_static_versionsig_def(out, tsandesh);
    generate_sandesh_creator(out, tsandesh);
    generate_sandesh_reader(out, tsandesh);
    generate_sandesh_writer(out, tsandesh);
    generate_sandesh_loggers(out, tsandesh);
    generate_sandesh_get_size(out, tsandesh);

    if (!is_trace) {
        generate_sandesh_static_seqnum_def(out, tsandesh);
    }
    if (is_request) {
        generate_sandesh_http_reader(out, tsandesh);
        // Generate a skeleton impl file
        generate_sandesh_request(f_request_impl_, tsandesh);
    }
    if (is_uve || is_alarm) {
        generate_sandesh_updater(out,tsandesh);
    }
    if (is_system) {
        generate_sandesh_static_rate_limit_log_def(out, tsandesh);
        generate_sandesh_static_rate_limit_mutex_def(out, tsandesh);
        generate_sandesh_static_rate_limit_buffer_def(out, tsandesh);
    }

}

/**
 *  Generates a sandesh request handler skeleton code
 *
 *  @param out Output stream
 *  @param tsandesh The request sandesh
 */
void t_cpp_generator::generate_sandesh_request(ofstream& out,
        t_sandesh* tsandesh) {
    out << "void " << tsandesh->get_name() << "::" <<
            "HandleRequest() const {" << endl;
    indent_up();

    indent(out) << "// This autogenerated skeleton file illustrates the " <<
            "function needed to" << endl;
    indent(out) << "// implement " << tsandesh->get_name() << "." << endl;
    indent(out) << "// You should copy it to another filename to avoid " <<
            "overwriting it. Add the" << endl;
    indent(out) << "// HandleRequest() implementation to it, and include it " <<
             "in your module's" << endl;
    indent(out) << "// SConscript. Alternatively, add the implementation of " <<
             "HandleRequest()" << endl;
    indent(out) << "// in your module's sources." << endl;
    scope_down(out);
}

/**
 * Generates definition and invocation for functions that have all but TYPE_STATIC_CONST_STRING members
 *
 * @param out Output stream
 * @param tsandesh The sandesh
 */
std::string t_cpp_generator::generate_sandesh_no_static_const_string_function(
        t_sandesh* tsandesh, bool signature, bool autogen_darg,
        bool trace, bool request, bool ctorcall) {
    string result = "";

    // Get members
    vector<t_field*>::const_iterator m_iter;
    const vector<t_field*>& members = tsandesh->get_members();

    bool init_function = false;

    if (trace) {
        result += "(";
        init_function = true;
        if (signature) {
            result += "SandeshTraceBufferPtr trace_buf, std::string file, int32_t line";
        } else {
            result += "trace_buf, file, line";
        }
    } else {
        if (signature) {
            if (!ctorcall) {
              result += "(";
              init_function = true;
              result += "uint32_t seqno";
            }
        } else {
            result += "(";
            init_function = true;
            if (ctorcall)
              result += "0";
            else
              result += "lseqnum_++";
        }
    }

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        t_type* t = get_true_type((*m_iter)->get_type());
        if (t->is_static_const_string()) {
            continue;
        }
        if ((*m_iter)->get_auto_generated() && trace) {
            continue;
        }
        if (((t_base_type *)tsandesh->get_type())->is_sandesh_object() &&
            ((*m_iter)->get_req() == t_field::T_OPTIONAL)) {
            continue;
        }
        if (!init_function) {
            result += "(";
            init_function = true;
        } else {
            result += ", ";
        }

        if (signature) {
            // Special handling for auto-generated members
            if (autogen_darg && (*m_iter)->get_auto_generated()) {
                result += declare_field(*m_iter, true, false, false, false, true);
            } else {
                bool use_const = !(t->is_base_type() || t->is_enum()) || t->is_string();
                result += declare_field(*m_iter, false, false, use_const, !t->is_base_type() || t->is_string(), true);
            }
        } else {
            result += (*m_iter)->get_name();
        }
    }
    if (!init_function) {
        result += "(";
    }

    if (request) {
        if (signature) {
            if (init_function) {
                result += ", ";
            }
            result += "const std::string& context, SandeshConnection * sconn = NULL";
        } else {
            if (init_function) {
                result += ", ";
            }
            result += "context, sconn";
        }
    }

    result += ")";
    return result;
}

std::string t_cpp_generator::generate_sandesh_async_creator(t_sandesh* tsandesh, bool signature,
        bool expand_autogen, bool skip_autogen, std::string prefix, std::string suffix,
        bool category_level_file_line_first, bool autogen_category_level, bool
        drop_log_reason, bool use_sandesh_object)
        {
    string result = "";
    string temp = "";
    string category_def = prefix + "category" + suffix;
    string level_def = prefix + "level" + suffix;
    string category_dec = "std::string " + category_def;
    string level_dec = "SandeshLevel::type " + level_def;
    string module_name = "\"\"";
    string drop_reason_dec;
    string drop_reason_def;
    bool is_flow = ((t_base_type *)tsandesh->get_type())->is_sandesh_flow();
    // Get members
    vector<t_field*>::const_iterator m_iter;
    const vector<t_field*>& members = tsandesh->get_members();
    bool init_function = false;
    if (drop_log_reason) {
        drop_reason_def = prefix + "drop_reason" + suffix + ", ";
        drop_reason_dec = "const std::string& " + drop_reason_def;
    }

    if (category_level_file_line_first) {
        if (signature) {
            result += "(" + drop_reason_dec + category_dec + ", " + level_dec;
            if (!is_flow) result += ", std::string file, int32_t line";
        } else {
            if (!autogen_category_level) {
                result += "(" + drop_reason_dec + category_def + ", " + level_def;
                if (!is_flow) result += ", __FILE__, __LINE__";
            } else {
                result += "(" + drop_reason_dec + module_name + ", SandeshLevel::SYS_INFO";
                if (!is_flow) result += ", __FILE__, __LINE__";
            }
        }
        init_function = true;
    } else {
        if (signature) {
            if (!autogen_category_level) {
                result += "(" + drop_reason_dec + category_dec + ", " +  level_dec;
                init_function = true;
            }
        } else {
            if (!autogen_category_level) {
                result += "(" + drop_reason_def + category_def + ", " + level_def;
                init_function = true;
            }
        }
    }
    if(use_sandesh_object) {
        if (!init_function) {
            result += "(snh";
            init_function = true;
        } else {
            if(signature) {
                result += ", " + tsandesh->get_name() + " *snh";
            } else {
                result += ", snh";
            }
        }
    } else {
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        temp = "";
        t_type* t = get_true_type((*m_iter)->get_type());
        if (t->is_static_const_string()) {
            continue;
        }
        bool is_optional = (*m_iter)->get_req() == t_field::T_OPTIONAL;
        if(is_optional) {
            continue;
        }
        bool skip = (*m_iter)->get_auto_generated() &&
                    ((*m_iter)->get_name() == "file" || (*m_iter)->get_name() == "line") &&
                    category_level_file_line_first;
        if (!init_function) {
            result += "(";
            init_function = true;
        } else {
            if (!skip) {
                temp += ", ";
            }
        }
        if (signature) {
            result += temp;
            // Special signature processing for autogen members
            if ((*m_iter)->get_auto_generated()) {
                if (!skip) {
                    result += declare_field(*m_iter, true, false, false, !t->is_base_type(), true);
                }
            } else {
                bool use_const = !(t->is_base_type() || t->is_enum()) || t->is_string();
                result += declare_field(*m_iter, false, false, use_const,
                                        !t->is_base_type() || t->is_string(), true);
            }
        } else {
            if (skip) {
                continue;
            }
            // Special processing for autogen members
            if ((*m_iter)->get_auto_generated()) {
                // Skip
                if (skip_autogen) {
                    continue;
                }
                // Map to preprocessor macros
                if (expand_autogen) {
                    if ((*m_iter)->get_name() == "file") {
                        result += temp;
                        result += "__FILE__";
                    } else if ((*m_iter)->get_name() == "line") {
                        result += temp;
                        result += "__LINE__";
                    }
                } else {
                    result += temp;
                    result += prefix + (*m_iter)->get_name() + suffix;
                }
            } else {
                result += temp;
                result += prefix + (*m_iter)->get_name() + suffix;
            }
        }
    }
    }
    if (!init_function) {
        result += "(";
    }
    result += ")";
    return result;
}

void t_cpp_generator::generate_sandesh_async_create_fn(ofstream &out,
    t_sandesh *tsandesh) {
    std::string creator_func_name = "Create";
    out << indent() << "static " << tsandesh->get_name() << "* "
        << creator_func_name << "(std::string file = \"\", int32_t line = 0) {"
        << endl;
    indent_up();
    out << indent() << tsandesh->get_name() <<
        " * snh = new " << tsandesh->get_name() << "(lseqnum_++, file, line);"
        << endl;
    out << indent() << "return snh;" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;
}

void t_cpp_generator::generate_sandesh_async_create_macro(ofstream &out,
    t_sandesh *tsandesh) {
    std::string creator_func_name = "Create";
    // Generate creator macro
    string creator_name = tsandesh->get_name() + creator_func_name;
    string creator_name_usc = underscore(creator_name);
    string creator_name_uc = uppercase(creator_name_usc);
    out << indent() << "#define " << creator_name_uc <<
            "() \\" << endl;
    indent_up();
    out << indent() << tsandesh->get_name() << "::" << creator_func_name <<
            "(__FILE__, __LINE__)" << endl;
    indent_down();
    indent(out) << endl << endl;
}

void t_cpp_generator::generate_sandesh_async_send_fn(
    ofstream &out, t_sandesh *tsandesh, bool generate_sandesh_object,
    bool generate_rate_limit, bool generate_system_log) {
    std::string sender_func_name = "Send";
    // Generate sender
    out << indent() << "static void " << sender_func_name;
    out << generate_sandesh_async_creator(tsandesh, true, false, false, "", "",
        true, false, false, generate_sandesh_object);
    out << " {" << endl;
    indent_up();
    if (generate_sandesh_object) {
        out << indent() << "snh->set_level(level);" << endl;
        out << indent() << "snh->set_category(category);" << endl;
    }
    out << indent() << "if (HandleTest(level, category)) {" << endl;
    indent_up();
    out << indent() << "Log";
    if (generate_sandesh_object) {
        out << "(category, level, snh);" << endl;
        out << indent() << "snh->Release();" << endl;
    } else {
        out << generate_sandesh_async_creator(tsandesh, false, false, false, "",
                                   "", false, false, false) << "; " << endl;
    }
    out << indent() << "return;" << endl;
    scope_down(out);
    if (generate_system_log) {
        out << indent() << "if (IsSendingAllMessagesDisabled() || " <<
            "IsSendingSystemLogsDisabled()) {" << endl;
    } else {
        out << indent() << "if (IsSendingAllMessagesDisabled() || " <<
            "IsSendingObjectLogsDisabled()) {" << endl;
    }
    indent_up();
    out << indent() << "UpdateTxMsgFailStats(\"" << tsandesh->get_name() <<
        "\", 0, SandeshTxDropReason::SendingDisabled);" << endl;
    out << indent() << "Log";
    if (generate_sandesh_object) {
        out << "(category, level, snh);" << endl;
        out << indent() << "snh->Release();" << endl;
    } else {
        out << generate_sandesh_async_creator(tsandesh, false, false, false, "",
                                   "", false, false, false) << "; " << endl;
    }
    out << indent() << "return;" << endl;
    scope_down(out);
    if (generate_rate_limit) {
        out << indent() << "if (!IsRatelimitPass()) {" << endl;
        indent_up();
        out << indent() << "UpdateTxMsgFailStats(\"" << tsandesh->get_name() <<
            "\", 0, SandeshTxDropReason::RatelimitDrop);" << endl;
        out << indent() << "if (do_rate_limit_drop_log_) {" << endl;
        indent_up();
        out << indent() << "std::stringstream ratelimit_val;" << endl;
        out << indent() << " ratelimit_val << Sandesh::get_send_rate_limit();"
            << endl;
        out << indent() << "std::string drop_reason = \"SANDESH: Ratelimit"
            " Drop (\" + ratelimit_val.str() + std::string(\" messages"
            "/second): \") ;" << endl;
        out << indent() << "DropLog";
        if (generate_sandesh_object) {
            out << "(drop_reason, category, level, snh);" << endl;
            out << indent() << "snh->Release();" << endl;
        } else {
            out << generate_sandesh_async_creator(tsandesh, false, false,
                false, "", "", false, false, true) << "; " << endl;
        }
        out << indent() << "do_rate_limit_drop_log_ = false;" << endl;
        scope_down(out);
        out << indent() << "return;" << endl;
        scope_down(out);
    }
    out << indent() << "if (level >= SendingLevel()) {" << endl;
    indent_up();
    out << indent() << "UpdateTxMsgFailStats(\"" << tsandesh->get_name() <<
        "\", 0, SandeshTxDropReason::QueueLevel);" << endl;
    out << indent() << "std::string drop_reason = \"SANDESH: Queue Drop:"
        " \";" << endl;
    out << indent() << "DropLog";
    if (generate_sandesh_object) {
        out << "(drop_reason, category, level, snh);" << endl;
        out << indent() << "snh->Release();" << endl;
    } else {
        out << generate_sandesh_async_creator(tsandesh, false, false, false, "",
                                   "", false, false, true) << "; " << endl;
    }
    out << indent() << "return;" << endl;
    scope_down(out);
    if (!generate_sandesh_object) {
        out << indent() << tsandesh->get_name() <<
            " * snh = new " << tsandesh->get_name() <<
            generate_sandesh_no_static_const_string_function(tsandesh,
                        false, false, false, false) << ";" << endl;
        out << indent() << "snh->set_level(level);" << endl;
        out << indent() << "snh->set_category(category);" << endl;
    }
    out << indent() << "snh->Dispatch();" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;
}

void t_cpp_generator::generate_sandesh_async_send_macros(ofstream &out,
    t_sandesh *tsandesh, bool generate_sandesh_object) {
    std::string sender_func_name = "Send";
    std::string logger_func_name = "Log";
    // Generate creator macro
    string creator_name = tsandesh->get_name() + logger_func_name;
    if (generate_sandesh_object) {
        creator_name += "Sandesh";
    }
    string creator_name_usc = underscore(creator_name);
    string creator_name_uc = uppercase(creator_name_usc);
    out << indent() << "#define " << creator_name_uc;
    out << generate_sandesh_async_creator(tsandesh, false, false, true, "_",
        "", false, false, false, generate_sandesh_object);
    out << "\\" << endl;
    indent_up();
    out << indent() << tsandesh->get_name() << "::" << sender_func_name;
    out << generate_sandesh_async_creator(tsandesh, false, true, false,
        "(_", ")", true, false, false, generate_sandesh_object) << endl;
    indent_down();
    indent(out) << endl << endl;

    // Generate creator macro (legacy)
    creator_name = tsandesh->get_name() + sender_func_name;
    if (generate_sandesh_object) {
        creator_name += "Sandesh";
    }
    creator_name_usc = underscore(creator_name);
    creator_name_uc = uppercase(creator_name_usc);
    out << indent() << "#define " << creator_name_uc;
    out << generate_sandesh_async_creator(tsandesh, false, false, true, "_",
        "", false, true, false, generate_sandesh_object);
    out << "\\" << endl;
    indent_up();
    out << indent() << tsandesh->get_name() << "::" << sender_func_name;
    out << generate_sandesh_async_creator(tsandesh, false, true, false,
        "(_", ")", true, true, false, generate_sandesh_object) << endl;
    indent_down();
    indent(out) << endl << endl;
}

void t_cpp_generator::generate_sandesh_rate_limit_fn(ofstream &out,
    t_sandesh *tsandesh) {
    out << indent() << "static bool IsRatelimitPass() {" << endl;
    indent_up();
    generate_isRatelimitPass(out, tsandesh);
    indent_down();
    indent(out) << "}" << endl << endl;
}

void t_cpp_generator::generate_sandesh_systemlog_creators(ofstream &out,
    t_sandesh *tsandesh) {
    bool generate_sandesh_object = false;
    bool generate_rate_limit = true;
    bool generate_system_log = true;
    // Generate send function and macros
    generate_sandesh_async_send_fn(out, tsandesh, generate_sandesh_object,
        generate_rate_limit, generate_system_log);
    generate_sandesh_async_send_macros(out, tsandesh, generate_sandesh_object);
    // Generate DropLog
    generate_sandesh_static_drop_logger(out, tsandesh,
        generate_sandesh_object);
    // Generate Log
    generate_sandesh_static_logger(out, tsandesh, generate_sandesh_object);
    // Generate rate limit
    generate_sandesh_rate_limit_fn(out, tsandesh);
}

void t_cpp_generator::generate_sandesh_objectlog_creators(ofstream &out,
    t_sandesh *tsandesh) {
    // First generate without sandesh object semantics
    bool generate_sandesh_object = false;
    bool generate_rate_limit = false;
    bool generate_system_log = false;
    // Generate send function and macros
    generate_sandesh_async_send_fn(out, tsandesh, generate_sandesh_object,
        generate_rate_limit, generate_system_log);
    generate_sandesh_async_send_macros(out, tsandesh, generate_sandesh_object);
    // Generate DropLog
    generate_sandesh_static_drop_logger(out, tsandesh,
        generate_sandesh_object);
    // Generate Log
    generate_sandesh_static_logger(out, tsandesh, generate_sandesh_object);
    // Next generate with sandesh object semantics
    generate_sandesh_object = true;
    // Generate create function and macros
    generate_sandesh_async_create_fn(out, tsandesh);
    generate_sandesh_async_create_macro(out, tsandesh);
    // Generate send function and macros
    generate_sandesh_async_send_fn(out, tsandesh, generate_sandesh_object,
        generate_rate_limit, generate_system_log);
    generate_sandesh_async_send_macros(out, tsandesh, generate_sandesh_object);
    // Generate DropLog
    generate_sandesh_static_drop_logger(out, tsandesh,
        generate_sandesh_object);
    // Generate Log
    generate_sandesh_static_logger(out, tsandesh, generate_sandesh_object);
}

void t_cpp_generator::generate_sandesh_session_log_unrolled_fn(ofstream &out,
    t_sandesh *tsandesh) {
    //To be removed
    out << indent() << "static void LogUnrolled";
    out << generate_sandesh_async_creator(tsandesh, true, false, false, "", "",
        true, false, false, false);
    out << ";" << endl;
}

void t_cpp_generator::generate_sandesh_session_adjust_session_end_point_objects_fn(ofstream &out,
    t_sandesh *tsandesh) {
    out << "static void adjust_session_end_point_objects(std::vector"
           " <SessionEndpoint> & session_data);" << endl;
    out << endl;
}

void t_cpp_generator::generate_sandesh_flow_send_fn(ofstream &out,
    t_sandesh *tsandesh) {
    std::string sender_func_name = "Send";
    std::string logger_func_name = "Log";
    // Generate sender
    out << indent() << "static void " << sender_func_name;
    out << generate_sandesh_async_creator(tsandesh, true, false, false, "", "",
        true, false, false, false);
    out << " {" << endl;
    indent_up();
    out << indent() << "if (HandleTest(level, category)) {" << endl;
    indent_up();
    out << indent() << "return;" << endl;
    scope_down(out);
    out << indent() << "if (IsSendingAllMessagesDisabled() ||" <<
        " IsSendingFlowsDisabled()) {" << endl;
    indent_up();
    out << indent() << "UpdateTxMsgFailStats(\"" << tsandesh->get_name() <<
        "\", 0, SandeshTxDropReason::SendingDisabled);" << endl;
    out << indent() << "if (IsLoggingDroppedAllowed(SandeshType::FLOW))" <<
        " {" << endl;
    indent_up();
    out << indent() << "Log" <<
        generate_sandesh_async_creator(tsandesh, false,
            false, false, "", "", false, false, false) << ";" << endl;
    scope_down(out);
    out << indent() << "return;" << endl;
    scope_down(out);

    out << indent() <<
        "if (is_send_slo_to_logger_enabled() || is_send_sampled_to_logger_enabled()) { " <<
 endl;
    indent_up();
    const t_type *t = tsandesh->get_type();
    if (((t_base_type *)t)->is_sandesh_session()) {
        out << indent() << "LogUnrolled(category, level, session_data);" << endl;
    }
    scope_down(out);
    out << indent() << "if (level >= SendingLevel()) {" << endl;
    indent_up();
    out << indent() << "UpdateTxMsgFailStats(\"" << tsandesh->get_name() <<
        "\", 0, SandeshTxDropReason::QueueLevel);" << endl;
    out << indent() << "if (IsLoggingDroppedAllowed(SandeshType::FLOW))" <<
        " {" << endl;
    indent_up();
    out << indent() << "std::string drop_reason = \"SANDESH: Queue Drop:"
        " \";" << endl;
    out << indent() << "DropLog" <<
        generate_sandesh_async_creator(tsandesh, false,
            false, false, "", "", false, false, true) << ";" << endl;
    scope_down(out);
    out << indent() << "return;" << endl;
    scope_down(out);
    out << indent() << tsandesh->get_name() <<
            " * snh = new " << tsandesh->get_name() <<
            generate_sandesh_no_static_const_string_function(tsandesh,
                        false, false, false, false) << ";" << endl;
     if (((t_base_type *)t)->is_sandesh_session()) {
        out << indent() << "if (!is_send_sampled_to_collector_enabled() && !is_send_slo_to_collector_enabled()) {" << endl;
        indent_up();
        out << indent() << "return;" << endl;
        scope_down(out);
        out << indent() << " if (is_send_sampled_to_collector_enabled() != is_send_slo_to_collector_enabled()) {" << endl;
        indent_up();
        out << indent() << "adjust_session_end_point_objects(snh->session_data);";
        scope_down(out);
    }
    out << endl;
    out << indent() << "snh->set_level(level);" << endl;
    out << indent() << "snh->set_category(category);" << endl;
    out << indent() << "snh->Dispatch();" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;
}

void t_cpp_generator::generate_sandesh_flow_creators(ofstream &out,
    t_sandesh *tsandesh) {
    bool generate_sandesh_object = false;
    const t_type *t = tsandesh->get_type();
    // Generate LogUnrolled declaration only for Session message
    if (((t_base_type *)t)->is_sandesh_session()) {
        generate_sandesh_session_log_unrolled_fn(out, tsandesh);
        generate_sandesh_session_adjust_session_end_point_objects_fn(out, tsandesh);
    }
    // Generate send function and macros
    generate_sandesh_flow_send_fn(out, tsandesh);
    generate_sandesh_async_send_macros(out, tsandesh, generate_sandesh_object);
    // Generate DropLog
    generate_sandesh_static_drop_logger(out, tsandesh,
        generate_sandesh_object);
    // Generate Log
    generate_sandesh_static_logger(out, tsandesh, generate_sandesh_object);
}

std::string t_cpp_generator::generate_sandesh_trace_creator(t_sandesh *tsandesh,
        bool signature, bool expand_autogen, bool skip_autogen,
        std::string prefix, std::string suffix) {
    string result = "";
    string temp = "";
    // Get members
    vector<t_field*>::const_iterator m_iter;
    const vector<t_field*>& members = tsandesh->get_members();

    result += "(";
    if (signature) {
        result += "SandeshTraceBufferPtr tracebuf";
        if (!skip_autogen) {
            result += ", std::string file, int32_t line";
        }
    } else {
        result += prefix + "trace_buf" + suffix;
        if (!skip_autogen) {
            if (expand_autogen) {
                result += ", __FILE__, __LINE__";
            } else {
                result += ", " + prefix + "file" + suffix + ", " +
                        prefix + "line" + suffix;
            }
        }
    }

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        temp = "";
        t_type* t = get_true_type((*m_iter)->get_type());
        if (t->is_static_const_string()) {
            continue;
        }
        if ((*m_iter)->get_auto_generated()) {
            continue;
        }
        temp += ", ";
        if (signature) {
            result += temp;
            bool use_const = !(t->is_base_type() || t->is_enum()) || t->is_string();
            result += declare_field(*m_iter, false, false, use_const,
                                    !t->is_base_type() || t->is_string(), true);
        } else {
            result += temp;
            result += prefix + (*m_iter)->get_name() + suffix;
        }
    }
    result += ")";
    return result;
}

/**
 * Generate initialization list for members of sandesh
 *
 * @param out Output stream
 * @param tsandesh The sandesh
 * @param init_dval Initialize default values
 */
void t_cpp_generator::generate_sandesh_member_init_list(ofstream& out,
        t_sandesh *tsandesh, bool init_dval) {
    // Get members
    vector<t_field*>::const_iterator m_iter;
    const vector<t_field*>& members = tsandesh->get_members();

    out << " : ";
    generate_sandesh_base_init(out, tsandesh, init_dval);

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        t_type* t = get_true_type((*m_iter)->get_type());
        // If initializing with default values ignore complex types
        if (init_dval && (!t->is_base_type() && !t->is_enum())) {
            continue;
        }
        if (t->is_static_const_string()) {
            continue;
        }
        if (((t_base_type *)tsandesh->get_type())->is_sandesh_object() &&
            ((*m_iter)->get_req() == t_field::T_OPTIONAL)) {
            continue;
        }
        string dval;
        if (t->is_enum()) {
            dval += "(" + type_name(t) + ")";
        }
        if (t->is_string() || t->is_xml()) {
            dval += "\"\"";
        } else if (t->is_uuid()) {
            dval += "boost::uuids::nil_uuid()";
        } else if (t->is_ipaddr()) {
            dval += "";
        } else {
            dval += "0";
        }
        t_const_value* cv = (*m_iter)->get_value();
        if (cv != NULL) {
            dval = render_const_value(out, (*m_iter)->get_name(), t, cv);
        } else {
            if (!init_dval) {
                dval = (*m_iter)->get_name();
            }
        }
        out << ", " << (*m_iter)->get_name() << "(" << dval << ")";
    }

}

/**
 * Generate sandesh sequence number
 *
 * @param out Output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_seqnum(ofstream& out,
                                              t_sandesh* tsandesh) {
    if (((t_base_type *)tsandesh->get_type())->is_sandesh_uve() ||
        ((t_base_type *)tsandesh->get_type())->is_sandesh_alarm()) {
      indent(out) << "static tbb::atomic<uint32_t> lseqnum_;" << endl;
    } else {
      indent(out) << "static uint32_t lseqnum_;" << endl;
    }
}


string generate_sandesh_base_name(t_sandesh* tsandesh, bool type) {
    const t_type *t = tsandesh->get_type();
    if (((t_base_type *)t)->is_sandesh_request()) {
        if (type) {
            return "SandeshType::REQUEST";
        } else {
            return "SandeshRequest";
        }
    } else if (((t_base_type *)t)->is_sandesh_response()) {
        if (type) {
            return "SandeshType::RESPONSE";
        } else {
            return "SandeshResponse";
        }
    } else if (((t_base_type *)t)->is_sandesh_uve()) {
        if (type) {
            return "SandeshType::UVE";
        } else {
            return "SandeshUVE";
        }
    } else if (((t_base_type *)t)->is_sandesh_alarm()) {
        if (type) {
            return "SandeshType::ALARM";
        } else {
            return "SandeshAlarm";
        }
    } else if (((t_base_type *)t)->is_sandesh_system()) {
        if (type) {
            return "SandeshType::SYSTEM";
        } else {
            return "SandeshSystem";
        }
    } else if (((t_base_type *)t)->is_sandesh_buffer()) {
        if (type) {
            return "SandeshType::BUFFER";
        } else {
            return "SandeshBuffer";
        }
    } else if (((t_base_type *)t)->is_sandesh_trace() ||
            ((t_base_type *)t)->is_sandesh_trace_object()) {
        if (type) {
            return "SandeshType::TRACE";
        } else {
            return "SandeshTrace";
        }
    } else if (((t_base_type *)t)->is_sandesh_object()) {
        if (type) {
            return "SandeshType::OBJECT";
        } else {
            return "SandeshObject";
        }
    } else if (((t_base_type *)t)->is_sandesh_flow()) {
        if (((t_base_type *)t)->is_sandesh_session()) {
            if (type) {
                return "SandeshType::SESSION";
            } else {
                return "SandeshFlowSession";
            }
        }
        else {
            if (type) {
                return "SandeshType::FLOW";
            } else {
                return "SandeshFlow";
            }
        }
    } else {
        return "";
    }
}

/**
 * Generate sandesh version signature
 *
 * @param out Output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_versionsig(ofstream& out,
                                                  t_sandesh* tsandesh) {
    indent(out) << "static uint32_t versionsig_;" << endl;
}

void t_cpp_generator::generate_sandesh_base_init(
        ofstream& out, t_sandesh* tsandesh, bool init_dval) {
    out << generate_sandesh_base_name(tsandesh, false);

    if (init_dval) {
        out << "(\"" << tsandesh->get_name() << "\",lseqnum_++)";
    } else {
        out << "(\"" << tsandesh->get_name() << "\",seqno)";
    }
}

void t_cpp_generator::generate_sandesh_trace_seqnum_ctor(ofstream& out,
                                                         t_sandesh* tsandesh) {
    indent(out) << "set_seqnum(0);" << endl;
}

/**
 * Generate sandesh context assignment code
 *
 * @param out Output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_context(ofstream& out,
                                               t_sandesh* tsandesh,
                                               string val) {
    indent(out) << "set_context(" << val << ");" << endl;
}


/**
 * Generate sandesh hints assignment code
 *
 * @param out Output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_hints(ofstream& out,
                                             t_sandesh* tsandesh) {
    bool has_key_annotation = tsandesh->has_key_annotation();
    string shints = "0";

    if (has_key_annotation) {
        shints += " | g_sandesh_constants.SANDESH_KEY_HINT";
    }
    indent(out) << "set_hints(" << shints << ");" << endl;
}

/**
 * Generate default constructor for sandesh
 *
 * @param out The output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_default_ctor(ofstream& out,
                                                    t_sandesh* tsandesh, bool is_request) {
    // Get members
    vector<t_field*>::const_iterator m_iter;
    const vector<t_field*>& members = tsandesh->get_members();
    // Generate default constructor
    indent(out) << tsandesh->get_name() << "()";
    generate_sandesh_member_init_list(out, tsandesh, true);
    out << " {" << endl;
    indent_up();
    // TODO(dreiss): When everything else in Thrift is perfect,
    // do more of these in the initializer list.
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        t_type* t = get_true_type((*m_iter)->get_type());
        if (!t->is_base_type()) {
            t_const_value* cv = (*m_iter)->get_value();
            if (cv != NULL) {
                print_const_value(out, (*m_iter)->get_name(), t, cv);
            }
        }
    }
    generate_sandesh_hints(out, tsandesh);
    if (is_request) {
        generate_sandesh_context(out, tsandesh, "\"\"");
    }
    scope_down(out);
}


/**
 * Writes the sandesh definition into the header file
 *
 * @param out Output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_definition(ofstream& out,
                                                  t_sandesh* tsandesh) {
    const t_type *t = tsandesh->get_type();
    assert(t->is_base_type());
    bool is_trace = ((t_base_type *)t)->is_sandesh_trace() ||
            ((t_base_type *)t)->is_sandesh_trace_object();
    bool is_request = ((t_base_type *)t)->is_sandesh_request();
    bool is_response = ((t_base_type *)t)->is_sandesh_response();
    bool is_uve = ((t_base_type *)t)->is_sandesh_uve();
    bool is_alarm = ((t_base_type *)t)->is_sandesh_alarm();
    bool is_buffer = ((t_base_type *)t)->is_sandesh_buffer();
    bool is_system = ((t_base_type *)t)->is_sandesh_system();
    bool is_object = ((t_base_type *)t)->is_sandesh_object();
    bool is_flow = ((t_base_type *)t)->is_sandesh_flow();
    bool is_session = ((t_base_type *)t)->is_sandesh_session();
    string extends;
    if (is_request) {
        extends = " : public SandeshRequest";
    } else if (is_response) {
        extends = " : public SandeshResponse";
    } else if (is_uve) {
        extends = " : public SandeshUVE";
    } else if (is_alarm) {
        extends = " : public SandeshAlarm";
    } else if (is_system) {
        extends = " : public SandeshSystem";
    } else if (is_buffer) {
        extends = " : public SandeshBuffer";
    } else if (is_trace) {
        extends = " : public SandeshTrace";
    } else if (is_object) {
        extends = " : public SandeshObject";
    } else if (is_flow) {
        if (is_session) {
            extends = " : public SandeshFlowSession";
        } else {
            extends = " : public SandeshFlow";
        }
    }

    // Get members
    vector<t_field*>::const_iterator m_iter;
    const vector<t_field*>& members = tsandesh->get_members();

    // Write the isset structure declaration outside the class. This makes
    // the generated code amenable to processing by SWIG.
    // We only declare the struct if it gets used in the class.

    // Isset struct has boolean fields, but only for non-required fields.
    bool has_nonrequired_fields = false;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        if ((*m_iter)->get_req() != t_field::T_REQUIRED)
            has_nonrequired_fields = true;
    }

    if (has_nonrequired_fields) {

        out <<
                indent() << "typedef struct _" << tsandesh->get_name() << "__isset {" << endl;
        indent_up();

        indent(out) <<
                "_" << tsandesh->get_name() << "__isset() ";
        bool first = true;
        for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
            if ((*m_iter)->get_req() == t_field::T_REQUIRED) {
                continue;
            }
            if (first) {
                first = false;
                out <<
                        ": " << (*m_iter)->get_name() << "(false)";
            } else {
                out <<
                        ", " << (*m_iter)->get_name() << "(false)";
            }
        }
        out << " {}" << endl;

        for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
            if ((*m_iter)->get_req() != t_field::T_REQUIRED) {
                indent(out) <<
                        "bool " << (*m_iter)->get_name() << ";" << endl;
            }
        }

        indent_down();
        indent(out) <<
                "} _" << tsandesh->get_name() << "__isset;" << endl;
    }

    out << endl;

    // Open sandesh def
    out << indent() << "class " << tsandesh->get_name() << extends << " {" << endl;

    // Public members
    out << indent() << " public:" << endl;
    indent_up();
    generate_sandesh_fingerprint(out, tsandesh, false);

    // Create a getter and setter function for all fields except
    // static const string
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        if (((*m_iter)->get_type())->is_static_const_string()) {
            continue;
        }
        out << endl << indent() <<
                "void set_" << (*m_iter)->get_name() <<
                "(" << type_name((*m_iter)->get_type(), false, true);
        out << " val) {" << endl << indent() <<
                indent() << (*m_iter)->get_name() << " = val;" << endl;

        // assume all fields are required except optional fields.
        // for optional fields change __isset.name to true
        bool is_optional = (*m_iter)->get_req() == t_field::T_OPTIONAL;
        if (is_optional) {
            out << indent() << indent() << "__isset." <<
                    (*m_iter)->get_name() << " = true;" << endl;
        }
        out << indent() << "}" << endl;

        out << endl << indent() <<
                type_name((*m_iter)->get_type(), false, true)
                << " get_" << (*m_iter)->get_name() <<
                "() const {" << endl;
        out << indent() << indent() << "return " <<
                (*m_iter)->get_name() << ";" << endl;
        out << indent() << "}" << endl;
    }
    out << endl;

    // Generate an equality testing operator.  Make it inline since the compiler
    // will do a better job than we would when deciding whether to inline it.
    out <<
            indent() << "bool operator == (const " << tsandesh->get_name() << " & " <<
            (members.size() > 0 ? "rhs" : "/* rhs */") << ") const" << endl;
    scope_up(out);
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        // Most existing Thrift code does not use isset or optional/required,
        // so we treat "default" fields as required.
        if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
            out <<
                    indent() << "if (!(" << (*m_iter)->get_name()
                    << " == rhs." << (*m_iter)->get_name() << "))" << endl <<
                    indent() << "  return false;" << endl;
        } else {
            out <<
                    indent() << "if (__isset." << (*m_iter)->get_name()
                    << " != rhs.__isset." << (*m_iter)->get_name() << ")" << endl <<
                    indent() << "  return false;" << endl <<
                    indent() << "else if (__isset." << (*m_iter)->get_name() << " && !("
                    << (*m_iter)->get_name() << " == rhs." << (*m_iter)->get_name()
                    << "))" << endl <<
                    indent() << "  return false;" << endl;
        }
    }
    indent(out) << "return true;" << endl;
    scope_down(out);
    out <<
            indent() << "bool operator != (const " << tsandesh->get_name() << " &rhs) const {" << endl <<
            indent() << "  return !(*this == rhs);" << endl <<
            indent() << "}" << endl << endl;

    // Destructor
    if (tsandesh->annotations_.find("final") == tsandesh->annotations_.end()) {
        out << endl << indent() << "virtual ~" <<
                tsandesh->get_name() << "() {}" << endl << endl;
    }

    // Add the __isset data member if we need it, using the definition from above
    if (has_nonrequired_fields) {
        out << endl << indent() << "_" << tsandesh->get_name() <<
            "__isset __isset;" << endl;
    }

    out << indent() << "virtual std::string ModuleName() const { " <<
        "return \"" << program_name_ << "\"; }" << endl << endl;

    // Is this a Sandesh Request ?
    if (is_request) {
        // Generate default constructor
        generate_sandesh_default_ctor(out, tsandesh, true);
        // Request registration
        indent(out) << "SANDESH_REGISTER_DEC_TYPE(" << tsandesh->get_name() <<
                ");" << endl;
        out << indent() << "virtual void HandleRequest() const;" << endl;
        indent(out) << "virtual bool RequestFromHttp(" <<
            "const std::string& ctx, const std::string& snh_query);" << endl;

        // Generate creator
        out << indent() << "static void Request" <<
                generate_sandesh_no_static_const_string_function(tsandesh, true, true, false, is_request, true) <<
                " {" << endl;
        indent_up();
        out << indent() << tsandesh->get_name() <<
              " * snh = new " << tsandesh->get_name() <<
              generate_sandesh_no_static_const_string_function(tsandesh, false, false, false, is_request) <<
              ";" << endl;
        out << indent() << "snh->Dispatch(sconn);" << endl;
        indent_down();
        indent(out) << "}" << endl << endl;
    } else if (is_response) {
        // Sandesh response
        // Generate default constructor
        generate_sandesh_default_ctor(out, tsandesh, false);
        // Generate creator
        string creator_func_name = "Response";
        out << indent() << "void " << creator_func_name <<
            "() { Dispatch(); }" << endl;
    } else if (is_uve || is_alarm) {
        const vector<t_field*>& fields = tsandesh->get_members();
        vector<t_field*>::const_iterator f_iter = fields.begin();
        assert((*f_iter)->get_name() == "data");

        bool is_proxy = false;
        std::map<std::string, std::string>::iterator ait;
        ait = ((*f_iter)->get_type())->annotations_.find("timeout");
        if (ait != ((*f_iter)->get_type())->annotations_.end()) {
          is_proxy = true;
        }

        if (is_proxy) {
          indent(out) << "static void Send(const " << type_name((*f_iter)->get_type()) <<
            "& data, std::string table = \"\", uint64_t mono_usec=0, int partition=-1);" << endl;
          indent(out) << "static void Send(const " << type_name((*f_iter)->get_type()) <<
            "& data, SandeshLevel::type Xlevel = SandeshLevel::SYS_NOTICE, " <<
            "std::string table = \"\", uint64_t mono_usec=0, int partition=-1);" << endl;
          std::map<std::string, std::string>::iterator pit;
          pit = ((*f_iter)->get_type())->annotations_.find("period");
          indent(out) << "static const uint64_t kProxyPeriod_us = " <<
            pit->second.c_str() << "000000;" << endl;

        } else {
          indent(out) << "static void Send(const " << type_name((*f_iter)->get_type()) <<
            "& data, std::string table = \"\", uint64_t mono_usec=0);" << endl;
          indent(out) << "static void Send(const " << type_name((*f_iter)->get_type()) <<
            "& data, SandeshLevel::type Xlevel, " <<
            "std::string table = \"\", uint64_t mono_usec=0);" << endl;
        }

        indent(out) << "static void Send(const " << type_name((*f_iter)->get_type()) <<
            "& cdata, SandeshUVE::SendType stype, uint32_t seqno," <<
            " uint32_t cycle, std::string ctx = \"\");" << endl;
        indent(out) << "static void Send(const " << type_name((*f_iter)->get_type()) <<
            "& cdata, SandeshLevel::type Xlevel, SandeshUVE::SendType stype, " <<
            "uint32_t seqno, uint32_t cycle, std::string ctx = \"\");" << endl;
    } else if (is_system) {
        generate_sandesh_systemlog_creators(out, tsandesh);
    } else if (is_flow) {
        generate_sandesh_flow_creators(out, tsandesh);
    } else if (is_object) {
        generate_sandesh_objectlog_creators(out, tsandesh);
    } else if (is_trace) {
        // Sandesh trace
        out << indent() << "virtual void SendTrace(" <<
            "const std::string& tcontext, bool more) {" << endl;
        indent_up();
        indent(out) << "set_context(tcontext);" << endl;
        indent(out) << "set_more(more);" << endl;
        out << indent() << tsandesh->get_name() <<
           " * snh = new " << tsandesh->get_name() << "(*this);" << endl;
        indent(out) << "snh->Dispatch();" << endl;
        indent_down();
        indent(out) << "}" << endl << endl;
        // Generate Trace function and Macro
        string creator_func_name = "TraceMsg";
        out << endl << indent() << "static void " << creator_func_name <<
                generate_sandesh_trace_creator(tsandesh, true, false, false, "", "") <<
                ";" << endl << endl;
        string creator_macro_name = tsandesh->get_name() + "Trace";
        string creator_name_usc = underscore(creator_macro_name);
        string creator_name_uc = uppercase(creator_name_usc);
        out << indent() << "#define " << creator_name_uc <<
                generate_sandesh_trace_creator(tsandesh, false, false, true, "_", "") <<
                "\\" << endl;
        indent_up();
        out << indent() << tsandesh->get_name() << "::" << creator_func_name <<
                generate_sandesh_trace_creator(tsandesh, false, true, false, "(_", ")") <<
                endl;
        indent_down();
    } else if (is_buffer) {
        // Sandesh kernel
        // Generate default constructor
         generate_sandesh_default_ctor(out, tsandesh, false);
         // Request registration
         indent(out) << "SANDESH_REGISTER_DEC_TYPE(" << tsandesh->get_name() <<
                 ");" << endl << endl;
    }

    //Generate versionsig return function
    out << indent() << "virtual const int32_t versionsig() const { return versionsig_;}" << endl;
    out << indent() << "static const int32_t sversionsig() { return versionsig_;}" << endl;

    if (!is_trace) {
      out << indent() << "static int32_t lseqnum() { return lseqnum_;}" << endl;
    }

    if (is_uve || is_alarm) {
        const vector<t_field*>& fields = tsandesh->get_members();
        vector<t_field*>::const_iterator f_iter = fields.begin();
        assert((*f_iter)->get_name() == "data");
        string dtype = type_name((*f_iter)->get_type());
        out << indent() <<
            "static std::map<std::string, std::string> _DSConf(void);" << endl;
        out << indent() << "static void _InitDerivedStats(" <<  dtype <<
            " & _data, const map<string,string> & _dsconf);" << endl;
        out << indent() << "static bool UpdateUVE(" <<  dtype <<
            " & _data, " << dtype <<
            " & tdata, uint64_t mono_usec, SandeshLevel::type Xlevel);" << endl;
        out << indent() << "bool LoadUVE(SendType stype, uint32_t cycle);" << endl;
    }

    out << indent() << "std::string ToString() const;" << endl;

    out << indent() << "size_t GetSize() const;" << endl;

    // Private members
    out << endl;
    out << "private:" << endl << endl;
    if (((t_base_type *)t)->is_sandesh_buffer()) {
        indent(out) << "virtual void Process(SandeshContext *context);" << endl << endl;
    } else if (!((t_base_type *)t)->is_sandesh_response()) {
        // Explicit constructor
        indent(out) << "explicit " << tsandesh->get_name();
        out << generate_sandesh_no_static_const_string_function(tsandesh, true, false, false, is_request);
        generate_sandesh_member_init_list(out, tsandesh, false);
        out << " {" << endl;
        indent_up();
        // TODO(dreiss): When everything else in Thrift is perfect,
        // do more of these in the initializer list.
        for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
            t_type* t = get_true_type((*m_iter)->get_type());
            if (!t->is_base_type()) {
                t_const_value* cv = (*m_iter)->get_value();
                if (cv != NULL) {
                    print_const_value(out, (*m_iter)->get_name(), t, cv);
                }
            }
        }
        if (is_trace) {
            generate_sandesh_trace_seqnum_ctor(out, tsandesh);
        }
        if (is_request) {
            generate_sandesh_context(out, tsandesh, "context");
            out << indent() << "if (context == \"ctrl\") " <<
                "set_hints(g_sandesh_constants.SANDESH_CONTROL_HINT);" << endl;
            out << indent() << "else set_hints(0);" << endl;
            out << indent() << "(void)sconn;" << endl;

        } else {
            generate_sandesh_hints(out, tsandesh);
        }
        scope_down(out);
    }

    // Create emplty constructor since objectlogs can have optional fields
    if (((t_base_type *)t)->is_sandesh_object()) {
        out << endl << indent() << "explicit " << tsandesh->get_name()
            << "(uint32_t seqno," " std::string file, int32_t line) : ";
        generate_sandesh_base_init(out, tsandesh, false);
        out << ", file(file), line(line) {" << endl;
        indent_up();
        generate_sandesh_hints(out, tsandesh);
        scope_down(out);
    }

    // Declare all fields
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        indent(out) << declare_field(*m_iter, false, false, false) << endl;
    }

    if (!is_trace) {
        generate_sandesh_seqnum(out, tsandesh);
    }

    generate_sandesh_versionsig(out, tsandesh);

    out << indent() << "static const char *name_;" << endl;

    out << indent() << "void Log() const;" << endl;

    out << indent() << "void ForcedLog() const;" << endl;

    out << indent() << "int32_t Read(" <<
        "boost::shared_ptr<contrail::sandesh::protocol::TProtocol> iprot);" << endl;

    out << indent() << "int32_t Write(" <<
        "boost::shared_ptr<contrail::sandesh::protocol::TProtocol> oprot) const;" << endl;

    if (((t_base_type *)t)->is_sandesh_system()) {
        out << indent() << "static bool do_rate_limit_drop_log_;" << endl;

        out << indent() << "static boost::circular_buffer<time_t>"
                            " rate_limit_buffer_;" << endl;

        out << indent() << "static tbb::mutex rate_limit_mutex_;" << endl;
    }

    out << endl;
    indent_down();
    indent(out) << "};" << endl << endl;
}

void t_cpp_generator::generate_sandesh_fingerprint(ofstream& out,
                                                  t_sandesh* tsandesh,
                                                  bool is_definition) {
  string stat, nspace, comment;
  if (is_definition) {
    stat = "";
    nspace = tsandesh->get_name() + "::";
    comment = " ";
  } else {
    stat = "static ";
    nspace = "";
    comment = "; // ";
  }

  if (tsandesh->has_fingerprint()) {
    out <<
      indent() << stat << "const char* " << nspace
        << "ascii_fingerprint" << comment << "= \"" <<
        tsandesh->get_ascii_fingerprint() << "\";" << endl <<
      indent() << stat << "const uint8_t " << nspace <<
        "binary_fingerprint[" << t_type::fingerprint_len << "]" << comment << "= {";
    const char* comma = "";
    for (int i = 0; i < t_type::fingerprint_len; i++) {
      out << comma << "0x" << t_struct::byte_to_hex(tsandesh->get_binary_fingerprint()[i]);
      comma = ",";
    }
    out << "};" << endl << endl;
  }
}

void t_cpp_generator::cache_attr_info(t_struct* tstruct,
    std::map<string, CacheAttribute> &attrs) {
  attrs.clear();
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    std::map<std::string, std::string>::iterator jt;
    jt = (*m_iter)->annotations_.find("hidden");
    if ((*m_iter)->get_req()!=t_field::T_OPTIONAL) {
      // Mandatory fields cannot have DerivedStats,
      // or be of hidden/periodic type
      assert(jt == (*m_iter)->annotations_.end());
      assert((*m_iter)->annotations_.find("stats") == (*m_iter)->annotations_.end());
      assert((*m_iter)->annotations_.find("mstats") == (*m_iter)->annotations_.end());
      attrs.insert(std::make_pair((*m_iter)->get_name(),
        MANDATORY));
      continue;
    }
    // DervivedStats attributes are not handled here
    if (((*m_iter)->annotations_.find("stats") != (*m_iter)->annotations_.end()) ||
      ((*m_iter)->annotations_.find("mstats") != (*m_iter)->annotations_.end())) {
      continue;
    }
    // All raw attributes are Non-periodic
    if (jt!=(*m_iter)->annotations_.end()) {
      attrs.insert(std::make_pair((*m_iter)->get_name(), HIDDEN));
    } else {
      attrs.insert(std::make_pair((*m_iter)->get_name(), INLINE));
    }
  }
}

void t_cpp_generator::derived_stats_info(t_struct* tstruct,
    // val is rawtype,resulttype,algo,annotation,compattr,subcompattr
    map<string,DSInfo> & dsmap,
    // val is set of ds attributes
    map<string,set<string> >& rawmap) {
  const vector<t_field*>& members = tstruct->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    std::map<std::string, std::string>::iterator jt,mt;
    jt = (*m_iter)->annotations_.find("stats");
    mt = (*m_iter)->annotations_.find("mstats");
    if ((jt != (*m_iter)->annotations_.end()) ||
        (mt != (*m_iter)->annotations_.end())) {
      assert((*m_iter)->get_req() == t_field::T_OPTIONAL);

      t_type* retype = get_true_type((*m_iter)->get_type());
      string restype;

      bool is_ds_map = false;
      // for map derived-stats
      if (mt!= (*m_iter)->annotations_.end()) {
        // out of mstats and stats, only one should be specified
        assert(jt == (*m_iter)->annotations_.end());
        is_ds_map = true;
        assert(retype->is_map());
        t_type* vtype = ((t_map*)retype)->get_val_type();
        restype = type_name(get_true_type(vtype));
      } else {
        restype = type_name(get_true_type((*m_iter)->get_type()));
      }

      const string &tstr = ((jt == (*m_iter)->annotations_.end()) ?
              mt->second : jt->second);
      size_t pos = tstr.find(':');

      string residual = tstr.substr(pos+1, string::npos);
      size_t rpos = residual.find(':');
      string algo, anno;
      if (rpos == string::npos) {
        // No Derived Stats arguments are present
        algo = residual;
        anno = string();
      } else {
        algo = residual.substr(0,rpos);
        anno = residual.substr(rpos+1, string::npos);
      }

      string rawfullattr;
      string rawperiodattr = tstr.substr(0,pos);
      size_t ppos = rawperiodattr.find("-");
      int period = -1;
      string prealgo;
      if (ppos != string::npos) {
        string periodpre = rawperiodattr.substr(0,ppos);
        size_t prepos = periodpre.find(".");
        if (prepos != string::npos) {
            period = atoi(periodpre.substr(0,prepos).c_str());
            prealgo = periodpre.substr(prepos+1, string::npos);
        } else {
            period = atoi(rawperiodattr.substr(0,ppos).c_str());
        }

        rawfullattr = rawperiodattr.substr(ppos+1, string::npos);
      } else {
        rawfullattr = rawperiodattr;
      }

      size_t cpos = rawfullattr.find('.');
      string rawattr;
      string compattr("");
      string subcompattr("");

      // Does the raw attribute have a composite underneath it?
      if (cpos != string::npos) {
        rawattr = rawfullattr.substr(0,cpos);
        string rescomp = rawfullattr.substr(cpos+1, string::npos);
        size_t upos = rescomp.find('.');
        if (upos != string::npos) {
            compattr = rescomp.substr(0,upos);
            subcompattr = rescomp.substr(upos+1, string::npos);
        } else {
            compattr = rescomp;
        }
      } else {
        rawattr = rawfullattr;
      }

      string rawtype;
      RawMetric rmt = RM_DIAL;
      vector<t_field*>::const_iterator s_iter;
      for (s_iter = members.begin(); s_iter != members.end(); ++s_iter) {
        if (rawattr.compare((*s_iter)->get_name())==0) {
          map<string,string>::const_iterator cit =
              (*s_iter)->annotations_.find("metric");
          if (cit != (*s_iter)->annotations_.end()) {
            if (cit->second.compare("agg") == 0) rmt = RM_AGG;
            else if (cit->second.compare("diff") == 0) rmt = RM_DIFF;
          }
          t_type* ratype = get_true_type((*s_iter)->get_type());
          t_type* vtype = ratype;
          if (is_ds_map) {
            assert(ratype->is_map());
            vtype = ((t_map*)ratype)->get_val_type();
          }
          rawtype = type_name(vtype);
          if (!compattr.empty()) {
            // The raw attribute is a composite/subcomposite
            // Find info for the composite attribute.
            assert(vtype->is_struct());
            rawtype = string("");
            const vector<t_field*>& cmembers = ((t_struct*)vtype)->get_members();
            vector<t_field*>::const_iterator c_iter;
            for (c_iter = cmembers.begin(); c_iter != cmembers.end(); ++c_iter) {
              if (compattr.compare((*c_iter)->get_name())==0) {
                t_type* catype = get_true_type((*c_iter)->get_type());
                if (!subcompattr.empty()) {
                  // The raw attribute is a subcomposite
                  assert(catype->is_struct());
                  const vector<t_field*>& umem =
                          ((t_struct*)catype)->get_members();
                  vector<t_field*>::const_iterator u_iter;
                  for (u_iter = umem.begin(); u_iter != umem.end(); ++u_iter) {
                    if (subcompattr.compare((*u_iter)->get_name())==0) {
                      t_type* uatype = get_true_type((*u_iter)->get_type());
                      rawtype = type_name(uatype);
                      break;
                    }
                  }

                } else {
                  rawtype = type_name(catype);
                }
                break;
              }
            }
          }
          break;
        }
      }
      assert(!rawtype.empty());

      bool is_hidden =
        (*m_iter)->annotations_.find("hidden") != (*m_iter)->annotations_.end();

      CacheAttribute cat = INLINE;
      if (tstruct->annotations_.find("period") != tstruct->annotations_.end()) {
        // By default, DS attributes in periodic structs have period 1
        if (period==-1) period=1;
        if (is_hidden) {
          if (period!=0) cat = HIDDEN_PER;
          else cat = HIDDEN;
        } else {
          if (period!=0) cat = PERIODIC;
          // else this DS stays INLINE
        }
      } else {
        // DS attributes in non-periodic structs must have period 0
        if (period==-1) period=0;
        if (is_hidden) {
          cat = HIDDEN;
        }
        // else this DS stays INLINE
        assert(period==0);
      }

      DSInfo dsi(is_ds_map, period, cat, rawtype, rmt,
          restype, algo, anno, compattr, subcompattr, prealgo);

      // map of derived stats
      dsmap.insert(make_pair((*m_iter)->get_name(), dsi));

      // map of raw attributes
      map<string,set<string> >::iterator r_iter = rawmap.find(rawattr);
      if (r_iter != rawmap.end()) {
          r_iter->second.insert((*m_iter)->get_name());
      } else {
          set<string> dss;
          dss.insert((*m_iter)->get_name());
          rawmap.insert(make_pair(rawattr,dss));
      }
    }
  }
}
#endif
/**
 * Writes the struct definition into the header file
 *
 * @param out Output stream
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_definition(ofstream& out,
                                                 t_struct* tstruct,
                                                 bool is_exception,
                                                 bool pointers,
                                                 bool read,
                                                 bool write) {
  string extends = "";
  if (is_exception) {
    extends = " : public ::contrail::sandesh::TException";
  }

  // Get members
  vector<t_field*>::const_iterator m_iter;
  const vector<t_field*>& members = tstruct->get_members();

  // Write the isset structure declaration outside the class. This makes
  // the generated code amenable to processing by SWIG.
  // We only declare the struct if it gets used in the class.

  // Isset struct has boolean fields, but only for non-required fields.
  bool has_nonrequired_fields = false;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if ((*m_iter)->get_req() != t_field::T_REQUIRED)
      has_nonrequired_fields = true;
  }

  bool del_support = false;
  bool proxy_support = false;
  if (has_nonrequired_fields && (!pointers || read)) {

    out <<
      indent() << "typedef struct _" << tstruct->get_name() << "__isset {" << endl;
    indent_up();

    indent(out) <<
      "_" << tstruct->get_name() << "__isset() ";
    bool first = true;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if ((*m_iter)->get_req() == t_field::T_REQUIRED) {
        continue;
      }
      t_type* t = get_true_type((*m_iter)->get_type());
      if (t->is_base_type() &&
          ((*m_iter)->get_name().compare("deleted") == 0)) {
        t_base_type::t_base tbase = ((t_base_type*)t)->get_base();
        if (tbase == t_base_type::TYPE_BOOL) del_support = true;
      }
      if (t->is_base_type() &&
          ((*m_iter)->get_name().compare("proxy") == 0)) {
        t_base_type::t_base tbase = ((t_base_type*)t)->get_base();
        if (tbase == t_base_type::TYPE_STRING) proxy_support = true;
      }

      if (first) {
        first = false;
        out <<
          ": " << (*m_iter)->get_name() << "(false)";
      } else {
        out <<
          ", " << (*m_iter)->get_name() << "(false)";
      }
    }
    out << " {}" << endl;

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if ((*m_iter)->get_req() != t_field::T_REQUIRED) {
        indent(out) <<
          "bool " << (*m_iter)->get_name() << ";" << endl;
        }
    }

    indent_down();
    indent(out) <<
      "} _" << tstruct->get_name() << "__isset;" << endl << endl;

  }

  out << endl;

  // Open struct def
  out <<
    indent() << "class " << tstruct->get_name() << extends << " {" << endl <<
    indent() << " public:" << endl <<
    endl;
  indent_up();

  // Put the fingerprint up top for all to see.
  generate_struct_fingerprint(out, tstruct, false);

#ifdef SANDESH
  bool is_table = false;

  std::map<string, CacheAttribute> cache_attrs;
  cache_attr_info(tstruct, cache_attrs);

  map<string,DSInfo> dsinfo;
  map<string,set<string> > rawmap;
  derived_stats_info(tstruct, dsinfo, rawmap);
  map<string,DSInfo >::iterator ds_iter;
#endif

  if (!pointers) {
    // Default constructor
#ifdef SANDESH
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      if ((*m_iter)->get_name() == "name") {
        std::map<std::string, std::string>::iterator it;
        it = (*m_iter)->annotations_.find("key");
        if (it != (*m_iter)->annotations_.end()) {
          is_table = true;
          indent(out) << tstruct->get_name() << "(std::string __tbl = \"" <<
            it->second << "\")";
          break;
        }
      }
    }
    if (!is_table) {
      indent(out) <<
        tstruct->get_name() << "()";
    }
#else
    indent(out) <<
      tstruct->get_name() << "()";
#endif

    bool init_ctor = false;

    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      t_type* t = get_true_type((*m_iter)->get_type());
#ifdef SANDESH
      if (t->is_static_const_string()) {
        continue;
      }
#endif
      if (t->is_base_type() || t->is_enum()) {
        string dval;
        if (t->is_enum()) {
          dval += "(" + type_name(t) + ")";
        }
        if (t->is_string() || t->is_xml()) {
          dval += "\"\"";
        } else if (t->is_uuid()) {
            dval += "boost::uuids::nil_uuid()";
        } else if (t->is_ipaddr()) {
            dval += "";
        } else {
            dval += "0";
        }
        t_const_value* cv = (*m_iter)->get_value();
        if (cv != NULL) {
          dval = render_const_value(out, (*m_iter)->get_name(), t, cv);
        }
        if (!init_ctor) {
          init_ctor = true;
          out << " : ";
          out << (*m_iter)->get_name() << "(" << dval << ")";
        } else {
          out << ", " << (*m_iter)->get_name() << "(" << dval << ")";
        }
      }
    }

#ifdef SANDESH
    if (is_table) {
      out << ", table_(__tbl)";
    }
#endif
    out << " {" << endl;
    indent_up();
    // TODO(dreiss): When everything else in Thrift is perfect,
    // do more of these in the initializer list.
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      t_type* t = get_true_type((*m_iter)->get_type());

      if (!t->is_base_type()) {
        t_const_value* cv = (*m_iter)->get_value();
        if (cv != NULL) {
          print_const_value(out, (*m_iter)->get_name(), t, cv);
        }
      }
    }
    scope_down(out);
  }

  if (tstruct->annotations_.find("final") == tstruct->annotations_.end()) {
    out <<
      endl <<
      indent() << "virtual ~" << tstruct->get_name() << "() throw() {}" << endl << endl;
  }

  // Pointer to this structure's reflection local typespec.
  if (gen_dense_) {
    indent(out) <<
      "static ::contrail::sandesh::reflection::local::TypeSpec* local_reflection;" <<
      endl << endl;
  }

  // Declare all fields
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    indent(out) <<
      declare_field(*m_iter, false, pointers && !(*m_iter)->get_type()->is_xception(), !read) << endl;
  }
#ifdef SANDESH
  for (ds_iter = dsinfo.begin(); ds_iter != dsinfo.end(); ++ds_iter) {

    // val is rawtype,resulttype,algo,annotation,compattr,subcompattr
    if ((ds_iter->second.cat_ == PERIODIC) ||
        (ds_iter->second.cat_ == HIDDEN_PER)) {
      if (ds_iter->second.prealgo_.empty()) {
        indent(out) << "boost::shared_ptr< ::contrail::sandesh::DerivedStatsPeriodicIf< ::contrail::sandesh::" <<
          ds_iter->second.algo_ << ", " <<
          ds_iter->second.rawtype_ << ", " <<
          ds_iter->second.resulttype_.substr(0, ds_iter->second.resulttype_.size() - 3) << ", " <<
          ds_iter->second.resulttype_ <<
          "> > __dsobj_" << ds_iter->first << ";" << endl;
      } else {
        indent(out) << "boost::shared_ptr< ::contrail::sandesh::DerivedStatsPeriodicAnomalyIf< ::contrail::sandesh::" <<
          ds_iter->second.algo_ << ", " <<
          ds_iter->second.rawtype_ << ", ::contrail::sandesh::" <<
          ds_iter->second.prealgo_ << ", " <<
          ds_iter->second.resulttype_ <<
          "> > __dsobj_" << ds_iter->first << ";" << endl;
      }
    } else {
      indent(out) << "boost::shared_ptr< ::contrail::sandesh::DerivedStatsIf< ::contrail::sandesh::" <<
        ds_iter->second.algo_ << ", " <<
        ds_iter->second.rawtype_ << ", " << ds_iter->second.resulttype_ <<
        "> > __dsobj_" << ds_iter->first << ";" << endl;
    }

  }
  if (is_table) {
    indent(out) << "std::string table_;" << endl;
  }

#endif

  // Add the __isset data member if we need it, using the definition from above
  if (has_nonrequired_fields && (!pointers || read)) {
    out <<
      endl <<
      indent() << "_" << tstruct->get_name() << "__isset __isset;" << endl;
  }

  // Create a setter and getter function for each field except static const
  // string
  vector<string> dskeys;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (pointers) {
      continue;
    }
#ifdef SANDESH
    if (((*m_iter)->get_type())->is_static_const_string()) {
      continue;
    }
    std::map<std::string, std::string>::iterator jt;
    jt = (*m_iter)->annotations_.find("aggtype");
    if (jt != (*m_iter)->annotations_.end()) {
        if (jt->second.compare("listkey")==0) {
            dskeys.push_back((*m_iter)->get_name());
        }
    }
#endif
    out <<
      endl <<
#ifdef SANDESH
      indent() << (dsinfo.find((*m_iter)->get_name()) == dsinfo.end() ?
                   "void set_" : "void __set_") << (*m_iter)->get_name() <<
#else
      indent() << "void __set_" << (*m_iter)->get_name() <<
#endif
        "(" << type_name((*m_iter)->get_type(), false, true);
    out << " val) {" << endl << indent() <<
      indent() << (*m_iter)->get_name() << " = val;" << endl;

    // assume all fields are required except optional fields.
    // for optional fields change __isset.name to true
    bool is_optional = (*m_iter)->get_req() == t_field::T_OPTIONAL;
    if (is_optional) {
      out <<
        indent() <<
        indent() << "__isset." << (*m_iter)->get_name() << " = true;" << endl;
    }
    out <<
      indent()<< "}" << endl;

#ifdef SANDESH
    out << endl << indent() << type_name((*m_iter)->get_type(), false, true)
      << " get_";
    out << (*m_iter)->get_name() << "() const {" << endl;
    out << indent() << indent() << "return " << (*m_iter)->get_name() << ";" << endl;
    out << indent() << "}" << endl;
#endif
  }
  out << endl;

  if (!pointers) {
    out <<
      indent() << "bool operator == (long " <<
      (members.size() > 0 ? "rhs" : "/* rhs */") << ") const" << endl;
    scope_up(out);
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      // Most existing Thrift code does not use isset or optional/required,
      // so we treat "default" fields as required.
      t_type* type = get_true_type((*m_iter)->get_type());
      if (type->is_base_type()) {
        t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
        switch (tbase) {
        case t_base_type::TYPE_BYTE:
        case t_base_type::TYPE_I16:
        case t_base_type::TYPE_I32:
        case t_base_type::TYPE_I64:
        case t_base_type::TYPE_U16:
        case t_base_type::TYPE_U32:
        case t_base_type::TYPE_U64:
        case t_base_type::TYPE_BOOL:
          if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
            out <<
              indent() << "if (!((long)" << (*m_iter)->get_name()
                << " == rhs))" << endl <<
              indent() << "  return false;" << endl;
          } else {
            out <<
              indent() << "if (__isset." << (*m_iter)->get_name()
                << " && !((long)"
                << (*m_iter)->get_name() << " == rhs))" << endl <<
              indent() << "  return false;" << endl;
          }
          break;
        default:
          out << indent() << "return false;" << endl;
          continue;
        }
      }
    }
    indent(out) << "return true;" << endl;
    scope_down(out);
    // Generate an equality testing operator.  Make it inline since the compiler
    // will do a better job than we would when deciding whether to inline it.
    out <<
      indent() << "bool operator == (const " << tstruct->get_name() << " & " <<
      (members.size() > 0 ? "rhs" : "/* rhs */") << ") const" << endl;
    scope_up(out);
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      // Most existing Thrift code does not use isset or optional/required,
      // so we treat "default" fields as required.
      if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
        out <<
          indent() << "if (!(" << (*m_iter)->get_name()
                   << " == rhs." << (*m_iter)->get_name() << "))" << endl <<
          indent() << "  return false;" << endl;
      } else {
        out <<
          indent() << "if (__isset." << (*m_iter)->get_name()
                   << " != rhs.__isset." << (*m_iter)->get_name() << ")" << endl <<
          indent() << "  return false;" << endl <<
          indent() << "else if (__isset." << (*m_iter)->get_name() << " && !("
                   << (*m_iter)->get_name() << " == rhs." << (*m_iter)->get_name()
                   << "))" << endl <<
          indent() << "  return false;" << endl;
      }
    }
    indent(out) << "return true;" << endl;
    scope_down(out);
    out <<
      indent() << "bool operator != (const " << tstruct->get_name() << " &rhs) const {" << endl <<
      indent() << "  return !(*this == rhs);" << endl <<
      indent() << "}" << endl << endl;

    // Generate the declaration of a less-than operator.  This must be
    // implemented by the application developer if they wish to use it.  (They
    // will get a link error if they try to use it without an implementation.)
    out <<
      indent() << "bool operator < (const "
               << tstruct->get_name() << " & ) const;" << endl << endl;
  }

  if (read) {
    if (gen_templates_) {
      out <<
        indent() << "template <class Protocol_>" << endl <<
        indent() << "uint32_t read(Protocol_* iprot);" << endl;
    } else {
      out <<
        indent() << "int32_t read(" <<
        "boost::shared_ptr<contrail::sandesh::protocol::TProtocol> iprot);" << endl;
    }
  }
  if (write) {
    if (gen_templates_) {
      out <<
        indent() << "template <class Protocol_>" << endl <<
        indent() << "uint32_t write(Protocol_* oprot) const;" << endl;
    } else {
      out <<
        indent() << "int32_t write(" <<
        "boost::shared_ptr<contrail::sandesh::protocol::TProtocol> oprot) const;" << endl;
    }
  }
#ifdef SANDESH
  out << indent() << "std::string log() const;" << endl;
  if (!pointers) {
    out <<  indent() <<
      tstruct->get_name() << " operator+(const " << tstruct->get_name() <<
      "& right) const" << endl;
    scope_up(out);
    out << indent() << tstruct->get_name() << " result;" << endl;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      std::string nn = (*m_iter)->get_name();
      ds_iter = dsinfo.find((*m_iter)->get_name());
      std::string sn;
      if (ds_iter == dsinfo.end()) sn = std::string("set_");
      else sn = std::string("__set_");

      t_type* type = get_true_type((*m_iter)->get_type());
      if (type->is_base_type()) {
        t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
        switch (tbase) {
        case t_base_type::TYPE_BYTE:
        case t_base_type::TYPE_I16:
        case t_base_type::TYPE_I32:
        case t_base_type::TYPE_I64:
        case t_base_type::TYPE_DOUBLE:
        case t_base_type::TYPE_U16:
        case t_base_type::TYPE_U32:
        case t_base_type::TYPE_U64:
          if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
            out <<
              indent() << "result." << sn << nn << "(get_" << nn <<
                "() + right.get_" << nn << "());" << endl << endl;
          } else {
            out <<
              indent() << "if ((__isset." << nn << ") && (right.__isset." <<
                nn << ")) result." << sn << nn << "(get_" << nn <<
                "() + right.get_" << nn << "());" << endl <<
              indent() << "else if (__isset." << nn << ") result." << sn <<
                nn << "(get_" << nn << "());" << endl <<
              indent() << "else if (right.__isset." << nn << ") result." << sn <<
                nn << "(right.get_" << nn << "());" << endl << endl;
          }
        default:
          continue;
        }
      }
    }
    indent(out) << "return result;" << endl;
    scope_down(out);

    out <<  indent() <<
      tstruct->get_name() << " operator/(int div) const" << endl;
    scope_up(out);
    out << indent() << tstruct->get_name() << " result;" << endl;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      std::string nn = (*m_iter)->get_name();
      ds_iter = dsinfo.find((*m_iter)->get_name());
      std::string sn;
      if (ds_iter == dsinfo.end()) sn = std::string("set_");
      else sn = std::string("__set_");

      t_type* type = get_true_type((*m_iter)->get_type());
      if (type->is_base_type()) {
        t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
        switch (tbase) {
        case t_base_type::TYPE_BYTE:
        case t_base_type::TYPE_I16:
        case t_base_type::TYPE_I32:
        case t_base_type::TYPE_I64:
        case t_base_type::TYPE_DOUBLE:
        case t_base_type::TYPE_U16:
        case t_base_type::TYPE_U32:
        case t_base_type::TYPE_U64:
          if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
            out <<
              indent() << "result." << sn << nn << "(get_" << nn <<
                "() / div);" << endl << endl;
          } else {
            out <<
              indent() << "if (__isset." << nn << ") result." << sn << nn << "(get_" <<
                nn << "() / div);" << endl << endl;
          }
        default:
          continue;
        }
      }
    }
    indent(out) << "return result;" << endl;
    scope_down(out);

    out <<  indent() <<
      tstruct->get_name() << " operator-(const " << tstruct->get_name() <<
      "& right) const" << endl;
    scope_up(out);
    out << indent() << tstruct->get_name() << " result;" << endl;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      std::string nn = (*m_iter)->get_name();
      ds_iter = dsinfo.find((*m_iter)->get_name());
      std::string sn;
      if (ds_iter == dsinfo.end()) sn = std::string("set_");
      else sn = std::string("__set_");
      t_type* type = get_true_type((*m_iter)->get_type());
      if (type->is_base_type()) {
        t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
        switch (tbase) {
        case t_base_type::TYPE_BYTE:
        case t_base_type::TYPE_I16:
        case t_base_type::TYPE_I32:
        case t_base_type::TYPE_I64:
        case t_base_type::TYPE_DOUBLE:
        case t_base_type::TYPE_U16:
        case t_base_type::TYPE_U32:
        case t_base_type::TYPE_U64:
          if ((*m_iter)->get_req() != t_field::T_OPTIONAL) {
            out <<
              indent() << "result." << sn <<  nn << "(get_" << nn <<
                "() - right.get_" << nn << "());" << endl << endl;
          } else {
            out <<
              indent() << "if ((__isset." << nn << ") && (right.__isset." <<
                nn << ")) {" << endl <<
              indent() << "  result." << sn << nn << "(get_" << nn <<
                "() - right.get_" << nn << "()); }" << endl <<
              indent() << "else if (__isset." << nn << ") result." << sn <<
                nn << "(get_" << nn << "());" << endl;
          }
        default:
          continue;
        }
      }
    }
    indent(out) << "return result;" << endl;
    scope_down(out);
  }
  out << indent() << "size_t GetSize() const;" << endl;
  out << indent() << "std::string __listkey(void) {" << endl;
  out << indent() << "    std::string __result;" << endl;
  bool first = true;
  vector<string>::iterator kit;
  for (kit = dskeys.begin(); kit != dskeys.end(); kit++) {
    if (!first) {
      out << indent() << "    __result.append(\":\");" << endl;
    }
    out << indent() << "    __result += " << *kit << ";" << endl;
    first = false;
  }
  out << indent() << "    return __result;" << endl;
  out << indent() << "}" << endl;
#endif
  out << endl;

  indent_down();
  indent(out) <<
    "};" << endl <<
    endl;

  if (del_support) {
    out <<
      indent() << "template <>" << endl <<
      indent() << "struct SandeshStructDeleteTrait<" <<
        tstruct->get_name() << "> {" << endl <<
      indent() << "  static bool get(const " <<
        tstruct->get_name() << "& s) { return s.get_deleted(); }" <<
        endl <<
      indent() << "};" << endl;
  }
  if (proxy_support) {
    out <<
      indent() << "template <>" << endl <<
      indent() << "struct SandeshStructProxyTrait<" <<
        tstruct->get_name() << "> {" << endl <<
      indent() << "  static std::string get(const " <<
        tstruct->get_name() << "& s) { return s.get_proxy(); }" <<
        endl <<
      indent() << "};" << endl;
  }
}

/**
 * Writes the fingerprint of a struct to either the header or implementation.
 *
 * @param out Output stream
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_fingerprint(ofstream& out,
                                                  t_struct* tstruct,
                                                  bool is_definition) {
  string stat, nspace, comment;
  if (is_definition) {
    stat = "";
    nspace = tstruct->get_name() + "::";
    comment = " ";
  } else {
    stat = "static ";
    nspace = "";
    comment = "; // ";
  }

  if (tstruct->has_fingerprint()) {
    out <<
      indent() << stat << "const char* " << nspace
        << "ascii_fingerprint" << comment << "= \"" <<
        tstruct->get_ascii_fingerprint() << "\";" << endl <<
      indent() << stat << "const uint8_t " << nspace <<
        "binary_fingerprint[" << t_type::fingerprint_len << "]" << comment << "= {";
    const char* comma = "";
    for (int i = 0; i < t_type::fingerprint_len; i++) {
      out << comma << "0x" << t_struct::byte_to_hex(tstruct->get_binary_fingerprint()[i]);
      comma = ",";
    }
    out << "};" << endl << endl;
  }
}

/**
 * Writes the local reflection of a type (either declaration or definition).
 */
void t_cpp_generator::generate_local_reflection(std::ofstream& out,
                                                t_type* ttype,
                                                bool is_definition) {
  if (!gen_dense_) {
    return;
  }
  ttype = get_true_type(ttype);
  assert(ttype->has_fingerprint());
  string key = ttype->get_ascii_fingerprint() + (is_definition ? "-defn" : "-decl");
  // Note that we have generated this fingerprint.  If we already did, bail out.
  if (!reflected_fingerprints_.insert(key).second) {
    return;
  }
  // Let each program handle its own structures.
  if (ttype->get_program() != NULL && ttype->get_program() != program_) {
    return;
  }

  // Do dependencies.
  if (ttype->is_list()) {
    generate_local_reflection(out, ((t_list*)ttype)->get_elem_type(), is_definition);
  } else if (ttype->is_set()) {
    generate_local_reflection(out, ((t_set*)ttype)->get_elem_type(), is_definition);
  } else if (ttype->is_map()) {
    generate_local_reflection(out, ((t_map*)ttype)->get_key_type(), is_definition);
    generate_local_reflection(out, ((t_map*)ttype)->get_val_type(), is_definition);
  } else if (ttype->is_struct() || ttype->is_xception()) {
    // Hacky hacky.  For efficiency and convenience, we need a dummy "T_STOP"
    // type at the end of our typespec array.  Unfortunately, there is no
    // T_STOP type, so we use the global void type, and special case it when
    // generating its typespec.

    const vector<t_field*>& members = ((t_struct*)ttype)->get_sorted_members();
    vector<t_field*>::const_iterator m_iter;
    for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
      generate_local_reflection(out, (**m_iter).get_type(), is_definition);
    }
    generate_local_reflection(out, g_type_void, is_definition);

    // For definitions of structures, do the arrays of metas and field specs also.
    if (is_definition) {
      out <<
        indent() << "::contrail::sandesh::reflection::local::FieldMeta" << endl <<
        indent() << local_reflection_name("metas", ttype) <<"[] = {" << endl;
      indent_up();
      for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        indent(out) << "{ " << (*m_iter)->get_key() << ", " <<
          (((*m_iter)->get_req() == t_field::T_OPTIONAL) ? "true" : "false") <<
          " }," << endl;
      }
      // Zero for the T_STOP marker.
      indent(out) << "{ 0, false }" << endl << "};" << endl;
      indent_down();

      out <<
        indent() << "::contrail::sandesh::reflection::local::TypeSpec*" << endl <<
        indent() << local_reflection_name("specs", ttype) <<"[] = {" << endl;
      indent_up();
      for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
        indent(out) << "&" <<
          local_reflection_name("typespec", (*m_iter)->get_type(), true) << "," << endl;
      }
      indent(out) << "&" <<
        local_reflection_name("typespec", g_type_void) << "," << endl;
      indent_down();
      indent(out) << "};" << endl;
    }
  }

  out <<
    indent() << "// " << ttype->get_fingerprint_material() << endl <<
    indent() << (is_definition ? "" : "extern ") <<
      "::contrail::sandesh::reflection::local::TypeSpec" << endl <<
      local_reflection_name("typespec", ttype) <<
      (is_definition ? "(" : ";") << endl;

  if (!is_definition) {
    out << endl;
    return;
  }

  indent_up();

  if (ttype->is_void()) {
    indent(out) << "::contrail::sandesh::protocol::T_STOP";
  } else {
    indent(out) << type_to_enum(ttype);
  }

  if (ttype->is_struct()) {
    out << "," << endl <<
      indent() << type_name(ttype) << "::binary_fingerprint," << endl <<
      indent() << local_reflection_name("metas", ttype) << "," << endl <<
      indent() << local_reflection_name("specs", ttype);
  } else if (ttype->is_list()) {
    out << "," << endl <<
      indent() << "&" << local_reflection_name("typespec", ((t_list*)ttype)->get_elem_type(), true) << "," << endl <<
      indent() << "NULL";
  } else if (ttype->is_set()) {
    out << "," << endl <<
      indent() << "&" << local_reflection_name("typespec", ((t_set*)ttype)->get_elem_type(), true) << "," << endl <<
      indent() << "NULL";
  } else if (ttype->is_map()) {
    out << "," << endl <<
      indent() << "&" << local_reflection_name("typespec", ((t_map*)ttype)->get_key_type(), true) << "," << endl <<
      indent() << "&" << local_reflection_name("typespec", ((t_map*)ttype)->get_val_type(), true);
  }

  out << ");" << endl << endl;

  indent_down();
}

/**
 * Writes the structure's static pointer to its local reflection typespec
 * into the implementation file.
 */
void t_cpp_generator::generate_local_reflection_pointer(std::ofstream& out,
                                                        t_type* ttype) {
  if (!gen_dense_) {
    return;
  }
  indent(out) <<
    "::contrail::sandesh::reflection::local::TypeSpec* " <<
      ttype->get_name() << "::local_reflection = " << endl <<
    indent() << "  &" << local_reflection_name("typespec", ttype) << ";" <<
    endl << endl;
}

/**
 * Makes a helper function to gen a struct reader.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_reader(ofstream& out,
                                             t_struct* tstruct,
                                             bool pointers) {
  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "int32_t " << tstruct->get_name() <<
      "::read(Protocol_* iprot) {" << endl;
  } else {
    indent(out) <<
      "int32_t " << tstruct->get_name() <<
      "::read(boost::shared_ptr<contrail::sandesh::protocol::TProtocol> iprot) {" << endl;
  }

  generate_common_struct_reader_body(out, tstruct, pointers);

  indent(out) <<
    "}" << endl << endl;
}

/**
 * Generates the write function.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_struct_writer(ofstream& out,
                                             t_struct* tstruct,
                                             bool pointers) {
  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "int32_t " << tstruct->get_name() <<
      "::write(Protocol_* oprot) const {" << endl;
  } else {
    indent(out) <<
      "int32_t " << tstruct->get_name() <<
      "::write(boost::shared_ptr<contrail::sandesh::protocol::TProtocol> oprot) const {" << endl;
  }

  generate_common_struct_writer_body(out, tstruct, pointers);

  indent(out) <<
    "}" << endl <<
    endl;
}

#ifdef SANDESH
/**
 * Generate get size for structs
 *
 * @param out Stream to write to
 * @param tstruct The struct tstruct->get_members() tstruct->get_name()
 */
void t_cpp_generator::generate_struct_get_size(ofstream& out, const string& name,
                                             const vector<t_field*>& fields) {
    //Generate GetSize function to return size of sandesh
     indent(out) << "size_t " << name <<
                "::GetSize() const {" << endl;
     indent_up();
     indent(out) << "size_t size = 0;" << endl;
     vector<t_field*>::const_iterator f_iter;
     for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
         generate_get_size_field(out, *f_iter);
     }
     indent(out) << "return size;" << endl;
     indent_down();
     indent(out) << "}" << endl << endl;
}

/**
 * Generate logger for structs
 *
 * @param out Stream to write to
 * @param tstruct The struct tstruct->get_members() tstruct->get_name()
 */
void t_cpp_generator::generate_struct_logger(ofstream& out, const string& name,
                                             const vector<t_field*>& fields) {
    indent(out) << "std::string " << name <<
            "::log() const {" << endl;
    indent_up();
    out << indent() << "std::stringstream Xbuf;" << endl;
    vector<t_field*>::const_iterator f_iter;
    bool init = false;
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        string prefix = "\" \" << ";
        if (!init) {
            init = true;
            prefix = "";
        }
        generate_logger_field(out, *f_iter, prefix, false, false);
    }
    if (!init) {
        out << indent() << "return std::string();" << endl;
    } else {
        out << indent() << "return Xbuf.str();" << endl;
    }
    indent_down();
    indent(out) << "}" << endl << endl;
}

/**
 * Generate definitions for static const strings.
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_static_const_string_definition(ofstream& out,
                                                              string name,
                                                              const vector<t_field*>& fields) {
    vector<t_field*>::const_iterator f_iter;
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if (!((*f_iter)->get_type())->is_static_const_string()) {
            continue;
        }
        t_const_value* cv = (*f_iter)->get_value();
        assert(cv != NULL);
        out << base_type_name(t_base_type::TYPE_STRING) << " " << name <<
                "::";
        print_const_value(out, (*f_iter)->get_name(), (*f_iter)->get_type(), cv);
    }
}

/**
 * Generate definitions for static const strings for a sandesh.
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_static_const_string_definition(ofstream& out,
                                                              t_sandesh* tsandesh) {
    generate_static_const_string_definition(out, tsandesh->get_name(),
                                            tsandesh->get_sorted_members());
}
/**
 * Generate definitions for static const strings for a struct.
 *
 * @param out Stream to write to
 * @param tstruct The struct
 */
void t_cpp_generator::generate_static_const_string_definition(ofstream& out,
                                                              t_struct* tstruct) {
    generate_static_const_string_definition(out, tstruct->get_name(),
                                            tstruct->get_sorted_members());
}

void t_cpp_generator::generate_sandesh_static_seqnum_def(ofstream& out,
                                                              t_sandesh* tsandesh) {
    if (((t_base_type *)tsandesh->get_type())->is_sandesh_uve() ||
        ((t_base_type *)tsandesh->get_type())->is_sandesh_alarm()) {
      out << "tbb::atomic<uint32_t> " << tsandesh->get_name() << "::lseqnum_;" << endl;
    } else {
      out << "uint32_t " << tsandesh->get_name() << "::lseqnum_ = 1;" << endl;
    }
}

void t_cpp_generator::generate_sandesh_static_versionsig_def(ofstream& out,
                                                              t_sandesh* tsandesh) {
    out << "uint32_t " << tsandesh->get_name() << "::versionsig_ = "
        << tsandesh->get_4byte_fingerprint() << "U;" << endl << endl;
}

void t_cpp_generator::generate_sandesh_static_rate_limit_buffer_def(
                                           ofstream& out, t_sandesh* tsandesh) {
    out << "boost::circular_buffer<time_t> " << tsandesh->get_name() <<
        "::rate_limit_buffer_(Sandesh::get_send_rate_limit());" << endl << endl;
}

void t_cpp_generator::generate_sandesh_static_rate_limit_mutex_def(
                                           ofstream& out,t_sandesh* tsandesh) {
    out << "tbb::mutex " << tsandesh->get_name() << "::rate_limit_mutex_; "
        << endl << endl;
}

void t_cpp_generator::generate_sandesh_static_rate_limit_log_def(
                                           ofstream& out, t_sandesh* tsandesh) {
    out << "bool " << tsandesh->get_name() << "::do_rate_limit_drop_log_ = true;"
        << endl << endl;
}


/**
 * Makes a helper function to gen a sandesh reader.
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_http_reader(ofstream& out,
                                                   t_sandesh* tsandesh) {
    indent(out) << "#include <boost/tokenizer.hpp>" << endl;
    indent(out) << "#include <base/util.h>" << endl << endl;
    indent(out) << "bool " << tsandesh->get_name() << "::RequestFromHttp" <<
        "(const std::string& ctx, const std::string& snh_query) {" << endl;

    indent_up();

    const vector<t_field*>& fields = tsandesh->get_members();
    vector<t_field*>::const_iterator f_iter;

    indent(out) << program_name_ << "_marker = 0;" << endl;
    // Declare stack tmp variables
    out << endl <<
    indent() << "using std::string;"  << endl <<
    indent() << "boost::char_separator<char> sep(\"&\");"  << endl <<
    indent() << "boost::char_separator<char> varsep(\"=\");" << endl <<
    endl <<
    indent() << "CURL * cr = curl_easy_init();" <<
    indent() << "boost::tokenizer<boost::char_separator<char> >" <<
            "tokens(snh_query, sep);" << endl <<
    indent() << "boost::tokenizer<boost::char_separator<char> >" <<
            "::iterator it1 = tokens.begin();" << endl << endl;

    indent(out) << "string entity_name;"  << endl;
    indent(out) << "int fields_vector_size =" << fields.size() << ";" << endl;


    // Required variables aren't in __isset, so we need tmp vars to check them.
    indent(out) << "for(int i =0; i < fields_vector_size; i++)" << endl;
    scope_up(out);
    indent(out) << "if (it1 != tokens.end())" << endl;
    scope_up(out);
    indent(out) << "string tok = (*it1++).c_str();" << endl ;
    indent(out) << "boost::tokenizer<boost::char_separator<char> >" <<
                   " var(tok, varsep);" << endl;
    indent(out) << "boost::tokenizer<boost::char_separator<char> >" <<
                   "::iterator it2 = var.begin();" << endl << endl;

    indent(out) << "string tok_new = (*it2).c_str();" << endl;
    indent(out) << "if (it2 != var.end()&& ++it2 != var.end()) " << endl;
    scope_up(out);


    indent(out) << "char *unescaped = NULL;" << endl;
    indent(out) << "unescaped =  (char*) (malloc(sizeof(char)*1024));" <<endl;

    indent(out) << "std::string tmpstr;" << endl;

    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter)
    {
        t_type *ftype = (*f_iter)->get_type();
        if (ftype->is_base_type())
        {
            t_base_type *btype = static_cast<t_base_type *>(ftype);
            if (btype->is_string() || btype->is_xml())
            {
                indent(out) << "entity_name=\""  << (*f_iter)->get_name() << "\";" <<  endl;
                indent(out) << "if((tok_new.substr(0,(entity_name.length()))).compare(entity_name) == 0)" << endl;
                scope_up(out);
                indent(out) << "unescaped = curl_easy_unescape(cr, (*it2).c_str(), 0, NULL);" << endl;
                indent(out) << "tmpstr=unescaped;" << endl;
                indent(out) << (*f_iter)->get_name() << " = boost::lexical_cast<std::string>((tmpstr));" << endl;
                indent(out) << "curl_free(unescaped);" << endl;
                // delete[] unescaped;
                indent(out) << "continue;" << endl;
                scope_down(out);
            }
            else if (btype->is_uuid())
            {
                indent(out) << "entity_name=\""  << (*f_iter)->get_name() << "\";" <<  endl;
                indent(out) << "if((tok_new.substr(0,(entity_name.length()))).compare(entity_name) == 0)" << endl;
                scope_up(out);
                indent(out) << "std::stringstream ss;" << endl;
                indent(out) << "ss << *it2;" << endl;
                indent(out) << "ss >> " << (*f_iter)->get_name() << ";" << endl;
                indent(out) << "continue;" << endl;
                scope_down(out);
            }
            else if (btype->is_ipaddr())
            {
                indent(out) << "entity_name=\""  << (*f_iter)->get_name() << "\";" <<  endl;
                indent(out) << "if((tok_new.substr(0,(entity_name.length()))).compare(entity_name) == 0)" << endl;
                scope_up(out);
                indent(out) << "boost::system::error_code ec;" << endl;
                indent(out) << (*f_iter)->get_name() <<
                    " = boost::asio::ip::address::from_string((*it2), ec);" << endl;
                indent(out) << "continue;" << endl;
                scope_down(out);
            }
            else
            {
                indent(out) << "entity_name=\""  << (*f_iter)->get_name() << "\";" <<  endl;
                indent(out) << "if((tok_new.substr(0,(entity_name.length()))).compare(entity_name) == 0)" << endl;
                scope_up(out);
                assert(btype->is_integer());
                indent(out) << "stringToInteger((*it2), " << (*f_iter)->get_name() << ");" << endl;
                indent(out) << "continue;" << endl;
                scope_down(out);
            }
        }
        else
        {
            // Ignore this field
            indent(out) << "++it2;" << endl;
        }
    }
    vector<t_field*>::const_iterator f_iter2 = fields.begin();
    if (f_iter2 != fields.end())
    {
        t_type *ftype = (*f_iter2)->get_type();
        if (ftype->is_base_type())
        {
            t_base_type *btype = static_cast<t_base_type *>(ftype);
            if (btype->is_string() || btype->is_xml())
            {
                indent(out) << "entity_name=\""  << (*f_iter2)->get_name() << "\";" <<  endl;
                indent(out) << "unescaped = curl_easy_unescape(cr, (*it2).c_str(), 0, NULL);" << endl;
                indent(out) << "tmpstr=unescaped;" << endl;
                indent(out) << (*f_iter2)->get_name() << " = boost::lexical_cast<std::string>((tmpstr));" << endl;
                indent(out) << "curl_free(unescaped);" << endl;
                indent(out) << "continue;" << endl;

            }
            else if (btype->is_uuid())
            {
                indent(out) << "entity_name=\""  << (*f_iter2)->get_name() << "\";" <<  endl;
                indent(out) << "std::stringstream ss;" << endl;
                indent(out) << "ss << *it2;" << endl;
                indent(out) << "ss >> " << (*f_iter2)->get_name() << ";" << endl;
                indent(out) << "continue;" << endl;
            }
            else if (btype->is_ipaddr())
            {
                indent(out) << "entity_name=\""  << (*f_iter2)->get_name() << "\";" <<  endl;
                indent(out) << "boost::system::error_code ec;" << endl;
                indent(out) << (*f_iter2)->get_name() <<
                    " = boost::asio::ip::address::from_string((*it2), ec);" << endl;
                indent(out) << "continue;" << endl;
            }
            else
            {
                indent(out) << "entity_name=\""  << (*f_iter2)->get_name() << "\";" <<  endl;
                assert(btype->is_integer());
                indent(out) << "stringToInteger((*it2), " << (*f_iter2)->get_name() << ");" << endl;
                indent(out) << "continue;" << endl;
            }
        }
    }
    indent(out) << "free(unescaped);" << endl;
    scope_down(out);
    scope_down(out);
    scope_down(out);


    indent(out) << "set_context(ctx);" << endl;
    indent(out) << "curl_easy_cleanup(cr);" << endl;
    indent(out) << "return true;" << endl;
    indent_down();
    indent(out) <<  "}" << endl << endl;
}


/**
 * Makes a helper function to gen a sandesh UVE updater.
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_updater(ofstream& out,
                                          t_sandesh* tsandesh) {

  const vector<t_field*>& fields = tsandesh->get_members();
  vector<t_field*>::const_iterator f_iter = fields.begin();
  assert((*f_iter)->get_name() == "data");

  bool periodic_ustruct = false;

  std::map<std::string, std::string>::iterator ait;
  ait = ((*f_iter)->get_type())->annotations_.find("period");
  if (ait != ((*f_iter)->get_type())->annotations_.end()) periodic_ustruct = true;

  string dtype = type_name((*f_iter)->get_type());
  t_type* t = get_true_type((*f_iter)->get_type());
  assert(t->is_struct());

  t_struct* ts = (t_struct*)t;

  map<string,DSInfo> dsinfo;
  map<string,set<string> > rawmap;
  derived_stats_info(ts, dsinfo, rawmap);

  std::map<string, CacheAttribute> cache_attrs;
  cache_attr_info(ts, cache_attrs);

  const vector<t_field*>& sfields = ts->get_members();
  vector<t_field*>::const_iterator s_iter;

  indent(out) << "bool " << tsandesh->get_name() <<
    "::LoadUVE(SendType stype, uint32_t cycle) {" << endl;
  indent_up();
  indent(out) << "(void)cycle;" << endl;
  indent(out) << "bool is_periodic_attr = false;" << endl << endl;

  for (s_iter = sfields.begin(); s_iter != sfields.end(); ++s_iter) {
    string snm = (*s_iter)->get_name();
    if (snm == "deleted") continue;

    // For non-derived stats, setting of attribute would have also set
    // the isset flag
    map<string,DSInfo>::const_iterator ds_iter = dsinfo.find(snm);
    if (ds_iter == dsinfo.end()) {

      CacheAttribute cat = (cache_attrs.find(snm))->second;
      if (cat == MANDATORY) continue;

      indent(out) << "if (data.__isset." << snm << ") {" << endl;
      indent_up();
      if (cat == HIDDEN) {
        indent(out) << "if (stype != ST_INTROSPECT) data.__isset." <<
          snm << " = false;" << endl;
      } else if (cat == INLINE) {
        indent(out) << "if (stype == ST_PERIODIC) " <<
          "data.__isset." << snm << " = false;" << endl;
      } else assert(0);
      indent_down();
      indent(out) <<  "}" << endl << endl;
    } else {
      indent(out) << "if (data.__dsobj_" << snm << "->IsResult()) {" << endl;
      indent_up();
      CacheAttribute cat = ds_iter->second.cat_;
      if (cat == HIDDEN) {
        indent(out) << "if (stype != ST_INTROSPECT) data.__isset." <<
          snm << " = false;" << endl;
      } else if (cat == HIDDEN_PER) {
        indent(out) << "if (stype == ST_PERIODIC) {" << endl;
        indent_up();
        indent(out) << "if (cycle % " << ds_iter->second.period_ <<
          " == 0) data.__dsobj_" << snm << "->Flush(data." <<
          snm << ");" << endl;
        indent(out) << "data.__isset." << snm << " = false;" << endl;
        indent_down();
        indent(out) << "} else if (stype == ST_SYNC) data.__isset." <<
          snm << " = false;" << endl;
      } else if (cat == INLINE) {
        indent(out) << "if (stype == ST_PERIODIC) " <<
          "data.__isset." << snm << " = false;" << endl;
      } else if (cat == PERIODIC) {
        indent(out) << "if (stype == ST_PERIODIC) {" << endl;
        indent_up();

        indent(out) << "if (cycle % " << ds_iter->second.period_ <<
          " == 0) {" << endl;
        indent_up();

        indent(out) << "data.__dsobj_" << snm << "->Flush(data." <<
          snm << ");" << endl;
        indent(out) << "data.__dsobj_" << snm << "->FillResult(data." <<
          snm << ", data.__isset." << snm << ", true);" << endl;

        indent_down();
        indent(out) << "} else data.__isset." << snm << " = false;" << endl;

        indent_down();
        indent(out) << "} else if (stype == ST_SYNC) data.__isset." <<
          snm << " = false;" << endl;
      } else assert(0);

      indent(out) << "else data.__dsobj_" << snm << "->FillResult(data." <<
        snm << ", data.__isset." << snm << ", true);" <<endl;

      if (cat == PERIODIC) {
          indent(out) << "is_periodic_attr |= data.__isset." << snm << ";" << endl;
      }
      indent_down();
      indent(out) <<  "}" << endl << endl;
    }
  }
  indent(out) <<  "if (stype == ST_PERIODIC) return ((is_periodic_attr) || (data.get_deleted()));" << endl;
  indent(out) <<  "else return true;" << endl;
  indent_down();
  indent(out) <<  "}" << endl << endl;

  indent(out) << "std::map<std::string, std::string> " << tsandesh->get_name() <<
    "::_DSConf(void) {" << endl;
  indent_up();

  bool is_dsconf = false;
  for (map<string,DSInfo>::const_iterator ds_iter = dsinfo.begin();
       ds_iter != dsinfo.end(); ++ds_iter) {
    if (!is_dsconf)
      indent(out) << "std::map<std::string, std::string> _dsconf = " <<
        "boost::assign::map_list_of" << endl;
    indent(out) << "(\"" << ds_iter->first << "\", \"" <<
      ds_iter->second.annotation_ << "\")" << endl;
    is_dsconf = true;
  }
  if (is_dsconf) indent(out) << ";" << endl;
  else indent(out) << "std::map<std::string, std::string> _dsconf;" << endl;

  indent(out) << "return _dsconf;" << endl;
  indent_down();
  indent(out) <<  "}" << endl << endl;

  indent(out) << "void " << tsandesh->get_name() <<
    "::_InitDerivedStats(" << dtype <<
    " & _data, const map<string,string> & _dsconf) {" << endl;
  indent_up();

  for (map<string,DSInfo>::const_iterator ds_iter = dsinfo.begin();
       ds_iter != dsinfo.end(); ++ds_iter) {

    indent(out) << "{" << endl;
    indent_up();
    indent(out) << "map<string,string>::const_iterator _dci = _dsconf.find(\"" <<
      ds_iter->first << "\");" << endl;
    if (ds_iter->second.rmtype_ == RM_AGG) {
        indent(out) << "bool is_agg = true;" << endl;
    }  else {
        indent(out) << "bool is_agg = false;" << endl;
    }
    indent(out) << "assert(_dci != _dsconf.end());" << endl;

    indent(out) << "_data.__dsobj_" << ds_iter->first << ".reset();" << endl;

    if ((ds_iter->second.cat_ == PERIODIC) ||
        (ds_iter->second.cat_ == HIDDEN_PER)) {

      if (ds_iter->second.prealgo_.empty()) {
        indent(out) << "_data.__dsobj_" << ds_iter->first << " = boost::make_shared<" <<
          " ::contrail::sandesh::DerivedStatsPeriodicIf< ::contrail::sandesh::" <<
          ds_iter->second.algo_ << ", " <<
          ds_iter->second.rawtype_ << ", " <<
          ds_iter->second.resulttype_.substr(0, ds_iter->second.resulttype_.size() - 3) << ", " <<
          ds_iter->second.resulttype_ << "> >(_dci->second, is_agg);" << endl;
      } else {
        indent(out) << "_data.__dsobj_" << ds_iter->first << " = boost::make_shared<" <<
          " ::contrail::sandesh::DerivedStatsPeriodicAnomalyIf< ::contrail::sandesh::" <<
          ds_iter->second.algo_ << ", " <<
          ds_iter->second.rawtype_ << ", ::contrail::sandesh::" <<
          ds_iter->second.prealgo_ << ", " <<
          ds_iter->second.resulttype_ << "> >(_dci->second, is_agg);" << endl;
      }
    } else {
      indent(out) << "_data.__dsobj_" << ds_iter->first << " = boost::make_shared<" <<
        " ::contrail::sandesh::DerivedStatsIf< ::contrail::sandesh::" <<
        ds_iter->second.algo_ << ", " <<
        ds_iter->second.rawtype_ << ", " << ds_iter->second.resulttype_ <<
        "> >(_dci->second, is_agg);" << endl;
    }
    indent_down();
    indent(out) << "}" << endl;
  }
  indent_down();
  indent(out) << "}" << endl << endl;

  indent(out) << "bool " << tsandesh->get_name() <<
    "::UpdateUVE(" <<  dtype <<
      " & _data, " << dtype <<
      " & tdata, uint64_t mono_usec, SandeshLevel::type Xlevel) {" << endl;

  indent_up();

  indent(out) << "bool send = false;" << endl;

  for (s_iter = sfields.begin(); s_iter != sfields.end(); ++s_iter) {
    string snm = (*s_iter)->get_name();

    // Only set those attributes that are NOT derived stats results
    if (dsinfo.find(snm) == dsinfo.end()) {
      CacheAttribute cat = (cache_attrs.find(snm))->second;
      if ((*s_iter)->get_req() == t_field::T_OPTIONAL) {
        // don't send  non-inline attributes
        if (cat != INLINE) {
          indent(out) << "if (_data.__isset." << snm << ") { _data.__isset." <<
            snm << " = false;" << endl;
        } else {
          indent(out) << "if (_data.__isset." << snm << ") { send = true;" << endl;
        }
      } else {
        if ((snm.compare("name") == 0) || (snm.compare("proxy") == 0)) {
          indent(out) << "{" << endl;
        } else {
          indent(out) << "{ send = true;" << endl;
        }
      }
      indent_up();
      indent(out) << "tdata.set_" << snm << "(_data.get_" <<
        snm << "());" << endl;
      map<string,set<string> >::const_iterator r_iter = rawmap.find(snm);
      // Update all derivied stats of this raw attribute
      if (r_iter != rawmap.end()) {
        t_type* ratype = (*s_iter)->get_type();
        if (ratype->is_map()) {
            t_type* vtype = ((t_map*)ratype)->get_val_type();
            indent(out) << "typedef " << type_name((*s_iter)->get_type()) <<
              " _T_" << snm << ";" << endl;
            indent(out) << "std::map<string,bool> _delmap_" << snm
              << ";" << endl;
            indent(out) << "BOOST_FOREACH(const _T_" << snm <<
              "::value_type& _tp, _data.get_" << snm << "())" << endl;
            indent(out) << "  _delmap_" << snm <<
              ".insert(make_pair(_tp.first, SandeshStructDeleteTrait<" <<
              type_name(vtype) << ">::get(_tp.second)));" << endl;
        }
        set<string>::const_iterator d_iter;
        for (d_iter = r_iter->second.begin(); d_iter != r_iter->second.end(); ++d_iter) {
          map<string,DSInfo>::const_iterator c_iter = dsinfo.find(*d_iter);
          CacheAttribute dat = c_iter->second.cat_;

          if (c_iter->second.compattr_.empty()) {
            if (!c_iter->second.is_map_) {
              indent(out) << "tdata.__dsobj_" << *d_iter <<
                "->Update(_data.get_" << snm << "(), mono_usec);" << endl;
            } else {
              indent(out) << "tdata.__dsobj_" << *d_iter <<
                "->Update(_data.get_" << snm << "(), _delmap_" <<
                snm << ", mono_usec);" << endl;
            }
          } else {
            string getexpr = string("get_") + c_iter->second.compattr_ +
              string("()");
            if (!c_iter->second.subcompattr_.empty()) {
              getexpr += string(".get_") + c_iter->second.subcompattr_ +
                string("()");
            }
            if (!c_iter->second.is_map_) {
              indent(out) << "tdata.__dsobj_" << *d_iter << "->Update(_data.get_" <<
                snm << "()." << getexpr << ", mono_usec);" << endl;
            } else {
              indent(out) << "std::map<string," << c_iter->second.rawtype_ << "> temp_" <<
                *d_iter << ";" << endl;
              indent(out) << "BOOST_FOREACH(const _T_" << snm <<
                "::value_type& _tp, _data.get_" << snm << "()) {" << endl;
              indent_up();
              indent(out) << "temp_" << *d_iter <<
                ".insert(make_pair(_tp.first, _tp.second." <<
                getexpr << "));" << endl;
              indent_down();
              indent(out) <<  "}" << endl;
              indent(out) << "tdata.__dsobj_" << *d_iter << "->Update(temp_" <<
                *d_iter << ", _delmap_" << snm << ", mono_usec);" << endl;
              indent(out) << "temp_" << *d_iter << ".clear();" << endl;
            }
          }
          // If the DS is inline or mandatory
          if (dat == INLINE) {
            indent(out) << "tdata.__dsobj_" << *d_iter << "->FillResult(_data." <<
              *d_iter << ", _data.__isset." << *d_iter << ");" << endl;
            indent(out) << "if (_data.__isset." << *d_iter <<") send = true;" << endl;
          }
        }
      }

      indent_down();
      indent(out) <<  "}" << endl << endl;
    }
  }
  if (!periodic_ustruct) {
    indent(out) <<  "send = true;" << endl;
  }
  indent(out) <<  "return send;" << endl;
  indent_down();
  indent(out) <<  "}" << endl << endl;

  f_iter++;
  assert(f_iter == fields.end());
}

/**
 * Makes a helper function to gen a sandesh reader.
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_reader(ofstream& out,
                                              t_sandesh* tsandesh) {
  indent(out) <<
    "int32_t " << tsandesh->get_name() <<
    "::Read(boost::shared_ptr<contrail::sandesh::protocol::TProtocol> iprot) {"
    << endl;

  generate_common_struct_reader_body(out, tsandesh, false);

  indent(out) <<
    "}" << endl << endl;
}

/**
 * Generates the write function.
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_writer(ofstream& out,
                                              t_sandesh* tsandesh) {
  indent(out) <<
    "int32_t " << tsandesh->get_name() <<
    "::Write(boost::shared_ptr<contrail::sandesh::protocol::TProtocol> oprot) const {" <<
    endl;

  generate_common_struct_writer_body(out, tsandesh, false);

  indent(out) <<
    "}" << endl <<
    endl;
}

/**
 * Makes a helper function to gen a uve sandesh creator.
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */

void t_cpp_generator::generate_sandesh_uve_creator(
        std::ofstream& out, t_sandesh* tsandesh) {
    const vector<t_field*>& fields = tsandesh->get_members();
    vector<t_field*>::const_iterator f_iter = fields.begin();
    assert((*f_iter)->get_name() == "data");

    bool is_proxy = false;
    std::map<std::string, std::string>::iterator ait;
    ait = ((*f_iter)->get_type())->annotations_.find("period");

    std::string sname = tsandesh->get_name();
    indent(out) << "SANDESH_UVE_DEF(" << sname << "," <<
        type_name((*f_iter)->get_type());

    if (ait == ((*f_iter)->get_type())->annotations_.end()) {
      indent(out) << ", 0, 0);" << endl;
    } else {
      std::map<std::string, std::string>::iterator tmit;
      tmit = ((*f_iter)->get_type())->annotations_.find("timeout");
      if (tmit == ((*f_iter)->get_type())->annotations_.end()) {
        indent(out) << ", " << atoi(ait->second.c_str()) << ", 0);" << endl;
      } else {
        indent(out) << ", " << atoi(ait->second.c_str()) << ", " <<
          atoi(tmit->second.c_str()) << ");" << endl;
        is_proxy = true;
      }
    }

    indent(out) << "void " << sname <<
        "::Send(const " << type_name((*f_iter)->get_type()) <<
        "& cdata, SandeshLevel::type Xlevel, SandeshUVE::SendType stype," <<
        " uint32_t seqno, uint32_t cycle, std::string ctx) {" << endl;
    indent_up();
    indent(out) << sname << " *snh = new " << sname << "(seqno, cdata);" << endl;
    indent(out) << "snh->set_level(Xlevel);" << endl;
    indent(out) << "if (snh->LoadUVE(stype, cycle)) {" << endl;
    indent_up();
    indent(out) << "snh->set_context(ctx); snh->set_more(!ctx.empty());" << endl;
    indent(out) << "if (stype == SandeshUVE::ST_SYNC) snh->set_hints(snh->hints() | " <<
      "g_sandesh_constants.SANDESH_SYNC_HINT);" << endl;
    indent(out) << "snh->Dispatch();" << endl;
    indent_down();
    indent(out) << "} else snh->Release();" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;
    // Generate legacy
    indent(out) << "void " << sname <<
        "::Send(const " << type_name((*f_iter)->get_type()) <<
        "& cdata, SandeshUVE::SendType stype, uint32_t seqno," <<
        " uint32_t cycle, std::string ctx) {" << endl;
    indent_up();
    indent(out) << sname << " *snh = new " << sname << "(seqno, cdata);" << endl;
    indent(out) << "snh->set_level(SandeshLevel::SYS_NOTICE);" << endl;
    indent(out) << "if (snh->LoadUVE(stype, cycle)) {" << endl;
    indent_up();
    indent(out) << "snh->set_context(ctx); snh->set_more(!ctx.empty());" << endl;
    indent(out) << "if (stype == SandeshUVE::ST_SYNC) snh->set_hints(snh->hints() | " <<
      "g_sandesh_constants.SANDESH_SYNC_HINT);" << endl;
    indent(out) << "snh->Dispatch();" << endl;
    indent_down();
    indent(out) << "} else snh->Release();" << endl;
    indent_down();
    indent(out) << "}" << endl << endl;

    if (is_proxy) {
      indent(out) << "void " << sname <<
        "::Send(const " << type_name((*f_iter)->get_type()) <<
        "& data, SandeshLevel::type Xlevel, std::string table, " <<
        "uint64_t mono_usec, int partition) {" << endl;
      indent_up();
    } else {
      indent(out) << "void " << sname <<
        "::Send(const " << type_name((*f_iter)->get_type()) <<
        "& data, SandeshLevel::type Xlevel, std::string table, " <<
        "uint64_t mono_usec) {" << endl;
      indent_up();
      indent(out) << "int partition = -1;" << endl;
    }

    indent(out) << type_name((*f_iter)->get_type()) <<
        " & cdata = const_cast<" << type_name((*f_iter)->get_type()) <<
        " &>(data);" << endl;
    indent(out) << "uint32_t msg_seqno = lseqnum_.fetch_and_increment() + 1;" << endl;
    indent(out) << "if (!table.empty()) cdata.table_ = table;" << endl;
    indent(out) << "if (uvemap" << sname <<
      ".UpdateUVE(cdata, msg_seqno, mono_usec, partition, Xlevel)) {" << endl;
    indent_up();
    indent(out) << sname << " *snh = new " << sname << "(msg_seqno, cdata);" << endl;
    indent(out) << "snh->set_level(Xlevel);" << endl;
    indent(out) << "snh->Dispatch();" << endl;
    indent_down();
    indent(out) << "}" << endl;

    indent_down();
    indent(out) << "}" << endl << endl;
    // Generate legacy
    if (is_proxy) {
      indent(out) << "void " << sname <<
        "::Send(const " << type_name((*f_iter)->get_type()) <<
        "& data, std::string table, " <<
        "uint64_t mono_usec, int partition) {" << endl;
      indent_up();
    } else {
      indent(out) << "void " << sname <<
        "::Send(const " << type_name((*f_iter)->get_type()) <<
        "& data, std::string table, " <<
        "uint64_t mono_usec) {" << endl;
      indent_up();
      indent(out) << "int partition = -1;" << endl;
    }

    indent(out) << type_name((*f_iter)->get_type()) <<
        " & cdata = const_cast<" << type_name((*f_iter)->get_type()) <<
        " &>(data);" << endl;
    indent(out) << "uint32_t msg_seqno = lseqnum_.fetch_and_increment() + 1;" << endl;
    indent(out) << "if (!table.empty()) cdata.table_ = table;" << endl;
    indent(out) << "if (uvemap" << sname <<
      ".UpdateUVE(cdata, msg_seqno, mono_usec, partition, SandeshLevel::SYS_NOTICE)) {" << endl;
    indent_up();
    indent(out) << sname << " *snh = new " << sname << "(msg_seqno, cdata);" << endl;
    indent(out) << "snh->set_level(SandeshLevel::SYS_NOTICE);" << endl;
    indent(out) << "snh->Dispatch();" << endl;
    indent_down();
    indent(out) << "}" << endl;

    indent_down();
    indent(out) << "}" << endl << endl;
}

/**
 * Makes a helper function to gen a sandesh creator.
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_creator(ofstream& out,
                                               t_sandesh* tsandesh) {
    const t_type *t = tsandesh->get_type();
    assert(t->is_base_type());

    // Creator function name
    if (((t_base_type *)t)->is_sandesh_request()) {
        indent(out) << "extern int " << program_name_ << "_marker;" << endl;
        // Request registration
        indent(out) << "SANDESH_REGISTER_DEF_TYPE(" << tsandesh->get_name() <<
                ");" << endl << endl;
        return;
    } else if (((t_base_type *)t)->is_sandesh_trace() ||
            ((t_base_type *)t)->is_sandesh_trace_object()) {
        generate_sandesh_trace(out, tsandesh);
        return;
    } else if (((t_base_type *)t)->is_sandesh_buffer()) {
        // Buffer registration
        indent(out) << "SANDESH_REGISTER_DEF_TYPE(" << tsandesh->get_name() <<
                ");" << endl << endl;
        return;
    } else if (((t_base_type *)t)->is_sandesh_uve() ||
               ((t_base_type *)t)->is_sandesh_alarm()) {
        generate_sandesh_uve_creator(out, tsandesh);
        return;
    }
    return;
}

/**
 * finds size of a field of any type.
 */
void t_cpp_generator::generate_get_size_field(ofstream& out, t_field *tfield) {
    t_type* type = get_true_type(tfield->get_type());
    string name = tfield->get_name();
    // Handle optional elements
    if (tfield->get_req() == t_field::T_OPTIONAL) {
        out << indent() << "if (__isset." << name << ") {" <<
                endl;
        indent_up();
    }
    if (type->is_struct()) {
        generate_get_size_struct(out, (t_struct *)type, name);
    } else if (type->is_container()) {
        generate_get_size_container(out, type, name);
    } else if (type->is_string() || type->is_xml() ||
               type->is_static_const_string()) {
        out << indent() << "size += " << name << ".length();" << endl;
    } else {
        out << indent() << "size += sizeof(" + name +");" << endl;
    }
    // Handle optional elements
    if (tfield->get_req() == t_field::T_OPTIONAL) {
        indent_down();
        out << indent() << "}" << endl;
    }
}

/**
 * Logs a field of any type.
 */
void t_cpp_generator::generate_logger_field(ofstream& out,
                                            t_field *tfield,
                                            string prefix,
                                            bool log_value_only,
                                            bool no_name_log,
                                            bool for_sandesh) {
    t_type* type = get_true_type(tfield->get_type());
    string name = tfield->get_name();
    // Handle optional elements
    if (tfield->get_req() == t_field::T_OPTIONAL) {
        if (for_sandesh) {
            return;
        }
        out << indent() << "if (__isset." << name << ") {" <<
                endl;
        indent_up();
    }
    if (type->is_struct()) {
        if (no_name_log) {
            out << indent() << "Xbuf << " << prefix << "\"[ \";" << endl;
        } else {
            out << indent() << "Xbuf << " << prefix << "\"" << name << "\"" << " << \"= [ \";" << endl;
        }
        generate_logger_struct(out, (t_struct *)type, prefix, name);
        out << indent() << "Xbuf << " << "\" ]\";" << endl;
    } else if (type->is_container()) {
        if (no_name_log) {
            out << indent() << "Xbuf << " << prefix << "\"[ \";" << endl;
        } else {
            out << indent() << "Xbuf << " << prefix << "\"" << name << "\"" << " << \"= [ \";" << endl;
        }
        generate_logger_container(out, type, name, log_value_only);
        out << indent() << "Xbuf << " << "\" ]\";" << endl;
    } else if (type->is_base_type() || type->is_enum()) {
        t_base_type *tbase = static_cast<t_base_type *>(type);
        if (log_value_only) {
            if (tbase->is_integer()) {
                out << indent() << "Xbuf << " << prefix <<
                        "integerToString(" << name <<
                        ");" << endl;
            } else {
                out << indent() << "Xbuf << " << prefix <<
                        name << ";" << endl;
            }
        } else {
            if (tbase->is_integer()) {
                out << indent() << "Xbuf << " << prefix <<
                        "\"" << name << "\"" <<
                        " << \" = \" << integerToString(" << name <<
                        ");" << endl;
            } else {
                if (tbase->get_name() == "ipv4") {
                   scope_up(out);
                   out << indent() << "struct in_addr ip_addr_"<< name <<";" << endl;
                   out << indent() << "ip_addr_" << name << ".s_addr = htonl(" << name << ");" << endl;
                   out << indent() << "Xbuf << " << prefix <<
                           "\"" << name << "\"" <<
                           " << \" = \" << inet_ntoa(ip_addr_" << name << ");" << endl;
                   scope_down(out);
                }
                else {
                   out << indent() << "Xbuf << " << prefix <<
                           "\"" << name << "\"" <<
                           " << \" = \" << " << name <<
                           ";" << endl;
                }
            }
        }
    }
    // Handle optional elements
    if (tfield->get_req() == t_field::T_OPTIONAL) {
        indent_down();
        out << indent() << "}" << endl;
    }
}

/**
 * Generate code to find size of a container
 */
void t_cpp_generator::generate_get_size_container(ofstream& out,
                                                t_type* ttype,
                                                string name) {
    scope_up(out);
    string iter = tmp("_iter");
    out << indent() << type_name(ttype) << "::const_iterator " << iter
        << ";" << endl;
    out << indent() << "for (" << iter << " = " << name  << ".begin(); "
        << iter << " != " << name << ".end(); ++" << iter << ")" << endl;
    scope_up(out);
    if (ttype->is_map()) {
        generate_get_size_map_element(out, (t_map *)ttype, iter);
    } else if(ttype->is_set()) {
        generate_get_size_set_element(out, (t_set *)ttype, iter);
    } else if(ttype->is_list()) {
        generate_get_size_list_element(out, (t_list *)ttype, iter);
    }
    scope_down(out);
    scope_down(out);
}

/**
 * Generate code to find size of a map element
 */
void t_cpp_generator::generate_get_size_map_element(ofstream& out,
                                                  t_map* tmap,
                                                  string iter) {
     t_field kfield(tmap->get_key_type(), iter + "->first");
     generate_get_size_field(out, &kfield);
     t_field vfield(tmap->get_val_type(), iter + "->second");
     generate_get_size_field(out, &vfield);
}

/**
 * Generate code to find size of list element
 */
void t_cpp_generator::generate_get_size_list_element(ofstream& out,
                                                  t_list* tlist,
                                                  string iter) {
    t_field efield(tlist->get_elem_type(), "(*" + iter + ")");
    generate_get_size_field(out, &efield);
}

/**
 * Generate code to find size of set element
 */
void t_cpp_generator::generate_get_size_set_element(ofstream& out,
                                                  t_set* tset,
                                                  string iter) {
    t_field efield(tset->get_elem_type(), "(*" + iter + ")");
    generate_get_size_field(out, &efield);
}

/**
 * Generate code to log a container
 */
void t_cpp_generator::generate_logger_container(ofstream& out,
                                                t_type* ttype,
                                                string name,
                                                bool log_value_only) {
    scope_up(out);
    string iter = tmp("_iter");
    string prefix = "\" \" << ";
    out << indent() << "Xbuf << " << prefix << "\"[\";" << endl;
    out <<
            indent() << type_name(ttype) << "::const_iterator " << iter << ";" << endl <<
            indent() << "for (" << iter << " = " << name  << ".begin(); " << iter << " != " << name << ".end(); ++" << iter << ")" << endl;
    scope_up(out);
    if (ttype->is_map()) {
        generate_logger_map_element(out, (t_map*)ttype, iter, log_value_only);
    } else if (ttype->is_set()) {
        generate_logger_set_element(out, (t_set*)ttype, iter, log_value_only);
    } else if (ttype->is_list()) {
        generate_logger_list_element(out, (t_list*)ttype, iter, log_value_only);
    }
    out << indent() << "Xbuf << " << "\", \";" << endl;
    scope_down(out);
    out << indent() << "Xbuf << " << prefix << "\"]\";" << endl;
    scope_down(out);
}

/**
 * Generate code to log a map element
 */
void t_cpp_generator::generate_logger_map_element(ofstream& out,
                                                  t_map* tmap,
                                                  string iter,
                                                  bool log_value_only) {
    string prefix = "\" \" << ";
    t_field kfield(tmap->get_key_type(), iter + "->first");
    generate_logger_field(out, &kfield, prefix, log_value_only, true);
    t_field vfield(tmap->get_val_type(), iter + "->second");
    generate_logger_field(out, &vfield, prefix, log_value_only, true);
}

/**
 * Generate code to log a set element
 */
void t_cpp_generator::generate_logger_set_element(ofstream& out,
                                                  t_set* tset,
                                                  string iter,
                                                  bool log_value_only) {
    string prefix = "\" \" << ";
    t_field efield(tset->get_elem_type(), "(*" + iter + ")");
    generate_logger_field(out, &efield, prefix, log_value_only, true);
}

/**
 * Generate code to log a list element
 */
void t_cpp_generator::generate_logger_list_element(ofstream& out,
                                                   t_list* tlist,
                                                   string iter,
                                                   bool log_value_only) {
    string prefix = "\" \" << ";
    t_field efield(tlist->get_elem_type(), "(*" + iter + ")");
    generate_logger_field(out, &efield, prefix, log_value_only, true);
}

/**
 * Generate code to find size of a struct.
 */
void t_cpp_generator::generate_get_size_struct(ofstream& out,
                                             t_struct *tstruct,
                                             string name) {
    (void) tstruct;
    out << indent() << "size += " << name << ".GetSize();" << endl;
}

/**
 * Generate code to log a struct.
 */
void t_cpp_generator::generate_logger_struct(ofstream& out,
                                             t_struct *tstruct,
                                             string prefix,
                                             string name) {
    (void) tstruct;
    out << indent() << "Xbuf << " << prefix << name << ".log();" << endl;
}

/**
 * Generate static drop logger for sandesh
 *
 * @param out The output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_static_drop_logger(ofstream &out,
    t_sandesh *tsandesh, bool generate_sandesh_object) {
    // Generate DropLog
    if (generate_sandesh_object) {
        out << indent() << "static void DropLog(const std::string& " <<
            "drop_reason, std::string category, SandeshLevel::type level, " <<
            tsandesh->get_name() << " *snh) {" << endl;
    } else {
        out << indent() << "static void DropLog" <<
            generate_sandesh_async_creator(tsandesh, true, false, false, "", "",
            false, false, true) << " {" << endl;
    }
    indent_up();
    out << indent() << "log4cplus::LogLevel Xlog4level(" <<
        "SandeshLevelTolog4Level(SandeshLevel::SYS_ERR));" << endl;
    out << indent() <<
        "log4cplus::Logger Xlogger = Sandesh::logger();" << endl;
    out << indent() <<
        "if (!Xlogger.isEnabledFor(Xlog4level)) {" << endl;
    indent_up();
    out << indent() << "return;" << endl;
    scope_down(out);
    out << indent() << "log4cplus::tostringstream Xbuf;" << endl;
    // Only systemlog, objectlog, flowlog, and UVE have valid level
    // and category
    if (tsandesh->is_level_category_supported()) {
        if (generate_sandesh_object) {
            out << indent() << "Xbuf << drop_reason << snh->ToString();"
                << endl;
        } else {
            out << indent() << "Xbuf << drop_reason << category << " <<
                "\" [\" << LevelToString(level) << \"]: " <<
                tsandesh->get_name() << ": \";" << endl;
        }
    } else {
        assert(!generate_sandesh_object);
        out << indent() << "Xbuf << drop_reason << \"" <<
            tsandesh->get_name() << ": \";" << endl;
    }

    bool is_system =
        ((t_base_type *)tsandesh->get_type())->is_sandesh_system();
    bool is_trace =
        ((t_base_type *)tsandesh->get_type())->is_sandesh_trace();
    bool log_value_only = is_system || is_trace;
    bool init = false;
    string prefix;
    const vector<t_field*>& fields = tsandesh->get_members();
    vector<t_field*>::const_iterator f_iter;
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if (init) {
            prefix = "\" \" << ";
        } else {
            init = true;
        }
        if (!generate_sandesh_object) {
            generate_logger_field(out, *f_iter, prefix, log_value_only, false,
                true);
        }
    }
    out << indent() << "Xlogger.forcedLog(Xlog4level, Xbuf.str());" <<
        endl;
    indent_down();
    indent(out) << "}" << endl << endl;
}

void t_cpp_generator::generate_sandesh_static_logger(ofstream &out,
    t_sandesh *tsandesh, bool generate_sandesh_object) {
    // Generate Log
    if (generate_sandesh_object) {
        out << indent() << "static void Log(std::string category, " <<
            "SandeshLevel::type level, " << tsandesh->get_name() <<
            " *snh) {" << endl;
    } else {
        out << indent() << "static void Log" <<
            generate_sandesh_async_creator(tsandesh, true, false, false, "", "",
            false, false, false) << " {" << endl;
    }
    indent_up();
    out << indent() <<
        "if (!IsLevelCategoryLoggingAllowed(" <<
        generate_sandesh_base_name(tsandesh, true) <<
        ", level, category)) {" << endl;
    indent_up();
    out << indent() << "return;" << endl;
    scope_down(out);
    out << indent() << "log4cplus::LogLevel Xlog4level(" <<
        "SandeshLevelTolog4Level(level));" << endl;
    out << indent() <<
        "log4cplus::Logger Xlogger = Sandesh::logger();" << endl;
    out << indent() <<
        "if (!Xlogger.isEnabledFor(Xlog4level)) {" << endl;
    indent_up();
    out << indent() << "return;" << endl;
    scope_down(out);
    out << indent() << "log4cplus::tostringstream Xbuf;" << endl;
    // Only systemlog, objectlog, flowlog, and UVE have valid level
    // and category
    if (tsandesh->is_level_category_supported()) {
        if (generate_sandesh_object) {
            out << indent() << "Xbuf << snh->ToString();" << endl;
        } else {
            out << indent() << "Xbuf << category << " <<
                "\" [\" << LevelToString(level) << \"]: " <<
                tsandesh->get_name() << ": \";" << endl;
        }
    } else {
        assert(!generate_sandesh_object);
        out << indent() << "Xbuf << \"" <<
            tsandesh->get_name() << ": \";" << endl;
    }

    bool is_system =
        ((t_base_type *)tsandesh->get_type())->is_sandesh_system();
    bool is_trace =
        ((t_base_type *)tsandesh->get_type())->is_sandesh_trace();
    bool log_value_only = is_system || is_trace;
    bool init = false;
    string prefix;
    const vector<t_field*>& fields = tsandesh->get_members();
    vector<t_field*>::const_iterator f_iter;
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        if (init) {
            prefix = "\" \" << ";
        } else {
            init = true;
        }
        if (!generate_sandesh_object) {
            generate_logger_field(out, *f_iter, prefix, log_value_only, false,
                true);
        }
    }
    out << indent() << "Xlogger.forcedLog(Xlog4level, Xbuf.str());" <<
        endl;
    indent_down();
    indent(out) << "}" << endl << endl;
}

/**
 * Generate logger for sandesh
 *
 * @param out The output stream
 * @param tsandesh The sandesh
 * @param string Generate logger printing to buffer
 */
void t_cpp_generator::generate_sandesh_logger(ofstream &out,
                                              t_sandesh *tsandesh,
                                              sandesh_logger::type ltype) {
    if (ltype == sandesh_logger::BUFFER) {
        indent(out) << "std::string " << tsandesh->get_name() <<
                "::ToString() const {" << endl;
        indent_up();
    } else if (ltype == sandesh_logger::LOG) {
        indent(out) << "void " << tsandesh->get_name() <<
                "::Log() const {" << endl;
        indent_up();
    } else if (ltype == sandesh_logger::FORCED_LOG) {
        indent(out) << "void " << tsandesh->get_name() <<
                "::ForcedLog() const {" << endl;
        indent_up();
    } else {
        assert(0);
    }
    const vector<t_field*>& fields = tsandesh->get_members();
    vector<t_field*>::const_iterator f_iter;
    bool init = false;
    bool log_value_only =
            ((t_base_type *)tsandesh->get_type())->is_sandesh_system() ||
            ((t_base_type *)tsandesh->get_type())->is_sandesh_trace();
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
        string prefix = "\" \" << ";
        // Handle init
        if (!init) {
            init = true;
            string category_str = "Sandesh::category()";
            string level_str = "Sandesh::level()";
            if (ltype == sandesh_logger::LOG) {
                out << indent() << "if (!IsLevelCategoryLoggingAllowed(" <<
                    generate_sandesh_base_name(tsandesh, true) << ", " <<
                    level_str << ", " << category_str << ")) {" << endl;
                indent_up();
                out << indent() << "return;" << endl;
                scope_down(out);
                out << indent() << "log4cplus::LogLevel Xlog4level(" <<
                    "SandeshLevelTolog4Level(" << level_str << "));" << endl;
                out << indent() <<
                    "log4cplus::Logger Xlogger = Sandesh::logger();" << endl;
                out << indent() <<
                    "if (!Xlogger.isEnabledFor(Xlog4level)) {" << endl;
                indent_up();
                out << indent() << "return;" << endl;
                scope_down(out);
                out << indent() << "log4cplus::tostringstream Xbuf;" << endl;
            } else if (ltype == sandesh_logger::FORCED_LOG) {
                out << indent() << "log4cplus::LogLevel Xlog4level(" <<
                    "SandeshLevelTolog4Level(" << level_str << "));" << endl;
                out << indent() <<
                    "log4cplus::Logger Xlogger = Sandesh::logger();" << endl;
                out << indent() << "log4cplus::tostringstream Xbuf;" << endl;
            } else if (ltype == sandesh_logger::BUFFER) {
                out << indent() << "std::stringstream Xbuf;" << endl;
                // Timestamp
                out << indent() <<
                        "Xbuf << integerToString(timestamp()) << \" \";" <<
                        endl;
            }
            prefix = "";
            // Only systemlog, objectlog, and flowlog have valid level and category
            if (tsandesh->is_level_category_supported()) {
                out << indent() << "Xbuf << " << category_str <<
                    " << \" [\" << LevelToString(" << level_str << ") << \"]: "
                    << tsandesh->get_name() << ": \";" << endl;
            } else {
                out << indent() << "Xbuf << \"" << tsandesh->get_name() <<
                    ": \";" << endl;
            }
        }
        generate_logger_field(out, *f_iter, prefix, log_value_only, false);
    }
    if (init) {
        if (ltype == sandesh_logger::BUFFER) {
            out << indent() << "return Xbuf.str();" << endl;
        } else {
            out << indent() << "Xlogger.forcedLog(Xlog4level, Xbuf.str());" <<
                endl;
        }
    } else {
        if (ltype == sandesh_logger::BUFFER) {
            out << indent() << "return std::string();" << endl;
        }
    }
    indent_down();
    indent(out) << "}" << endl << endl;
}

/**
 * Generate GetSize for sandesh
 *
 * @param out The output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_get_size(ofstream& out,
                                               t_sandesh* tsandesh) {
     //Generate GetSize function to return size of sandesh
     indent(out) << "size_t " << tsandesh->get_name() <<
                "::GetSize() const {" << endl;
     indent_up();
     indent(out) << "size_t size = 0;" << endl;
     const vector<t_field*>& fields = tsandesh->get_members();
     vector<t_field*>::const_iterator f_iter;
     for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
         generate_get_size_field(out, *f_iter);
     }
     indent(out) << "return size;" << endl;
     indent_down();
     indent(out) << "}" << endl << endl;
}

/**
 * Generate loggers for sandesh
 *
 * @param out The output stream
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_loggers(ofstream& out,
                                               t_sandesh* tsandesh) {
    // Generate Log printing to log4cplus
    generate_sandesh_logger(out, tsandesh, sandesh_logger::LOG);
    // Generate forcedLog printing to log4cplus
    generate_sandesh_logger(out, tsandesh, sandesh_logger::FORCED_LOG);
    // Generate Log printing to buffer
    generate_sandesh_logger(out, tsandesh, sandesh_logger::BUFFER);
}

/**
 * Generate trace for sandesh
 *
 * @param out Stream to write to
 * @param tsandesh The sandesh
 */
void t_cpp_generator::generate_sandesh_trace(ofstream& out,
                                             t_sandesh* tsandesh) {
    std::string creator_name = "trace_sandesh";
    indent(out) << "void " << tsandesh->get_name() << "::TraceMsg" <<
            generate_sandesh_no_static_const_string_function(tsandesh, true, false, true) <<
            " {" << endl;
    indent_up();
    out << indent() << "TraceSandeshType *trace = " <<
            "TraceSandeshType::GetInstance();" << endl;
    out << indent() << "if (trace != NULL && trace->IsTraceOn() && trace_buf->IsTraceOn()) {" << endl;
    indent_up();
    out << indent() << tsandesh->get_name() << " *" << creator_name << " = " <<
            "new " << tsandesh->get_name() <<
            generate_sandesh_no_static_const_string_function(tsandesh, false, false, false, false, true) <<
            ";" << endl;
    out << indent() << creator_name << "->set_category(trace_buf->Name());" << endl;
    out << indent() << "uint32_t seqnum(trace_buf->GetNextSeqNum());" << endl;
    out << indent() << creator_name << "->set_seqnum(seqnum);" << endl;
    out << indent() << "trace_buf->TraceWrite(" << creator_name << ");" << endl;
    out << indent() << "if ((IsLocalLoggingEnabled() && IsTracePrintEnabled()) || IsUnitTest()) " << creator_name << "->Log();" << endl;
    indent_down();
    out << indent() << "}" <<endl;
    indent_down();
    indent(out) <<
            "}" << endl << endl;
}

/*
 * Generate IsRatelimitPass function
 *
 */
void t_cpp_generator::generate_isRatelimitPass(ofstream& out,
                                           t_sandesh* tsandesh) {
    out << indent() << "uint32_t send_rate_limit = Sandesh::get_send_rate_limit();" << endl;
    out << indent() << "if (send_rate_limit == 0) {" << endl;
    indent_up();
    out << indent() << "return false;" << endl;
    indent_down();
    out << indent() << "}" << endl;
    out << indent() << "tbb::mutex::scoped_lock lock(rate_limit_mutex_);" << endl;
    out << indent() << "if (rate_limit_buffer_.capacity() !="
        " send_rate_limit) {" << endl;
    indent_up();
    out << indent() << "//Resize the buffer to the "
        "buffer_threshold_" << endl;
    out << indent() << "rate_limit_buffer_.rresize(send_rate_limit);"
        << endl;
    out << indent() << "rate_limit_buffer_.set_capacity("
        "send_rate_limit);" << endl;
    indent_down();
    out << indent() << "}" << endl;
    out << indent() << "time_t current_time = time(0);" << endl;
    out << indent() << "if (rate_limit_buffer_.capacity() == rate_limit_buffer_"
        ".size()) {" << endl;
    indent_up();
    out << indent() << "if (*rate_limit_buffer_.begin() == current_time) {" << endl;
    indent_up();
    out << indent() << "//update tx and call droplog" << endl;
    out << indent() << "//Dont have to log more than once" << endl;
    out << indent() << "return false;" << endl;
    indent_down();
    out << indent() << "}" << endl;
    indent_down();
    out << indent() << "}" << endl;
    out << indent() << "//Should log failure after a sucessful write" << endl;
    out << indent() << "do_rate_limit_drop_log_ = true;" << endl;
    out << indent() << "rate_limit_buffer_.push_back(current_time);" << endl;
    out << indent() << "return true;" << endl;
}


#endif

/**
 * Struct writer for result of a function, which can have only one of its
 * fields set and does a conditional if else look up into the __isset field
 * of the struct.
 *
 * @param out Output stream
 * @param tstruct The result struct
 */
void t_cpp_generator::generate_struct_result_writer(ofstream& out,
                                                    t_struct* tstruct,
                                                    bool pointers) {
  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_sorted_members();
  vector<t_field*>::const_iterator f_iter;

  if (gen_templates_) {
    out <<
      indent() << "template <class Protocol_>" << endl <<
      indent() << "uint32_t " << tstruct->get_name() <<
      "::write(Protocol_* oprot) const {" << endl;
  } else {
    indent(out) <<
      "uint32_t " << tstruct->get_name() <<
      "::write(::contrail::sandesh::protocol::TProtocol* oprot) const {" << endl;
  }
  indent_up();

  out <<
    endl <<
    indent() << "uint32_t xfer = 0;" << endl <<
    endl;

  indent(out) <<
    "xfer += oprot->writeStructBegin(\"" << name << "\");" << endl;

  bool first = true;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
      out <<
        endl <<
        indent() << "if ";
    } else {
      out <<
        " else if ";
    }

    out << "(this->__isset." << (*f_iter)->get_name() << ") {" << endl;

    indent_up();

    // Write field header
    out <<
      indent() << "xfer += oprot->writeFieldBegin(" <<
      "\"" << (*f_iter)->get_name() << "\", " <<
      type_to_enum((*f_iter)->get_type()) << ", " <<
      (*f_iter)->get_key() << ");" << endl;
    // Write field contents
    if (pointers) {
      generate_serialize_field(out, *f_iter, "(*(this->", "))");
    } else {
      generate_serialize_field(out, *f_iter, "this->");
    }
    // Write field closer
    indent(out) << "xfer += oprot->writeFieldEnd();" << endl;

    indent_down();
    indent(out) << "}";
  }

  // Write the struct map
  out <<
    endl <<
    indent() << "xfer += oprot->writeFieldStop();" << endl <<
    indent() << "xfer += oprot->writeStructEnd();" << endl <<
    indent() << "return xfer;" << endl;

  indent_down();
  indent(out) <<
    "}" << endl <<
    endl;
}

/**
 * Generates a thrift service. In C++, this comprises an entirely separate
 * header and source file. The header file defines the methods and includes
 * the data types defined in the main header file, and the implementation
 * file contains implementations of the basic printer and default interfaces.
 *
 * @param tservice The service definition
 */
void t_cpp_generator::generate_service(t_service* tservice) {
  string svcname = tservice->get_name();

  // Make output files
  string f_header_name = get_out_dir()+svcname+".h";
  f_header_.open(f_header_name.c_str());

  // Print header file includes
  f_header_ <<
    autogen_comment();
  f_header_ <<
    "#ifndef " << svcname << "_H" << endl <<
    "#define " << svcname << "_H" << endl <<
    endl;
  if (gen_cob_style_) {
    f_header_ <<
      "#include <transport/TBufferTransports.h>" << endl << // TMemoryBuffer
      "#include <tr1/functional>" << endl <<
      "namespace apache { namespace thrift { namespace async {" << endl <<
      "class TAsyncChannel;" << endl <<
      "}}}" << endl;
  }
  f_header_ <<
    "#include <TProcessor.h>" << endl;
  if (gen_cob_style_) {
    f_header_ <<
      "#include <async/TAsyncProcessor.h>" << endl;
  }
  f_header_ <<
    "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
    "_types.h\"" << endl;

  t_service* extends_service = tservice->get_extends();
  if (extends_service != NULL) {
    f_header_ <<
      "#include \"" << get_include_prefix(*(extends_service->get_program())) <<
      extends_service->get_name() << ".h\"" << endl;
  }

  f_header_ <<
    endl <<
    ns_open_ << endl <<
    endl;

  // Service implementation file includes
  string f_service_name = get_out_dir()+svcname+".cpp";
  f_service_.open(f_service_name.c_str());
  f_service_ <<
    autogen_comment();
  f_service_ <<
    "#include \"" << get_include_prefix(*get_program()) << svcname << ".h\"" << endl;
  if (gen_cob_style_) {
    f_service_ <<
      "#include \"async/TAsyncChannel.h\"" << endl;
  }
  if (gen_templates_) {
    f_service_ <<
      "#include \"" << get_include_prefix(*get_program()) << svcname <<
      ".tcc\"" << endl;

    string f_service_tcc_name = get_out_dir()+svcname+".tcc";
    f_service_tcc_.open(f_service_tcc_name.c_str());
    f_service_tcc_ <<
      autogen_comment();
    f_service_tcc_ <<
      "#include \"" << get_include_prefix(*get_program()) << svcname <<
      ".h\"" << endl;

    f_service_tcc_ <<
      "#ifndef " << svcname << "_TCC" << endl <<
      "#define " << svcname << "_TCC" << endl <<
      endl;

    if (gen_cob_style_) {
      f_service_tcc_ <<
        "#include \"async/TAsyncChannel.h\"" << endl;
    }
  }

  f_service_ <<
    endl << ns_open_ << endl << endl;
  f_service_tcc_ <<
    endl << ns_open_ << endl << endl;

  // Generate all the components
  generate_service_interface(tservice, "");
  generate_service_interface_factory(tservice, "");
  generate_service_null(tservice, "");
  generate_service_helpers(tservice);
  generate_service_client(tservice, "");
  generate_service_processor(tservice, "");
  generate_service_multiface(tservice);
  generate_service_skeleton(tservice);

  // Generate all the cob components
  if (gen_cob_style_) {
    generate_service_interface(tservice, "CobCl");
    generate_service_interface(tservice, "CobSv");
    generate_service_interface_factory(tservice, "CobSv");
    generate_service_null(tservice, "CobSv");
    generate_service_client(tservice, "Cob");
    generate_service_processor(tservice, "Cob");
    generate_service_async_skeleton(tservice);
  }

  // Close the namespace
  f_service_ <<
    ns_close_ << endl <<
    endl;
  f_service_tcc_ <<
    ns_close_ << endl <<
    endl;
  f_header_ <<
    ns_close_ << endl <<
    endl;

  // TODO(simpkins): Make this a separate option
  if (gen_templates_) {
    f_header_ <<
      "#include \"" << get_include_prefix(*get_program()) << svcname <<
      ".tcc\"" << endl <<
      "#include \"" << get_include_prefix(*get_program()) << program_name_ <<
      "_types.tcc\"" << endl <<
      endl;
  }

  f_header_ <<
    "#endif" << endl;
  f_service_tcc_ <<
    "#endif" << endl;

  // Close the files
  f_service_tcc_.close();
  f_service_.close();
  f_header_.close();
}

/**
 * Generates helper functions for a service. Basically, this generates types
 * for all the arguments and results to functions.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_helpers(t_service* tservice) {
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* ts = (*f_iter)->get_arglist();
    string name_orig = ts->get_name();

    // TODO(dreiss): Why is this stuff not in generate_function_helpers?
    ts->set_name(tservice->get_name() + "_" + (*f_iter)->get_name() + "_args");
    generate_struct_definition(f_header_, ts, false);
    generate_struct_reader(out, ts);
    generate_struct_writer(out, ts);
    ts->set_name(tservice->get_name() + "_" + (*f_iter)->get_name() + "_pargs");
    generate_struct_definition(f_header_, ts, false, true, false, true);
    generate_struct_writer(out, ts, true);
    ts->set_name(name_orig);

    generate_function_helpers(tservice, *f_iter);
  }
}

/**
 * Generates a service interface definition.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_interface(t_service* tservice, string style) {

  string service_if_name = service_name_ + style + "If";
  if (style == "CobCl") {
    // Forward declare the client.
    string client_name = service_name_ + "CobClient";
    if (gen_templates_) {
      client_name += "T";
      service_if_name += "T";
      indent(f_header_) <<
        "template <class Protocol_>" << endl;
    }
    indent(f_header_) << "class " << client_name << ";" <<
      endl << endl;
  }

  string extends = "";
  if (tservice->get_extends() != NULL) {
    extends = " : virtual public " + type_name(tservice->get_extends()) +
      style + "If";
    if (style == "CobCl" && gen_templates_) {
      // TODO(simpkins): If gen_templates_ is enabled, we currently assume all
      // parent services were also generated with templates enabled.
      extends += "T<Protocol_>";
    }
  }

  if (style == "CobCl" && gen_templates_) {
    f_header_ << "template <class Protocol_>" << endl;
  }
  f_header_ <<
    "class " << service_if_name << extends << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << "virtual ~" << service_if_name << "() {}" << endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_header_ <<
      indent() << "virtual " << function_signature(*f_iter, style) << " = 0;" << endl;
  }
  indent_down();
  f_header_ <<
    "};" << endl << endl;

  if (style == "CobCl" && gen_templates_) {
    // generate a backwards-compatible typedef for clients that do not
    // know about the new template-style code
    f_header_ <<
      "typedef " << service_if_name <<
      "< ::contrail::sandesh::protocol::TProtocol> " <<
      service_name_ << style << "If;" <<
      endl << endl;
  }
}

/**
 * Generates a service interface factory.
 *
 * @param tservice The service to generate an interface factory for.
 */
void t_cpp_generator::generate_service_interface_factory(t_service* tservice,
                                                         string style) {
  string service_if_name = service_name_ + style + "If";

  // Figure out the name of the upper-most parent class.
  // Getting everything to work out properly with inheritance is annoying.
  // Here's what we're doing for now:
  //
  // - All handlers implement getHandler(), but subclasses use covariant return
  //   types to return their specific service interface class type.  We have to
  //   use raw pointers because of this; shared_ptr<> can't be used for
  //   covariant return types.
  //
  // - Since we're not using shared_ptr<>, we also provide a releaseHandler()
  //   function that must be called to release a pointer to a handler obtained
  //   via getHandler().
  //
  //   releaseHandler() always accepts a pointer to the upper-most parent class
  //   type.  This is necessary since the parent versions of releaseHandler()
  //   may accept any of the parent types, not just the most specific subclass
  //   type.  Implementations can use dynamic_cast to cast the pointer to the
  //   subclass type if desired.
  t_service* base_service = tservice;
  while (base_service->get_extends() != NULL) {
    base_service = base_service->get_extends();
  }
  string base_if_name = type_name(base_service) + style + "If";

  // Generate the abstract factory class
  string factory_name = service_if_name + "Factory";
  string extends;
  if (tservice->get_extends() != NULL) {
    extends = " : virtual public " + type_name(tservice->get_extends()) +
      style + "IfFactory";
  }

  f_header_ <<
    "class " << factory_name << extends << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << "typedef " << service_if_name << " Handler;" << endl <<
    endl <<
    indent() << "virtual ~" << factory_name << "() {}" << endl <<
    endl <<
    indent() << "virtual " << service_if_name << "* getHandler(" <<
      "const ::contrail::sandesh::TConnectionInfo& connInfo) = 0;" <<
    endl <<
    indent() << "virtual void releaseHandler(" << base_if_name <<
    "* /* handler */) = 0;" << endl;

  indent_down();
  f_header_ <<
    "};" << endl << endl;

  // Generate the singleton factory class
  string singleton_factory_name = service_if_name + "SingletonFactory";
  f_header_ <<
    "class " << singleton_factory_name <<
    " : virtual public " << factory_name << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << singleton_factory_name << "(const boost::shared_ptr<" <<
    service_if_name << ">& iface) : iface_(iface) {}" << endl <<
    indent() << "virtual ~" << singleton_factory_name << "() {}" << endl <<
    endl <<
    indent() << "virtual " << service_if_name << "* getHandler(" <<
      "const ::contrail::sandesh::TConnectionInfo&) {" << endl <<
    indent() << "  return iface_.get();" << endl <<
    indent() << "}" << endl <<
    indent() << "virtual void releaseHandler(" << base_if_name <<
    "* /* handler */) {}" << endl;

  f_header_ <<
    endl <<
    " protected:" << endl <<
    indent() << "boost::shared_ptr<" << service_if_name << "> iface_;" << endl;

  indent_down();
  f_header_ <<
    "};" << endl << endl;
}

/**
 * Generates a null implementation of the service.
 *
 * @param tservice The service to generate a header definition for
 */
void t_cpp_generator::generate_service_null(t_service* tservice, string style) {
  string extends = "";
  if (tservice->get_extends() != NULL) {
    extends = " , virtual public " + type_name(tservice->get_extends()) + style + "Null";
  }
  f_header_ <<
    "class " << service_name_ << style << "Null : virtual public " << service_name_ << style << "If" << extends << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << "virtual ~" << service_name_ << style << "Null() {}" << endl;
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_header_ <<
      indent() << function_signature(*f_iter, style, "", false) << " {" << endl;
    indent_up();

    t_type* returntype = (*f_iter)->get_returntype();
    t_field returnfield(returntype, "_return");

    if (style == "") {
      if (returntype->is_void() || is_complex_type(returntype)) {
        f_header_ << indent() << "return;" << endl;
      } else {
        f_header_ <<
          indent() << declare_field(&returnfield, true) << endl <<
          indent() << "return _return;" << endl;
      }
    } else if (style == "CobSv") {
      if (returntype->is_void()) {
        f_header_ << indent() << "return cob();" << endl;
    } else {
      t_field returnfield(returntype, "_return");
      f_header_ <<
        indent() << declare_field(&returnfield, true) << endl <<
        indent() << "return cob(_return);" << endl;
    }

    } else {
      throw "UNKNOWN STYLE";
    }

    indent_down();
    f_header_ <<
      indent() << "}" << endl;
  }
  indent_down();
  f_header_ <<
    "};" << endl << endl;
}

void t_cpp_generator::generate_function_call(ostream& out, t_function* tfunction, string target, string iface, string arg_prefix) {
  bool first = true;
  t_type* ret_type = get_true_type(tfunction->get_returntype());
  out << indent();
  if (!tfunction->is_oneway() && !ret_type->is_void()) {
    if (is_complex_type(ret_type)) {
      first = false;
      out << iface << "->" << tfunction->get_name() << "(" << target;
    } else {
      out << target << " = " << iface << "->" << tfunction->get_name() << "(";
    }
  } else {
    out << iface << "->" << tfunction->get_name() << "(";
  }
  const std::vector<t_field*>& fields = tfunction->get_arglist()->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      out << ", ";
    }
    out << arg_prefix << (*f_iter)->get_name();
  }
  out << ");" << endl;
}

void t_cpp_generator::generate_service_async_skeleton(t_service* tservice) {
  string svcname = tservice->get_name();

  // Service implementation file includes
  string f_skeleton_name = get_out_dir()+svcname+"_async_server.skeleton.cpp";

  string ns = namespace_prefix(tservice->get_program()->get_namespace("cpp"));

  ofstream f_skeleton;
  f_skeleton.open(f_skeleton_name.c_str());
  f_skeleton <<
    "// This autogenerated skeleton file illustrates one way to adapt a synchronous" << endl <<
    "// interface into an asynchronous interface. You should copy it to another" << endl <<
    "// filename to avoid overwriting it and rewrite as asynchronous any functions" << endl <<
    "// that would otherwise introduce unwanted latency." << endl <<
    endl <<
    "#include \"" << get_include_prefix(*get_program()) << svcname << ".h\"" << endl <<
    "#include <protocol/TBinaryProtocol.h>" << endl <<
    "#include <async/TEventServer.h>" << endl <<
    endl <<
    "using namespace ::contrail::sandesh;" << endl <<
    "using namespace ::contrail::sandesh::protocol;" << endl <<
    "using namespace ::contrail::sandesh::transport;" << endl <<
    "using namespace ::contrail::sandesh::async;" << endl <<
    endl <<
    "using boost::shared_ptr;" << endl <<
    endl;

  if (!ns.empty()) {
    f_skeleton <<
      "using namespace " << string(ns, 0, ns.size()-2) << ";" << endl <<
      endl;
  }

  f_skeleton <<
    "class " << svcname << "AsyncHandler : " <<
    "public " << svcname << "CobSvIf {" << endl <<
    " public:" << endl;
  indent_up();
  f_skeleton <<
    indent() << svcname << "AsyncHandler() {" << endl <<
    indent() << "  syncHandler_ = std::unique_ptr<" << svcname <<
                "Handler>(new " << svcname << "Handler);" << endl <<
    indent() << "  // Your initialization goes here" << endl <<
    indent() << "}" << endl;
  f_skeleton <<
    indent() << "virtual ~" << service_name_ << "AsyncHandler();" << endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_skeleton <<
      endl <<
      indent() << function_signature(*f_iter, "CobSv", "", true) << " {" << endl;
    indent_up();

    t_type* returntype = (*f_iter)->get_returntype();
    t_field returnfield(returntype, "_return");

    string target = returntype->is_void() ? "" : "_return";
    if (!returntype->is_void()) {
      f_skeleton <<
        indent() << declare_field(&returnfield, true) << endl;
    }
    generate_function_call(f_skeleton, *f_iter, target, "syncHandler_", "");
    f_skeleton << indent() << "return cob(" << target << ");" << endl;

    scope_down(f_skeleton);
  }
  f_skeleton << endl <<
    " protected:" << endl <<
    indent() << "std::unique_ptr<" << svcname << "Handler> syncHandler_;" << endl;
  indent_down();
  f_skeleton <<
    "};" << endl << endl;
}

/**
 * Generates a multiface, which is a single server that just takes a set
 * of objects implementing the interface and calls them all, returning the
 * value of the last one to be called.
 *
 * @param tservice The service to generate a multiserver for.
 */
void t_cpp_generator::generate_service_multiface(t_service* tservice) {
  // Generate the dispatch methods
  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;

  string extends = "";
  string extends_multiface = "";
  if (tservice->get_extends() != NULL) {
    extends = type_name(tservice->get_extends());
    extends_multiface = ", public " + extends + "Multiface";
  }

  string list_type = string("std::vector<boost::shared_ptr<") + service_name_ + "If> >";

  // Generate the header portion
  f_header_ <<
    "class " << service_name_ << "Multiface : " <<
    "virtual public " << service_name_ << "If" <<
    extends_multiface << " {" << endl <<
    " public:" << endl;
  indent_up();
  f_header_ <<
    indent() << service_name_ << "Multiface(" << list_type << "& ifaces) : ifaces_(ifaces) {" << endl;
  if (!extends.empty()) {
    f_header_ <<
      indent() << "  std::vector<boost::shared_ptr<" + service_name_ + "If> >::iterator iter;" << endl <<
      indent() << "  for (iter = ifaces.begin(); iter != ifaces.end(); ++iter) {" << endl <<
      indent() << "    " << extends << "Multiface::add(*iter);" << endl <<
      indent() << "  }" << endl;
  }
  f_header_ <<
    indent() << "}" << endl <<
    indent() << "virtual ~" << service_name_ << "Multiface() {}" << endl;
  indent_down();

  // Protected data members
  f_header_ <<
    " protected:" << endl;
  indent_up();
  f_header_ <<
    indent() << list_type << " ifaces_;" << endl <<
    indent() << service_name_ << "Multiface() {}" << endl <<
    indent() << "void add(boost::shared_ptr<" << service_name_ << "If> iface) {" << endl;
  if (!extends.empty()) {
    f_header_ <<
      indent() << "  " << extends << "Multiface::add(iface);" << endl;
  }
  f_header_ <<
    indent() << "  ifaces_.push_back(iface);" << endl <<
    indent() << "}" << endl;
  indent_down();

  f_header_ <<
    indent() << " public:" << endl;
  indent_up();

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    t_struct* arglist = (*f_iter)->get_arglist();
    const vector<t_field*>& args = arglist->get_members();
    vector<t_field*>::const_iterator a_iter;

    string call = string("ifaces_[i]->") + (*f_iter)->get_name() + "(";
    bool first = true;
    if (is_complex_type((*f_iter)->get_returntype())) {
      call += "_return";
      first = false;
    }
    for (a_iter = args.begin(); a_iter != args.end(); ++a_iter) {
      if (first) {
        first = false;
      } else {
        call += ", ";
      }
      call += (*a_iter)->get_name();
    }
    call += ")";

    f_header_ <<
      indent() << function_signature(*f_iter, "") << " {" << endl;
    indent_up();
    f_header_ <<
      indent() << "size_t sz = ifaces_.size();" << endl <<
      indent() << "for (size_t i = 0; i < sz; ++i) {" << endl;
    if (!(*f_iter)->get_returntype()->is_void()) {
      f_header_ <<
        indent() << "  if (i == sz - 1) {" << endl;
      if (is_complex_type((*f_iter)->get_returntype())) {
        f_header_ <<
          indent() << "    " << call << ";" << endl <<
          indent() << "    return;" << endl;
      } else {
        f_header_ <<
          indent() << "    return " << call << ";" << endl;
      }
      f_header_ <<
        indent() << "  } else {" << endl <<
        indent() << "    " << call << ";" << endl <<
        indent() << "  }" << endl;
    } else {
      f_header_ <<
        indent() << "  " << call << ";" << endl;
    }

    f_header_ <<
      indent() << "}" << endl;

    indent_down();
    f_header_ <<
      indent() << "}" << endl <<
      endl;
  }

  indent_down();
  f_header_ <<
    indent() << "};" << endl <<
    endl;
}

/**
 * Generates a service client definition.
 *
 * @param tservice The service to generate a server for.
 */
void t_cpp_generator::generate_service_client(t_service* tservice, string style) {
  string ifstyle;
  if (style == "Cob") {
    ifstyle = "CobCl";
  }

  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);
  string template_header, template_suffix, short_suffix, protocol_type, _this;
  string const prot_factory_type =
    "::contrail::sandesh::protocol::TProtocolFactory";
  if (gen_templates_) {
    template_header = "template <class Protocol_>\n";
    short_suffix = "T";
    template_suffix = "T<Protocol_>";
    protocol_type = "Protocol_";
    _this = "this->";
  } else {
    protocol_type = "::contrail::sandesh::protocol::TProtocol";
  }
  string prot_ptr = "boost::shared_ptr< " + protocol_type + ">";
  string client_suffix = "Client" + template_suffix;
  string if_suffix = "If";
  if (style == "Cob") {
    if_suffix += template_suffix;
  }

  string extends = "";
  string extends_client = "";
  if (tservice->get_extends() != NULL) {
    // TODO(simpkins): If gen_templates_ is enabled, we currently assume all
    // parent services were also generated with templates enabled.
    extends = type_name(tservice->get_extends());
    extends_client = ", public " + extends + style + client_suffix;
  }

  // Generate the header portion
  f_header_ <<
    template_header <<
    "class " << service_name_ << style << "Client" << short_suffix << " : " <<
    "virtual public " << service_name_ << ifstyle << if_suffix <<
    extends_client << " {" << endl <<
    " public:" << endl;

  indent_up();
  if (style != "Cob") {
    f_header_ <<
      indent() << service_name_ << style << "Client" << short_suffix <<
      "(" << prot_ptr << " prot) :" <<
      endl;
    if (extends.empty()) {
      f_header_ <<
        indent() << "  piprot_(prot)," << endl <<
        indent() << "  poprot_(prot) {" << endl <<
        indent() << "  iprot_ = prot.get();" << endl <<
        indent() << "  oprot_ = prot.get();" << endl <<
        indent() << "}" << endl;
    } else {
      f_header_ <<
        indent() << "  " << extends << style << client_suffix <<
        "(prot, prot) {}" << endl;
    }

    f_header_ <<
      indent() << service_name_ << style << "Client" << short_suffix <<
      "(" << prot_ptr << " iprot, " << prot_ptr << " oprot) :" << endl;
    if (extends.empty()) {
      f_header_ <<
        indent() << "  piprot_(iprot)," << endl <<
        indent() << "  poprot_(oprot) {" << endl <<
        indent() << "  iprot_ = iprot.get();" << endl <<
        indent() << "  oprot_ = oprot.get();" << endl <<
        indent() << "}" << endl;
    } else {
      f_header_ <<
        indent() << "  " << extends << style << client_suffix <<
        "(iprot, oprot) {}" << endl;
    }

    // Generate getters for the protocols.
    // Note that these are not currently templated for simplicity.
    // TODO(simpkins): should they be templated?
    f_header_ <<
      indent() << "boost::shared_ptr< ::contrail::sandesh::protocol::TProtocol> getInputProtocol() {" << endl <<
      indent() << "  return " << _this << "piprot_;" << endl <<
      indent() << "}" << endl;

    f_header_ <<
      indent() << "boost::shared_ptr< ::contrail::sandesh::protocol::TProtocol> getOutputProtocol() {" << endl <<
      indent() << "  return " << _this << "poprot_;" << endl <<
      indent() << "}" << endl;

  } else /* if (style == "Cob") */ {
    f_header_ <<
      indent() << service_name_ << style << "Client" << short_suffix << "(" <<
      "boost::shared_ptr< ::contrail::sandesh::async::TAsyncChannel> channel, " <<
      "::contrail::sandesh::protocol::TProtocolFactory* protocolFactory) :" <<
      endl;
    if (extends.empty()) {
      f_header_ <<
        indent() << "  channel_(channel)," << endl <<
        indent() << "  itrans_(new ::contrail::sandesh::transport::TMemoryBuffer())," << endl <<
        indent() << "  otrans_(new ::contrail::sandesh::transport::TMemoryBuffer())," << endl;
      if (gen_templates_) {
        // TProtocolFactory classes return generic TProtocol pointers.
        // We have to dynamic cast to the Protocol_ type we are expecting.
        f_header_ <<
          indent() << "  piprot_(boost::dynamic_pointer_cast<Protocol_>(" <<
          "protocolFactory->getProtocol(itrans_)))," << endl <<
          indent() << "  poprot_(boost::dynamic_pointer_cast<Protocol_>(" <<
          "protocolFactory->getProtocol(otrans_))) {" << endl;
        // Throw a TException if either dynamic cast failed.
        f_header_ <<
          indent() << "  if (!piprot_ || !poprot_) {" << endl <<
          indent() << "    throw ::contrail::sandesh::TException(\"" <<
          "TProtocolFactory returned unexpected protocol type in " <<
          service_name_ << style << "Client" << short_suffix <<
          " constructor\");" << endl <<
          indent() << "  }" << endl;
      } else {
        f_header_ <<
          indent() << "  piprot_(protocolFactory->getProtocol(itrans_))," <<
          endl <<
          indent() << "  poprot_(protocolFactory->getProtocol(otrans_)) {" <<
          endl;
      }
      f_header_ <<
        indent() << "  iprot_ = piprot_.get();" << endl <<
        indent() << "  oprot_ = poprot_.get();" << endl <<
        indent() << "}" << endl;
    } else {
      f_header_ <<
        indent() << "  " << extends << style << client_suffix <<
        "(channel, protocolFactory) {}" << endl;
    }
  }

  if (style == "Cob") {
    f_header_ <<
      indent() << "boost::shared_ptr< ::contrail::sandesh::async::TAsyncChannel> getChannel() {" << endl <<
      indent() << "  return " << _this << "channel_;" << endl <<
      indent() << "}" << endl;
    if (!gen_no_client_completion_) {
      f_header_ <<
        indent() << "virtual void completed__(bool /* success */) {}" << endl;
    }
  }

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::const_iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(f_header_) << function_signature(*f_iter, ifstyle) << ";" << endl;
    // TODO(dreiss): Use private inheritance to avoid generating thise in cob-style.
    t_function send_function(g_type_void,
        string("send_") + (*f_iter)->get_name(),
        (*f_iter)->get_arglist());
    indent(f_header_) << function_signature(&send_function, "") << ";" << endl;
    if (!(*f_iter)->is_oneway()) {
      t_struct noargs(program_);
      t_function recv_function((*f_iter)->get_returntype(),
          string("recv_") + (*f_iter)->get_name(),
          &noargs);
      indent(f_header_) << function_signature(&recv_function, "") << ";" << endl;
    }
  }
  indent_down();

  if (extends.empty()) {
    f_header_ <<
      " protected:" << endl;
    indent_up();

    if (style == "Cob") {
      f_header_ <<
        indent() << "boost::shared_ptr< ::contrail::sandesh::async::TAsyncChannel> channel_;"  << endl <<
        indent() << "boost::shared_ptr< ::contrail::sandesh::transport::TMemoryBuffer> itrans_;"  << endl <<
        indent() << "boost::shared_ptr< ::contrail::sandesh::transport::TMemoryBuffer> otrans_;"  << endl;
    }
    f_header_ <<
      indent() << prot_ptr << " piprot_;"  << endl <<
      indent() << prot_ptr << " poprot_;"  << endl <<
      indent() << protocol_type << "* iprot_;"  << endl <<
      indent() << protocol_type << "* oprot_;"  << endl;

    indent_down();
  }

  f_header_ <<
    "};" << endl <<
    endl;

  if (gen_templates_) {
    // Output a backwards compatibility typedef using
    // TProtocol as the template parameter.
    f_header_ <<
      "typedef " << service_name_ << style <<
      "ClientT< ::contrail::sandesh::protocol::TProtocol> " <<
      service_name_ << style << "Client;" << endl <<
      endl;
  }

  string scope = service_name_ + style + client_suffix + "::";

  // Generate client method implementations
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    string funname = (*f_iter)->get_name();

    // Open function
    if (gen_templates_) {
      indent(out) << template_header;
    }
    indent(out) <<
      function_signature(*f_iter, ifstyle, scope) << endl;
    scope_up(out);
    indent(out) <<
      "send_" << funname << "(";

    // Get the struct of function call params
    t_struct* arg_struct = (*f_iter)->get_arglist();

    // Declare the function arguments
    const vector<t_field*>& fields = arg_struct->get_members();
    vector<t_field*>::const_iterator fld_iter;
    bool first = true;
    for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      out << (*fld_iter)->get_name();
    }
    out << ");" << endl;

    if (style != "Cob") {
      if (!(*f_iter)->is_oneway()) {
        out << indent();
        if (!(*f_iter)->get_returntype()->is_void()) {
          if (is_complex_type((*f_iter)->get_returntype())) {
            out << "recv_" << funname << "(_return);" << endl;
          } else {
            out << "return recv_" << funname << "();" << endl;
          }
        } else {
          out <<
            "recv_" << funname << "();" << endl;
        }
      }
    } else {
      if (!(*f_iter)->is_oneway()) {
        out <<
          indent() << _this << "channel_->sendAndRecvMessage(" <<
          "std::tr1::bind(cob, this), " << _this << "otrans_.get(), " <<
          _this << "itrans_.get());" << endl;
      } else {
        out <<
        indent() << _this << "channel_->sendMessage(" <<
          "std::tr1::bind(cob, this), " << _this << "otrans_.get());" << endl;
      }
    }
    scope_down(out);
    out << endl;

    //if (style != "Cob") // TODO(dreiss): Libify the client and don't generate this for cob-style
    if (true) {
      // Function for sending
      t_function send_function(g_type_void,
                               string("send_") + (*f_iter)->get_name(),
                               (*f_iter)->get_arglist());

      // Open the send function
      if (gen_templates_) {
        indent(out) << template_header;
      }
      indent(out) <<
        function_signature(&send_function, "", scope) << endl;
      scope_up(out);

      // Function arguments and results
      string argsname = tservice->get_name() + "_" + (*f_iter)->get_name() + "_pargs";
      string resultname = tservice->get_name() + "_" + (*f_iter)->get_name() + "_presult";

      // Serialize the request
      out <<
        indent() << "int32_t cseqid = 0;" << endl <<
        indent() << _this << "oprot_->writeMessageBegin(\"" <<
        (*f_iter)->get_name() <<
        "\", ::contrail::sandesh::protocol::T_CALL, cseqid);" << endl <<
        endl <<
        indent() << argsname << " args;" << endl;

      for (fld_iter = fields.begin(); fld_iter != fields.end(); ++fld_iter) {
        out <<
          indent() << "args." << (*fld_iter)->get_name() << " = &" << (*fld_iter)->get_name() << ";" << endl;
      }

      out <<
        indent() << "args.write(" << _this << "oprot_);" << endl <<
        endl <<
        indent() << _this << "oprot_->writeMessageEnd();" << endl <<
        indent() << _this << "oprot_->getTransport()->writeEnd();" << endl <<
        indent() << _this << "oprot_->getTransport()->flush();" << endl;

      scope_down(out);
      out << endl;

      // Generate recv function only if not an oneway function
      if (!(*f_iter)->is_oneway()) {
        t_struct noargs(program_);
        t_function recv_function((*f_iter)->get_returntype(),
                                 string("recv_") + (*f_iter)->get_name(),
                                 &noargs);
        // Open the recv function
        if (gen_templates_) {
          indent(out) << template_header;
        }
        indent(out) <<
          function_signature(&recv_function, "", scope) << endl;
        scope_up(out);

        out <<
          endl <<
          indent() << "int32_t rseqid = 0;" << endl <<
          indent() << "std::string fname;" << endl <<
          indent() << "::contrail::sandesh::protocol::TMessageType mtype;" << endl;
        if (style == "Cob" && !gen_no_client_completion_) {
          out <<
            indent() << "bool completed = false;" << endl << endl <<
            indent() << "try {";
          indent_up();
        }
        out << endl <<
          indent() << _this << "iprot_->readMessageBegin(fname, mtype, rseqid);" << endl <<
          indent() << "if (mtype == ::contrail::sandesh::protocol::T_EXCEPTION) {" << endl <<
          indent() << "  ::contrail::sandesh::TApplicationException x;" << endl <<
          indent() << "  x.read(" << _this << "iprot_);" << endl <<
          indent() << "  " << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << "  " << _this << "iprot_->getTransport()->readEnd();" <<
          endl;
        if (style == "Cob" && !gen_no_client_completion_) {
          out <<
            indent() << "  completed = true;" << endl <<
            indent() << "  completed__(true);" << endl;
        }
        out <<
          indent() << "  throw x;" << endl <<
          indent() << "}" << endl <<
          indent() << "if (mtype != ::contrail::sandesh::protocol::T_REPLY) {" << endl <<
          indent() << "  " << _this << "iprot_->skip(" <<
          "::contrail::sandesh::protocol::T_STRUCT);" << endl <<
          indent() << "  " << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << "  " << _this << "iprot_->getTransport()->readEnd();" <<
          endl;
        if (style == "Cob" && !gen_no_client_completion_) {
          out <<
            indent() << "  completed = true;" << endl <<
            indent() << "  completed__(false);" << endl;
        }
        out <<
          indent() << "}" << endl <<
          indent() << "if (fname.compare(\"" << (*f_iter)->get_name() << "\") != 0) {" << endl <<
          indent() << "  " << _this << "iprot_->skip(" <<
          "::contrail::sandesh::protocol::T_STRUCT);" << endl <<
          indent() << "  " << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << "  " << _this << "iprot_->getTransport()->readEnd();" <<
          endl;
        if (style == "Cob" && !gen_no_client_completion_) {
          out <<
            indent() << "  completed = true;" << endl <<
            indent() << "  completed__(false);" << endl;
        }
        out <<
          indent() << "}" << endl;

        if (!(*f_iter)->get_returntype()->is_void() &&
            !is_complex_type((*f_iter)->get_returntype())) {
          t_field returnfield((*f_iter)->get_returntype(), "_return");
          out <<
            indent() << declare_field(&returnfield) << endl;
        }

        out <<
          indent() << resultname << " result;" << endl;

        if (!(*f_iter)->get_returntype()->is_void()) {
          out <<
            indent() << "result.success = &_return;" << endl;
        }

        out <<
          indent() << "result.read(" << _this << "iprot_);" << endl <<
          indent() << _this << "iprot_->readMessageEnd();" << endl <<
          indent() << _this << "iprot_->getTransport()->readEnd();" << endl <<
          endl;

        // Careful, only look for _result if not a void function
        if (!(*f_iter)->get_returntype()->is_void()) {
          if (is_complex_type((*f_iter)->get_returntype())) {
            out <<
              indent() << "if (result.__isset.success) {" << endl <<
              indent() << "  // _return pointer has now been filled" << endl;
            if (style == "Cob" && !gen_no_client_completion_) {
              out <<
                indent() << "  completed = true;" << endl <<
                indent() << "  completed__(true);" << endl;
            }
            out <<
              indent() << "  return;" << endl <<
              indent() << "}" << endl;
          } else {
            out <<
              indent() << "if (result.__isset.success) {" << endl;
            if (style == "Cob" && !gen_no_client_completion_) {
              out <<
                indent() << "  completed = true;" << endl <<
                indent() << "  completed__(true);" << endl;
            }
            out <<
              indent() << "  return _return;" << endl <<
              indent() << "}" << endl;
          }
        }

        t_struct* xs = (*f_iter)->get_xceptions();
        const std::vector<t_field*>& xceptions = xs->get_members();
        vector<t_field*>::const_iterator x_iter;
        for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
          out <<
            indent() << "if (result.__isset." << (*x_iter)->get_name() << ") {" << endl;
          if (style == "Cob" && !gen_no_client_completion_) {
            out <<
              indent() << "  completed = true;" << endl <<
              indent() << "  completed__(true);" << endl;
          }
          out  <<
            indent() << "  throw result." << (*x_iter)->get_name() << ";" << endl <<
            indent() << "}" << endl;
        }

        // We only get here if we are a void function
        if ((*f_iter)->get_returntype()->is_void()) {
          if (style == "Cob" && !gen_no_client_completion_) {
            out <<
              indent() << "completed = true;" << endl <<
              indent() << "completed__(true);" << endl;
          }
          indent(out) <<
            "return;" << endl;
        } else {
          if (style == "Cob" && !gen_no_client_completion_) {
            out <<
              indent() << "completed = true;" << endl <<
              indent() << "completed__(true);" << endl;
          }
          out <<
            indent() << "throw ::contrail::sandesh::TApplicationException(::contrail::sandesh::TApplicationException::MISSING_RESULT, \"" << (*f_iter)->get_name() << " failed: unknown result\");" << endl;
        }
        if (style == "Cob" && !gen_no_client_completion_) {
          indent_down();
          out <<
            indent() << "} catch (...) {" << endl <<
            indent() << "  if (!completed) {" << endl <<
            indent() << "    completed__(false);" << endl <<
            indent() << "  }" << endl <<
            indent() << "  throw;" << endl <<
            indent() << "}" << endl;
        }
        // Close function
        scope_down(out);
        out << endl;
      }
    }
  }
}

class ProcessorGenerator {
 public:
  ProcessorGenerator(t_cpp_generator* generator, t_service* service,
                     const string& style);

  void run() {
    generate_class_definition();
    // Generate the process() function
    generate_process();
    // Generate the process_fn() function
    generate_process_fn();
    // Generate all of the process subfunctions
    generate_process_functions();

    generate_factory();
  }

  void generate_class_definition();
  void generate_process();
  void generate_process_fn();
  void generate_process_functions();
  void generate_factory();

 protected:
  std::string type_name(t_type* ttype, bool in_typedef=false, bool arg=false) {
    return generator_->type_name(ttype, in_typedef, arg);
  }

  std::string indent() {
    return generator_->indent();
  }
  std::ostream& indent(std::ostream &os) {
    return generator_->indent(os);
  }

  void indent_up() {
    generator_->indent_up();
  }
  void indent_down() {
    generator_->indent_down();
  }

  t_cpp_generator* generator_;
  t_service* service_;
  std::ofstream& f_header_;
  std::ofstream& f_out_;
  string service_name_;
  string style_;
  string pstyle_;
  string class_name_;
  string if_name_;
  string factory_class_name_;
  string finish_cob_;
  string finish_cob_decl_;
  string ret_type_;
  string call_context_;
  string cob_arg_;
  string call_context_arg_;
  string call_context_decl_;
  string template_header_;
  string template_suffix_;
  string typename_str_;
  string class_suffix_;
  string extends_;
  string extends_processor_;
};

ProcessorGenerator::ProcessorGenerator(t_cpp_generator* generator,
                                       t_service* service,
                                       const string& style)
  : generator_(generator),
    service_(service),
    f_header_(generator->f_header_),
    f_out_(generator->gen_templates_ ?
           generator->f_service_tcc_ : generator->f_service_),
    service_name_(generator->service_name_),
    style_(style) {
  if (style_ == "Cob") {
    pstyle_ = "Async";
    class_name_ = service_name_ + pstyle_ + "Processor";
    if_name_ = service_name_ + "CobSvIf";

    finish_cob_ = "std::tr1::function<void(bool ok)> cob, ";
    finish_cob_decl_ = "std::tr1::function<void(bool ok)>, ";
    cob_arg_ = "cob, ";
    ret_type_ = "void ";
  } else {
    class_name_ = service_name_ + "Processor";
    if_name_ = service_name_ + "If";

    ret_type_ = "bool ";
    // TODO(edhall) callContext should eventually be added to TAsyncProcessor
    call_context_ = ", void* callContext";
    call_context_arg_ = ", callContext";
    call_context_decl_ = ", void*";
  }

  factory_class_name_ = class_name_ + "Factory";

  if (generator->gen_templates_) {
    template_header_ = "template <class Protocol_>\n";
    template_suffix_ = "<Protocol_>";
    typename_str_ = "typename ";
    class_name_ += "T";
    factory_class_name_ += "T";
  }

  if (service_->get_extends() != NULL) {
    extends_ = type_name(service_->get_extends()) + pstyle_ + "Processor";
    if (generator_->gen_templates_) {
      // TODO(simpkins): If gen_templates_ is enabled, we currently assume all
      // parent services were also generated with templates enabled.
      extends_ += "T<Protocol_>";
    }
    extends_processor_ = extends_;
  } else {
    extends_processor_ = "::contrail::sandesh::T" + pstyle_ + "Processor";
  }
}

void ProcessorGenerator::generate_class_definition() {
  // Generate the dispatch methods
  vector<t_function*> functions = service_->get_functions();
  vector<t_function*>::iterator f_iter;

  // Generate the header portion
  f_header_ <<
    template_header_ <<
    "class " << class_name_ <<
    " : public " << extends_processor_ << " {" << endl;

  // Protected data members
  f_header_ <<
    " protected:" << endl;
  indent_up();
  f_header_ <<
    indent() << "boost::shared_ptr<" << if_name_ << "> iface_;" << endl;
  f_header_ <<
    indent() << "virtual " << ret_type_ << "process_fn(" << finish_cob_ << "contrail::sandesh::protocol::TProtocol* iprot, contrail::sandesh::protocol::TProtocol* oprot, std::string& fname, int32_t seqid" << call_context_ << ");" << endl;
  indent_down();

  // Process function declarations
  f_header_ <<
    " private:" << endl;
  indent_up();
  f_header_ <<
    indent() << "std::map<std::string, void (" <<
    class_name_ << "::*)(" << finish_cob_decl_ <<
    "int32_t, contrail::sandesh::protocol::TProtocol*, " <<
    "contrail::sandesh::protocol::TProtocol*" << call_context_decl_ <<
    ")> processMap_;" << endl;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    indent(f_header_) <<
      "void process_" << (*f_iter)->get_name() << "(" << finish_cob_ << "int32_t seqid, contrail::sandesh::protocol::TProtocol* iprot, contrail::sandesh::protocol::TProtocol* oprot" << call_context_ << ");" << endl;
    if (generator_->gen_templates_) {
      indent(f_header_) <<
        "void process_" << (*f_iter)->get_name() << "(" << finish_cob_ <<
        "int32_t seqid, Protocol_* iprot, Protocol_* oprot" <<
        call_context_ << ");" << endl;
    }
    if (style_ == "Cob") {
      // XXX Factor this out, even if it is a pain.
      string ret_arg = ((*f_iter)->get_returntype()->is_void()
                        ? ""
                        : ", const " + type_name((*f_iter)->get_returntype()) + "& _return");
      f_header_ <<
        indent() << "void return_" << (*f_iter)->get_name() <<
        "(std::tr1::function<void(bool ok)> cob, int32_t seqid, " <<
        "::contrail::sandesh::protocol::TProtocol* oprot, " <<
        "void* ctx" << ret_arg << ");" << endl;
      if (generator_->gen_templates_) {
        f_header_ <<
          indent() << "void return_" << (*f_iter)->get_name() <<
          "(std::tr1::function<void(bool ok)> cob, int32_t seqid, " <<
          "Protocol_* oprot, void* ctx" << ret_arg << ");" << endl;
      }
      // XXX Don't declare throw if it doesn't exist
      f_header_ <<
        indent() << "void throw_" << (*f_iter)->get_name() <<
        "(std::tr1::function<void(bool ok)> cob, int32_t seqid, " <<
        "contrail::sandesh::protocol::TProtocol* oprot, void* ctx, " <<
        "contrail::sandesh::TDelayedException* _throw);" << endl;
      if (generator_->gen_templates_) {
        f_header_ <<
          indent() << "void throw_" << (*f_iter)->get_name() <<
          "(std::tr1::function<void(bool ok)> cob, int32_t seqid, " <<
          "Protocol_* oprot, void* ctx, " <<
          "::contrail::sandesh::TDelayedException* _throw);" << endl;
      }
    }
  }
  indent_down();

  indent_up();
  string declare_map = "";
  indent_up();

  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    declare_map += indent();
    declare_map += "processMap_[\"";
    declare_map += (*f_iter)->get_name();
    declare_map += "\"] = &";
    declare_map += class_name_;
    declare_map += "::process_";
    declare_map += (*f_iter)->get_name();
    declare_map += ";\n";
  }
  indent_down();

  f_header_ <<
    " public:" << endl <<
    indent() << class_name_ <<
    "(boost::shared_ptr<" << if_name_ << "> iface) :" << endl;
  if (extends_.empty()) {
    f_header_ <<
      indent() << "  iface_(iface) {" << endl;
  } else {
    f_header_ <<
      indent() << "  " << extends_ << "(iface)," << endl <<
      indent() << "  iface_(iface) {" << endl;
  }
  f_header_ <<
    declare_map <<
    indent() << "}" << endl <<
    endl <<
    indent() << "virtual " << ret_type_ << "process(" << finish_cob_ << "boost::shared_ptr<contrail::sandesh::protocol::TProtocol> piprot, boost::shared_ptr<contrail::sandesh::protocol::TProtocol> poprot" << call_context_ << ");" << endl <<
    indent() << "virtual ~" << class_name_ <<
    "() {}" << endl;
  indent_down();
  f_header_ <<
    "};" << endl << endl;

  if (generator_->gen_templates_) {
    // Generate a backwards compatible typedef, for callers who don't know
    // about the new template-style code.
    //
    // We can't use TProtocol as the template parameter, since ProcessorT
    // provides overloaded versions of most methods, one of which accepts
    // TProtocol pointers, and one which accepts Protocol_ pointers.  This
    // results in a compile error if instantiated with Protocol_ == TProtocol.
    // Therefore, we define TDummyProtocol solely so we can use it as the
    // template parameter here.
    f_header_ <<
      "typedef " << class_name_ <<
      "<contrail::sandesh::protocol::TDummyProtocol> " <<
      service_name_ << pstyle_ << "Processor;" << endl << endl;
  }
}

void ProcessorGenerator::generate_process() {
  f_out_ <<
    template_header_ <<
    ret_type_ << class_name_ <<
    template_suffix_ << "::process(" << finish_cob_ <<
    "boost::shared_ptr<contrail::sandesh::protocol::TProtocol> piprot, " <<
    "boost::shared_ptr<contrail::sandesh::protocol::TProtocol> poprot" <<
    call_context_ << ") {" << endl;
  indent_up();

  f_out_ <<
    endl <<
    indent() << "::contrail::sandesh::protocol::TProtocol* iprot = piprot.get();" << endl <<
    indent() << "::contrail::sandesh::protocol::TProtocol* oprot = poprot.get();" << endl <<
    indent() << "std::string fname;" << endl <<
    indent() << "::contrail::sandesh::protocol::TMessageType mtype;" << endl <<
    indent() << "int32_t seqid;" << endl <<
    endl <<
    indent() << "iprot->readMessageBegin(fname, mtype, seqid);" << endl <<
    endl <<
    indent() << "if (mtype != ::contrail::sandesh::protocol::T_CALL && mtype != ::contrail::sandesh::protocol::T_ONEWAY) {" << endl <<
    indent() << "  iprot->skip(::contrail::sandesh::protocol::T_STRUCT);" << endl <<
    indent() << "  iprot->readMessageEnd();" << endl <<
    indent() << "  iprot->getTransport()->readEnd();" << endl <<
    indent() << "  ::contrail::sandesh::TApplicationException x(::contrail::sandesh::TApplicationException::INVALID_MESSAGE_TYPE);" << endl <<
    indent() << "  oprot->writeMessageBegin(fname, ::contrail::sandesh::protocol::T_EXCEPTION, seqid);" << endl <<
    indent() << "  x.write(oprot);" << endl <<
    indent() << "  oprot->writeMessageEnd();" << endl <<
    indent() << "  oprot->getTransport()->writeEnd();" << endl <<
    indent() << "  oprot->getTransport()->flush();" << endl <<
    indent() << (style_ == "Cob" ? "  return cob(true);" : "  return true;") << endl <<
    indent() << "}" << endl <<
    endl <<
    indent() << "return process_fn(" << (style_ == "Cob" ? "cob, " : "")
             << "iprot, oprot, fname, seqid" << call_context_arg_ << ");" <<
    endl;

  indent_down();
  f_out_ <<
    indent() << "}" << endl <<
    endl;
}

void ProcessorGenerator::generate_process_fn() {
  f_out_ <<
    template_header_ <<
    ret_type_ << class_name_ <<
    template_suffix_ << "::process_fn(" << finish_cob_ <<
    "contrail::sandesh::protocol::TProtocol* iprot, " <<
    "contrail::sandesh::protocol::TProtocol* oprot, " <<
    "std::string& fname, int32_t seqid" << call_context_ << ") {" << endl;
  indent_up();

  // HOT: member function pointer map
  f_out_ <<
    indent() << typename_str_ << "std::map<std::string, void (" <<
    class_name_ << "::*)(" << finish_cob_decl_ <<
    "int32_t, contrail::sandesh::protocol::TProtocol*, " <<
    "contrail::sandesh::protocol::TProtocol*" << call_context_decl_ << ")>::iterator pfn;" << endl <<
    indent() << "pfn = processMap_.find(fname);" << endl <<
    indent() << "if (pfn == processMap_.end()) {" << endl;
  if (extends_.empty()) {
    f_out_ <<
      indent() << "  iprot->skip(contrail::sandesh::protocol::T_STRUCT);" << endl <<
      indent() << "  iprot->readMessageEnd();" << endl <<
      indent() << "  iprot->getTransport()->readEnd();" << endl <<
      indent() << "  ::contrail::sandesh::TApplicationException x(::contrail::sandesh::TApplicationException::UNKNOWN_METHOD, \"Invalid method name: '\"+fname+\"'\");" << endl <<
      indent() << "  oprot->writeMessageBegin(fname, ::contrail::sandesh::protocol::T_EXCEPTION, seqid);" << endl <<
      indent() << "  x.write(oprot);" << endl <<
      indent() << "  oprot->writeMessageEnd();" << endl <<
      indent() << "  oprot->getTransport()->writeEnd();" << endl <<
      indent() << "  oprot->getTransport()->flush();" << endl <<
      indent() << (style_ == "Cob" ? "  return cob(true);" : "  return true;") << endl;
  } else {
    f_out_ <<
      indent() << "  return "
               << extends_ << "::process_fn("
               << (style_ == "Cob" ? "cob, " : "")
               << "iprot, oprot, fname, seqid" << call_context_arg_ << ");" << endl;
  }
  f_out_ <<
    indent() << "}" << endl <<
    indent() << "(this->*(pfn->second))(" << cob_arg_ << "seqid, iprot, oprot" << call_context_arg_ << ");" << endl;

  // TODO(dreiss): return pfn ret?
  if (style_ == "Cob") {
    f_out_ <<
      indent() << "return;" << endl;
  } else {
    f_out_ <<
      indent() << "return true;" << endl;
  }

  indent_down();
  f_out_ <<
    "}" << endl <<
    endl;
}

void ProcessorGenerator::generate_process_functions() {
  vector<t_function*> functions = service_->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    if (generator_->gen_templates_) {
      generator_->generate_process_function(service_, *f_iter, style_, false);
      generator_->generate_process_function(service_, *f_iter, style_, true);
    } else {
      generator_->generate_process_function(service_, *f_iter, style_, false);
    }
  }
}

void ProcessorGenerator::generate_factory() {
  string if_factory_name = if_name_ + "Factory";

  // Generate the factory class definition
  f_header_ <<
    template_header_ <<
    "class " << factory_class_name_ <<
      " : public ::contrail::sandesh::" <<
        (style_ == "Cob" ? "async::TAsyncProcessorFactory" :  "TProcessorFactory") <<
        " {" << endl <<
    " public:" << endl;
  indent_up();

  f_header_ <<
    indent() << factory_class_name_ << "(const ::boost::shared_ptr< " <<
      if_factory_name << " >& handlerFactory) :" << endl <<
    indent() << "    handlerFactory_(handlerFactory) {}" << endl <<
    endl <<
    indent() << "::boost::shared_ptr< ::contrail::sandesh::" <<
      (style_ == "Cob" ? "async::TAsyncProcessor" :  "TProcessor") << " > " <<
      "getProcessor(const ::contrail::sandesh::TConnectionInfo& connInfo);" <<
      endl;

  f_header_ <<
    endl <<
    " protected:" << endl <<
    indent() << "::boost::shared_ptr< " << if_factory_name <<
      " > handlerFactory_;" << endl;

  indent_down();
  f_header_ <<
    "};" << endl << endl;

  // If we are generating templates, output a typedef for the plain
  // factory name.
  if (generator_->gen_templates_) {
    f_header_ <<
      "typedef " << factory_class_name_ <<
      "< ::contrail::sandesh::protocol::TDummyProtocol > " <<
      service_name_ << pstyle_ << "ProcessorFactory;" << endl << endl;
  }

  // Generate the getProcessor() method
  f_out_ <<
    template_header_ <<
    indent() << "::boost::shared_ptr< ::contrail::sandesh::" <<
      (style_ == "Cob" ? "async::TAsyncProcessor" :  "TProcessor") << " > " <<
      factory_class_name_ << template_suffix_ << "::getProcessor(" <<
      "const ::contrail::sandesh::TConnectionInfo& connInfo) {" << endl;
  indent_up();

  f_out_ <<
    indent() << "::contrail::sandesh::ReleaseHandler< " << if_factory_name <<
      " > cleanup(handlerFactory_);" << endl <<
    indent() << "::boost::shared_ptr< " << if_name_ << " > handler(" <<
      "handlerFactory_->getHandler(connInfo), cleanup);" << endl <<
    indent() << "::boost::shared_ptr< ::contrail::sandesh::" <<
      (style_ == "Cob" ? "async::TAsyncProcessor" :  "TProcessor") << " > " <<
      "processor(new " << class_name_ << template_suffix_ <<
      "(handler));" << endl <<
    indent() << "return processor;" << endl;

  indent_down();
  f_out_ <<
    indent() << "}" << endl;
}

/**
 * Generates a service processor definition.
 *
 * @param tservice The service to generate a processor for.
 */
void t_cpp_generator::generate_service_processor(t_service* tservice,
                                                 string style) {
  ProcessorGenerator generator(this, tservice, style);
  generator.run();
}

/**
 * Generates a struct and helpers for a function.
 *
 * @param tfunction The function
 */
void t_cpp_generator::generate_function_helpers(t_service* tservice,
                                                t_function* tfunction) {
  if (tfunction->is_oneway()) {
    return;
  }

  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);

  t_struct result(program_, tservice->get_name() + "_" + tfunction->get_name() + "_result");
  t_field success(tfunction->get_returntype(), "success", 0);
  if (!tfunction->get_returntype()->is_void()) {
    result.append(&success);
  }

  t_struct* xs = tfunction->get_xceptions();
  const vector<t_field*>& fields = xs->get_members();
  vector<t_field*>::const_iterator f_iter;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    result.append(*f_iter);
  }

  generate_struct_definition(f_header_, &result, false);
  generate_struct_reader(out, &result);
  generate_struct_result_writer(out, &result);

  result.set_name(tservice->get_name() + "_" + tfunction->get_name() + "_presult");
  generate_struct_definition(f_header_, &result, false, true, true, gen_cob_style_);
  generate_struct_reader(out, &result, true);
  if (gen_cob_style_) {
    generate_struct_writer(out, &result, true);
  }

}

/**
 * Generates a process function definition.
 *
 * @param tfunction The function to write a dispatcher for
 */
void t_cpp_generator::generate_process_function(t_service* tservice,
                                                t_function* tfunction,
                                                string style,
                                                bool specialized) {
  t_struct* arg_struct = tfunction->get_arglist();
  const std::vector<t_field*>& fields = arg_struct->get_members();
  vector<t_field*>::const_iterator f_iter;

  t_struct* xs = tfunction->get_xceptions();
  const std::vector<t_field*>& xceptions = xs->get_members();
  vector<t_field*>::const_iterator x_iter;
  string service_func_name = tservice->get_name() + "." +
    tfunction->get_name();

  std::ofstream& out = (gen_templates_ ? f_service_tcc_ : f_service_);

  string prot_type =
    (specialized ? "Protocol_" : "::contrail::sandesh::protocol::TProtocol");
  string class_suffix;
  if (gen_templates_) {
    class_suffix = "T<Protocol_>";
  }

  // I tried to do this as one function.  I really did.  But it was too hard.
  if (style != "Cob") {
    // Open function
    if (gen_templates_) {
      out <<
        indent() << "template <class Protocol_>" << endl;
    }
    const bool unnamed_oprot_seqid = tfunction->is_oneway() &&
      !(gen_templates_ && !specialized);
    out <<
      "void " << tservice->get_name() << "Processor" << class_suffix << "::" <<
      "process_" << tfunction->get_name() << "(" <<
      "int32_t" << (unnamed_oprot_seqid ? ", " : " seqid, ") <<
      prot_type << "* iprot, " <<
      prot_type << "*" << (unnamed_oprot_seqid ? ", " : " oprot, ") <<
      "void* callContext)" << endl;
    scope_up(out);

    if (gen_templates_ && !specialized) {
      // If these are instances of Protocol_, instead of any old TProtocol,
      // use the specialized process function instead.
      out <<
        indent() << "Protocol_* _iprot = dynamic_cast<Protocol_*>(iprot);" <<
        endl <<
        indent() << "Protocol_* _oprot = dynamic_cast<Protocol_*>(oprot);" <<
        endl <<
        indent() << "if (_iprot && _oprot) {" << endl <<
        indent() << "  return process_" << tfunction->get_name() <<
        "(seqid, _iprot, _oprot, callContext);" << endl <<
        indent() << "}" << endl <<
        indent() << "T_GENERIC_PROTOCOL(this, iprot, _iprot);" << endl <<
        indent() << "T_GENERIC_PROTOCOL(this, oprot, _oprot);" << endl << endl;
    }

    string argsname = tservice->get_name() + "_" + tfunction->get_name() +
      "_args";
    string resultname = tservice->get_name() + "_" + tfunction->get_name() +
      "_result";

    out <<
      indent() << "void* ctx = NULL;" << endl <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  ctx = this->eventHandler_->getContext(\"" <<
        service_func_name << "\", callContext);" << endl <<
      indent() << "}" << endl <<
      indent() << "contrail::sandesh::TProcessorContextFreer freer(" <<
        "this->eventHandler_.get(), ctx, \"" << service_func_name << "\");" <<
        endl << endl <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  this->eventHandler_->preRead(ctx, \"" <<
        service_func_name << "\");" << endl <<
      indent() << "}" << endl << endl <<
      indent() << argsname << " args;" << endl <<
      indent() << "args.read(iprot);" << endl <<
      indent() << "iprot->readMessageEnd();" << endl <<
      indent() << "uint32_t bytes = iprot->getTransport()->readEnd();" <<
        endl << endl <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  this->eventHandler_->postRead(ctx, \"" <<
        service_func_name << "\", bytes);" << endl <<
      indent() << "}" << endl <<
      endl;

    // Declare result
    if (!tfunction->is_oneway()) {
      out <<
        indent() << resultname << " result;" << endl;
    }

    // Try block for functions with exceptions
    out <<
      indent() << "try {" << endl;
    indent_up();

    // Generate the function call
    bool first = true;
    out << indent();
    if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
      if (is_complex_type(tfunction->get_returntype())) {
        first = false;
        out << "iface_->" << tfunction->get_name() << "(result.success";
      } else {
        out << "result.success = iface_->" << tfunction->get_name() << "(";
      }
    } else {
      out <<
        "iface_->" << tfunction->get_name() << "(";
    }
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      out << "args." << (*f_iter)->get_name();
    }
    out << ");" << endl;

    // Set isset on success field
    if (!tfunction->is_oneway() && !tfunction->get_returntype()->is_void()) {
      out <<
        indent() << "result.__isset.success = true;" << endl;
    }

    indent_down();
    out << indent() << "}";

    if (!tfunction->is_oneway()) {
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        out << " catch (" << type_name((*x_iter)->get_type()) << " &" <<
          (*x_iter)->get_name() << ") {" << endl;
        if (!tfunction->is_oneway()) {
          indent_up();
          out <<
            indent() << "result." << (*x_iter)->get_name() << " = " <<
              (*x_iter)->get_name() << ";" << endl <<
            indent() << "result.__isset." << (*x_iter)->get_name() <<
              " = true;" << endl;
          indent_down();
          out << indent() << "}";
        } else {
          out << "}";
        }
      }
    }

    out << " catch (const std::exception& e) {" << endl;

    indent_up();
    out <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  this->eventHandler_->handlerError(ctx, \"" <<
        service_func_name << "\");" << endl <<
      indent() << "}" << endl;

    if (!tfunction->is_oneway()) {
      out <<
        endl <<
        indent() << "contrail::sandesh::TApplicationException x(e.what());" <<
          endl <<
        indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
          "\", contrail::sandesh::protocol::T_EXCEPTION, seqid);" << endl <<
        indent() << "x.write(oprot);" << endl <<
        indent() << "oprot->writeMessageEnd();" << endl <<
        indent() << "oprot->getTransport()->writeEnd();" << endl <<
        indent() << "oprot->getTransport()->flush();" << endl;
    }
    out << indent() << "return;" << endl;
    indent_down();
    out << indent() << "}" << endl << endl;

    // Shortcut out here for oneway functions
    if (tfunction->is_oneway()) {
      out <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  this->eventHandler_->asyncComplete(ctx, \"" <<
          service_func_name << "\");" << endl <<
        indent() << "}" << endl << endl <<
        indent() << "return;" << endl;
      indent_down();
      out << "}" << endl <<
        endl;
      return;
    }

    // Serialize the result into a struct
    out <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  this->eventHandler_->preWrite(ctx, \"" <<
        service_func_name << "\");" << endl <<
      indent() << "}" << endl << endl <<
      indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
        "\", contrail::sandesh::protocol::T_REPLY, seqid);" << endl <<
      indent() << "result.write(oprot);" << endl <<
      indent() << "oprot->writeMessageEnd();" << endl <<
      indent() << "bytes = oprot->getTransport()->writeEnd();" << endl <<
      indent() << "oprot->getTransport()->flush();" << endl << endl <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  this->eventHandler_->postWrite(ctx, \"" <<
        service_func_name << "\", bytes);" << endl <<
      indent() << "}" << endl;

    // Close function
    scope_down(out);
    out << endl;
  }

  // Cob style.
  else {
    // Processor entry point.
    // TODO(edhall) update for callContext when TEventServer is ready
    if (gen_templates_) {
      out <<
        indent() << "template <class Protocol_>" << endl;
    }
    out <<
      "void " << tservice->get_name() << "AsyncProcessor" << class_suffix <<
      "::process_" << tfunction->get_name() <<
      "(std::tr1::function<void(bool ok)> cob, int32_t seqid, " <<
      prot_type << "* iprot, " << prot_type << "* oprot)" << endl;
    scope_up(out);

    // TODO(simpkins): we could try to consoldate this
    // with the non-cob code above
    if (gen_templates_ && !specialized) {
      // If these are instances of Protocol_, instead of any old TProtocol,
      // use the specialized process function instead.
      out <<
        indent() << "Protocol_* _iprot = dynamic_cast<Protocol_*>(iprot);" <<
        endl <<
        indent() << "Protocol_* _oprot = dynamic_cast<Protocol_*>(oprot);" <<
        endl <<
        indent() << "if (_iprot && _oprot) {" << endl <<
        indent() << "  return process_" << tfunction->get_name() <<
        "(cob, seqid, _iprot, _oprot);" << endl <<
        indent() << "}" << endl <<
        indent() << "T_GENERIC_PROTOCOL(this, iprot, _iprot);" << endl <<
        indent() << "T_GENERIC_PROTOCOL(this, oprot, _oprot);" << endl << endl;
    }

    if (tfunction->is_oneway()) {
      out <<
        indent() << "(void) seqid;" << endl <<
        indent() << "(void) oprot;" << endl;
    }

    out <<
      indent() << tservice->get_name() + "_" + tfunction->get_name() <<
        "_args args;" << endl <<
      indent() << "void* ctx = NULL;" << endl <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  ctx = this->eventHandler_->getContext(\"" <<
        service_func_name << "\", NULL);" << endl <<
      indent() << "}" << endl <<
      indent() << "contrail::sandesh::TProcessorContextFreer freer(" <<
        "this->eventHandler_.get(), ctx, \"" << service_func_name << "\");" <<
        endl << endl <<
      indent() << "try {" << endl;
    indent_up();
    out <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  this->eventHandler_->preRead(ctx, \"" <<
        service_func_name << "\");" << endl <<
      indent() << "}" << endl <<
      indent() << "args.read(iprot);" << endl <<
      indent() << "iprot->readMessageEnd();" << endl <<
      indent() << "uint32_t bytes = iprot->getTransport()->readEnd();" <<
        endl <<
      indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "  this->eventHandler_->postRead(ctx, \"" <<
        service_func_name << "\", bytes);" << endl <<
      indent() << "}" << endl;
    scope_down(out);

    // TODO(dreiss): Handle TExceptions?  Expose to server?
    out <<
      indent() << "catch (const std::exception& exn) {" << endl <<
      indent() << "  if (this->eventHandler_.get() != NULL) {" << endl <<
      indent() << "    this->eventHandler_->handlerError(ctx, \"" <<
        service_func_name << "\");" << endl <<
      indent() << "  }" << endl <<
      indent() << "  return cob(false);" << endl <<
      indent() << "}" << endl;

    if (tfunction->is_oneway()) {
      out <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  this->eventHandler_->asyncComplete(ctx, \"" <<
          service_func_name << "\");" << endl <<
        indent() << "}" << endl;
    }
    // TODO(dreiss): Figure out a strategy for exceptions in async handlers.
    out <<
      indent() << "freer.unregister();" << endl;
    if (tfunction->is_oneway()) {
      // No return.  Just hand off our cob.
      // TODO(dreiss): Call the cob immediately?
      out <<
        indent() << "iface_->" << tfunction->get_name() << "(" <<
        "std::tr1::bind(cob, true)" << endl;
      indent_up(); indent_up();
    } else {
      string ret_arg, ret_placeholder;
      if (!tfunction->get_returntype()->is_void()) {
        ret_arg = ", const " + type_name(tfunction->get_returntype()) +
          "& _return";
        ret_placeholder = ", std::tr1::placeholders::_1";
      }

      // When gen_templates_ is true, the return_ and throw_ functions are
      // overloaded.  We have to declare pointers to them so that the compiler
      // can resolve the correct overloaded version.
      out <<
        indent() << "void (" << tservice->get_name() << "AsyncProcessor" <<
        class_suffix << "::*return_fn)(std::tr1::function<void(bool ok)> " <<
        "cob, int32_t seqid, " << prot_type << "* oprot, void* ctx" <<
        ret_arg << ") =" << endl;
      out <<
        indent() << "  &" << tservice->get_name() << "AsyncProcessor" <<
        class_suffix << "::return_" << tfunction->get_name() << ";" << endl;
      if (!xceptions.empty()) {
        out <<
          indent() << "void (" << tservice->get_name() << "AsyncProcessor" <<
          class_suffix << "::*throw_fn)(std::tr1::function<void(bool ok)> " <<
          "cob, int32_t seqid, " << prot_type << "* oprot, void* ctx, " <<
          "::contrail::sandesh::TDelayedException* _throw) =" << endl;
        out <<
          indent() << "  &" << tservice->get_name() << "AsyncProcessor" <<
          class_suffix << "::throw_" << tfunction->get_name() << ";" << endl;
      }

      out <<
        indent() << "iface_->" << tfunction->get_name() << "(" << endl;
      indent_up(); indent_up();
      out <<
        indent() << "std::tr1::bind(return_fn, this, cob, seqid, oprot, ctx" <<
        ret_placeholder << ")";
      if (!xceptions.empty()) {
        out
          << ',' << endl <<
          indent() << "std::tr1::bind(throw_fn, this, cob, seqid, oprot, " <<
          "ctx, std::tr1::placeholders::_1)";
      }
    }

    // XXX Whitespace cleanup.
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      out
                 << ',' << endl <<
        indent() << "args." << (*f_iter)->get_name();
    }
    out << ");" << endl;
    indent_down(); indent_down();
    scope_down(out);
    out << endl;

    // Normal return.
    if (!tfunction->is_oneway()) {
      string ret_arg_decl, ret_arg_name;
      if (!tfunction->get_returntype()->is_void()) {
        ret_arg_decl = ", const " + type_name(tfunction->get_returntype()) +
          "& _return";
        ret_arg_name = ", _return";
      }
      if (gen_templates_) {
        out <<
          indent() << "template <class Protocol_>" << endl;
      }
      out <<
        "void " << tservice->get_name() << "AsyncProcessor" << class_suffix <<
        "::return_" << tfunction->get_name() <<
        "(std::tr1::function<void(bool ok)> cob, int32_t seqid, " <<
        prot_type << "* oprot, void* ctx" << ret_arg_decl << ')' << endl;
      scope_up(out);

      if (gen_templates_ && !specialized) {
        // If oprot is a Protocol_ instance,
        // use the specialized return function instead.
        out <<
          indent() << "Protocol_* _oprot = dynamic_cast<Protocol_*>(oprot);" <<
          endl <<
          indent() << "if (_oprot) {" << endl <<
          indent() << "  return return_" << tfunction->get_name() <<
          "(cob, seqid, _oprot, ctx" << ret_arg_name << ");" << endl <<
          indent() << "}" << endl <<
          indent() << "T_GENERIC_PROTOCOL(this, oprot, _oprot);" <<
          endl << endl;
      }

      out <<
        indent() << tservice->get_name() << "_" << tfunction->get_name() <<
          "_presult result;" << endl;
      if (!tfunction->get_returntype()->is_void()) {
        // The const_cast here is unfortunate, but it would be a pain to avoid,
        // and we only do a write with this struct, which is const-safe.
        out <<
          indent() << "result.success = const_cast<" <<
            type_name(tfunction->get_returntype()) << "*>(&_return);" <<
            endl <<
          indent() << "result.__isset.success = true;" << endl;
      }
      // Serialize the result into a struct
      out <<
        endl <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  ctx = this->eventHandler_->getContext(\"" <<
          service_func_name << "\", NULL);" << endl <<
        indent() << "}" << endl <<
        indent() << "contrail::sandesh::TProcessorContextFreer freer(" <<
          "this->eventHandler_.get(), ctx, \"" << service_func_name <<
          "\");" << endl << endl <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  this->eventHandler_->preWrite(ctx, \"" <<
          service_func_name << "\");" << endl <<
        indent() << "}" << endl << endl <<
        indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
          "\", contrail::sandesh::protocol::T_REPLY, seqid);" << endl <<
        indent() << "result.write(oprot);" << endl <<
        indent() << "oprot->writeMessageEnd();" << endl <<
        indent() << "uint32_t bytes = oprot->getTransport()->writeEnd();" <<
          endl <<
        indent() << "oprot->getTransport()->flush();" << endl <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  this->eventHandler_->postWrite(ctx, \"" <<
          service_func_name << "\", bytes);" << endl <<
        indent() << "}" << endl <<
        indent() << "return cob(true);" << endl;
      scope_down(out);
      out << endl;
    }

    // Exception return.
    if (!tfunction->is_oneway() && !xceptions.empty()) {
      if (gen_templates_) {
        out <<
          indent() << "template <class Protocol_>" << endl;
      }
      out <<
        "void " << tservice->get_name() << "AsyncProcessor" << class_suffix <<
        "::throw_" << tfunction->get_name() <<
        "(std::tr1::function<void(bool ok)> cob, int32_t seqid, " <<
        prot_type << "* oprot, void* ctx, " <<
        "::contrail::sandesh::TDelayedException* _throw)" << endl;
      scope_up(out);

      if (gen_templates_ && !specialized) {
        // If oprot is a Protocol_ instance,
        // use the specialized throw function instead.
        out <<
          indent() << "Protocol_* _oprot = dynamic_cast<Protocol_*>(oprot);" <<
          endl <<
          indent() << "if (_oprot) {" << endl <<
          indent() << "  return throw_" << tfunction->get_name() <<
          "(cob, seqid, _oprot, ctx, _throw);" << endl <<
          indent() << "}" << endl <<
          indent() << "T_GENERIC_PROTOCOL(this, oprot, _oprot);" <<
          endl << endl;
      }

      // Get the event handler context
      out <<
        endl <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  ctx = this->eventHandler_->getContext(\"" <<
          service_func_name << "\", NULL);" << endl <<
        indent() << "}" << endl <<
        indent() << "contrail::sandesh::TProcessorContextFreer freer(" <<
          "this->eventHandler_.get(), ctx, \"" << service_func_name << "\");" <<
          endl << endl;

      // Throw the TDelayedException, and catch the result
      out <<
        indent() << tservice->get_name() << "_" << tfunction->get_name() <<
          "_result result;" << endl << endl <<
        indent() << "try {" << endl;
      indent_up();
      out <<
        indent() << "_throw->throw_it();" << endl <<
        indent() << "return cob(false);" << endl;  // Is this possible?  TBD.
      indent_down();
      out <<
        indent() << '}';
      for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
        out << "  catch (" << type_name((*x_iter)->get_type()) << " &" <<
          (*x_iter)->get_name() << ") {" << endl;
        indent_up();
        out <<
          indent() << "result." << (*x_iter)->get_name() << " = " <<
            (*x_iter)->get_name() << ";" << endl <<
          indent() << "result.__isset." << (*x_iter)->get_name() <<
            " = true;" << endl;
        scope_down(out);
      }

      // Handle the case where an undeclared exception is thrown
      out << " catch (std::exception& e) {" << endl;
      indent_up();
      out <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  this->eventHandler_->handlerError(ctx, \"" <<
          service_func_name << "\");" << endl <<
        indent() << "}" << endl <<
        endl <<
        indent() << "contrail::sandesh::TApplicationException x(e.what());" <<
          endl <<
        indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
          "\", contrail::sandesh::protocol::T_EXCEPTION, seqid);" << endl <<
        indent() << "x.write(oprot);" << endl <<
        indent() << "oprot->writeMessageEnd();" << endl <<
        indent() << "oprot->getTransport()->writeEnd();" << endl <<
        indent() << "oprot->getTransport()->flush();" << endl <<
        // We pass true to the cob here, since we did successfully write a
        // response, even though it is an exception response.
        // It looks like the argument is currently ignored, anyway.
        indent() << "return cob(true);" << endl;
      scope_down(out);

      // Serialize the result into a struct
      out <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  this->eventHandler_->preWrite(ctx, \"" <<
          service_func_name << "\");" << endl <<
        indent() << "}" << endl << endl <<
        indent() << "oprot->writeMessageBegin(\"" << tfunction->get_name() <<
          "\", contrail::sandesh::protocol::T_REPLY, seqid);" << endl <<
        indent() << "result.write(oprot);" << endl <<
        indent() << "oprot->writeMessageEnd();" << endl <<
        indent() << "uint32_t bytes = oprot->getTransport()->writeEnd();" <<
          endl <<
        indent() << "oprot->getTransport()->flush();" << endl <<
        indent() << "if (this->eventHandler_.get() != NULL) {" << endl <<
        indent() << "  this->eventHandler_->postWrite(ctx, \"" <<
          service_func_name << "\", bytes);" << endl <<
        indent() << "}" << endl <<
        indent() << "return cob(true);" << endl;
      scope_down(out);
      out << endl;
    } // for each function
  } // cob style
}

/**
 * Generates a skeleton file of a server
 *
 * @param tservice The service to generate a server for.
 */
void t_cpp_generator::generate_service_skeleton(t_service* tservice) {
  string svcname = tservice->get_name();

  // Service implementation file includes
  string f_skeleton_name = get_out_dir()+svcname+"_server.skeleton.cpp";

  string ns = namespace_prefix(tservice->get_program()->get_namespace("cpp"));

  ofstream f_skeleton;
  f_skeleton.open(f_skeleton_name.c_str());
  f_skeleton <<
    "// This autogenerated skeleton file illustrates how to build a server." << endl <<
    "// You should copy it to another filename to avoid overwriting it." << endl <<
    endl <<
    "#include \"" << get_include_prefix(*get_program()) << svcname << ".h\"" << endl <<
    "#include <protocol/TBinaryProtocol.h>" << endl <<
    "#include <server/TSimpleServer.h>" << endl <<
    "#include <transport/TServerSocket.h>" << endl <<
    "#include <transport/TBufferTransports.h>" << endl <<
    endl <<
    "using namespace ::contrail::sandesh;" << endl <<
    "using namespace ::contrail::sandesh::protocol;" << endl <<
    "using namespace ::contrail::sandesh::transport;" << endl <<
    "using namespace ::contrail::sandesh::server;" << endl <<
    endl <<
    "using boost::shared_ptr;" << endl <<
    endl;

  if (!ns.empty()) {
    f_skeleton <<
      "using namespace " << string(ns, 0, ns.size()-2) << ";" << endl <<
      endl;
  }

  f_skeleton <<
    "class " << svcname << "Handler : virtual public " << svcname << "If {" << endl <<
    " public:" << endl;
  indent_up();
  f_skeleton <<
    indent() << svcname << "Handler() {" << endl <<
    indent() << "  // Your initialization goes here" << endl <<
    indent() << "}" << endl <<
    endl;

  vector<t_function*> functions = tservice->get_functions();
  vector<t_function*>::iterator f_iter;
  for (f_iter = functions.begin(); f_iter != functions.end(); ++f_iter) {
    f_skeleton <<
      indent() << function_signature(*f_iter, "") << " {" << endl <<
      indent() << "  // Your implementation goes here" << endl <<
      indent() << "  printf(\"" << (*f_iter)->get_name() << "\\n\");" << endl <<
      indent() << "}" << endl <<
      endl;
  }

  indent_down();
  f_skeleton <<
    "};" << endl <<
    endl;

  f_skeleton <<
    indent() << "int main(int argc, char **argv) {" << endl;
  indent_up();
  f_skeleton <<
    indent() << "int port = 9090;" << endl <<
    indent() << "shared_ptr<" << svcname << "Handler> handler(new " << svcname << "Handler());" << endl <<
    indent() << "shared_ptr<TProcessor> processor(new " << svcname << "Processor(handler));" << endl <<
    indent() << "shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));" << endl <<
    indent() << "shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());" << endl <<
    indent() << "shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());" << endl <<
    endl <<
    indent() << "TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);" << endl <<
    indent() << "server.serve();" << endl <<
    indent() << "return 0;" << endl;
  indent_down();
  f_skeleton <<
    "}" << endl <<
    endl;

  // Close the files
  f_skeleton.close();
}

/**
 * Deserializes a field of any type.
 */
void t_cpp_generator::generate_deserialize_field(ofstream& out,
                                                 t_field* tfield,
                                                 string prefix,
                                                 string suffix) {
  t_type* type = get_true_type(tfield->get_type());

  if (type->is_void()) {
    throw "CANNOT GENERATE DESERIALIZE CODE FOR void TYPE: " +
      prefix + tfield->get_name();
  }

  string name = prefix + tfield->get_name() + suffix;

  if (type->is_struct() || type->is_xception()) {
    generate_deserialize_struct(out, (t_struct*)type, name);
  } else if (type->is_container()) {
    generate_deserialize_container(out, type, name);
  } else if (type->is_base_type()) {
    indent(out) <<
      "if ((ret = iprot->";
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "compiler error: cannot serialize void field in a struct: " + name;
      break;
    case t_base_type::TYPE_STRING:
#ifdef SANDESH
    case t_base_type::TYPE_STATIC_CONST_STRING:
#endif
      if (((t_base_type*)type)->is_binary()) {
        out << "readBinary(" << name << ")) < 0) {" << endl;
      }
      else {
        out << "readString(" << name << ")) < 0) {" << endl;
      }
      break;
#ifdef SANDESH
    case t_base_type::TYPE_XML:
      out << "readXML(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_UUID:
      out << "readUUID(" << name << ")) < 0) {" << endl;
      break;
#endif
    case t_base_type::TYPE_BOOL:
      out << "readBool(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_BYTE:
      out << "readByte(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_I16:
      out << "readI16(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_I32:
      out << "readI32(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_I64:
      out << "readI64(" << name << ")) < 0) {" << endl;
      break;
#ifdef SANDESH
    case t_base_type::TYPE_U16:
      out << "readU16(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_U32:
      out << "readU32(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_U64:
      out << "readU64(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_IPV4:
      out << "readIPV4(" << name << ")) < 0) {" << endl;
      break;
    case t_base_type::TYPE_IPADDR:
      out << "readIPADDR(" << name << ")) < 0) {" << endl;
      break;
#endif
    case t_base_type::TYPE_DOUBLE:
      out << "readDouble(" << name << ")) < 0) {" << endl;
      break;
    default:
      throw "compiler error: no C++ reader for base type " + t_base_type::t_base_name(tbase) + name;
    }
    indent_up();
    out << indent() << "return ret;" << endl;
    scope_down(out);
    out << indent() << "xfer += ret;" << endl;
    out <<
      endl;
  } else if (type->is_enum()) {
    string t = tmp("ecast");
    out <<
      indent() << "int32_t " << t << ";" << endl <<
      indent() << "if ((ret = iprot->readI32(" << t << ")) < 0) {" << endl <<
      indent() << "  return ret;" << endl <<
      indent() << "}" << endl <<
      indent() << "xfer += ret;" << endl <<
      indent() << name << " = (" << type_name(type) << ")" << t << ";" << endl;
  } else {
    printf("DO NOT KNOW HOW TO DESERIALIZE FIELD '%s' TYPE '%s'\n",
           tfield->get_name().c_str(), type_name(type).c_str());
  }
}

/**
 * Generates an unserializer for a variable. This makes two key assumptions,
 * first that there is a const char* variable named data that points to the
 * buffer for deserialization, and that there is a variable protocol which
 * is a reference to a TProtocol serialization object.
 */
void t_cpp_generator::generate_deserialize_struct(ofstream& out,
                                                  t_struct* tstruct,
                                                  string prefix) {
  (void) tstruct;
  indent(out) <<
    "if ((ret = " << prefix << ".read(iprot)) < 0) {" << endl;
  indent_up();
  indent(out) << "return ret;" << endl;
  scope_down(out);
  indent(out) << "xfer += ret;" << endl;
}

void t_cpp_generator::generate_deserialize_container(ofstream& out,
                                                     t_type* ttype,
                                                     string prefix) {
  scope_up(out);

  string size = tmp("_size");
  string ktype = tmp("_ktype");
  string vtype = tmp("_vtype");
  string etype = tmp("_etype");

  t_container* tcontainer = (t_container*)ttype;
  bool use_push = tcontainer->has_cpp_name();

  indent(out) <<
    prefix << ".clear();" << endl <<
    indent() << "uint32_t " << size << ";" << endl;

  // Declare variables, read header
  if (ttype->is_map()) {
    out <<
      indent() << "::contrail::sandesh::protocol::TType " << ktype << ";" << endl <<
      indent() << "::contrail::sandesh::protocol::TType " << vtype << ";" << endl <<
      indent() << "if ((ret = iprot->readMapBegin(" <<
                   ktype << ", " << vtype << ", " << size << ")) < 0) {" << endl <<
      indent() << "  return ret;" << endl <<
      indent() << "}" << endl <<
      indent() << "xfer += ret;" << endl;
  } else if (ttype->is_set()) {
    out <<
      indent() << "::contrail::sandesh::protocol::TType " << etype << ";" << endl <<
      indent() << "if ((ret = iprot->readSetBegin(" <<
                   etype << ", " << size << ")) < 0) {" << endl <<
      indent() << "  return ret;" << endl <<
      indent() << "}" << endl <<
      indent() << "xfer += ret;" << endl;
  } else if (ttype->is_list()) {
    out <<
      indent() << "::contrail::sandesh::protocol::TType " << etype << ";" << endl <<
      indent() << "if ((ret = iprot->readListBegin(" <<
      etype << ", " << size << ")) < 0) {" << endl <<
      indent() << "  return ret;" << endl <<
      indent() << "}" << endl <<
      indent() << "xfer += ret;" << endl;
    if (!use_push) {
      indent(out) << prefix << ".resize(" << size << ");" << endl;
    }
  }


  // For loop iterates over elements
  string i = tmp("_i");
  out <<
    indent() << "uint32_t " << i << ";" << endl <<
    indent() << "for (" << i << " = 0; " << i << " < " << size << "; ++" << i << ")" << endl;

    scope_up(out);

    if (ttype->is_map()) {
      generate_deserialize_map_element(out, (t_map*)ttype, prefix);
    } else if (ttype->is_set()) {
#ifdef SANDESH
      t_type *etype = ((t_set*)ttype)->get_elem_type();
      if (etype->is_base_type() || etype->is_enum()) {
        indent(out) << "if ((ret = iprot->readContainerElementBegin()) < 0) {" << endl;
        indent_up();
        indent(out) << "return ret;" << endl;
        scope_down(out);
        indent(out) << "xfer += ret;" << endl;
      }
#endif
      generate_deserialize_set_element(out, (t_set*)ttype, prefix);
#ifdef SANDESH
      if (etype->is_base_type() || etype->is_enum()) {
        indent(out) << "if ((ret = iprot->readContainerElementEnd()) < 0) {" << endl;
        indent_up();
        indent(out) << "return ret;" << endl;
        scope_down(out);
        indent(out) << "xfer += ret;" << endl;
      }
#endif
    } else if (ttype->is_list()) {
#ifdef SANDESH
      t_type *etype = ((t_list*)ttype)->get_elem_type();
      if (etype->is_base_type() || etype->is_enum()) {
        indent(out) << "if ((ret = iprot->readContainerElementBegin()) < 0) {" << endl;
        indent_up();
        indent(out) << "return ret;" << endl;
        scope_down(out);
        indent(out) << "xfer += ret;" << endl;
      }
#endif
      generate_deserialize_list_element(out, (t_list*)ttype, prefix, use_push, i);
#ifdef SANDESH
      if (etype->is_base_type() || etype->is_enum()) {
        indent(out) << "if ((ret = iprot->readContainerElementEnd()) < 0) {" << endl;
        indent_up();
        indent(out) << "return ret;" << endl;
        scope_down(out);
        indent(out) << "xfer += ret;" << endl;
      }
#endif
    }

    scope_down(out);

  // Read container end
  if (ttype->is_map()) {
#ifdef SANDESH
    indent(out) << "if ((ret = iprot->readMapEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
#else
    indent(out) << "iprot->readMapEnd();" << endl;
#endif
  } else if (ttype->is_set()) {
#ifdef SANDESH
    indent(out) << "if ((ret = iprot->readSetEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
#else
    indent(out) << "iprot->readSetEnd();" << endl;
#endif
  } else if (ttype->is_list()) {
#ifdef SANDESH
    indent(out) << "if ((ret = iprot->readListEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
#else
    indent(out) << "iprot->readListEnd();" << endl;
#endif
  }

  scope_down(out);
}


/**
 * Generates code to deserialize a map
 */
void t_cpp_generator::generate_deserialize_map_element(ofstream& out,
                                                       t_map* tmap,
                                                       string prefix) {
  string key = tmp("_key");
  string val = tmp("_val");
  t_field fkey(tmap->get_key_type(), key);
  t_field fval(tmap->get_val_type(), val);

  out <<
    indent() << declare_field(&fkey) << endl;

#ifdef SANDESH
  t_type* kttype = tmap->get_key_type();
  if (kttype->is_base_type() || kttype->is_enum()) {
    indent(out) << "if ((ret = iprot->readContainerElementBegin()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }
#endif
  generate_deserialize_field(out, &fkey);
#ifdef SANDESH
  if (kttype->is_base_type() || kttype->is_enum()) {
    indent(out) << "if ((ret = iprot->readContainerElementEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }
#endif
  indent(out) <<
    declare_field(&fval, false, false, false, true) << " = " <<
    prefix << "[" << key << "];" << endl;

#ifdef SANDESH
  t_type* vttype = tmap->get_val_type();
  if (vttype->is_base_type() || vttype->is_enum()) {
    indent(out) << "if ((ret = iprot->readContainerElementBegin()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }
#endif
  generate_deserialize_field(out, &fval);
#ifdef SANDESH
  if (vttype->is_base_type() || vttype->is_enum()) {
    indent(out) << "if ((ret = iprot->readContainerElementEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }
#endif
}

void t_cpp_generator::generate_deserialize_set_element(ofstream& out,
                                                       t_set* tset,
                                                       string prefix) {
  string elem = tmp("_elem");
  t_field felem(tset->get_elem_type(), elem);

  indent(out) <<
    declare_field(&felem) << endl;

  generate_deserialize_field(out, &felem);

  indent(out) <<
    prefix << ".insert(" << elem << ");" << endl;
}

void t_cpp_generator::generate_deserialize_list_element(ofstream& out,
                                                        t_list* tlist,
                                                        string prefix,
                                                        bool use_push,
                                                        string index) {
  if (use_push) {
    string elem = tmp("_elem");
    t_field felem(tlist->get_elem_type(), elem);
    indent(out) << declare_field(&felem) << endl;
    generate_deserialize_field(out, &felem);
    indent(out) << prefix << ".push_back(" << elem << ");" << endl;
  } else {
    t_field felem(tlist->get_elem_type(), prefix + "[" + index + "]");
    generate_deserialize_field(out, &felem);
  }
}


/**
 * Serializes a field of any type.
 *
 * @param tfield The field to serialize
 * @param prefix Name to prepend to field name
 */
void t_cpp_generator::generate_serialize_field(ofstream& out,
                                               t_field* tfield,
                                               string prefix,
                                               string suffix) {
  t_type* type = get_true_type(tfield->get_type());

  string name = prefix + tfield->get_name() + suffix;

  // Do nothing for void types
  if (type->is_void()) {
    throw "CANNOT GENERATE SERIALIZE CODE FOR void TYPE: " + name;
  }



  if (type->is_struct() || type->is_xception()) {
    generate_serialize_struct(out,
                              (t_struct*)type,
                              name);
  } else if (type->is_container()) {
    generate_serialize_container(out, type, name);
  } else if (type->is_base_type() || type->is_enum()) {

    indent(out) <<
      "if ((ret = oprot->";

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        throw
          "compiler error: cannot serialize void field in a struct: " + name;
        break;
      case t_base_type::TYPE_STRING:
#ifdef SANDESH
      case t_base_type::TYPE_STATIC_CONST_STRING:
#endif
        if (((t_base_type*)type)->is_binary()) {
          out << "writeBinary(" << name << ")) < 0) {" << endl;
        }
        else {
          out << "writeString(" << name << ")) < 0) {" << endl;
        }
        break;
#ifdef SANDESH
      case t_base_type::TYPE_XML:
        out << "writeXML(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_UUID:
        out << "writeUUID(" << name << ")) < 0) {" << endl;
        break;
#endif
      case t_base_type::TYPE_BOOL:
        out << "writeBool(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_BYTE:
        out << "writeByte(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_I16:
        out << "writeI16(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_I32:
        out << "writeI32(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_I64:
        out << "writeI64(" << name << ")) < 0) {" << endl;
        break;
#ifdef SANDESH
      case t_base_type::TYPE_U16:
        out << "writeU16(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_U32:
        out << "writeU32(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_U64:
        out << "writeU64(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_IPV4:
        out << "writeIPV4(" << name << ")) < 0) {" << endl;
        break;
      case t_base_type::TYPE_IPADDR:
        out << "writeIPADDR(" << name << ")) < 0) {" << endl;
        break;
#endif
      case t_base_type::TYPE_DOUBLE:
        out << "writeDouble(" << name << ")) < 0) {" << endl;
        break;
      default:
        throw "compiler error: no C++ writer for base type " + t_base_type::t_base_name(tbase) + name;
      }
    } else if (type->is_enum()) {
      out << "writeI32((int32_t)" << name << ")) < 0) {" << endl;
    }
    indent_up();
    out << indent() << "return ret;" << endl;
    scope_down(out);
    out << indent() << "xfer += ret;" << endl;
    out << endl;
#ifdef SANDESH
  } else if (type->is_sandesh()) {
    generate_serialize_sandesh(out, (t_sandesh*)type, name);
#endif
  } else {
    printf("DO NOT KNOW HOW TO SERIALIZE FIELD '%s' TYPE '%s'\n",
           name.c_str(),
           type_name(type).c_str());
  }
}

#ifdef SANDESH
/**
 * Serializes all the members of a sandesh.
 *
 * @param tstruct The sandesh to serialize
 * @param prefix  String prefix to attach to all fields
 */
void t_cpp_generator::generate_serialize_sandesh(ofstream& out,
                                                 t_sandesh* tsandesh,
                                                 string prefix) {
  (void) tsandesh;
  indent(out) <<
    "if ((ret = " << prefix << ".write(oprot)) < 0) {" << endl;
  indent_up();
  indent(out) << "return ret;" << endl;
  scope_down(out);
  indent(out) << "xfer += ret;" << endl;
}
#endif

/**
 * Serializes all the members of a struct.
 *
 * @param tstruct The struct to serialize
 * @param prefix  String prefix to attach to all fields
 */
void t_cpp_generator::generate_serialize_struct(ofstream& out,
                                                t_struct* tstruct,
                                                string prefix) {
  (void) tstruct;
  indent(out) <<
    "if ((ret = " << prefix << ".write(oprot)) < 0) {" << endl;
  indent_up();
  indent(out) << "return ret;" << endl;
  scope_down(out);
  indent(out) << "xfer += ret;" << endl;
}

void t_cpp_generator::generate_serialize_container(ofstream& out,
                                                   t_type* ttype,
                                                   string prefix) {
  scope_up(out);

  if (ttype->is_map()) {
    indent(out) <<
      "if ((ret = oprot->writeMapBegin(" <<
      type_to_enum(((t_map*)ttype)->get_key_type()) << ", " <<
      type_to_enum(((t_map*)ttype)->get_val_type()) << ", " <<
      "static_cast<uint32_t>(" << prefix << ".size()))) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "if ((ret = oprot->writeSetBegin(" <<
      type_to_enum(((t_set*)ttype)->get_elem_type()) << ", " <<
      "static_cast<uint32_t>(" << prefix << ".size()))) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "if ((ret = oprot->writeListBegin(" <<
      type_to_enum(((t_list*)ttype)->get_elem_type()) << ", " <<
      "static_cast<uint32_t>(" << prefix << ".size()))) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }

  string iter = tmp("_iter");
  out <<
    indent() << type_name(ttype) << "::const_iterator " << iter << ";" << endl <<
    indent() << "for (" << iter << " = " << prefix  << ".begin(); " << iter << " != " << prefix << ".end(); ++" << iter << ")" << endl;
  scope_up(out);
    if (ttype->is_map()) {
      generate_serialize_map_element(out, (t_map*)ttype, iter);
    } else if (ttype->is_set()) {
#ifdef SANDESH
      t_type *etype = ((t_set*)ttype)->get_elem_type();
      if (etype->is_base_type() || etype->is_enum()) {
        indent(out) << "if ((ret = oprot->writeContainerElementBegin()) < 0) {" << endl;
        indent_up();
        indent(out) << "return ret;" << endl;
        scope_down(out);
        indent(out) << "xfer += ret;" << endl;
      }
#endif
      generate_serialize_set_element(out, (t_set*)ttype, iter);
#ifdef SANDESH
      if (etype->is_base_type() || etype->is_enum()) {
        indent(out) << "if ((ret = oprot->writeContainerElementEnd()) < 0) {" << endl;
        indent_up();
        indent(out) << "return ret;" << endl;
        scope_down(out);
        indent(out) << "xfer += ret;" << endl;
      }
#endif
    } else if (ttype->is_list()) {
#ifdef SANDESH
      t_type *etype = ((t_list*)ttype)->get_elem_type();
      if (etype->is_base_type() || etype->is_enum()) {
        indent(out) << "if ((ret = oprot->writeContainerElementBegin()) < 0) {" << endl;
        indent_up();
        indent(out) << "return ret;" << endl;
        scope_down(out);
        indent(out) << "xfer += ret;" << endl;
      }
#endif
      generate_serialize_list_element(out, (t_list*)ttype, iter);
#ifdef SANDESH
      if (etype->is_base_type() || etype->is_enum()) {
        indent(out) << "if ((ret = oprot->writeContainerElementEnd()) < 0) {" << endl;
        indent_up();
        indent(out) << "return ret;" << endl;
        scope_down(out);
        indent(out) << "xfer += ret;" << endl;
      }
#endif
    }
  scope_down(out);

  if (ttype->is_map()) {
    indent(out) <<
      "if ((ret = oprot->writeMapEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  } else if (ttype->is_set()) {
    indent(out) <<
      "if ((ret = oprot->writeSetEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  } else if (ttype->is_list()) {
    indent(out) <<
      "if ((ret = oprot->writeListEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }

  scope_down(out);
}

/**
 * Serializes the members of a map.
 *
 */
void t_cpp_generator::generate_serialize_map_element(ofstream& out,
                                                     t_map* tmap,
                                                     string iter) {
#ifdef SANDESH
  t_type* kttype = tmap->get_key_type();
  if (kttype->is_base_type() || kttype->is_enum()) {
    indent(out) << "if ((ret = oprot->writeContainerElementBegin()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }
#endif
  t_field kfield(tmap->get_key_type(), iter + "->first");
  generate_serialize_field(out, &kfield, "");
#ifdef SANDESH
  if (kttype->is_base_type() || kttype->is_enum()) {
    indent(out) << "if ((ret = oprot->writeContainerElementEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }
#endif
#ifdef SANDESH
  t_type* vttype = tmap->get_val_type();
  if (vttype->is_base_type() || vttype->is_enum()) {
    indent(out) << "if ((ret = oprot->writeContainerElementBegin()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }
#endif
  t_field vfield(tmap->get_val_type(), iter + "->second");
  generate_serialize_field(out, &vfield, "");
#ifdef SANDESH
  if (vttype->is_base_type() || vttype->is_enum()) {
    indent(out) << "if ((ret = oprot->writeContainerElementEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
  }
#endif
}

/**
 * Serializes the members of a set.
 */
void t_cpp_generator::generate_serialize_set_element(ofstream& out,
                                                     t_set* tset,
                                                     string iter) {
  t_field efield(tset->get_elem_type(), "(*" + iter + ")");
  generate_serialize_field(out, &efield, "");
}

/**
 * Serializes the members of a list.
 */
void t_cpp_generator::generate_serialize_list_element(ofstream& out,
                                                      t_list* tlist,
                                                      string iter) {
  t_field efield(tlist->get_elem_type(), "(*" + iter + ")");
  generate_serialize_field(out, &efield, "");
}

/**
 * Makes a :: prefix for a namespace
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_prefix(string ns) {
  // Always start with "::", to avoid possible name collisions with
  // other names in one of the current namespaces.
  //
  // We also need a leading space, in case the name is used inside of a
  // template parameter.  "MyTemplate<::foo::Bar>" is not valid C++,
  // since "<:" is an alternative token for "[".
  string result = " ::";

  if (ns.size() == 0) {
    return result;
  }
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += ns.substr(0, loc);
    result += "::";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += ns + "::";
  }
  return result;
}

/**
 * Opens namespace.
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_open(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "";
  string separator = "";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += separator;
    result += "namespace ";
    result += ns.substr(0, loc);
    result += " {";
    separator = " ";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += separator + "namespace " + ns + " {";
  }
  return result;
}

/**
 * Closes namespace.
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_close(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "}";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += "}";
    ns = ns.substr(loc+1);
  }
  result += " // namespace";
  return result;
}

/**
 * Returns a C++ type name
 *
 * @param ttype The type
 * @return String of the type name, i.e. std::set<type>
 */
string t_cpp_generator::type_name(t_type* ttype, bool in_typedef, bool arg) {
  if (ttype->is_base_type()) {
    string bname = base_type_name(((t_base_type*)ttype)->get_base());
    if (!arg) {
      return bname;
    }

    if (((t_base_type*)ttype)->get_base() == t_base_type::TYPE_STRING
#ifdef SANDESH
        || ((t_base_type*)ttype)->get_base() == t_base_type::TYPE_XML
        || ((t_base_type*)ttype)->get_base() == t_base_type::TYPE_UUID
        || ((t_base_type*)ttype)->get_base() == t_base_type::TYPE_IPADDR
#endif
       ) {
      return "const " + bname + "&";
    } else {
      return "const " + bname;
    }
  }

  // Check for a custom overloaded C++ name
  if (ttype->is_container()) {
    string cname;

    t_container* tcontainer = (t_container*) ttype;
    if (tcontainer->has_cpp_name()) {
      cname = tcontainer->get_cpp_name();
    } else if (ttype->is_map()) {
      t_map* tmap = (t_map*) ttype;
      cname = "std::map<" +
        type_name(tmap->get_key_type(), in_typedef) + ", " +
        type_name(tmap->get_val_type(), in_typedef) + "> ";
    } else if (ttype->is_set()) {
      t_set* tset = (t_set*) ttype;
      cname = "std::set<" + type_name(tset->get_elem_type(), in_typedef) + "> ";
    } else if (ttype->is_list()) {
      t_list* tlist = (t_list*) ttype;
      cname = "std::vector<" + type_name(tlist->get_elem_type(), in_typedef) + "> ";
    }

    if (arg) {
      return "const " + cname + "&";
    } else {
      return cname;
    }
  }

  string class_prefix;
  if (in_typedef && (ttype->is_struct() || ttype->is_xception())) {
    class_prefix = "class ";
  }

  // Check if it needs to be namespaced
  string pname;
  t_program* program = ttype->get_program();
  if (program != NULL && program != program_) {
    pname =
      class_prefix +
      namespace_prefix(program->get_namespace("cpp")) +
      ttype->get_name();
  } else {
    pname = class_prefix + ttype->get_name();
  }

  if (ttype->is_enum() && !gen_pure_enums_) {
    pname += "::type";
  }

  if (arg) {
    if (is_complex_type(ttype)) {
      return "const " + pname + "&";
    } else {
      return "const " + pname;
    }
  } else {
    return pname;
  }
}

/**
 * Returns the C++ type that corresponds to the thrift type.
 *
 * @param tbase The base type
 * @return Explicit C++ type, i.e. "int32_t"
 */
string t_cpp_generator::base_type_name(t_base_type::t_base tbase) {
  switch (tbase) {
  case t_base_type::TYPE_VOID:
    return "void";
#ifdef SANDESH
  case t_base_type::TYPE_STATIC_CONST_STRING:
    return "static std::string";
#endif
  case t_base_type::TYPE_STRING:
#ifdef SANDESH
  case t_base_type::TYPE_XML:
#endif
    return "std::string";
  case t_base_type::TYPE_BOOL:
    return "bool";
  case t_base_type::TYPE_BYTE:
    return "int8_t";
  case t_base_type::TYPE_I16:
    return "int16_t";
  case t_base_type::TYPE_I32:
    return "int32_t";
  case t_base_type::TYPE_I64:
    return "int64_t";
#ifdef SANDESH
  case t_base_type::TYPE_U16:
    return "uint16_t";
  case t_base_type::TYPE_U32:
    return "uint32_t";
  case t_base_type::TYPE_U64:
    return "uint64_t";
  case t_base_type::TYPE_IPV4:
    return "uint32_t";
  case t_base_type::TYPE_IPADDR:
    return "boost::asio::ip::address";
  case t_base_type::TYPE_UUID:
    return "boost::uuids::uuid";
#endif
  case t_base_type::TYPE_DOUBLE:
    return "double";
  default:
    throw "compiler error: no C++ base type name for base type " + t_base_type::t_base_name(tbase);
  }
}

/**
 * Declares a field, which may include initialization as necessary.
 *
 * @param ttype The type
 * @return Field declaration, i.e. int x = 0;
 */
#ifdef SANDESH
string t_cpp_generator::declare_field(t_field* tfield, bool init, bool pointer, bool constant, bool reference, bool constructor) {
#else
string t_cpp_generator::declare_field(t_field* tfield, bool init, bool pointer, bool constant, bool reference) {
#endif
  // TODO(mcslee): do we ever need to initialize the field?
  string result = "";
  if (constant) {
    result += "const ";
  }
  result += type_name(tfield->get_type());
  if (pointer) {
    result += "*";
  }
  if (reference) {
    result += "&";
  }
  result += " " + tfield->get_name();
  if (init) {
    t_type* type = get_true_type(tfield->get_type());

    if (type->is_base_type()) {
      t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
      switch (tbase) {
      case t_base_type::TYPE_VOID:
        break;
#ifdef SANDESH
      case t_base_type::TYPE_UUID:
        result += " = boost::uuids::nil_uuid()";
        break;
      case t_base_type::TYPE_STATIC_CONST_STRING:
      case t_base_type::TYPE_XML:
#endif
      case t_base_type::TYPE_STRING:
        result += " = \"\"";
        break;
      case t_base_type::TYPE_BOOL:
        result += " = false";
        break;
      case t_base_type::TYPE_BYTE:
      case t_base_type::TYPE_I16:
      case t_base_type::TYPE_I32:
      case t_base_type::TYPE_I64:
#ifdef SANDESH
      case t_base_type::TYPE_U16:
      case t_base_type::TYPE_U32:
      case t_base_type::TYPE_U64:
      case t_base_type::TYPE_IPV4:
#endif
        result += " = 0";
        break;
      case t_base_type::TYPE_DOUBLE:
        result += " = (double)0";
        break;
#ifdef SANDESH
      case t_base_type::TYPE_IPADDR:
        result += " = boost::asio::ip::address()";
        break;
#endif
      default:
        throw "compiler error: no C++ initializer for base type " + t_base_type::t_base_name(tbase);
      }
    } else if (type->is_enum()) {
      result += " = (" + type_name(type) + ")0";
    }
  }
  if (!reference
#ifdef SANDESH
    && !constructor
#endif
  ) {
    result += ";";
  }
  return result;
}

/**
 * Renders a function signature of the form 'type name(args)'
 *
 * @param tfunction Function definition
 * @return String of rendered function definition
 */
string t_cpp_generator::function_signature(t_function* tfunction,
                                           string style,
                                           string prefix,
                                           bool name_params) {
  t_type* ttype = tfunction->get_returntype();
  t_struct* arglist = tfunction->get_arglist();
  bool has_xceptions = !tfunction->get_xceptions()->get_members().empty();
#ifdef SANDESH
  bool is_static = tfunction->is_static();
  bool is_virtual = tfunction->is_virtual();
  string func_begin_modifier = "", func_end_modifier = "";
#endif

#ifdef SANDESH
  if (is_static) {
    func_begin_modifier = "static ";
  } else if (is_virtual) {
    func_begin_modifier = "virtual ";
    func_end_modifier = " = 0";
  }
#endif

  if (style == "") {
    if (is_complex_type(ttype)) {
      return
#ifdef SANDESH
        func_begin_modifier +
#endif
        "void " + prefix + tfunction->get_name() +
        "(" + type_name(ttype) + (name_params ? "& _return" : "& /* _return */") +
        argument_list(arglist, name_params, true) + ")"
#ifdef SANDESH
        + func_end_modifier
#endif
        ;
    } else {
      return
#ifdef SANDESH
        func_begin_modifier +
#endif
        type_name(ttype) + " " + prefix + tfunction->get_name() +
        "(" + argument_list(arglist, name_params) + ")"
#ifdef SANDESH
        + func_end_modifier
#endif
        ;
    }
  } else if (style.substr(0,3) == "Cob") {
    string cob_type;
    string exn_cob;
    if (style == "CobCl") {
      cob_type = "(" + service_name_ + "CobClient";
      if (gen_templates_) {
        cob_type += "T<Protocol_>";
      }
      cob_type += "* client)";
    } else if (style =="CobSv") {
      cob_type = (ttype->is_void()
                  ? "()"
                  : ("(" + type_name(ttype) + " const& _return)"));
      if (has_xceptions) {
        exn_cob = ", std::tr1::function<void(::contrail::sandesh::TDelayedException* _throw)> /* exn_cob */";
      }
    } else {
      throw "UNKNOWN STYLE";
    }

    return
      "void " + prefix + tfunction->get_name() +
      "(std::tr1::function<void" + cob_type + "> cob" + exn_cob +
      argument_list(arglist, name_params, true) + ")";
  } else {
    throw "UNKNOWN STYLE";
  }
}

/**
 * Renders a field list
 *
 * @param tstruct The struct definition
 * @return Comma sepearated list of all field names in that struct
 */
string t_cpp_generator::argument_list(t_struct* tstruct, bool name_params, bool start_comma) {
  string result = "";

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;
  bool first = !start_comma;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }
    result += type_name((*f_iter)->get_type(), false, true) + " " +
      (name_params ? (*f_iter)->get_name() : "/* " + (*f_iter)->get_name() + " */");
  }
  return result;
}

/**
 * Converts the parse type to a C++ enum string for the given type.
 *
 * @param type Thrift Type
 * @return String of C++ code to definition of that type constant
 */
string t_cpp_generator::type_to_enum(t_type* type) {
  type = get_true_type(type);

  if (type->is_base_type()) {
    t_base_type::t_base tbase = ((t_base_type*)type)->get_base();
    switch (tbase) {
    case t_base_type::TYPE_VOID:
      throw "NO T_VOID CONSTRUCT";
    case t_base_type::TYPE_STRING:
      return "::contrail::sandesh::protocol::T_STRING";
    case t_base_type::TYPE_BOOL:
      return "::contrail::sandesh::protocol::T_BOOL";
    case t_base_type::TYPE_BYTE:
      return "::contrail::sandesh::protocol::T_BYTE";
    case t_base_type::TYPE_I16:
      return "::contrail::sandesh::protocol::T_I16";
    case t_base_type::TYPE_I32:
      return "::contrail::sandesh::protocol::T_I32";
    case t_base_type::TYPE_I64:
      return "::contrail::sandesh::protocol::T_I64";
    case t_base_type::TYPE_DOUBLE:
      return "::contrail::sandesh::protocol::T_DOUBLE";
#ifdef SANDESH
    case t_base_type::TYPE_XML:
      return "::contrail::sandesh::protocol::T_XML";
    case t_base_type::TYPE_UUID:
      return "::contrail::sandesh::protocol::T_UUID";
    case t_base_type::TYPE_U16:
      return "::contrail::sandesh::protocol::T_U16";
    case t_base_type::TYPE_U32:
      return "::contrail::sandesh::protocol::T_U32";
    case t_base_type::TYPE_U64:
      return "::contrail::sandesh::protocol::T_U64";
    case t_base_type::TYPE_IPV4:
      return "::contrail::sandesh::protocol::T_IPV4";
    case t_base_type::TYPE_IPADDR:
      return "::contrail::sandesh::protocol::T_IPADDR";
    case t_base_type::TYPE_STATIC_CONST_STRING:
    case t_base_type::TYPE_SANDESH_SYSTEM:
    case t_base_type::TYPE_SANDESH_REQUEST:
    case t_base_type::TYPE_SANDESH_RESPONSE:
    case t_base_type::TYPE_SANDESH_TRACE:
    case t_base_type::TYPE_SANDESH_TRACE_OBJECT:
    case t_base_type::TYPE_SANDESH_BUFFER:
    case t_base_type::TYPE_SANDESH_UVE:
    case t_base_type::TYPE_SANDESH_ALARM:
    case t_base_type::TYPE_SANDESH_OBJECT:
    case t_base_type::TYPE_SANDESH_FLOW:
    case t_base_type::TYPE_SANDESH_SESSION:
      return "::contrail::sandesh::protocol::T_STRING";
#endif
    }
  } else if (type->is_enum()) {
    return "::contrail::sandesh::protocol::T_I32";
  } else if (type->is_struct()) {
    return "::contrail::sandesh::protocol::T_STRUCT";
  } else if (type->is_xception()) {
    return "::contrail::sandesh::protocol::T_STRUCT";
#ifdef SANDESH
  } else if (type->is_sandesh()) {
    return "::contrail::sandesh::protocol::T_SANDESH";
#endif
  } else if (type->is_map()) {
    return "::contrail::sandesh::protocol::T_MAP";
  } else if (type->is_set()) {
    return "::contrail::sandesh::protocol::T_SET";
  } else if (type->is_list()) {
    return "::contrail::sandesh::protocol::T_LIST";
  }

  throw "INVALID TYPE IN type_to_enum: " + type->get_name();
}

/**
 * Returns the symbol name of the local reflection of a type.
 */
string t_cpp_generator::local_reflection_name(const char* prefix, t_type* ttype, bool external) {
  ttype = get_true_type(ttype);

  // We have to use the program name as part of the identifier because
  // if two thrift "programs" are compiled into one actual program
  // you would get a symbol collision if they both defined list<i32>.
  // trlo = Thrift Reflection LOcal.
  string prog;
  string name;
  string nspace;

  // TODO(dreiss): Would it be better to pregenerate the base types
  //               and put them in Thrift.{h,cpp} ?

  if (ttype->is_base_type()) {
    prog = program_->get_name();
    name = ttype->get_ascii_fingerprint();
  } else if (ttype->is_enum()) {
    assert(ttype->get_program() != NULL);
    prog = ttype->get_program()->get_name();
    name = ttype->get_ascii_fingerprint();
  } else if (ttype->is_container()) {
    prog = program_->get_name();
    name = ttype->get_ascii_fingerprint();
  } else {
    assert(ttype->is_struct() || ttype->is_xception());
    assert(ttype->get_program() != NULL);
    prog = ttype->get_program()->get_name();
    name = ttype->get_ascii_fingerprint();
  }

  if (external &&
      ttype->get_program() != NULL &&
      ttype->get_program() != program_) {
    nspace = namespace_prefix(ttype->get_program()->get_namespace("cpp"));
  }

  return nspace + "trlo_" + prefix + "_" + prog + "_" + name;
}

string t_cpp_generator::get_include_prefix(const t_program& program) const {
  string include_prefix = program.get_include_prefix();
  if (!use_include_prefix_ ||
      (include_prefix.size() > 0 && include_prefix[0] == '/')) {
    // if flag is turned off or this is absolute path, return empty prefix
    return "";
  }

  string::size_type last_slash = string::npos;
  if ((last_slash = include_prefix.rfind("/")) != string::npos) {
    return include_prefix.substr(0, last_slash) + "/" + out_dir_base_ + "/";
  }

  return "";
}

void t_cpp_generator::generate_common_struct_reader_body(
    std::ofstream& out,
    t_struct_common* tstruct,
    bool pointers) {
  indent_up();

  std::string struct_type_name = tstruct->get_struct_type_name();

  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  // Declare stack tmp variables
  out <<
    endl <<
    indent() << "int32_t xfer = 0, ret;" << endl <<
    indent() << "std::string fname;" << endl <<
    indent() << "::contrail::sandesh::protocol::TType ftype;" << endl <<
    indent() << "int16_t fid;" << endl <<
      endl <<
    indent() <<
      "if ((ret = iprot->read" << struct_type_name << "Begin(fname)) < 0) {" <<
      endl <<
    indent() << "  return ret;" << endl <<
    indent() << "}" << endl <<
    indent() << "xfer += ret;" << endl <<
      endl;

  // Required variables aren't in __isset, so we need tmp vars to check them.
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED)
      indent(out) << "bool isset_" << (*f_iter)->get_name() << " = false;" << endl;
  }
  out << endl;

  // Loop over reading in fields
  indent(out) <<
    "while (true)" << endl;
  scope_up(out);

  // Read beginning field marker
  indent(out) <<
    "if ((ret = iprot->readFieldBegin(fname, ftype, fid)) < 0) {" << endl;
  indent_up();
  indent(out) << "return ret;" << endl;
  scope_down(out);
  indent(out) << "xfer += ret;" << endl;

  // Check for field STOP marker
  out <<
    indent() << "if (ftype == ::contrail::sandesh::protocol::T_STOP) {" << endl <<
    indent() << "  break;" << endl <<
    indent() << "}" << endl;

  // Generate 'switch' statement only when there is at least
  // one 'case' in it. Otherwise, generate just the contents
  // of 'default' label, without a 'switch'.
  if (fields.empty()) {
    out <<
      indent() << "if ((ret = iprot->skip(ftype)) < 0) {" << endl <<
      indent() << "  return ret;" << endl <<
      indent() << "}" << endl <<
      indent() << "xfer += ret;" << endl;
  } else {
    // Switch statement on the field we are reading
    indent(out) <<
      "switch (fid)" << endl;

    scope_up(out);

    // Generate deserialization code for known cases
    for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
      indent(out) <<
        "case " << (*f_iter)->get_key() << ":" << endl;
      indent_up();
      indent(out) <<
        "if (ftype == " << type_to_enum((*f_iter)->get_type()) << ") {" << endl;
      indent_up();

      const char *isset_prefix =
        ((*f_iter)->get_req() != t_field::T_REQUIRED) ? "this->__isset." : "isset_";

#if 0
      // This code throws an exception if the same field is encountered twice.
      // We've decided to leave it out for performance reasons.
      // TODO(dreiss): Generate this code and "if" it out to make it easier
      // for people recompiling thrift to include it.
      out <<
        indent() << "if (" << isset_prefix << (*f_iter)->get_name() << ")" << endl <<
        indent() << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl;
#endif

      if (pointers && !(*f_iter)->get_type()->is_xception()) {
        generate_deserialize_field(out, *f_iter, "(*(this->", "))");
      } else {
        generate_deserialize_field(out, *f_iter, "this->");
      }
      out <<
        indent() << isset_prefix << (*f_iter)->get_name() << " = true;" << endl;
      indent_down();
      out <<
        indent() << "} else {" << endl <<
        indent() << "  if ((ret = iprot->skip(ftype)) < 0) {" << endl <<
        indent() << "    return ret;" << endl <<
        indent() << "  }" << endl <<
        indent() << "  xfer += ret;" << endl <<
        // TODO(dreiss): Make this an option when thrift structs
        // have a common base class.
        // indent() << "  throw TProtocolException(TProtocolException::INVALID_DATA);" << endl <<
        indent() << "}" << endl <<
        indent() << "break;" << endl;
      indent_down();
    }

    // In the default case we skip the field
    out <<
      indent() << "default:" << endl <<
      indent() << "  if ((ret = iprot->skip(ftype)) < 0) {" << endl <<
      indent() << "    return ret;" << endl <<
      indent() << "  }" << endl <<
      indent() << "  xfer += ret;" << endl <<
      indent() << "  break;" << endl;

    scope_down(out);
  }

  // Read field end marker
  indent(out) <<
    "if ((ret = iprot->readFieldEnd()) < 0) {" << endl;
  indent_up();
  indent(out) << "return ret;" << endl;
  scope_down(out);
  indent(out) << "xfer += ret;" << endl;

  scope_down(out);

  out <<
    endl <<
    indent() <<
      "if ((ret = iprot->read" << struct_type_name << "End()) < 0) {" << endl;

  indent_up();
  indent(out) << "return ret;" << endl;
  scope_down(out);
  indent(out) << "xfer += ret;" << endl;

  // Throw if any required fields are missing.
  // We do this after reading the struct end so that
  // there might possibly be a chance of continuing.
  out << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_REQUIRED)
      out <<
        indent() << "if (!isset_" << (*f_iter)->get_name() << ") {" << endl <<
        indent() << "  LOG(ERROR, __func__ << \": Required field " << (*f_iter)->get_name()
                 << " not set\");" << endl <<
        indent() << "  return -1;" << endl <<
        indent() << "}" << endl;
  }

  indent(out) << "return xfer;" << endl;

  indent_down();
}

void t_cpp_generator::generate_common_struct_writer_body(
    std::ofstream& out,
    t_struct_common* tstruct,
    bool pointers) {
  indent_up();

  std::string struct_type_name = tstruct->get_struct_type_name();

  string name = tstruct->get_name();
  const vector<t_field*>& fields = tstruct->get_members();
  vector<t_field*>::const_iterator f_iter;

  out <<
    indent() << "int32_t xfer = 0, ret;" << endl;

  indent(out) <<
    "if ((ret = oprot->write" << struct_type_name << "Begin(\"" << name << "\")) < 0) {" <<
    endl;

  indent_up();
  indent(out) << "return ret;" << endl;
  scope_down(out);
  indent(out) << "xfer += ret;" << endl;
  for (f_iter = fields.begin(); f_iter != fields.end(); ++f_iter) {
    if ((*f_iter)->get_req() == t_field::T_OPTIONAL) {
      indent(out) << "if (this->__isset." << (*f_iter)->get_name() << ") {" << endl;
      indent_up();
    }
    // Write field header
    if (!((*f_iter)->annotations_.empty())) {
      out << indent() << "{" << endl;
      indent_up();
      out << indent() << "std::map<std::string, std::string> annotations_;" << endl;
      std::map<std::string, std::string>::iterator it;
      for (it = (*f_iter)->annotations_.begin(); it != (*f_iter)->annotations_.end(); it++) {
        if (!tstruct->is_sandesh() &&
            ((*f_iter)->get_name() == "name") && (it->first == "key")) {
          out << indent() << "assert(!table_.empty());" << endl;
          out << indent() << "annotations_.insert("
              "std::make_pair(\"key\", table_));" << endl;
        } else {
          out << indent() << "annotations_.insert(std::make_pair(\"" << (*it).first
              << "\"" << ", \"" << (*it).second << "\"));" << endl;
        }
      }
      out <<
        indent() << "if ((ret = oprot->writeFieldBegin(" <<
          "\"" << (*f_iter)->get_name() << "\", " <<
          type_to_enum((*f_iter)->get_type()) << ", " <<
          (*f_iter)->get_key() << ", &annotations_)) < 0) {" << endl;
      indent_up();
      indent(out) << "return ret;" << endl;
      scope_down(out);
      indent(out) << "xfer += ret;" << endl;
      indent_down();
      out << indent() << "}" << endl;
    } else {
      out <<
        indent() << "if ((ret = oprot->writeFieldBegin(" <<
          "\"" << (*f_iter)->get_name() << "\", " <<
          type_to_enum((*f_iter)->get_type()) << ", " <<
          (*f_iter)->get_key() << ")) < 0) {" << endl;
      indent_up();
      indent(out) << "return ret;" << endl;
      scope_down(out);
      indent(out) << "xfer += ret;" << endl;
    }

    // Write field contents
    if (pointers && !(*f_iter)->get_type()->is_xception()) {
      generate_serialize_field(out, *f_iter, "(*(this->", "))");
    } else {
      generate_serialize_field(out, *f_iter, "this->");
    }
    // Write field closer
    indent(out) <<
      "if ((ret = oprot->writeFieldEnd()) < 0) {" << endl;
    indent_up();
    indent(out) << "return ret;" << endl;
    scope_down(out);
    indent(out) << "xfer += ret;" << endl;
    if ((*f_iter)->get_req() == t_field::T_OPTIONAL) {
      indent_down();
      indent(out) << '}' << endl;
    }
  }

  // Write the struct map
  out <<
    indent() << "if ((ret = oprot->writeFieldStop()) < 0) {" << endl <<
    indent() << "  return ret;" << endl <<
    indent() << "}" << endl <<
    indent() << "xfer += ret;" << endl <<
    indent() <<
      "if ((ret = oprot->write" << struct_type_name << "End()) < 0) {" <<
      endl <<
    indent() << "  return ret;" << endl <<
    indent() << "}" << endl <<
    indent() << "xfer += ret;" << endl <<
    indent() << "return xfer;" << endl;

  indent_down();
}


THRIFT_REGISTER_GENERATOR(cpp, "C++",
"    cob_style:       Generate \"Continuation OBject\"-style classes.\n"
"    no_client_completion:\n"
"                     Omit calls to completion__() in CobClient class.\n"
"    templates:       Generate templatized reader/writer methods.\n"
"    pure_enums:      Generate pure enums instead of wrapper classes.\n"
"    dense:           Generate type specifications for the dense protocol.\n"
"    include_prefix:  Use full include paths in generated files.\n"
)
