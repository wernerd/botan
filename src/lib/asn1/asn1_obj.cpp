/*
* ASN.1 Internals
* (C) 1999-2007,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/asn1_obj.h>
#include <botan/der_enc.h>
#include <botan/data_src.h>
#include <botan/internal/stl_util.h>
#include <sstream>

namespace Botan {

std::vector<uint8_t> ASN1_Object::BER_encode() const
   {
   std::vector<uint8_t> output;
   DER_Encoder der(output);
   this->encode_into(der);
   return output;
   }

/*
* Check a type invariant on BER data
*/
void BER_Object::assert_is_a(ASN1_Tag expected_type_tag, ASN1_Tag expected_class_tag,
                             const std::string& descr) const
   {
   if(this->is_a(expected_type_tag, expected_class_tag) == false)
      {
      std::stringstream msg;

      msg << "Tag mismatch when decoding " << descr << " got ";

      if(m_class_tag == ASN1_Tag::NO_OBJECT && m_type_tag == ASN1_Tag::NO_OBJECT)
         {
         msg << "EOF";
         }
      else
         {
         if(m_class_tag == ASN1_Tag::UNIVERSAL || m_class_tag == ASN1_Tag::CONSTRUCTED)
            {
            msg << asn1_tag_to_string(m_type_tag);
            }
         else
            {
            msg << std::to_string(static_cast<uint32_t>(m_type_tag));
            }

         msg << "/" << asn1_class_to_string(m_class_tag);
         }

      msg << " expected ";

      if(expected_class_tag == ASN1_Tag::UNIVERSAL || expected_class_tag == ASN1_Tag::CONSTRUCTED)
         {
         msg << asn1_tag_to_string(expected_type_tag);
         }
      else
         {
         msg << std::to_string(static_cast<uint32_t>(expected_type_tag));
         }

      msg << "/" << asn1_class_to_string(expected_class_tag);

      throw BER_Decoding_Error(msg.str());
      }
   }

bool BER_Object::is_a(ASN1_Tag expected_type_tag, ASN1_Tag expected_class_tag) const
   {
   return (m_type_tag == expected_type_tag && m_class_tag == expected_class_tag);
   }

bool BER_Object::is_a(int expected_type_tag, ASN1_Tag expected_class_tag) const
   {
   return is_a(ASN1_Tag(expected_type_tag), expected_class_tag);
   }

void BER_Object::set_tagging(ASN1_Tag type_tag, ASN1_Tag class_tag)
   {
   m_type_tag = type_tag;
   m_class_tag = class_tag;
   }

std::string asn1_class_to_string(ASN1_Tag type)
   {
   switch(type)
      {
      case ASN1_Tag::UNIVERSAL:
         return "UNIVERSAL";
      case ASN1_Tag::CONSTRUCTED:
         return "CONSTRUCTED";
      case ASN1_Tag::CONTEXT_SPECIFIC:
         return "CONTEXT_SPECIFIC";
      case ASN1_Tag::APPLICATION:
         return "APPLICATION";
      case ASN1_Tag::PRIVATE:
         return "PRIVATE";
      case ASN1_Tag::NO_OBJECT:
         return "NO_OBJECT";
      default:
         return "CLASS(" + std::to_string(static_cast<size_t>(type)) + ")";
      }
   }

std::string asn1_tag_to_string(ASN1_Tag type)
   {
   switch(type)
      {
      case ASN1_Tag::SEQUENCE:
         return "SEQUENCE";

      case ASN1_Tag::SET:
         return "SET";

      case ASN1_Tag::PRINTABLE_STRING:
         return "PRINTABLE STRING";

      case ASN1_Tag::NUMERIC_STRING:
         return "NUMERIC STRING";

      case ASN1_Tag::IA5_STRING:
         return "IA5 STRING";

      case ASN1_Tag::T61_STRING:
         return "T61 STRING";

      case ASN1_Tag::UTF8_STRING:
         return "UTF8 STRING";

      case ASN1_Tag::VISIBLE_STRING:
         return "VISIBLE STRING";

      case ASN1_Tag::BMP_STRING:
         return "BMP STRING";

      case ASN1_Tag::UNIVERSAL_STRING:
         return "UNIVERSAL STRING";

      case ASN1_Tag::UTC_TIME:
         return "UTC TIME";

      case ASN1_Tag::GENERALIZED_TIME:
         return "GENERALIZED TIME";

      case ASN1_Tag::OCTET_STRING:
         return "OCTET STRING";

      case ASN1_Tag::BIT_STRING:
         return "BIT STRING";

      case ASN1_Tag::ENUMERATED:
         return "ENUMERATED";

      case ASN1_Tag::INTEGER:
         return "INTEGER";

      case ASN1_Tag::NULL_TAG:
         return "NULL";

      case ASN1_Tag::OBJECT_ID:
         return "OBJECT";

      case ASN1_Tag::BOOLEAN:
         return "BOOLEAN";

      case ASN1_Tag::NO_OBJECT:
         return "NO_OBJECT";

      default:
         return "TAG(" + std::to_string(static_cast<uint32_t>(type)) + ")";
      }
   }

/*
* BER Decoding Exceptions
*/
BER_Decoding_Error::BER_Decoding_Error(const std::string& str) :
   Decoding_Error("BER: " + str) {}

BER_Bad_Tag::BER_Bad_Tag(const std::string& str, ASN1_Tag tag) :
   BER_Decoding_Error(str + ": " + std::to_string(static_cast<uint32_t>(tag))) {}

BER_Bad_Tag::BER_Bad_Tag(const std::string& str,
                         ASN1_Tag tag1, ASN1_Tag tag2) :
   BER_Decoding_Error(str + ": " +
                      std::to_string(static_cast<uint32_t>(tag1)) + "/" +
                      std::to_string(static_cast<uint32_t>(tag2))) {}

namespace ASN1 {

/*
* Put some arbitrary bytes into a SEQUENCE
*/
std::vector<uint8_t> put_in_sequence(const std::vector<uint8_t>& contents)
   {
   return ASN1::put_in_sequence(contents.data(), contents.size());
   }

std::vector<uint8_t> put_in_sequence(const uint8_t bits[], size_t len)
   {
   std::vector<uint8_t> output;
   DER_Encoder(output)
      .start_sequence()
         .raw_bytes(bits, len)
      .end_cons();
   return output;
   }

/*
* Convert a BER object into a string object
*/
std::string to_string(const BER_Object& obj)
   {
   return std::string(cast_uint8_ptr_to_char(obj.bits()),
                      obj.length());
   }

/*
* Do heuristic tests for BER data
*/
bool maybe_BER(DataSource& source)
   {
   uint8_t first_u8;
   if(!source.peek_byte(first_u8))
      {
      BOTAN_ASSERT_EQUAL(source.read_byte(first_u8), 0, "Expected EOF");
      throw Stream_IO_Error("ASN1::maybe_BER: Source was empty");
      }

   const auto cons_seq = ASN1_Tag::CONSTRUCTED | ASN1_Tag::SEQUENCE;
   if(first_u8 == static_cast<uint8_t>(cons_seq))
      return true;
   return false;
   }

}

}
