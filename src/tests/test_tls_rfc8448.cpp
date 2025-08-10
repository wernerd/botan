/*
* (C) 2021 Jack Lloyd
* (C) 2021 René Meusel
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "tests.h"
#include <memory>
#include <utility>
// Since RFC 8448 uses a specific set of cipher suites we can only run this
// test if all of them are enabled.
#if defined(BOTAN_HAS_TLS_13) && \
   defined(BOTAN_HAS_AEAD_CHACHA20_POLY1305) && \
   defined(BOTAN_HAS_AEAD_GCM) && \
   defined(BOTAN_HAS_AES) && \
   defined(BOTAN_HAS_CURVE_25519) && \
   defined(BOTAN_HAS_SHA2_32) && \
   defined(BOTAN_HAS_SHA2_64)
   #define BOTAN_CAN_RUN_TEST_TLS_RFC8448
#endif

#if defined(BOTAN_CAN_RUN_TEST_TLS_RFC8448)
   #include "test_rng.h"
   #include "test_tls_utils.h"

   #include <botan/auto_rng.h>  // TODO: replace me, otherwise we depend on auto_rng module
   #include <botan/credentials_manager.h>
   #include <botan/rsa.h>
   #include <botan/tls_alert.h>
   #include <botan/tls_callbacks.h>
   #include <botan/tls_client.h>
   #include <botan/tls_policy.h>
   #include <botan/tls_messages.h>
   #include <botan/internal/tls_reader.h>
   #include <botan/tls_server.h>
   #include <botan/tls_server_info.h>
   #include <botan/tls_session.h>
   #include <botan/tls_session_manager.h>
   #include <botan/tls_version.h>

   #include <botan/assert.h>
   #include <botan/internal/stl_util.h>

   #include <botan/internal/loadstor.h>

   #include <botan/pkcs8.h>
   #include <botan/data_src.h>
#endif

namespace Botan_Tests {

#if defined(BOTAN_CAN_RUN_TEST_TLS_RFC8448)

namespace {

void add_entropy(Botan_Tests::Fixed_Output_RNG& rng, const std::vector<uint8_t>& bin)
   {
   rng.add_entropy(bin.data(), bin.size());
   }

// TODO: use this once the server side is being implemented
// std::unique_ptr<Botan::Private_Key> server_private_key()
//    {
//    Botan::DataSource_Memory in(Test::read_data_file("tls_13_rfc8448/server_key.pem"));
//    return Botan::PKCS8::load_key(in);
//    }

Botan::X509_Certificate server_certificate()
   {
   // self-signed certificate with an RSA1024 public key valid until:
   //   Jul 30 01:23:59 2026 GMT
   Botan::DataSource_Memory in(Test::read_data_file("tls_13_rfc8448/server_certificate.pem"));
   return Botan::X509_Certificate(in);
   }

/**
* Simple version of the Padding extension (RFC 7685) to reproduce the
* 2nd Client_Hello in RFC8448 Section 5 (HelloRetryRequest)
*/
class Padding final : public Botan::TLS::Extension
   {
   public:
      static Botan::TLS::Handshake_Extension_Type static_type()
         { return Botan::TLS::Handshake_Extension_Type(21); }

      Botan::TLS::Handshake_Extension_Type type() const override { return static_type(); }

      explicit Padding(const size_t padding_bytes) :
         m_padding_bytes(padding_bytes) {}

      std::vector<uint8_t> serialize(Botan::TLS::Connection_Side) const override
         {
         return std::vector<uint8_t>(m_padding_bytes, 0x00);
         }

      bool empty() const override { return m_padding_bytes == 0; }
   private:
      size_t m_padding_bytes;
   };

using namespace Botan;
using namespace Botan::TLS;

std::chrono::system_clock::time_point from_milliseconds_since_epoch(uint64_t msecs)
   {
   const int64_t  secs_since_epoch  = msecs / 1000;
   const uint32_t additional_millis = msecs % 1000;

   BOTAN_ASSERT_NOMSG(secs_since_epoch <= std::numeric_limits<time_t>::max());
   return std::chrono::system_clock::from_time_t(static_cast<time_t>(secs_since_epoch)) +
          std::chrono::milliseconds(additional_millis);
   }

