/*
 * Copyright (c) 2018 Juniper Networks, Inc. All rights reserved.
 * Copyright Contributors to the OpenSDN project
 */

#ifndef _SANDESH_PROTOCOL_TJSONPROTOCOL_H_
#define _SANDESH_PROTOCOL_TJSONPROTOCOL_H_ 1

#include <string.h>
#include "TVirtualProtocol.h"

#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/set.hpp>
#include <base/time_util.h>

namespace contrail { namespace sandesh { namespace protocol {

/**
 * Protocol that prints the payload in JSON format.
 */
class TJSONProtocol : public TVirtualProtocol<TJSONProtocol> {
 private:
  enum write_state_t
  { UNINIT
  , STRUCT
  , LIST
  , SET
  , MAP
  , SNDESH
  };

 public:
  TJSONProtocol(boost::shared_ptr<TTransport> trans)
    : TVirtualProtocol<TJSONProtocol>(trans)
    , trans_(trans.get())
    , string_limit_(DEFAULT_STRING_LIMIT)
    , string_prefix_size_(DEFAULT_STRING_PREFIX_SIZE)
    , reader_(*trans)
  {
    TType ttypes_arr[] = { T_STRUCT, T_MAP, T_SET, T_LIST, T_SANDESH };
    complexTypeSet[T_STRUCT] = true;
    complexTypeSet[T_MAP] = true;
    complexTypeSet[T_SET] = true;
    complexTypeSet[T_LIST] = true;
    write_state_.push_back(UNINIT);
  }

  static const int32_t DEFAULT_STRING_LIMIT = 256;
  static const int32_t DEFAULT_STRING_PREFIX_SIZE = 16;

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setStringPrefixSize(int32_t string_prefix_size) {
    string_prefix_size_ = string_prefix_size;
  }

  static std::string escapeJSONControlCharsInternal(const std::string& str) {
    std::string xmlstr;
    xmlstr.reserve(str.length());
    for (std::string::const_iterator it = str.begin();
         it != str.end(); ++it) {
      switch(*it) {
       case '&':  xmlstr += "&amp;";  break;
       case '\'': xmlstr += "&apos;"; break;
       case '<':  xmlstr += "&lt;";   break;
       case '>':  xmlstr += "&gt;";   break;
       default:   xmlstr += *it;
      }
    }
    return xmlstr;
  }

  static std::string escapeJSONControlChars(const std::string& str) {
    if (strpbrk(str.c_str(), "&'<>") != NULL) {
      return escapeJSONControlCharsInternal(str);
    } else {
      return str;
    }
  }

  static void unescapeJSONControlChars(std::string& str) {
      boost::algorithm::replace_all(str, "&amp;", "&");
      boost::algorithm::replace_all(str, "&apos;", "\'");
      boost::algorithm::replace_all(str, "&lt;", "<");
      boost::algorithm::replace_all(str, "&gt;", ">");
  }

  /**
   * Writing functions
   */

  int32_t writeMessageBegin(const std::string& name,
                            const TMessageType messageType,
                            const int32_t seqid);

  int32_t writeMessageEnd();

  int32_t writeStructBegin(const char* name);

  int32_t writeStructEnd();

  int32_t writeSandeshBegin(const char* name);

  int32_t writeSandeshEnd();

  int32_t writeContainerElementBegin();

  int32_t writeContainerElementEnd();

  int32_t writeFieldBegin(const char* name,
                          const TType fieldType,
                          const int16_t fieldId,
                          const std::map<std::string, std::string> *const amap = NULL);

  int32_t writeFieldEnd();

  int32_t writeFieldStop();

  int32_t writeMapBegin(const TType keyType,
                        const TType valType,
                        const uint32_t size);

  int32_t writeMapEnd();

  int32_t writeListBegin(const TType elemType,
                         const uint32_t size);

  int32_t writeListEnd();

  int32_t writeSetBegin(const TType elemType,
                        const uint32_t size);

  int32_t writeSetEnd();

  int32_t writeBool(const bool value);

  int32_t writeByte(const int8_t byte);

  int32_t writeI16(const int16_t i16);

