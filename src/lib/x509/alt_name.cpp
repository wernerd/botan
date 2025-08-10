/*
* (C) 2024 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/pkix_types.h>

#include <botan/ber_dec.h>
#include <botan/der_enc.h>
#include <botan/internal/loadstor.h>
#include <botan/internal/parsing.h>

namespace Botan {

void AlternativeName::add_uri(std::string_view uri) {
   if(!uri.empty()) {
      m_uri.insert(std::string(uri));
   }
}

void AlternativeName::add_email(std::string_view addr) {
   if(!addr.empty()) {
      m_email.insert(std::string(addr));
   }
}

void AlternativeName::add_dns(std::string_view dns) {
   if(!dns.empty()) {
      m_dns.insert(std::string(dns));
   }
}

void AlternativeName::add_other_name(const OID& oid, const ASN1_String& value) {
   m_othernames.insert(std::make_pair(oid, value));
}

void AlternativeName::add_dn(const X509_DN& dn) {
   m_dn_names.insert(dn);
}

void AlternativeName::add_ip_address(std::string_view ip) {
   if(!ip.empty()) {
      m_ip_addr.insert(std::string(ip));
   }
}

bool AlternativeName::has_items() const {
   if(!this->dns().empty()) {
      return true;
   }
   if(!this->uris().empty()) {
      return true;
   }
   if(!this->email().empty()) {
      return true;
   }
   if(!this->ip_address().empty()) {
      return true;
   }
   if(!this->directory_names().empty()) {
      return true;
   }
   if(!this->other_names().empty()) {
      return true;
   }
   return false;
}

void AlternativeName::encode_into(DER_Encoder& der) const {
   der.start_sequence();

   /*
   GeneralName ::= CHOICE {
        otherName                       [0]     OtherName,
        rfc822Name                      [1]     IA5String,
        dNSName                         [2]     IA5String,
        x400Address                     [3]     ORAddress,
        directoryName                   [4]     Name,
        ediPartyName                    [5]     EDIPartyName,
        uniformResourceIdentifier       [6]     IA5String,
        iPAddress                       [7]     OCTET STRING,
        registeredID                    [8]     OBJECT IDENTIFIER }
   */

   for(const auto& othername : m_othernames) {
      der.start_explicit(0)
         .encode(othername.first)
         .start_explicit(0)
         .encode(othername.second)
         .end_explicit()
         .end_explicit();
   }

   for(const auto& name : m_email) {
      ASN1_String str(name, ASN1_Type::Ia5String);
      der.add_object(ASN1_Type(1), ASN1_Class::ContextSpecific, str.value());
   }

   for(const auto& name : m_dns) {
      ASN1_String str(name, ASN1_Type::Ia5String);
      der.add_object(ASN1_Type(2), ASN1_Class::ContextSpecific, str.value());
   }

   for(const auto& name : m_dn_names) {
      der.add_object(ASN1_Type(4), ASN1_Class::ExplicitContextSpecific, name.DER_encode());
   }

   for(const auto& name : m_uri) {
      ASN1_String str(name, ASN1_Type::Ia5String);
      der.add_object(ASN1_Type(6), ASN1_Class::ContextSpecific, str.value());
   }

   for(const auto& ip : m_ip_addr) {
      auto ip_buf = store_be(string_to_ipv4(ip));
      der.add_object(ASN1_Type(7), ASN1_Class::ContextSpecific, ip_buf.data(), 4);
   }

   der.end_cons();
}

void AlternativeName::decode_from(BER_Decoder& source) {
   BER_Decoder names = source.start_sequence();

   while(names.more_items()) {
      BER_Object obj = names.get_next_object();

      if(obj.is_a(0, ASN1_Class::ContextSpecific)) {
         BER_Decoder othername(obj);

         OID oid;
         othername.decode(oid);
         if(othername.more_items()) {
            BER_Object othername_value_outer = othername.get_next_object();
            othername.verify_end();

            if(!othername_value_outer.is_a(0, ASN1_Class::ExplicitContextSpecific)) {
               throw Decoding_Error("Invalid tags on otherName value");
            }

            BER_Decoder othername_value_inner(othername_value_outer);

            BER_Object value = othername_value_inner.get_next_object();
            othername_value_inner.verify_end();

            if(ASN1_String::is_string_type(value.type()) && value.get_class() == ASN1_Class::Universal) {
               add_othername(oid, ASN1::to_string(value), value.type());
            }
         }
      } else if(obj.is_a(1, ASN1_Class::ContextSpecific)) {
         add_email(ASN1::to_string(obj));
      } else if(obj.is_a(2, ASN1_Class::ContextSpecific)) {
         add_dns(ASN1::to_string(obj));
      } else if(obj.is_a(4, ASN1_Class::ContextSpecific | ASN1_Class::Constructed)) {
         BER_Decoder dec(obj);
         X509_DN dn;
         dec.decode(dn);
         this->add_dn(dn);
      } else if(obj.is_a(6, ASN1_Class::ContextSpecific)) {
         this->add_uri(ASN1::to_string(obj));
      } else if(obj.is_a(7, ASN1_Class::ContextSpecific)) {
         if(obj.length() == 4) {
            const uint32_t ip = load_be<uint32_t>(obj.bits(), 0);
            this->add_ip_address(ipv4_to_string(ip));
         }
      }
   }
}

}  // namespace Botan