/**
 * Subclass of the Botan::TLS::Callbacks instrumenting all available callbacks.
 * The invocation counts can be checked in the integration tests to make sure
 * all expected callbacks are hit. Furthermore collects the received application
 * data and sent record bytes for further inspection by the test cases.
 */
using Modify_Exts_Fn = std::function<void(Botan::TLS::Extensions&, Botan::TLS::Connection_Side)>;
class Test_TLS_13_Callbacks : public Botan::TLS::Callbacks
   {
   public:
      Test_TLS_13_Callbacks(Modify_Exts_Fn modify_exts_cb, uint64_t timestamp) :
         session_activated_called(false),
         m_modify_exts(std::move(modify_exts_cb)),
         m_timestamp(from_milliseconds_since_epoch(timestamp))
         {}

      void tls_emit_data(const uint8_t data[], size_t size) override
         {
         count_callback_invocation("tls_emit_data");
         send_buffer.insert(send_buffer.end(), data, data + size);
         }

      void tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size) override
         {
         count_callback_invocation("tls_record_received");
         received_seq_no = seq_no;
         receive_buffer.insert(receive_buffer.end(), data, data + size);
         }

      void tls_alert(Botan::TLS::Alert alert) override
         {
         count_callback_invocation("tls_alert");
         BOTAN_UNUSED(alert);
         // handle a tls alert received from the tls server
         }

      bool tls_session_established(const Botan::TLS::Session& session) override
         {
         count_callback_invocation("tls_session_established");
         BOTAN_UNUSED(session);
         // the session with the tls client was established
         // return false to prevent the session from being cached, true to
         // cache the session in the configured session manager
         return false;
         }

      void tls_session_activated() override
         {
         count_callback_invocation("tls_session_activated");
         session_activated_called = true;
         }

      void tls_verify_cert_chain(
         const std::vector<Botan::X509_Certificate>& cert_chain,
         const std::vector<std::optional<Botan::OCSP::Response>>&,
         const std::vector<Botan::Certificate_Store*>&,
         Botan::Usage_Type,
         const std::string&,
         const Botan::TLS::Policy&) override
         {
         count_callback_invocation("tls_verify_cert_chain");
         certificate_chain = cert_chain;
         }

      std::chrono::milliseconds tls_verify_cert_chain_ocsp_timeout() const override
         {
         count_callback_invocation("tls_verify_cert_chain");
         return std::chrono::milliseconds(0);
         }

      std::vector<uint8_t> tls_provide_cert_status(const std::vector<X509_Certificate>& chain,
            const Certificate_Status_Request& csr) override
         {
         count_callback_invocation("tls_provide_cert_status");
         return Callbacks::tls_provide_cert_status(chain, csr);
         }

      std::vector<uint8_t> tls_sign_message(
         const Private_Key& key,
         RandomNumberGenerator& rng,
         const std::string& emsa,
         Signature_Format format,
         const std::vector<uint8_t>& msg) override
         {
         count_callback_invocation("tls_sign_message");
         return Callbacks::tls_sign_message(key, rng, emsa, format, msg);
         }


      bool tls_verify_message(
         const Public_Key& key,
         const std::string& emsa,
         Signature_Format format,
         const std::vector<uint8_t>& msg,
         const std::vector<uint8_t>& sig) override
         {
         count_callback_invocation("tls_verify_message");
         return Callbacks::tls_verify_message(key, emsa, format, msg, sig);
         }

      std::pair<secure_vector<uint8_t>, std::vector<uint8_t>> tls_dh_agree(
               const std::vector<uint8_t>& modulus,
               const std::vector<uint8_t>& generator,
               const std::vector<uint8_t>& peer_public_value,
               const Policy& policy,
               RandomNumberGenerator& rng) override
         {
         count_callback_invocation("tls_dh_agree");
         return Callbacks::tls_dh_agree(modulus, generator, peer_public_value, policy, rng);
         }

      std::pair<secure_vector<uint8_t>, std::vector<uint8_t>> tls_ecdh_agree(
               const std::string& curve_name,
               const std::vector<uint8_t>& peer_public_value,
               const Policy& policy,
               RandomNumberGenerator& rng,
               bool compressed) override
         {
         count_callback_invocation("tls_ecdh_agree");
         return Callbacks::tls_ecdh_agree(curve_name, peer_public_value, policy, rng, compressed);
         }

      void tls_inspect_handshake_msg(const Handshake_Message& message) override
         {
         count_callback_invocation("tls_inspect_handshake_msg_" + message.type_string());
         return Callbacks::tls_inspect_handshake_msg(message);
         }

      std::string tls_server_choose_app_protocol(const std::vector<std::string>& client_protos) override
         {
         count_callback_invocation("tls_server_choose_app_protocol");
         return Callbacks::tls_server_choose_app_protocol(client_protos);
         }

      void tls_modify_extensions(Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side) override
         {
         count_callback_invocation("tls_modify_extensions");
         m_modify_exts(exts, side);
         }

      void tls_examine_extensions(const Botan::TLS::Extensions& extn, Connection_Side which_side) override
         {
         count_callback_invocation("tls_examine_extensions");
         return Callbacks::tls_examine_extensions(extn, which_side);
         }

      std::string tls_decode_group_param(Group_Params group_param) override
         {
         count_callback_invocation("tls_decode_group_param");
         return Callbacks::tls_decode_group_param(group_param);
         }

      std::string tls_peer_network_identity() override
         {
         count_callback_invocation("tls_peer_network_identity");
         return Callbacks::tls_peer_network_identity();
         }

      std::chrono::system_clock::time_point tls_current_timestamp() override
         {
         count_callback_invocation("tls_current_timestamp");
         return m_timestamp;
         }

      std::vector<uint8_t> pull_send_buffer()
         {
         return std::exchange(send_buffer, std::vector<uint8_t>());
         }

      std::vector<uint8_t> pull_receive_buffer()
         {
         return std::exchange(receive_buffer, std::vector<uint8_t>());
         }

      uint64_t last_received_seq_no() const { return received_seq_no; }

      const std::map<std::string, unsigned int>& callback_invocations() const
         {
         return m_callback_invocations;
         }

      void reset_callback_invocation_counters()
         {
         m_callback_invocations.clear();
         }

   private:
      void count_callback_invocation(const std::string& callback_name) const
         {
         if(m_callback_invocations.count(callback_name) == 0)
            { m_callback_invocations[callback_name] = 0; }

         m_callback_invocations[callback_name]++;
         }

   public:
      bool session_activated_called;

      std::vector<Botan::X509_Certificate> certificate_chain;

   private:
      std::vector<uint8_t>                  send_buffer;
      std::vector<uint8_t>                  receive_buffer;
      uint64_t                              received_seq_no;
      Modify_Exts_Fn                        m_modify_exts;
      std::chrono::system_clock::time_point m_timestamp;

      mutable std::map<std::string, unsigned int> m_callback_invocations;
   };