  int32_t writeI32(const int32_t i32);

  int32_t writeI64(const int64_t i64);

  int32_t writeU16(const uint16_t u16);

  int32_t writeU32(const uint32_t u32);
  
  int32_t writeU64(const uint64_t u64);

  int32_t writeIPV4(const uint32_t ip4);

  int32_t writeIPADDR(const boost::asio::ip::address& ipaddress);
  
  int32_t writeDouble(const double dub);

  int32_t writeString(const std::string& str);

  int32_t writeBinary(const std::string& str);

  int32_t writeJSON(const std::string& str);

  int32_t writeUUID(const boost::uuids::uuid& uuid);

  /**
   * Reading functions
   */
  void setSandeshEnd(bool val) {
      sandesh_end_ = val;
  }

  class LookaheadReader {

   public:

    LookaheadReader(TTransport &trans) :
      trans_(&trans),
      hasData_(false),
      has2Data_(false),
      firstRead_(false) {
    }

    uint8_t read() {
      if (hasData_) {
        hasData_ = false;
      }
      else {
        if (has2Data_) {
          if (firstRead_) {
            has2Data_ = false;
            firstRead_ = false;
            return data2_[1];
          } else {
            firstRead_ = true;
            return data2_[0];
          }
        } else {
          trans_->readAll(&data_, 1);
        }
      }
      return data_;
    }

    uint8_t peek() {
      if (!hasData_) {
        trans_->readAll(&data_, 1);
      }
      hasData_ = true;
      return data_;
    }

    uint8_t peek2() {
      bool first = false;
      if (!has2Data_) {
        trans_->readAll(data2_, 2);
        first = true;
      }
      has2Data_ = true;
      return first ? data2_[0] : data2_[1];
    }

   private:
    TTransport *trans_;
    bool hasData_;
    uint8_t data_;
    bool has2Data_;
    uint8_t data2_[2];
    bool firstRead_;
  };

 private:
  typedef boost::tokenizer<boost::char_separator<char> >
      tokenizer;
  void indentUp();
  void indentDown();
  int32_t writePlain(const std::string& str);
  int32_t writeIndented(const std::string& str);

  static const std::string& fieldTypeName(TType type);
  static TType getTypeIDForTypeName(const std::string &name);

  TTransport* trans_;

  int32_t string_limit_;
  int32_t string_prefix_size_;

  std::string indent_str_;

  static const int indent_inc = 2;

  std::vector<write_state_t> write_state_;

  bool sandesh_begin_;
  
  bool sandesh_end_;

  std::vector<bool> is_struct_begin_list_;
 
  std::vector<bool> is_list_begin_list_;

  bool is_primitive_list_begin_;

  bool is_first_primitve_list_elem_;

  bool is_data_map_key_;

  bool is_beginning_of_map;

  bool is_string_begin_;

  bool is_struct_begin_;

  bool in_list_context_;

  bool is_list_begin_;

  bool is_map_begin_;

  bool in_map_context_;

  bool in_non_primitive_map_context_;

  bool in_primitive_list_context_;

  bool in_non_primitive_list_context_;

  bool is_map_primitive_;

  bool is_map_val_;

  bool is_list_elem_string_;

  std::vector<std::string> current_sandesh_context_;

  std::vector<bool> is_first_element_context_;

  std::vector<bool> is_primitive_element_list_;

  std::vector<bool> is_map_val_primitive_;

  std::vector<bool> in_map_val_context_;
  std::vector<std::string> xml_state_;
  LookaheadReader reader_;
  std::map<TType, bool> complexTypeSet;

};

/**
 * Constructs JSON protocol handlers
 */
class TJSONProtocolFactory : public TProtocolFactory {
 public:
  TJSONProtocolFactory() {}
  virtual ~TJSONProtocolFactory() {}

  boost::shared_ptr<TProtocol> getProtocol(boost::shared_ptr<TTransport> trans) {
    return boost::shared_ptr<TProtocol>(new TJSONProtocol(trans));
  }

};

}}} // contrail::sandesh::protocol

#endif // #ifndef _SANDESH_PROTOCOL_TJSONPROTOCOL_H_


