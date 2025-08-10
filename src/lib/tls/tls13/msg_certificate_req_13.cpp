/*
* (C) 2022 Jack Lloyd
* (C) 2022 Hannes Rantzsch, René Meusel - neXenio GmbH
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/tls_messages.h>
#include <botan/internal/tls_reader.h>
#include <botan/tls_exceptn.h>
#include <botan/tls_callbacks.h>
#include <botan/credentials_manager.h>

namespace Botan::TLS
{

Handshake_Type Certificate_Request_13::type() const
   {
   return TLS::Handshake_Type::CERTIFICATE_REQUEST;
   }

Certificate_Request_13::Certificate_Request_13(const std::vector<uint8_t>& buf, const Connection_Side side)
   {
   TLS_Data_Reader reader("Certificate_Request_13", buf);

   // RFC 8446 4.3.2
   //    A server which is authenticating with a certificate MAY optionally
   //    request a certificate from the client.
   if(side != Connection_Side::SERVER)
      {
      throw TLS_Exception(Alert::UNEXPECTED_MESSAGE, "Received a Certificate_Request message from a client");
      }

   m_context = reader.get_tls_length_value(1);
   m_extensions.deserialize(reader, side, type());

   // RFC 8446 4.3.2
   //    The "signature_algorithms" extension MUST be specified, and other
   //    extensions may optionally be included if defined for this message.
   //    Clients MUST ignore unrecognized extensions.

   if(!m_extensions.has<Signature_Algorithms>())
      {
      throw TLS_Exception(Alert::MISSING_EXTENSION, "Certificate_Request message did not provide a signature_algorithms extension");
      }

   // RFC 8446 4.2.
   //    The table below indicates the messages where a given extension may
   //    appear [...].  If an implementation receives an extension which it
   //    recognizes and which is not specified for the message in which it
   //    appears, it MUST abort the handshake with an "illegal_parameter" alert.
   //
   // For Certificate Request said table states:
   //    "status_request", "signature_algorithms", "signed_certificate_timestamp",
   //     "certificate_authorities", "oid_filters", "signature_algorithms_cert",
   std::set<Extension_Code> allowed_extensions =
      {
      Extension_Code::CertificateStatusRequest,
      Extension_Code::SignatureAlgorithms,
      // Extension_Code::SignedCertificateTimestamp,  // NYI
      Extension_Code::CertificateAuthorities,
      // Extension_Code::OidFilters,                   // NYI
      Extension_Code::CertSignatureAlgorithms,
      };

   if(m_extensions.contains_implemented_extensions_other_than(allowed_extensions))
      {
      throw TLS_Exception(Alert::ILLEGAL_PARAMETER,
                          "Certificate Request contained an extension that is not allowed");
      }
   }

Certificate_Request_13::Certificate_Request_13(std::vector<Signature_Scheme> signature_schemes,
                                               std::vector<X509_DN> acceptable_CAs,
                                               Callbacks&)
   {
   // RFC 8446 4.3.2
   //    The certificate_request_context [here: m_context] MUST be unique within
   //    the scope of this connection (thus preventing replay of client
   //    CertificateVerify messages).  This field SHALL be zero length unless
   //    used for the post-handshake authentication exchanges described in
   //    Section 4.6.2.
   //
   // TODO: Post-Handshake auth must fill m_context in an unpredictable way

   // RFC 8446 4.3.2
   //    [Supported signature algorithms are] expressed by sending the
   //    "signature_algorithms" and optionally "signature_algorithms_cert"
   //    extensions. [A list of certificate authorities which the server would
   //    accept] is expressed by sending the "certificate_authorities" extension.
   //
   //    The "signature_algorithms" extension MUST be specified, and other
   //    extensions may optionally be included if defined for this message.
   //
   // TODO: fully support 'signature_algorithms_cert'
   m_extensions.add(std::make_unique<Signature_Algorithms>(std::move(signature_schemes)));

   if(!acceptable_CAs.empty())
      {
      m_extensions.add(std::make_unique<Certificate_Authorities>(std::move(acceptable_CAs)));
      }

   // TODO: Support cert_status_request for OCSP stapling

   // TODO: give the application a chance to modifying extensions
   //       (after GH #2988 is merged)
   // callbacks.tls_modify_extensions(m_extensions, Connection_Side::SERVER);
   }

std::optional<Certificate_Request_13>
Certificate_Request_13::maybe_create(const Client_Hello_13& client_hello,
                                     Credentials_Manager& cred_mgr,
                                     Callbacks& callbacks,
                                     const Policy& policy)
   {
   const auto trusted_CAs = cred_mgr.trusted_certificate_authorities("tls-server", client_hello.sni_hostname());

   std::vector<X509_DN> client_auth_CAs;
   for(const auto store : trusted_CAs)
      {
      const auto subjects = store->all_subjects();
      client_auth_CAs.insert(client_auth_CAs.end(), subjects.begin(), subjects.end());
      }

   if(client_auth_CAs.empty() && !policy.request_client_certificate_authentication())
      {
      return std::nullopt;
      }

   return Certificate_Request_13(policy.acceptable_signature_schemes(),
                                 std::move(client_auth_CAs),
                                 callbacks);
   }

std::vector<X509_DN> Certificate_Request_13::acceptable_CAs() const
   {
   if(m_extensions.has<Certificate_Authorities>())
      return m_extensions.get<Certificate_Authorities>()->distinguished_names();
   return {};
   }

const std::vector<Signature_Scheme>& Certificate_Request_13::signature_schemes() const
   {
   // RFC 8446 4.3.2
   //    The "signature_algorithms" extension MUST be specified
   BOTAN_ASSERT_NOMSG(m_extensions.has<Signature_Algorithms>());

   return m_extensions.get<Signature_Algorithms>()->supported_schemes();
   }

std::vector<uint8_t> Certificate_Request_13::serialize() const
   {
   std::vector<uint8_t> buf;
   append_tls_length_value(buf, m_context, 1);
   buf += m_extensions.serialize(Connection_Side::SERVER);
   return buf;
   }

}  // namespace Botan::TLS