class RFC8448_Text_Policy : public Botan::TLS::Text_Policy
   {
   public:
      RFC8448_Text_Policy(const Botan::TLS::Text_Policy& other)
         : Text_Policy(other) {}

      std::vector<Botan::TLS::Signature_Scheme> allowed_signature_schemes() const override
         {
         // We extend the allowed signature schemes with algorithms that we don't
         // actually support. The nature of the RFC 8448 test forces us to generate
         // bit-compatible TLS messages. Unfortunately, the test data offers all
         // those algorithms in its Client Hellos.
         return
            {
            Botan::TLS::Signature_Scheme::ECDSA_SHA256,
            Botan::TLS::Signature_Scheme::ECDSA_SHA384,
            Botan::TLS::Signature_Scheme::ECDSA_SHA512,
            Botan::TLS::Signature_Scheme::ECDSA_SHA1,       // not actually supported
            Botan::TLS::Signature_Scheme::RSA_PSS_SHA256,
            Botan::TLS::Signature_Scheme::RSA_PSS_SHA384,
            Botan::TLS::Signature_Scheme::RSA_PSS_SHA512,
            Botan::TLS::Signature_Scheme::RSA_PKCS1_SHA256,
            Botan::TLS::Signature_Scheme::RSA_PKCS1_SHA384,
            Botan::TLS::Signature_Scheme::RSA_PKCS1_SHA512,
            Botan::TLS::Signature_Scheme::RSA_PKCS1_SHA1,   // not actually supported
            Botan::TLS::Signature_Scheme::DSA_SHA256,       // not actually supported
            Botan::TLS::Signature_Scheme::DSA_SHA384,       // not actually supported
            Botan::TLS::Signature_Scheme::DSA_SHA512,       // not actually supported
            Botan::TLS::Signature_Scheme::DSA_SHA1          // not actually supported
            };
         }
   };

