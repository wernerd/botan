/*
* (C) 2009,2010,2014,2015,2017,2018 Jack Lloyd
* (C) 2015 Simon Warta (Kullo GmbH)
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "cli.h"
#include "perf.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

// Always available:
#include <botan/entropy_src.h>
#include <botan/version.h>
#include <botan/internal/cpuid.h>
#include <botan/internal/fmt.h>
#include <botan/internal/os_utils.h>
#include <botan/internal/stl_util.h>
#include <botan/internal/timer.h>

#if defined(BOTAN_HAS_BLOCK_CIPHER)
   #include <botan/block_cipher.h>
#endif

#if defined(BOTAN_HAS_STREAM_CIPHER)
   #include <botan/stream_cipher.h>
#endif

#if defined(BOTAN_HAS_HASH)
   #include <botan/hash.h>
#endif

#if defined(BOTAN_HAS_XOF)
   #include <botan/xof.h>
#endif

#if defined(BOTAN_HAS_CIPHER_MODES)
   #include <botan/cipher_mode.h>
#endif

#if defined(BOTAN_HAS_MAC)
   #include <botan/mac.h>
#endif

#if defined(BOTAN_HAS_PUBLIC_KEY_CRYPTO)
   #include <botan/pk_algs.h>
   #include <botan/pkcs8.h>
   #include <botan/pubkey.h>
   #include <botan/x509_key.h>
#endif

#if defined(BOTAN_HAS_ECC_GROUP)
   #include <botan/ec_group.h>
#endif

#if defined(BOTAN_HAS_PCURVES)
   #include <botan/internal/pcurves.h>
#endif

#if defined(BOTAN_HAS_MCELIECE)
   #include <botan/mceliece.h>
#endif

#if defined(BOTAN_HAS_KYBER) || defined(BOTAN_HAS_KYBER_90S)
   #include <botan/kyber.h>
#endif

#if defined(BOTAN_HAS_DILITHIUM) || defined(BOTAN_HAS_DILITHIUM_AES)
   #include <botan/dilithium.h>
#endif

#if defined(BOTAN_HAS_HSS_LMS)
   #include <botan/hss_lms.h>
#endif

#if defined(BOTAN_HAS_SPHINCS_PLUS_WITH_SHA2) || defined(BOTAN_HAS_SPHINCS_PLUS_WITH_SHAKE)
   #include <botan/sphincsplus.h>
#endif

#if defined(BOTAN_HAS_FRODOKEM)
   #include <botan/frodokem.h>
#endif

#if defined(BOTAN_HAS_ECDSA)
   #include <botan/ecdsa.h>
#endif

namespace Botan_CLI {

using Botan::Timer;

namespace {

class JSON_Output final {
   public:
      void add(const Timer& timer) { m_results.push_back(timer); }

      std::string print() const {
         std::ostringstream out;

         out << "[\n";

         for(size_t i = 0; i != m_results.size(); ++i) {
            const Timer& t = m_results[i];

            out << "{"
                << "\"algo\": \"" << t.get_name() << "\", "
                << "\"op\": \"" << t.doing() << "\", "
                << "\"events\": " << t.events() << ", ";

            if(t.cycles_consumed() > 0) {
               out << "\"cycles\": " << t.cycles_consumed() << ", ";
            }

            if(t.buf_size() > 0) {
               out << "\"bps\": " << static_cast<uint64_t>(t.events() / (t.value() / 1000000000.0)) << ", ";
               out << "\"buf_size\": " << t.buf_size() << ", ";
            }

            out << "\"nanos\": " << t.value() << "}";

            if(i != m_results.size() - 1) {
               out << ",";
            }

            out << "\n";
         }
         out << "]\n";

         return out.str();
      }

   private:
      std::vector<Timer> m_results;
};

class Summary final {
   public:
      Summary() = default;

      void add(const Timer& t) {
         if(t.buf_size() == 0) {
            m_ops_entries.push_back(t);
         } else {
            m_bps_entries[std::make_pair(t.doing(), t.get_name())].push_back(t);
         }
      }

      std::string print() {
         const size_t name_padding = 35;
         const size_t op_name_padding = 16;
         const size_t op_padding = 16;

         std::ostringstream result_ss;
         result_ss << std::fixed;

         if(!m_bps_entries.empty()) {
            result_ss << "\n";

            // add table header
            result_ss << std::setw(name_padding) << std::left << "algo" << std::setw(op_name_padding) << std::left
                      << "operation";

            for(const Timer& t : m_bps_entries.begin()->second) {
               result_ss << std::setw(op_padding) << std::right << (std::to_string(t.buf_size()) + " bytes");
            }
            result_ss << "\n";

            // add table entries
            for(const auto& entry : m_bps_entries) {
               if(entry.second.empty()) {
                  continue;
               }

               result_ss << std::setw(name_padding) << std::left << (entry.first.second) << std::setw(op_name_padding)
                         << std::left << (entry.first.first);

               for(const Timer& t : entry.second) {
                  if(t.events() == 0) {
                     result_ss << std::setw(op_padding) << std::right << "N/A";
                  } else {
                     result_ss << std::setw(op_padding) << std::right << std::setprecision(2)
                               << (t.bytes_per_second() / 1000.0);
                  }
               }

               result_ss << "\n";
            }

            result_ss << "\n[results are the number of 1000s bytes processed per second]\n";
         }

         if(!m_ops_entries.empty()) {
            result_ss << std::setprecision(6) << "\n";

            // sort entries
            std::sort(m_ops_entries.begin(), m_ops_entries.end());

            // add table header
            result_ss << std::setw(name_padding) << std::left << "algo" << std::setw(op_name_padding) << std::left
                      << "operation" << std::setw(op_padding) << std::right << "sec/op" << std::setw(op_padding)
                      << std::right << "op/sec"
                      << "\n";

            // add table entries
            for(const Timer& entry : m_ops_entries) {
               result_ss << std::setw(name_padding) << std::left << entry.get_name() << std::setw(op_name_padding)
                         << std::left << entry.doing() << std::setw(op_padding) << std::right
                         << entry.seconds_per_event() << std::setw(op_padding) << std::right
                         << entry.events_per_second() << "\n";
            }
         }

         return result_ss.str();
      }

   private:
      std::map<std::pair<std::string, std::string>, std::vector<Timer>> m_bps_entries;
      std::vector<Timer> m_ops_entries;
};

std::vector<size_t> unique_buffer_sizes(const std::string& cmdline_arg) {
   const size_t MAX_BUF_SIZE = 64 * 1024 * 1024;

   std::set<size_t> buf;
   for(const std::string& size_str : Command::split_on(cmdline_arg, ',')) {
      size_t x = 0;
      try {
         size_t converted = 0;
         x = static_cast<size_t>(std::stoul(size_str, &converted, 0));

         if(converted != size_str.size()) {
            throw CLI_Usage_Error("Invalid integer");
         }
      } catch(std::exception&) {
         throw CLI_Usage_Error("Invalid integer value '" + size_str + "' for option buf-size");
      }

      if(x == 0) {
         throw CLI_Usage_Error("Cannot have a zero-sized buffer");
      }

      if(x > MAX_BUF_SIZE) {
         throw CLI_Usage_Error("Specified buffer size is too large");
      }

      buf.insert(x);
   }

   return std::vector<size_t>(buf.begin(), buf.end());
}

}  // namespace

class Speed final : public Command {
   public:
      Speed() :
            Command(
               "speed --msec=500 --format=default --ecc-groups= --provider= --buf-size=1024 --clear-cpuid= --cpu-clock-speed=0 --cpu-clock-ratio=1.0 *algos") {
      }

      static std::vector<std::string> default_benchmark_list() {
         /*
         This is not intended to be exhaustive: it just hits the high
         points of the most interesting or widely used algorithms.
         */
         // clang-format off
         return {
            /* Block ciphers */
            "AES-128",
            "AES-192",
            "AES-256",
            "ARIA-128",
            "ARIA-192",
            "ARIA-256",
            "Blowfish",
            "CAST-128",
            "Camellia-128",
            "Camellia-192",
            "Camellia-256",
            "DES",
            "TripleDES",
            "GOST-28147-89",
            "IDEA",
            "Noekeon",
            "SHACAL2",
            "SM4",
            "Serpent",
            "Threefish-512",
            "Twofish",

            /* Cipher modes */
            "AES-128/CBC",
            "AES-128/CTR-BE",
            "AES-128/EAX",
            "AES-128/OCB",
            "AES-128/GCM",
            "AES-128/XTS",
            "AES-128/SIV",

            "Serpent/CBC",
            "Serpent/CTR-BE",
            "Serpent/EAX",
            "Serpent/OCB",
            "Serpent/GCM",
            "Serpent/XTS",
            "Serpent/SIV",

            "ChaCha20Poly1305",

            /* Stream ciphers */
            "RC4",
            "Salsa20",
            "ChaCha20",

            /* Hashes */
            "SHA-1",
            "SHA-256",
            "SHA-512",
            "SHA-3(256)",
            "SHA-3(512)",
            "RIPEMD-160",
            "Skein-512",
            "Blake2b",
            "Whirlpool",

            /* XOFs */
            "SHAKE-128",
            "SHAKE-256",

            /* MACs */
            "CMAC(AES-128)",
            "HMAC(SHA-256)",

            /* pubkey */
            "RSA",
            "DH",
            "ECDH",
            "ECDSA",
            "Ed25519",
            "Ed448",
            "X25519",
            "X448",
            "McEliece",
            "Kyber",
            "SPHINCS+",
            "FrodoKEM",
            "HSS-LMS",
         };
         // clang-format on
      }

      std::string group() const override { return "misc"; }

      std::string description() const override { return "Measures the speed of algorithms"; }

      void go() override {
         std::chrono::milliseconds msec(get_arg_sz("msec"));
         const std::string provider = get_arg("provider");
         std::vector<std::string> ecc_groups = Command::split_on(get_arg("ecc-groups"), ',');
         const std::string format = get_arg("format");
         const std::string clock_ratio = get_arg("cpu-clock-ratio");
         m_clock_speed = get_arg_sz("cpu-clock-speed");

         m_clock_cycle_ratio = std::strtod(clock_ratio.c_str(), nullptr);

         /*
         * This argument is intended to be the ratio between the cycle counter
         * and the actual machine cycles. It is extremely unlikely that there is
         * any machine where the cycle counter increments faster than the actual
         * clock.
         */
         if(m_clock_cycle_ratio < 0.0 || m_clock_cycle_ratio > 1.0) {
            throw CLI_Usage_Error("Unlikely CPU clock ratio of " + clock_ratio);
         }

         m_clock_cycle_ratio = 1.0 / m_clock_cycle_ratio;

         if(m_clock_speed != 0 && Botan::OS::get_cpu_cycle_counter() != 0) {
            error_output() << "The --cpu-clock-speed option is only intended to be used on "
                              "platforms without access to a cycle counter.\n"
                              "Expected incorrect results\n\n";
         }

         if(format == "table") {
            m_summary = std::make_unique<Summary>();
         } else if(format == "json") {
            m_json = std::make_unique<JSON_Output>();
         } else if(format != "default") {
            throw CLI_Usage_Error("Unknown --format type '" + format + "'");
         }

#if defined(BOTAN_HAS_ECC_GROUP)
         if(ecc_groups.empty()) {
            ecc_groups = {"secp256r1", "secp384r1", "secp521r1", "brainpool256r1", "brainpool384r1", "brainpool512r1"};
         } else if(ecc_groups.size() == 1 && ecc_groups[0] == "all") {
            auto all = Botan::EC_Group::known_named_groups();
            ecc_groups.assign(all.begin(), all.end());
         }
#endif

         std::vector<std::string> algos = get_arg_list("algos");

         const std::vector<size_t> buf_sizes = unique_buffer_sizes(get_arg("buf-size"));

         for(const std::string& cpuid_to_clear : Command::split_on(get_arg("clear-cpuid"), ',')) {
            auto bits = Botan::CPUID::bit_from_string(cpuid_to_clear);
            if(bits.empty()) {
               error_output() << "Warning don't know CPUID flag '" << cpuid_to_clear << "'\n";
            }

            for(auto bit : bits) {
               Botan::CPUID::clear_cpuid_bit(bit);
            }
         }

         if(verbose() || m_summary) {
            output() << Botan::version_string() << "\n"
                     << "CPUID: " << Botan::CPUID::to_string() << "\n\n";
         }

         const bool using_defaults = (algos.empty());
         if(using_defaults) {
            algos = default_benchmark_list();
         }

         class PerfConfig_Cli final : public PerfConfig {
            public:
               PerfConfig_Cli(std::chrono::milliseconds runtime,
                              const std::vector<size_t>& buffer_sizes,
                              Speed* speed) :
                     m_runtime(runtime), m_buffer_sizes(buffer_sizes), m_speed(speed) {}

               const std::vector<size_t>& buffer_sizes() const override { return m_buffer_sizes; }

               std::chrono::milliseconds runtime() const override { return m_runtime; }

               std::ostream& error_output() const override { return m_speed->error_output(); }

               Botan::RandomNumberGenerator& rng() const override { return m_speed->rng(); }

               void record_result(const Botan::Timer& timer) const override { m_speed->record_result(timer); }

               std::unique_ptr<Botan::Timer> make_timer(const std::string& alg,
                                                        uint64_t event_mult,
                                                        const std::string& what,
                                                        const std::string& provider,
                                                        size_t buf_size) const override {
                  return m_speed->make_timer(alg, event_mult, what, provider, buf_size);
               }

            private:
               std::chrono::milliseconds m_runtime;
               std::vector<size_t> m_buffer_sizes;
               Speed* m_speed;
         };

         PerfConfig_Cli perf_config(msec, buf_sizes, this);

         for(const auto& algo : algos) {
            using namespace std::placeholders;

            if(auto perf = PerfTest::get(algo)) {
               perf->go(perf_config);
            }
#if defined(BOTAN_HAS_HASH)
            else if(!Botan::HashFunction::providers(algo).empty()) {
               bench_providers_of<Botan::HashFunction>(
                  algo, provider, msec, buf_sizes, std::bind(&Speed::bench_hash, this, _1, _2, _3, _4));
            }
#endif
#if defined(BOTAN_HAS_XOF)
            else if(!Botan::XOF::providers(algo).empty()) {
               bench_providers_of<Botan::XOF>(
                  algo, provider, msec, buf_sizes, std::bind(&Speed::bench_xof, this, _1, _2, _3, _4));
            }
#endif
#if defined(BOTAN_HAS_BLOCK_CIPHER)
            else if(!Botan::BlockCipher::providers(algo).empty()) {
               bench_providers_of<Botan::BlockCipher>(
                  algo, provider, msec, buf_sizes, std::bind(&Speed::bench_block_cipher, this, _1, _2, _3, _4));
            }
#endif
#if defined(BOTAN_HAS_STREAM_CIPHER)
            else if(!Botan::StreamCipher::providers(algo).empty()) {
               bench_providers_of<Botan::StreamCipher>(
                  algo, provider, msec, buf_sizes, std::bind(&Speed::bench_stream_cipher, this, _1, _2, _3, _4));
            }
#endif
#if defined(BOTAN_HAS_CIPHER_MODES)
            else if(auto enc = Botan::Cipher_Mode::create(algo, Botan::Cipher_Dir::Encryption, provider)) {
               auto dec = Botan::Cipher_Mode::create_or_throw(algo, Botan::Cipher_Dir::Decryption, provider);
               bench_cipher_mode(*enc, *dec, msec, buf_sizes);
            }
#endif
#if defined(BOTAN_HAS_MAC)
            else if(!Botan::MessageAuthenticationCode::providers(algo).empty()) {
               bench_providers_of<Botan::MessageAuthenticationCode>(
                  algo, provider, msec, buf_sizes, std::bind(&Speed::bench_mac, this, _1, _2, _3, _4));
            }
#endif
#if defined(BOTAN_HAS_RSA)
            else if(algo == "RSA") {
               bench_rsa(provider, msec);
            } else if(algo == "RSA_keygen") {
               bench_rsa_keygen(provider, msec);
            }
#endif

#if defined(BOTAN_HAS_PCURVES)
            else if(algo == "ECDSA-pcurves") {
               bench_pcurve_ecdsa(ecc_groups, msec);
            } else if(algo == "ECDH-pcurves") {
               bench_pcurve_ecdh(ecc_groups, msec);
            } else if(algo == "pcurves") {
               bench_pcurves(ecc_groups, msec);
            }
#endif

#if defined(BOTAN_HAS_ECDSA)
            else if(algo == "ECDSA") {
               bench_ecdsa(ecc_groups, provider, msec);
            } else if(algo == "ecdsa_recovery") {
               bench_ecdsa_recovery(ecc_groups, provider, msec);
            }
#endif
#if defined(BOTAN_HAS_SM2)
            else if(algo == "SM2") {
               bench_sm2(ecc_groups, provider, msec);
            }
#endif
#if defined(BOTAN_HAS_ECKCDSA)
            else if(algo == "ECKCDSA") {
               bench_eckcdsa(ecc_groups, provider, msec);
            }
#endif
#if defined(BOTAN_HAS_GOST_34_10_2001)
            else if(algo == "GOST-34.10") {
               bench_gost_3410(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_ECGDSA)
            else if(algo == "ECGDSA") {
               bench_ecgdsa(ecc_groups, provider, msec);
            }
#endif
#if defined(BOTAN_HAS_ED25519)
            else if(algo == "Ed25519") {
               bench_ed25519(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_ED448)
            else if(algo == "Ed448") {
               bench_ed448(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_DIFFIE_HELLMAN)
            else if(algo == "DH") {
               bench_dh(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_DSA)
            else if(algo == "DSA") {
               bench_dsa(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_ELGAMAL)
            else if(algo == "ElGamal") {
               bench_elgamal(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_ECDH)
            else if(algo == "ECDH") {
               bench_ecdh(ecc_groups, provider, msec);
            }
#endif
#if defined(BOTAN_HAS_X25519)
            else if(algo == "X25519") {
               bench_x25519(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_X448)
            else if(algo == "X448") {
               bench_x448(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_MCELIECE)
            else if(algo == "McEliece") {
               bench_mceliece(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_KYBER) || defined(BOTAN_HAS_KYBER_90S)
            else if(algo == "Kyber") {
               bench_kyber(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_DILITHIUM) || defined(BOTAN_HAS_DILITHIUM_AES)
            else if(algo == "Dilithium") {
               bench_dilithium(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_XMSS_RFC8391)
            else if(algo == "XMSS") {
               bench_xmss(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_HSS_LMS)
            else if(algo == "HSS-LMS") {
               bench_hss_lms(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_SPHINCS_PLUS_WITH_SHA2) || defined(BOTAN_HAS_SPHINCS_PLUS_WITH_SHAKE)
            else if(algo == "SPHINCS+") {
               bench_sphincs_plus(provider, msec);
            }
#endif
#if defined(BOTAN_HAS_FRODOKEM)
            else if(algo == "FrodoKEM") {
               bench_frodokem(provider, msec);
            }
#endif

#if defined(BOTAN_HAS_ECC_GROUP)
            else if(algo == "ecc_mult") {
               bench_ecc_mult(ecc_groups, msec);
            } else if(algo == "ecc_init") {
               bench_ecc_init(ecc_groups, msec);
            } else if(algo == "os2ecp") {
               bench_os2ecp(ecc_groups, msec);
            }
#endif
#if defined(BOTAN_HAS_EC_HASH_TO_CURVE)
            else if(algo == "ec_h2c") {
               bench_ec_h2c(msec);
            }
#endif
            else {
               if(verbose() || !using_defaults) {
                  error_output() << "Unknown algorithm '" << algo << "'\n";
               }
            }
         }

         if(m_json) {
            output() << m_json->print();
         }
         if(m_summary) {
            output() << m_summary->print() << "\n";
         }

         if(verbose() && m_clock_speed == 0 && m_cycles_consumed > 0 && m_ns_taken > 0) {
            const double seconds = static_cast<double>(m_ns_taken) / 1000000000;
            const double Hz = static_cast<double>(m_cycles_consumed) / seconds;
            const double MHz = Hz / 1000000;
            output() << "\nEstimated clock speed " << MHz << " MHz\n";
         }
      }

   private:
      size_t m_clock_speed = 0;
      double m_clock_cycle_ratio = 0.0;
      uint64_t m_cycles_consumed = 0;
      uint64_t m_ns_taken = 0;
      std::unique_ptr<Summary> m_summary;
      std::unique_ptr<JSON_Output> m_json;

      void record_result(const Timer& t) {
         m_ns_taken += t.value();
         m_cycles_consumed += t.cycles_consumed();
         if(m_json) {
            m_json->add(t);
         } else {
            output() << t.to_string() << std::flush;
            if(m_summary) {
               m_summary->add(t);
            }
         }
      }

      void record_result(const std::unique_ptr<Timer>& t) { record_result(*t); }

      template <typename T>
      using bench_fn = std::function<void(T&, std::string, std::chrono::milliseconds, const std::vector<size_t>&)>;

      template <typename T>
      void bench_providers_of(const std::string& algo,
                              const std::string& provider, /* user request, if any */
                              const std::chrono::milliseconds runtime,
                              const std::vector<size_t>& buf_sizes,
                              bench_fn<T> bench_one) {
         for(const auto& prov : T::providers(algo)) {
            if(provider.empty() || provider == prov) {
               auto p = T::create(algo, prov);

               if(p) {
                  bench_one(*p, prov, runtime, buf_sizes);
               }
            }
         }
      }

      std::unique_ptr<Timer> make_timer(const std::string& name,
                                        uint64_t event_mult = 1,
                                        const std::string& what = "",
                                        const std::string& provider = "",
                                        size_t buf_size = 0) {
         return std::make_unique<Timer>(name, provider, what, event_mult, buf_size, m_clock_cycle_ratio, m_clock_speed);
      }

      std::unique_ptr<Timer> make_timer(const std::string& algo, const std::string& provider, const std::string& what) {
         return make_timer(algo, 1, what, provider, 0);
      }

#if defined(BOTAN_HAS_BLOCK_CIPHER)
      void bench_block_cipher(Botan::BlockCipher& cipher,
                              const std::string& provider,
                              std::chrono::milliseconds runtime,
                              const std::vector<size_t>& buf_sizes) {
         auto ks_timer = make_timer(cipher.name(), provider, "key schedule");

         const Botan::SymmetricKey key(rng(), cipher.maximum_keylength());
         ks_timer->run([&]() { cipher.set_key(key); });

         const size_t bs = cipher.block_size();
         std::set<size_t> buf_sizes_in_blocks;
         for(size_t buf_size : buf_sizes) {
            if(buf_size % bs == 0) {
               buf_sizes_in_blocks.insert(buf_size);
            } else {
               buf_sizes_in_blocks.insert(buf_size + bs - (buf_size % bs));
            }
         }

         for(size_t buf_size : buf_sizes_in_blocks) {
            std::vector<uint8_t> buffer(buf_size);
            const size_t mult = std::max<size_t>(1, 65536 / buf_size);
            const size_t blocks = buf_size / bs;

            auto encrypt_timer = make_timer(cipher.name(), mult * buffer.size(), "encrypt", provider, buf_size);
            auto decrypt_timer = make_timer(cipher.name(), mult * buffer.size(), "decrypt", provider, buf_size);

            encrypt_timer->run_until_elapsed(runtime, [&]() {
               for(size_t i = 0; i != mult; ++i) {
                  cipher.encrypt_n(&buffer[0], &buffer[0], blocks);
               }
            });
            record_result(encrypt_timer);

            decrypt_timer->run_until_elapsed(runtime, [&]() {
               for(size_t i = 0; i != mult; ++i) {
                  cipher.decrypt_n(&buffer[0], &buffer[0], blocks);
               }
            });
            record_result(decrypt_timer);
         }
      }
#endif

#if defined(BOTAN_HAS_STREAM_CIPHER)
      void bench_stream_cipher(Botan::StreamCipher& cipher,
                               const std::string& provider,
                               const std::chrono::milliseconds runtime,
                               const std::vector<size_t>& buf_sizes) {
         for(auto buf_size : buf_sizes) {
            const Botan::SymmetricKey key(rng(), cipher.maximum_keylength());
            cipher.set_key(key);

            if(cipher.valid_iv_length(12)) {
               const Botan::InitializationVector iv(rng(), 12);
               cipher.set_iv(iv.begin(), iv.size());
            }

            Botan::secure_vector<uint8_t> buffer = rng().random_vec(buf_size);

            const size_t mult = std::max<size_t>(1, 65536 / buf_size);

            auto encrypt_timer = make_timer(cipher.name(), mult * buffer.size(), "encrypt", provider, buf_size);

            encrypt_timer->run_until_elapsed(runtime, [&]() {
               for(size_t i = 0; i != mult; ++i) {
                  cipher.encipher(buffer);
               }
            });

            record_result(encrypt_timer);

            if(verbose()) {
               auto ks_timer = make_timer(cipher.name(), buffer.size(), "write_keystream", provider, buf_size);

               while(ks_timer->under(runtime)) {
                  ks_timer->run([&]() { cipher.write_keystream(buffer.data(), buffer.size()); });
               }
               record_result(ks_timer);
            }
         }
      }
#endif

#if defined(BOTAN_HAS_HASH)
      void bench_hash(Botan::HashFunction& hash,
                      const std::string& provider,
                      const std::chrono::milliseconds runtime,
                      const std::vector<size_t>& buf_sizes) {
         std::vector<uint8_t> output(hash.output_length());

         for(auto buf_size : buf_sizes) {
            Botan::secure_vector<uint8_t> buffer = rng().random_vec(buf_size);

            const size_t mult = std::max<size_t>(1, 65536 / buf_size);

            auto timer = make_timer(hash.name(), mult * buffer.size(), "hash", provider, buf_size);
            timer->run_until_elapsed(runtime, [&]() {
               for(size_t i = 0; i != mult; ++i) {
                  hash.update(buffer);
                  hash.final(output.data());
               }
            });
            record_result(timer);
         }
      }
#endif

#if defined(BOTAN_HAS_XOF)
      void bench_xof(Botan::XOF& xof,
                     const std::string& provider,
                     const std::chrono::milliseconds runtime,
                     const std::vector<size_t>& buf_sizes) {
         for(auto buf_size : buf_sizes) {
            Botan::secure_vector<uint8_t> in = rng().random_vec(buf_size);
            Botan::secure_vector<uint8_t> out(buf_size);

            auto in_timer = make_timer(xof.name(), in.size(), "input", provider, buf_size);
            in_timer->run_until_elapsed(runtime / 2, [&]() { xof.update(in); });

            auto out_timer = make_timer(xof.name(), out.size(), "output", provider, buf_size);
            out_timer->run_until_elapsed(runtime / 2, [&] { xof.output(out); });

            record_result(in_timer);
            record_result(out_timer);
         }
      }
#endif

#if defined(BOTAN_HAS_MAC)
      void bench_mac(Botan::MessageAuthenticationCode& mac,
                     const std::string& provider,
                     const std::chrono::milliseconds runtime,
                     const std::vector<size_t>& buf_sizes) {
         std::vector<uint8_t> output(mac.output_length());

         for(auto buf_size : buf_sizes) {
            Botan::secure_vector<uint8_t> buffer = rng().random_vec(buf_size);
            const size_t mult = std::max<size_t>(1, 65536 / buf_size);

            const Botan::SymmetricKey key(rng(), mac.maximum_keylength());
            mac.set_key(key);

            auto timer = make_timer(mac.name(), mult * buffer.size(), "mac", provider, buf_size);
            timer->run_until_elapsed(runtime, [&]() {
               for(size_t i = 0; i != mult; ++i) {
                  if(mac.fresh_key_required_per_message()) {
                     mac.set_key(key);
                  }
                  mac.start(nullptr, 0);
                  mac.update(buffer);
                  mac.final(output.data());
               }
            });

            record_result(timer);
         }
      }
#endif

#if defined(BOTAN_HAS_CIPHER_MODES)
      void bench_cipher_mode(Botan::Cipher_Mode& enc,
                             Botan::Cipher_Mode& dec,
                             const std::chrono::milliseconds runtime,
                             const std::vector<size_t>& buf_sizes) {
         auto ks_timer = make_timer(enc.name(), enc.provider(), "key schedule");

         const Botan::SymmetricKey key(rng(), enc.key_spec().maximum_keylength());

         ks_timer->run([&]() { enc.set_key(key); });
         ks_timer->run([&]() { dec.set_key(key); });

         record_result(ks_timer);

         for(auto buf_size : buf_sizes) {
            Botan::secure_vector<uint8_t> buffer = rng().random_vec(buf_size);
            const size_t mult = std::max<size_t>(1, 65536 / buf_size);

            auto encrypt_timer = make_timer(enc.name(), mult * buffer.size(), "encrypt", enc.provider(), buf_size);
            auto decrypt_timer = make_timer(dec.name(), mult * buffer.size(), "decrypt", dec.provider(), buf_size);

            Botan::secure_vector<uint8_t> iv = rng().random_vec(enc.default_nonce_length());

            if(buf_size >= enc.minimum_final_size()) {
               encrypt_timer->run_until_elapsed(runtime, [&]() {
                  for(size_t i = 0; i != mult; ++i) {
                     enc.start(iv);
                     enc.finish(buffer);
                     buffer.resize(buf_size);  // remove any tag or padding
                  }
               });

               while(decrypt_timer->under(runtime)) {
                  if(!iv.empty()) {
                     iv[iv.size() - 1] += 1;
                  }

                  // Create a valid ciphertext/tag for decryption to run on
                  buffer.resize(buf_size);
                  enc.start(iv);
                  enc.finish(buffer);

                  Botan::secure_vector<uint8_t> dbuffer;

                  decrypt_timer->run([&]() {
                     for(size_t i = 0; i != mult; ++i) {
                        dbuffer = buffer;
                        dec.start(iv);
                        dec.finish(dbuffer);
                     }
                  });
               }
            }
            record_result(encrypt_timer);
            record_result(decrypt_timer);
         }
      }
#endif

#if defined(BOTAN_HAS_ECC_GROUP)
      void bench_ecc_init(const std::vector<std::string>& groups, const std::chrono::milliseconds runtime) {
         for(std::string group_name : groups) {
            auto timer = make_timer(group_name + " initialization");

            while(timer->under(runtime)) {
               Botan::EC_Group::clear_registered_curve_data();
               timer->run([&]() { Botan::EC_Group::from_name(group_name); });
            }

            record_result(timer);
         }
      }

      void bench_ecc_mult(const std::vector<std::string>& groups, const std::chrono::milliseconds runtime) {
         for(const std::string& group_name : groups) {
            const auto group = Botan::EC_Group::from_name(group_name);

            auto bp_timer = make_timer(group_name + " base point");
            auto vp_timer = make_timer(group_name + " variable point");

            std::vector<Botan::BigInt> ws;

            auto g = Botan::EC_AffinePoint::generator(group);

            while(bp_timer->under(runtime) && vp_timer->under(runtime)) {
               const auto k = Botan::EC_Scalar::random(group, rng());

               const auto r1 = bp_timer->run([&]() { return Botan::EC_AffinePoint::g_mul(k, rng(), ws); });

               const auto r2 = vp_timer->run([&]() { return g.mul(k, rng(), ws); });

               BOTAN_ASSERT_EQUAL(
                  r1.serialize_uncompressed(), r2.serialize_uncompressed(), "Same result for multiplication");
            }

            record_result(bp_timer);
            record_result(vp_timer);
         }
      }

      void bench_os2ecp(const std::vector<std::string>& groups, const std::chrono::milliseconds runtime) {
         for(const std::string& group_name : groups) {
            auto uncmp_timer = make_timer("OS2ECP uncompressed " + group_name);
            auto cmp_timer = make_timer("OS2ECP compressed " + group_name);

            const auto ec_group = Botan::EC_Group::from_name(group_name);

            while(uncmp_timer->under(runtime) && cmp_timer->under(runtime)) {
               const Botan::BigInt k(rng(), 256);
               const Botan::EC_Point p = ec_group.get_base_point() * k;
               const std::vector<uint8_t> os_cmp = p.encode(Botan::EC_Point_Format::Compressed);
               const std::vector<uint8_t> os_uncmp = p.encode(Botan::EC_Point_Format::Uncompressed);

               uncmp_timer->run([&]() { ec_group.OS2ECP(os_uncmp); });
               cmp_timer->run([&]() { ec_group.OS2ECP(os_cmp); });
            }

            record_result(uncmp_timer);
            record_result(cmp_timer);
         }
      }

#endif

#if defined(BOTAN_HAS_EC_HASH_TO_CURVE)
      void bench_ec_h2c(const std::chrono::milliseconds runtime) {
         for(std::string group_name : {"secp256r1", "secp384r1", "secp521r1"}) {
            auto h2c_ro_timer = make_timer(group_name + "-RO", "", "hash to curve");
            auto h2c_nu_timer = make_timer(group_name + "-NU", "", "hash to curve");

            const auto group = Botan::EC_Group::from_name(group_name);

            const std::string hash_fn = "SHA-256";

            while(h2c_ro_timer->under(runtime)) {
               const auto input = rng().random_array<32>();
               const auto domain_sep = rng().random_array<32>();

               h2c_ro_timer->run(
                  [&]() { return Botan::EC_AffinePoint::hash_to_curve_ro(group, hash_fn, input, domain_sep); });

               h2c_nu_timer->run(
                  [&]() { return Botan::EC_AffinePoint::hash_to_curve_nu(group, hash_fn, input, domain_sep); });
            }

            record_result(h2c_ro_timer);
            record_result(h2c_nu_timer);
         }
      }
#endif

#if defined(BOTAN_HAS_PCURVES)

      void bench_pcurves(const std::vector<std::string>& groups, const std::chrono::milliseconds runtime) {
         for(const auto& group_name : groups) {
            if(auto curve = Botan::PCurve::PrimeOrderCurve::from_name(group_name)) {
               auto base_timer = make_timer(group_name + " pcurve base mul");
               auto var_timer = make_timer(group_name + " pcurve var mul");
               auto mul2_setup_timer = make_timer(group_name + " pcurve mul2 setup");
               auto mul2_timer = make_timer(group_name + " pcurve mul2");

               auto scalar_invert = make_timer(group_name + " pcurve scalar invert");
               auto to_affine = make_timer(group_name + " pcurve proj->affine");

               auto g = curve->generator();
               auto h = curve->mul(g, curve->random_scalar(rng()), rng()).to_affine();
               auto gh_tab = curve->mul2_setup(g, h);

               while(base_timer->under(runtime)) {
                  const auto scalar = curve->random_scalar(rng());
                  base_timer->run([&]() { return curve->mul_by_g(scalar, rng()).to_affine(); });
               }

               while(var_timer->under(runtime)) {
                  const auto scalar = curve->random_scalar(rng());
                  var_timer->run([&]() { return curve->mul(h, scalar, rng()).to_affine(); });
               }

               while(mul2_setup_timer->under(runtime)) {
                  mul2_setup_timer->run([&]() { return curve->mul2_setup(g, h); });
               }

               while(mul2_timer->under(runtime)) {
                  const auto scalar = curve->random_scalar(rng());
                  const auto scalar2 = curve->random_scalar(rng());
                  mul2_timer->run([&]() -> std::optional<Botan::PCurve::PrimeOrderCurve::AffinePoint> {
                     if(auto pt = curve->mul2_vartime(*gh_tab, scalar, scalar2)) {
                        return pt->to_affine();
                     } else {
                        return {};
                     }
                  });
               }

               auto pt = curve->mul(g, curve->random_scalar(rng()), rng());
               to_affine->run_until_elapsed(runtime, [&]() { pt.to_affine(); });

               while(scalar_invert->under(runtime)) {
                  const auto scalar = curve->random_scalar(rng());
                  scalar_invert->run([&]() { scalar.invert(); });
               }

               record_result(base_timer);
               record_result(var_timer);
               record_result(mul2_setup_timer);
               record_result(mul2_timer);
               record_result(to_affine);
               record_result(scalar_invert);
            }
         }
      }

      void bench_pcurve_ecdsa(const std::vector<std::string>& groups, const std::chrono::milliseconds runtime) {
         for(const auto& group_name : groups) {
            auto curve = Botan::PCurve::PrimeOrderCurve::from_name(group_name);
            if(!curve) {
               continue;
            }

            // Setup (not timed)
            const auto g = curve->generator();
            const auto x = curve->random_scalar(rng());
            const auto y = curve->mul_by_g(x, rng()).to_affine();
            const auto e = curve->random_scalar(rng());

            const auto gy_tab = curve->mul2_setup(g, y);

            auto b = curve->random_scalar(rng());
            auto b_inv = b.invert();

            auto sign_timer = make_timer("ECDSA sign pcurves " + group_name);
            auto verify_timer = make_timer("ECDSA verify pcurves " + group_name);

            while(sign_timer->under(runtime)) {
               sign_timer->start();

               const auto signature = [&]() {
                  const auto k = curve->random_scalar(rng());
                  const auto r = curve->base_point_mul_x_mod_order(k, rng());
                  const auto k_inv = (b * k).invert() * b;
                  b = b.square();
                  b_inv = b_inv.square();
                  const auto be = b * e;
                  const auto bx = b * x;
                  const auto bxr_e = (bx * r) + be;
                  const auto s = (k_inv * bxr_e) * b_inv;

                  return Botan::concat(r.serialize(), s.serialize());
               }();

               sign_timer->stop();

               verify_timer->start();

               auto result = [&](std::span<const uint8_t> sig) {
                  const size_t scalar_bytes = curve->scalar_bytes();
                  if(sig.size() != 2 * scalar_bytes) {
                     return false;
                  }

                  const auto r = curve->deserialize_scalar(sig.first(scalar_bytes));
                  const auto s = curve->deserialize_scalar(sig.last(scalar_bytes));

                  if(r && s) {
                     if(r->is_zero() || s->is_zero()) {
                        return false;
                     }

                     auto w = s->invert();

                     auto u1 = e * w;
                     auto u2 = *r * w;

                     return curve->mul2_vartime_x_mod_order_eq(*gy_tab, *r, u1, u2);
                  }

                  return false;
               }(signature);

               BOTAN_ASSERT(result, "ECDSA-pcurves signature ok");

               verify_timer->stop();
            }

            record_result(sign_timer);
            record_result(verify_timer);
         }
      }

      void bench_pcurve_ecdh(const std::vector<std::string>& groups, const std::chrono::milliseconds runtime) {
         for(const auto& group_name : groups) {
            auto curve = Botan::PCurve::PrimeOrderCurve::from_name(group_name);
            if(!curve) {
               continue;
            }

            auto ka_timer = make_timer("ECDH agree pcurves " + group_name);

            auto agree = [&](const Botan::PCurve::PrimeOrderCurve::Scalar& sk, std::span<const uint8_t> pt_bytes) {
               const auto pt = curve->deserialize_point(pt_bytes);
               if(pt) {
                  return curve->mul(*pt, sk, rng()).to_affine().serialize();
               } else {
                  return std::vector<uint8_t>();
               }
            };

            while(ka_timer->under(runtime)) {
               const auto g = curve->generator();
               const auto x1 = curve->random_scalar(rng());
               const auto x2 = curve->random_scalar(rng());

               const auto y1 = curve->mul_by_g(x1, rng()).to_affine().serialize();
               const auto y2 = curve->mul_by_g(x2, rng()).to_affine().serialize();

               ka_timer->start();
               const auto ss1 = agree(x1, y2);
               ka_timer->stop();

               ka_timer->start();
               const auto ss2 = agree(x1, y2);
               ka_timer->stop();

               BOTAN_ASSERT(ss1 == ss2, "Key agreement worked");
            }

            record_result(ka_timer);
         }
      }

#endif

#if defined(BOTAN_HAS_PUBLIC_KEY_CRYPTO)
      void bench_pk_enc(const Botan::Private_Key& key,
                        const std::string& nm,
                        const std::string& provider,
                        const std::string& padding,
                        std::chrono::milliseconds msec) {
         std::vector<uint8_t> plaintext, ciphertext;

         Botan::PK_Encryptor_EME enc(key, rng(), padding, provider);
         Botan::PK_Decryptor_EME dec(key, rng(), padding, provider);

         auto enc_timer = make_timer(nm + " " + padding, provider, "encrypt");
         auto dec_timer = make_timer(nm + " " + padding, provider, "decrypt");

         while(enc_timer->under(msec) || dec_timer->under(msec)) {
            // Generate a new random ciphertext to decrypt
            if(ciphertext.empty() || enc_timer->under(msec)) {
               rng().random_vec(plaintext, enc.maximum_input_size());
               ciphertext = enc_timer->run([&]() { return enc.encrypt(plaintext, rng()); });
            }

            if(dec_timer->under(msec)) {
               const auto dec_pt = dec_timer->run([&]() { return dec.decrypt(ciphertext); });

               if(!(Botan::unlock(dec_pt) == plaintext))  // sanity check
               {
                  error_output() << "Bad roundtrip in PK encrypt/decrypt bench\n";
               }
            }
         }

         record_result(enc_timer);
         record_result(dec_timer);
      }

      void bench_pk_ka(const std::string& algo,
                       const std::string& nm,
                       const std::string& params,
                       const std::string& provider,
                       std::chrono::milliseconds msec) {
         const std::string kdf = "KDF2(SHA-256)";  // arbitrary choice

         auto keygen_timer = make_timer(nm, provider, "keygen");

         std::unique_ptr<Botan::Private_Key> key1(
            keygen_timer->run([&] { return Botan::create_private_key(algo, rng(), params); }));
         std::unique_ptr<Botan::Private_Key> key2(
            keygen_timer->run([&] { return Botan::create_private_key(algo, rng(), params); }));

         record_result(keygen_timer);

         const Botan::PK_Key_Agreement_Key& ka_key1 = dynamic_cast<const Botan::PK_Key_Agreement_Key&>(*key1);
         const Botan::PK_Key_Agreement_Key& ka_key2 = dynamic_cast<const Botan::PK_Key_Agreement_Key&>(*key2);

         Botan::PK_Key_Agreement ka1(ka_key1, rng(), kdf, provider);
         Botan::PK_Key_Agreement ka2(ka_key2, rng(), kdf, provider);

         const std::vector<uint8_t> ka1_pub = ka_key1.public_value();
         const std::vector<uint8_t> ka2_pub = ka_key2.public_value();

         auto ka_timer = make_timer(nm, provider, "key agreements");

         while(ka_timer->under(msec)) {
            Botan::SymmetricKey symkey1 = ka_timer->run([&]() { return ka1.derive_key(32, ka2_pub); });
            Botan::SymmetricKey symkey2 = ka_timer->run([&]() { return ka2.derive_key(32, ka1_pub); });

            if(symkey1 != symkey2) {
               error_output() << "Key agreement mismatch in PK bench\n";
            }
         }

         record_result(ka_timer);
      }

      void bench_pk_kem(const Botan::Private_Key& key,
                        const std::string& nm,
                        const std::string& provider,
                        const std::string& kdf,
                        std::chrono::milliseconds msec) {
         Botan::PK_KEM_Decryptor dec(key, rng(), kdf, provider);
         Botan::PK_KEM_Encryptor enc(key, kdf, provider);

         auto kem_enc_timer = make_timer(nm, provider, "KEM encrypt");
         auto kem_dec_timer = make_timer(nm, provider, "KEM decrypt");

         while(kem_enc_timer->under(msec) && kem_dec_timer->under(msec)) {
            Botan::secure_vector<uint8_t> salt = rng().random_vec(16);

            kem_enc_timer->start();
            const auto kem_result = enc.encrypt(rng(), 64, salt);
            kem_enc_timer->stop();

            kem_dec_timer->start();
            Botan::secure_vector<uint8_t> dec_shared_key = dec.decrypt(kem_result.encapsulated_shared_key(), 64, salt);
            kem_dec_timer->stop();

            if(kem_result.shared_key() != dec_shared_key) {
               error_output() << "KEM mismatch in PK bench\n";
            }
         }

         record_result(kem_enc_timer);
         record_result(kem_dec_timer);
      }

      void bench_pk_sig_ecc(const std::string& algo,
                            const std::string& emsa,
                            const std::string& provider,
                            const std::vector<std::string>& params,
                            std::chrono::milliseconds msec) {
         for(std::string grp : params) {
            const std::string nm = grp.empty() ? algo : Botan::fmt("{}-{}", algo, grp);

            auto keygen_timer = make_timer(nm, provider, "keygen");

            std::unique_ptr<Botan::Private_Key> key(
               keygen_timer->run([&] { return Botan::create_private_key(algo, rng(), grp); }));

            record_result(keygen_timer);
            bench_pk_sig(*key, nm, provider, emsa, msec);
         }
      }

      size_t bench_pk_sig(const Botan::Private_Key& key,
                          const std::string& nm,
                          const std::string& provider,
                          const std::string& padding,
                          std::chrono::milliseconds msec) {
         std::vector<uint8_t> message, signature, bad_signature;

         Botan::PK_Signer sig(key, rng(), padding, Botan::Signature_Format::Standard, provider);
         Botan::PK_Verifier ver(key, padding, Botan::Signature_Format::Standard, provider);

         auto sig_timer = make_timer(nm + " " + padding, provider, "sign");
         auto ver_timer = make_timer(nm + " " + padding, provider, "verify");

         size_t invalid_sigs = 0;

         while(ver_timer->under(msec) || sig_timer->under(msec)) {
            if(signature.empty() || sig_timer->under(msec)) {
               /*
               Length here is kind of arbitrary, but 48 bytes fits into a single
               hash block so minimizes hashing overhead versus the PK op itself.
               */
               rng().random_vec(message, 48);

               signature = sig_timer->run([&]() { return sig.sign_message(message, rng()); });

               bad_signature = signature;
               bad_signature[rng().next_byte() % bad_signature.size()] ^= rng().next_nonzero_byte();
            }

            if(ver_timer->under(msec)) {
               const bool verified = ver_timer->run([&] { return ver.verify_message(message, signature); });

               if(!verified) {
                  invalid_sigs += 1;
               }

               const bool verified_bad = ver_timer->run([&] { return ver.verify_message(message, bad_signature); });

               if(verified_bad) {
                  error_output() << "Bad signature accepted in " << nm << " signature bench\n";
               }
            }
         }

         if(invalid_sigs > 0) {
            error_output() << invalid_sigs << " generated signatures rejected in " << nm << " signature bench\n";
         }

         const size_t events = static_cast<size_t>(std::min(sig_timer->events(), ver_timer->events()));

         record_result(sig_timer);
         record_result(ver_timer);

         return events;
      }
#endif

#if defined(BOTAN_HAS_RSA)
      void bench_rsa_keygen(const std::string& provider, std::chrono::milliseconds msec) {
         for(size_t keylen : {1024, 2048, 3072, 4096}) {
            const std::string nm = "RSA-" + std::to_string(keylen);
            auto keygen_timer = make_timer(nm, provider, "keygen");

            while(keygen_timer->under(msec)) {
               std::unique_ptr<Botan::Private_Key> key(
                  keygen_timer->run([&] { return Botan::create_private_key("RSA", rng(), std::to_string(keylen)); }));

               BOTAN_ASSERT(key->check_key(rng(), true), "Key is ok");
            }

            record_result(keygen_timer);
         }
      }

      void bench_rsa(const std::string& provider, std::chrono::milliseconds msec) {
         for(size_t keylen : {1024, 2048, 3072, 4096}) {
            const std::string nm = "RSA-" + std::to_string(keylen);

            auto keygen_timer = make_timer(nm, provider, "keygen");

            std::unique_ptr<Botan::Private_Key> key(
               keygen_timer->run([&] { return Botan::create_private_key("RSA", rng(), std::to_string(keylen)); }));

            record_result(keygen_timer);

            // Using PKCS #1 padding so OpenSSL provider can play along
            bench_pk_sig(*key, nm, provider, "EMSA-PKCS1-v1_5(SHA-256)", msec);

            //bench_pk_sig(*key, nm, provider, "PSSR(SHA-256)", msec);
            //bench_pk_enc(*key, nm, provider, "EME-PKCS1-v1_5", msec);
            //bench_pk_enc(*key, nm, provider, "OAEP(SHA-1)", msec);
         }
      }
#endif

#if defined(BOTAN_HAS_ECDSA)
      void bench_ecdsa(const std::vector<std::string>& groups,
                       const std::string& provider,
                       std::chrono::milliseconds msec) {
         return bench_pk_sig_ecc("ECDSA", "SHA-256", provider, groups, msec);
      }

      void bench_ecdsa_recovery(const std::vector<std::string>& groups,
                                const std::string& /*unused*/,
                                std::chrono::milliseconds msec) {
         for(const std::string& group_name : groups) {
            const auto group = Botan::EC_Group::from_name(group_name);
            auto recovery_timer = make_timer("ECDSA recovery " + group_name);

            while(recovery_timer->under(msec)) {
               Botan::ECDSA_PrivateKey key(rng(), group);

               std::vector<uint8_t> message(group.get_order_bits() / 8);
               rng().randomize(message.data(), message.size());

               Botan::PK_Signer signer(key, rng(), "Raw");
               signer.update(message);
               std::vector<uint8_t> signature = signer.signature(rng());

               Botan::PK_Verifier verifier(key, "Raw", Botan::Signature_Format::Standard, "base");
               verifier.update(message);
               BOTAN_ASSERT(verifier.check_signature(signature), "Valid signature");

               Botan::BigInt r(signature.data(), signature.size() / 2);
               Botan::BigInt s(signature.data() + signature.size() / 2, signature.size() / 2);

               const uint8_t v = key.recovery_param(message, r, s);

               recovery_timer->run([&]() {
                  Botan::ECDSA_PublicKey pubkey(group, message, r, s, v);
                  BOTAN_ASSERT(pubkey.public_point() == key.public_point(), "Recovered public key");
               });
            }

            record_result(recovery_timer);
         }
      }

#endif

#if defined(BOTAN_HAS_ECKCDSA)
      void bench_eckcdsa(const std::vector<std::string>& groups,
                         const std::string& provider,
                         std::chrono::milliseconds msec) {
         return bench_pk_sig_ecc("ECKCDSA", "SHA-256", provider, groups, msec);
      }
#endif

#if defined(BOTAN_HAS_GOST_34_10_2001)
      void bench_gost_3410(const std::string& provider, std::chrono::milliseconds msec) {
         return bench_pk_sig_ecc("GOST-34.10", "GOST-34.11", provider, {"gost_256A"}, msec);
      }
#endif

#if defined(BOTAN_HAS_SM2)
      void bench_sm2(const std::vector<std::string>& groups,
                     const std::string& provider,
                     std::chrono::milliseconds msec) {
         return bench_pk_sig_ecc("SM2_Sig", "SM3", provider, groups, msec);
      }
#endif

#if defined(BOTAN_HAS_ECGDSA)
      void bench_ecgdsa(const std::vector<std::string>& groups,
                        const std::string& provider,
                        std::chrono::milliseconds msec) {
         return bench_pk_sig_ecc("ECGDSA", "SHA-256", provider, groups, msec);
      }
#endif

#if defined(BOTAN_HAS_ED25519)
      void bench_ed25519(const std::string& provider, std::chrono::milliseconds msec) {
         return bench_pk_sig_ecc("Ed25519", "Pure", provider, std::vector<std::string>{""}, msec);
      }
#endif

#if defined(BOTAN_HAS_ED448)
      void bench_ed448(const std::string& provider, std::chrono::milliseconds msec) {
         return bench_pk_sig_ecc("Ed448", "Pure", provider, std::vector<std::string>{""}, msec);
      }
#endif

#if defined(BOTAN_HAS_DIFFIE_HELLMAN)
      void bench_dh(const std::string& provider, std::chrono::milliseconds msec) {
         for(size_t bits : {2048, 3072, 4096, 6144, 8192}) {
            bench_pk_ka("DH", "DH-" + std::to_string(bits), "ffdhe/ietf/" + std::to_string(bits), provider, msec);
         }
      }
#endif

#if defined(BOTAN_HAS_DSA)
      void bench_dsa(const std::string& provider, std::chrono::milliseconds msec) {
         for(size_t bits : {1024, 2048, 3072}) {
            const std::string nm = "DSA-" + std::to_string(bits);

            const std::string params = (bits == 1024) ? "dsa/jce/1024" : ("dsa/botan/" + std::to_string(bits));

            auto keygen_timer = make_timer(nm, provider, "keygen");

            std::unique_ptr<Botan::Private_Key> key(
               keygen_timer->run([&] { return Botan::create_private_key("DSA", rng(), params); }));

            record_result(keygen_timer);

            bench_pk_sig(*key, nm, provider, "SHA-256", msec);
         }
      }
#endif

#if defined(BOTAN_HAS_ELGAMAL)
      void bench_elgamal(const std::string& provider, std::chrono::milliseconds msec) {
         for(size_t keylen : {1024, 2048, 3072, 4096}) {
            const std::string nm = "ElGamal-" + std::to_string(keylen);

            const std::string params = "modp/ietf/" + std::to_string(keylen);

            auto keygen_timer = make_timer(nm, provider, "keygen");

            std::unique_ptr<Botan::Private_Key> key(
               keygen_timer->run([&] { return Botan::create_private_key("ElGamal", rng(), params); }));

            record_result(keygen_timer);

            bench_pk_enc(*key, nm, provider, "EME-PKCS1-v1_5", msec);
         }
      }
#endif

#if defined(BOTAN_HAS_ECDH)
      void bench_ecdh(const std::vector<std::string>& groups,
                      const std::string& provider,
                      std::chrono::milliseconds msec) {
         for(const std::string& grp : groups) {
            bench_pk_ka("ECDH", "ECDH-" + grp, grp, provider, msec);
         }
      }
#endif

#if defined(BOTAN_HAS_X25519)
      void bench_x25519(const std::string& provider, std::chrono::milliseconds msec) {
         bench_pk_ka("X25519", "X25519", "", provider, msec);
      }
#endif

#if defined(BOTAN_HAS_X448)
      void bench_x448(const std::string& provider, std::chrono::milliseconds msec) {
         bench_pk_ka("X448", "X448", "", provider, msec);
      }
#endif

#if defined(BOTAN_HAS_MCELIECE)
      void bench_mceliece(const std::string& provider, std::chrono::milliseconds msec) {
         /*
         SL=80 n=1632 t=33 - 59 KB pubkey 140 KB privkey
         SL=107 n=2480 t=45 - 128 KB pubkey 300 KB privkey
         SL=128 n=2960 t=57 - 195 KB pubkey 459 KB privkey
         SL=147 n=3408 t=67 - 265 KB pubkey 622 KB privkey
         SL=191 n=4624 t=95 - 516 KB pubkey 1234 KB privkey
         SL=256 n=6624 t=115 - 942 KB pubkey 2184 KB privkey
         */

         const std::vector<std::pair<size_t, size_t>> mce_params = {
            {2480, 45}, {2960, 57}, {3408, 67}, {4624, 95}, {6624, 115}};

         for(auto params : mce_params) {
            size_t n = params.first;
            size_t t = params.second;

            const std::string nm = "McEliece-" + std::to_string(n) + "," + std::to_string(t) +
                                   " (WF=" + std::to_string(Botan::mceliece_work_factor(n, t)) + ")";

            auto keygen_timer = make_timer(nm, provider, "keygen");

            std::unique_ptr<Botan::Private_Key> key =
               keygen_timer->run([&] { return std::make_unique<Botan::McEliece_PrivateKey>(rng(), n, t); });

            record_result(keygen_timer);
            bench_pk_kem(*key, nm, provider, "KDF2(SHA-256)", msec);
         }
      }
#endif

#if defined(BOTAN_HAS_KYBER) || defined(BOTAN_HAS_KYBER_90S)
      void bench_kyber(const std::string& provider, std::chrono::milliseconds msec) {
         const Botan::KyberMode::Mode all_modes[] = {
            Botan::KyberMode::Kyber512_R3,
            Botan::KyberMode::Kyber512_90s,
            Botan::KyberMode::Kyber768_R3,
            Botan::KyberMode::Kyber768_90s,
            Botan::KyberMode::Kyber1024_R3,
            Botan::KyberMode::Kyber1024_90s,
         };

         for(auto modet : all_modes) {
            Botan::KyberMode mode(modet);

   #if !defined(BOTAN_HAS_KYBER)
            if(mode.is_modern())
               continue;
   #endif

   #if !defined(BOTAN_HAS_KYBER_90S)
            if(mode.is_90s())
               continue;
   #endif

            auto keygen_timer = make_timer(mode.to_string(), provider, "keygen");

            auto key = keygen_timer->run([&] { return Botan::Kyber_PrivateKey(rng(), mode); });

            record_result(keygen_timer);

            bench_pk_kem(key, mode.to_string(), provider, "KDF2(SHA-256)", msec);
         }
      }
#endif

#if defined(BOTAN_HAS_DILITHIUM) || defined(BOTAN_HAS_DILITHIUM_AES)
      void bench_dilithium(const std::string& provider, std::chrono::milliseconds msec) {
         const Botan::DilithiumMode::Mode all_modes[] = {Botan::DilithiumMode::Dilithium4x4,
                                                         Botan::DilithiumMode::Dilithium4x4_AES,
                                                         Botan::DilithiumMode::Dilithium6x5,
                                                         Botan::DilithiumMode::Dilithium6x5_AES,
                                                         Botan::DilithiumMode::Dilithium8x7,
                                                         Botan::DilithiumMode::Dilithium8x7_AES};

         for(auto modet : all_modes) {
            Botan::DilithiumMode mode(modet);

   #if !defined(BOTAN_HAS_DILITHIUM)
            if(mode.is_modern())
               continue;
   #endif

   #if !defined(BOTAN_HAS_DILITHIUM_AES)
            if(mode.is_aes())
               continue;
   #endif

            auto keygen_timer = make_timer(mode.to_string(), provider, "keygen");

            auto key = keygen_timer->run([&] { return Botan::Dilithium_PrivateKey(rng(), mode); });

            record_result(keygen_timer);

            bench_pk_sig(key, mode.to_string(), provider, "", msec);
         }
      }
#endif

#if defined(BOTAN_HAS_SPHINCS_PLUS_WITH_SHA2) || defined(BOTAN_HAS_SPHINCS_PLUS_WITH_SHAKE)
      void bench_sphincs_plus(const std::string& provider, std::chrono::milliseconds msec) {
         // Sphincs_Parameter_Set set, Sphincs_Hash_Type hash
         std::vector<std::string> sphincs_params{"SphincsPlus-sha2-128s-r3.1",
                                                 "SphincsPlus-sha2-128f-r3.1",
                                                 "SphincsPlus-sha2-192s-r3.1",
                                                 "SphincsPlus-sha2-192f-r3.1",
                                                 "SphincsPlus-sha2-256s-r3.1",
                                                 "SphincsPlus-sha2-256f-r3.1",
                                                 "SphincsPlus-shake-128s-r3.1",
                                                 "SphincsPlus-shake-128f-r3.1",
                                                 "SphincsPlus-shake-192s-r3.1",
                                                 "SphincsPlus-shake-192f-r3.1",
                                                 "SphincsPlus-shake-256s-r3.1",
                                                 "SphincsPlus-shake-256f-r3.1"};

         for(auto params : sphincs_params) {
            try {
               auto keygen_timer = make_timer(params, provider, "keygen");

               std::unique_ptr<Botan::Private_Key> key(
                  keygen_timer->run([&] { return Botan::create_private_key("SPHINCS+", rng(), params); }));

               record_result(keygen_timer);
               if(bench_pk_sig(*key, params, provider, "", msec) == 1) {
                  break;
               }
            } catch(Botan::Not_Implemented&) {
               continue;
            }
         }
      }
#endif

#if defined(BOTAN_HAS_FRODOKEM)
      void bench_frodokem(const std::string& provider, std::chrono::milliseconds msec) {
         std::vector<Botan::FrodoKEMMode> frodo_modes{
            Botan::FrodoKEMMode::FrodoKEM640_SHAKE,
            Botan::FrodoKEMMode::FrodoKEM976_SHAKE,
            Botan::FrodoKEMMode::FrodoKEM1344_SHAKE,
            Botan::FrodoKEMMode::eFrodoKEM640_SHAKE,
            Botan::FrodoKEMMode::eFrodoKEM976_SHAKE,
            Botan::FrodoKEMMode::eFrodoKEM1344_SHAKE,
            Botan::FrodoKEMMode::FrodoKEM640_AES,
            Botan::FrodoKEMMode::FrodoKEM976_AES,
            Botan::FrodoKEMMode::FrodoKEM1344_AES,
            Botan::FrodoKEMMode::eFrodoKEM640_AES,
            Botan::FrodoKEMMode::eFrodoKEM976_AES,
            Botan::FrodoKEMMode::eFrodoKEM1344_AES,
         };

         for(auto modet : frodo_modes) {
            if(!modet.is_available()) {
               continue;
            }

            Botan::FrodoKEMMode mode(modet);

            auto keygen_timer = make_timer(mode.to_string(), provider, "keygen");

            auto key = keygen_timer->run([&] { return Botan::FrodoKEM_PrivateKey(rng(), mode); });

            record_result(keygen_timer);

            bench_pk_kem(key, mode.to_string(), provider, "KDF2(SHA-256)", msec);
         }
      }
#endif

#if defined(BOTAN_HAS_XMSS_RFC8391)
      void bench_xmss(const std::string& provider, std::chrono::milliseconds msec) {
         /*
         We only test H10 signatures here since already they are quite slow (a
         few seconds per signature). On a fast machine, H16 signatures take 1-2
         minutes to generate and H20 signatures take 5-10 minutes to generate
         */
         std::vector<std::string> xmss_params{
            "XMSS-SHA2_10_256",
            "XMSS-SHAKE_10_256",
            "XMSS-SHA2_10_512",
            "XMSS-SHAKE_10_512",
         };

         for(std::string params : xmss_params) {
            auto keygen_timer = make_timer(params, provider, "keygen");

            std::unique_ptr<Botan::Private_Key> key(
               keygen_timer->run([&] { return Botan::create_private_key("XMSS", rng(), params); }));

            record_result(keygen_timer);
            if(bench_pk_sig(*key, params, provider, "", msec) == 1) {
               break;
            }
         }
      }
#endif

#if defined(BOTAN_HAS_HSS_LMS)
      void bench_hss_lms(const std::string& provider, std::chrono::milliseconds msec) {
         // At first we compare instances with multiple hash functions. LMS trees with
         // height 10 are suitable, since they can be used for enough signatures and are
         // fast enough for speed testing.
         // Afterward, setups with multiple HSS layers are tested
         std::vector<std::string> hss_lms_instances{"SHA-256,HW(10,1)",
                                                    "SHAKE-256(256),HW(10,1)",
                                                    "SHAKE-256(192),HW(10,1)",
                                                    "Truncated(SHA-256,192),HW(10,1)",
                                                    "SHA-256,HW(10,1),HW(10,1)",
                                                    "SHA-256,HW(10,1),HW(10,1),HW(10,1)"};

         for(const auto& params : hss_lms_instances) {
            auto keygen_timer = make_timer(params, provider, "keygen");

            std::unique_ptr<Botan::Private_Key> key(
               keygen_timer->run([&] { return Botan::create_private_key("HSS-LMS", rng(), params); }));

            record_result(keygen_timer);
            if(bench_pk_sig(*key, params, provider, "", msec) == 1) {
               break;
            }
         }
      }
#endif
};

BOTAN_REGISTER_COMMAND("speed", Speed);

}  // namespace Botan_CLI