/**
 * This steers the TLS client handle and is the central entry point for the
 * test cases to interact with the TLS 1.3 implementation.
 *
 * Note: This class is abstract to be subclassed for both client and server tests.
 */
class TLS_Context
   {
   protected:
      TLS_Context(std::unique_ptr<Botan::RandomNumberGenerator> rng_in,
                  RFC8448_Text_Policy policy,
                  Modify_Exts_Fn modify_exts_cb,
                  uint64_t timestamp)
         : m_callbacks(std::move(modify_exts_cb), timestamp)
         , m_rng(std::move(rng_in))
         , m_session_mgr(*m_rng)
         , m_policy(std::move(policy))
         {}

   public:
      virtual ~TLS_Context() = default;

      TLS_Context(TLS_Context&) = delete;
      TLS_Context& operator=(const TLS_Context&) = delete;

      TLS_Context(TLS_Context&&) = delete;
      TLS_Context& operator=(TLS_Context&&) = delete;

      std::vector<uint8_t> pull_send_buffer()
         {
         return m_callbacks.pull_send_buffer();
         }

      std::vector<uint8_t> pull_receive_buffer()
         {
         return m_callbacks.pull_receive_buffer();
         }

      uint64_t last_received_seq_no() const { return m_callbacks.last_received_seq_no(); }

      /**
       * Checks that all of the listed callbacks were called at least once, no other
       * callbacks were called in addition to the expected ones. After the checks are
       * done, the callback invocation counters are reset.
       */
      void check_callback_invocations(Test::Result& result, const std::string& context,
                                      const std::vector<std::string>& callback_names)
         {
         const auto& invokes = m_callbacks.callback_invocations();
         for(const auto& cbn : callback_names)
            {
            result.confirm(cbn + " was invoked (Context: " + context + ")", invokes.count(cbn) > 0 && invokes.at(cbn) > 0);
            }

         for(const auto& invoke : invokes)
            {
            if(invoke.second == 0)
               { continue; }
            result.confirm(invoke.first + " was expected (Context: " + context + ")", std::find(callback_names.cbegin(),
                           callback_names.cend(), invoke.first) != callback_names.cend());
            }

         m_callbacks.reset_callback_invocation_counters();
         }

      const std::vector<Botan::X509_Certificate>& certs_verified() const
         {
         return m_callbacks.certificate_chain;
         }

      virtual void send(const std::vector<uint8_t>& data) = 0;

   protected:
      Test_TLS_13_Callbacks      m_callbacks;
      Botan::Credentials_Manager m_creds;

      std::unique_ptr<Botan::RandomNumberGenerator> m_rng;
      Botan::TLS::Session_Manager_In_Memory         m_session_mgr;
      RFC8448_Text_Policy                           m_policy;
   };

class Client_Context : public TLS_Context
   {
   public:
      Client_Context(std::unique_ptr<Botan::RandomNumberGenerator> rng_in,
                     RFC8448_Text_Policy policy,
                     Modify_Exts_Fn modify_exts_cb,
                     uint64_t timestamp)
         : TLS_Context(std::move(rng_in), std::move(policy), std::move(modify_exts_cb), timestamp)
         , client(m_callbacks, m_session_mgr, m_creds, m_policy, *m_rng,
                  Botan::TLS::Server_Information("server"),
                  Botan::TLS::Protocol_Version::TLS_V13)
         {}

      void send(const std::vector<uint8_t>& data) override
         {
         client.send(data.data(), data.size());
         }

      Botan::TLS::Client client;
   };

/**
 * Because of the nature of the RFC 8448 test data we need to produce bit-compatible
 * TLS messages. Hence we sort the generated TLS extensions exactly as expected.
 */
void sort_extensions(Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side)
   {
   if(side == Botan::TLS::Connection_Side::CLIENT)
      {
      const std::vector<Botan::TLS::Handshake_Extension_Type> expected_order =
         {
         Botan::TLS::Handshake_Extension_Type::TLSEXT_SERVER_NAME_INDICATION,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_SAFE_RENEGOTIATION,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_SUPPORTED_GROUPS,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_SESSION_TICKET,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_KEY_SHARE,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_SUPPORTED_VERSIONS,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_SIGNATURE_ALGORITHMS,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_COOKIE,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_PSK_KEY_EXCHANGE_MODES,
         Botan::TLS::Handshake_Extension_Type::TLSEXT_RECORD_SIZE_LIMIT,
         Padding::static_type()
         };

      for(const auto ext_type : expected_order)
         {
         auto ext = exts.take(ext_type);
         if(ext != nullptr)
            {
            exts.add(std::move(ext));
            }
         }
      }
   }

void add_psk_exchange_modes(Botan::TLS::Extensions& exts)
   {
   // Currently we do not support PSK and session resumption in TLS 1.3.
   // Hence, we add this extension to please the test vector. The actual
   // resumption is not exercised in this test, though. Once PSK is
   // implemented, this should be removed and added in Client_Hello_13.
   exts.add(new PSK_Key_Exchange_Modes({PSK_Key_Exchange_Mode::PSK_DHE_KE}));
   }

void add_renegotiation_extension(Botan::TLS::Extensions& exts)
   {
   // Renegotiation is not possible in TLS 1.3. Nevertheless, RFC 8448 requires
   // to add this to the Client Hello for reasons.
   exts.add(new Renegotiation_Extension());
   }

}  // namespace

/**
 * Traffic transcripts and supporting data for the TLS RFC 8448 and TLS policy
 * configuration is kept in data files (accessible via `Test:::data_file()`).
 *
 * tls_13_rfc8448/transcripts.vec
 *   The record transcripts and RNG outputs as defined/required in RFC 8448 in
 *   Botan's Text_Based_Test vector format. Data from each RFC 8448 section is
 *   placed in a sub-section of the *.vec file. Each of those sections needs a
 *   specific test case implementation that is dispatched in `run_one_test()`.
 *
 * tls_13_rfc8448/client_certificate.pem
 *   The client certificate provided in RFC 8448 used to perform client auth.
 *   Note that RFC 8448 _does not_ provide the associated private key but only
 *   the resulting signature in the client's CertificateVerify message.
 *
 * tls_13_rfc8448/server_certificate.pem
 * tls_13_rfc8448/server_key.pem
 *   The server certificate and its associated private key.
 *
 * tls-policy/rfc8448_*.txt
 *   Each RFC 8448 section test required a slightly adapted Botan TLS policy
 *   to enable/disable certain features under test.
 */
class Test_TLS_RFC8448 final : public Text_Based_Test
   {
   public:
      Test_TLS_RFC8448()
         : Text_Based_Test("tls_13_rfc8448/transcripts.vec", "RNG_Pool,CurrentTimestamp,ClientHello_1,ServerHello,ServerHandshakeMessages,ClientFinished,Client_CloseNotify,Server_CloseNotify", "HelloRetryRequest,ClientHello_2,NewSessionTicket,Client_AppData,Client_AppData_Record,Server_AppData,Server_AppData_Record") {}

   Test::Result run_one_test(const std::string& header,
                             const VarMap& vars) override
      {
      if(header == "Simple_1RTT_Handshake")
         return Test::Result("Simple 1-RTT (Client side)", simple_1_rtt_client_hello(vars));
      else if(header == "HelloRetryRequest_Handshake")
         return Test::Result("Handshake involving Hello Retry Request (Client side)", hello_retry_request(vars));
      else if(header == "Middlebox_Compatibility_Mode")
         return Test::Result("Middlebox Compatibility Mode (Client side)", middlebox_compatibility(vars));
      else
         return Test::Result::Failure("test dispatcher", "unknown sub-test: " + header);
      }

   private:
      static std::vector<Test::Result> simple_1_rtt_client_hello(const VarMap& vars)
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");
         rng->add_entropy(std::vector<uint8_t>(32).data(), 32);  // used by session mgr for session key

         // 32 - for client hello random
         // 32 - for KeyShare (eph. x25519 key pair)
         add_entropy(*rng, vars.get_req_bin("RNG_Pool"));

         auto add_extensions_and_sort = [](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side)
            {
            // For some reason, presumably checking compatibility, the RFC 8448 Client
            // Hello includes a (TLS 1.2) Session_Ticket extension. We don't normally add
            // this obsoleted extension in a TLS 1.3 client.
            exts.add(new Botan::TLS::Session_Ticket());

            add_psk_exchange_modes(exts);
            add_renegotiation_extension(exts);
            sort_extensions(exts, side);
            };

         std::unique_ptr<Client_Context> ctx;

         return {
            Botan_Tests::CHECK("Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Client_Context>(std::move(rng), read_tls_policy("rfc8448_1rtt"), add_extensions_and_sort, vars.get_req_u64("CurrentTimestamp"));

               result.confirm("client not closed", !ctx->client.is_closed());
               ctx->check_callback_invocations(result, "client hello prepared", { "tls_emit_data", "tls_inspect_handshake_msg_client_hello", "tls_modify_extensions" });

               result.test_eq("TLS client hello", ctx->pull_send_buffer(), vars.get_req_bin("ClientHello_1"));
               }),

            Botan_Tests::CHECK("Server Hello", [&](Test::Result& result)
               {
               const auto server_hello = vars.get_req_bin("ServerHello");
               // splitting the input data to test partial reads
               const std::vector<uint8_t> server_hello_a(server_hello.begin(), server_hello.begin() + 20);
               const std::vector<uint8_t> server_hello_b(server_hello.begin() + 20, server_hello.end());

               ctx->client.received_data(server_hello_a);
               ctx->check_callback_invocations(result, "server hello partially received", { });

               ctx->client.received_data(server_hello_b);
               ctx->check_callback_invocations(result, "server hello received", { "tls_inspect_handshake_msg_server_hello", "tls_examine_extensions" });

               result.confirm("client is not yet active", !ctx->client.is_active());
               }),

            Botan_Tests::CHECK("Server HS messages .. Client Finished", [&](Test::Result& result)
               {
               ctx->client.received_data(vars.get_req_bin("ServerHandshakeMessages"));

               ctx->check_callback_invocations(result, "encrypted handshake messages received",
                  {
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished",
                  "tls_examine_extensions",
                  "tls_emit_data",
                  "tls_session_activated",
                  "tls_verify_cert_chain",
                  "tls_verify_message"
                  });
               result.require("correct certificate", ctx->certs_verified().front() == server_certificate());
               result.require("client is active", ctx->client.is_active());

               result.test_eq("correct handshake finished", ctx->pull_send_buffer(),
                              vars.get_req_bin("ClientFinished"));
               }),

            Botan_Tests::CHECK("Post-Handshake: NewSessionTicket", [&](Test::Result& result)
               {
               ctx->client.received_data(vars.get_req_bin("NewSessionTicket"));

               // TODO: once we implement session resumption, this should probably expect some callback
               ctx->check_callback_invocations(result, "new session ticket received", { });
               }),

            Botan_Tests::CHECK("Send Application Data", [&](Test::Result& result)
               {
               ctx->send(vars.get_req_bin("Client_AppData"));

               ctx->check_callback_invocations(result, "application data sent", { "tls_emit_data" });

               result.test_eq("correct client application data", ctx->pull_send_buffer(),
                              vars.get_req_bin("Client_AppData_Record"));
               }),

            Botan_Tests::CHECK("Receive Application Data", [&](Test::Result& result)
               {
               ctx->client.received_data(vars.get_req_bin("Server_AppData_Record"));

               ctx->check_callback_invocations(result, "application data sent", { "tls_record_received" });

               const auto rcvd = ctx->pull_receive_buffer();
               result.test_eq("decrypted application traffic", rcvd, vars.get_req_bin("Server_AppData"));
               result.test_is_eq("sequence number", ctx->last_received_seq_no(), uint64_t(1));
               }),

            Botan_Tests::CHECK("Close Connection", [&](Test::Result& result)
               {
               ctx->client.close();

               result.test_eq("close payload", ctx->pull_send_buffer(), vars.get_req_bin("Client_CloseNotify"));
               ctx->check_callback_invocations(result, "CLOSE_NOTIFY sent", { "tls_emit_data" });

               ctx->client.received_data(vars.get_req_bin("Server_CloseNotify"));
               ctx->check_callback_invocations(result, "CLOSE_NOTIFY received", { "tls_alert" });

               result.confirm("connection is closed", ctx->client.is_closed());
               }),
            };
         }

      static std::vector<Test::Result> hello_retry_request(const VarMap& vars)
         {
         auto add_extensions_and_sort = [flights = 0](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side) mutable
            {
            ++flights;

            if(flights == 1)
               {
               add_psk_exchange_modes(exts);
               add_renegotiation_extension(exts);
               }

            // For some reason RFC8448 decided to require this (fairly obscure) extension
            // in the second flight of the Client_Hello.
            if(flights == 2)
               {
               exts.add(new Padding(175));
               }

            sort_extensions(exts, side);
            };

         // Fallback RNG is required to for blinding in ECDH with P-256
         auto& fallback_rng = Test::rng();
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>(fallback_rng);

         rng->add_entropy(std::vector<uint8_t>(32).data(), 32);  // used by session mgr for session key

         // 32 - client hello random
         // 32 - eph. x25519 key pair
         // 32 - eph. P-256 key pair
         add_entropy(*rng, vars.get_req_bin("RNG_Pool"));

         std::unique_ptr<Client_Context> ctx;

         return {
            Botan_Tests::CHECK("Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Client_Context>(std::move(rng), read_tls_policy("rfc8448_hrr"), add_extensions_and_sort, vars.get_req_u64("CurrentTimestamp"));
               result.confirm("client not closed", !ctx->client.is_closed());

               ctx->check_callback_invocations(result, "client hello prepared", { "tls_emit_data", "tls_inspect_handshake_msg_client_hello", "tls_modify_extensions" });

               result.test_eq("TLS client hello (1)", ctx->pull_send_buffer(), vars.get_req_bin("ClientHello_1"));
               }),

            Botan_Tests::CHECK("Hello Retry Request .. second Client Hello", [&](Test::Result& result)
               {
               ctx->client.received_data(vars.get_req_bin("HelloRetryRequest"));

               ctx->check_callback_invocations(result, "hello retry request received",
                  {
                  "tls_emit_data",
                  "tls_inspect_handshake_msg_hello_retry_request",
                  "tls_examine_extensions",
                  "tls_inspect_handshake_msg_client_hello",
                  "tls_modify_extensions",
                  "tls_decode_group_param"
                  });

               result.test_eq("TLS client hello (2)", ctx->pull_send_buffer(), vars.get_req_bin("ClientHello_2"));
               }),

            Botan_Tests::CHECK("Server Hello", [&](Test::Result& result)
               {
               ctx->client.received_data(vars.get_req_bin("ServerHello"));

               ctx->check_callback_invocations(result, "server hello received", { "tls_inspect_handshake_msg_server_hello", "tls_examine_extensions", "tls_decode_group_param" });
               }),

            Botan_Tests::CHECK("Server HS Messages .. Client Finished", [&](Test::Result& result)
               {
               ctx->client.received_data(vars.get_req_bin("ServerHandshakeMessages"));

               ctx->check_callback_invocations(result, "encrypted handshake messages received",
                  {
                  "tls_inspect_handshake_msg_encrypted_extensions",
                  "tls_inspect_handshake_msg_certificate",
                  "tls_inspect_handshake_msg_certificate_verify",
                  "tls_inspect_handshake_msg_finished",
                  "tls_examine_extensions",
                  "tls_emit_data",
                  "tls_session_activated",
                  "tls_verify_cert_chain",
                  "tls_verify_message"
                  });

               result.test_eq("client finished", ctx->pull_send_buffer(), vars.get_req_bin("ClientFinished"));
               }),

            Botan_Tests::CHECK("Close Connection", [&](Test::Result& result)
               {
               ctx->client.close();
               ctx->check_callback_invocations(result, "encrypted handshake messages received", { "tls_emit_data" });
               result.test_eq("client close notify", ctx->pull_send_buffer(), vars.get_req_bin("Client_CloseNotify"));

               ctx->client.received_data(vars.get_req_bin("Server_CloseNotify"));
               ctx->check_callback_invocations(result, "encrypted handshake messages received", { "tls_alert" });

               result.confirm("connection is closed", ctx->client.is_closed());
               }),
            };
         }

      static std::vector<Test::Result> middlebox_compatibility(const VarMap& vars)
         {
         auto rng = std::make_unique<Botan_Tests::Fixed_Output_RNG>("");
         rng->add_entropy(std::vector<uint8_t>(32).data(), 32);  // used by session mgr for session key

         // 32 - client hello random
         // 32 - legacy session ID
         // 32 - eph. x25519 key pair
         add_entropy(*rng, vars.get_req_bin("RNG_Pool"));

         auto add_extensions_and_sort = [&](Botan::TLS::Extensions& exts, Botan::TLS::Connection_Side side)
            {
            add_renegotiation_extension(exts);
            add_psk_exchange_modes(exts);
            sort_extensions(exts, side);
            };

         std::unique_ptr<Client_Context> ctx;

         return {
            Botan_Tests::CHECK("Client Hello", [&](Test::Result& result)
               {
               ctx = std::make_unique<Client_Context>(std::move(rng), read_tls_policy("rfc8448_compat"), add_extensions_and_sort, vars.get_req_u64("CurrentTimestamp"));

               result.test_eq("Client Hello", ctx->pull_send_buffer(), vars.get_req_bin("ClientHello_1"));
               }),

            Botan_Tests::CHECK("Server Hello + other handshake messages", [&](Test::Result& result)
               {
               ctx->client.received_data(Botan::concat(vars.get_req_bin("ServerHello"),
                                                      // ServerHandshakeMessages contains the expected ChangeCipherSpec record
                                                      vars.get_req_bin("ServerHandshakeMessages")));

               result.test_eq("CCS + Client Finished", ctx->pull_send_buffer(),
                              // ClientFinished contains the expected ChangeCipherSpec record
                              vars.get_req_bin("ClientFinished"));

               result.confirm("client is ready to send application traffic", ctx->client.is_active());
               }),

            Botan_Tests::CHECK("Close connection", [&](Test::Result& result)
               {
               ctx->client.close();

               result.test_eq("Client close_notify", ctx->pull_send_buffer(), vars.get_req_bin("Client_CloseNotify"));

               result.require("client cannot send application traffic anymore", !ctx->client.is_active());
               result.require("client is not fully closed yet", !ctx->client.is_closed());

               ctx->client.received_data(vars.get_req_bin("Server_CloseNotify"));

               result.confirm("client connection was terminated", ctx->client.is_closed());
               }),
            };
         }
   };

BOTAN_REGISTER_TEST("tls", "tls_rfc8448", Test_TLS_RFC8448);

#endif

}
