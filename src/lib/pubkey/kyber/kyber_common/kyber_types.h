/*
 * Crystals Kyber key encapsulation mechanism
 *
 * Strong Type definitions used throughout the Kyber implementation
 *
 * (C) 2024 Jack Lloyd
 * (C) 2024 René Meusel, Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_KYBER_TYPES_H_
#define BOTAN_KYBER_TYPES_H_

#include <botan/secmem.h>
#include <botan/strong_type.h>

#include <variant>
#include <vector>

namespace Botan {

/// Principal seed used to generate Kyber key pairs
using KyberSeedRandomness = Strong<secure_vector<uint8_t>, struct KyberSeedRandomness_>;

/// Public seed value to generate the Kyber matrix A
using KyberSeedRho = Strong<std::vector<uint8_t>, struct KyberSeedRho_>;

/// Private seed used to generate polynomial vectors s and e during key generation
using KyberSeedSigma = Strong<secure_vector<uint8_t>, struct KyberSeedSigma_>;

/// Secret random value (called Z in the spec), used for implicit rejection in the decapsulation
using KyberImplicitRejectionValue = Strong<secure_vector<uint8_t>, struct KyberImplicitRejectionValue_>;

/// Random message value to be encrypted by the CPA-secure Kyber encryption scheme
using KyberMessage = Strong<secure_vector<uint8_t>, struct KyberMessage_>;

/// Random value used to generate the Kyber ciphertext
using KyberEncryptionRandomness = Strong<secure_vector<uint8_t>, struct KyberEncryptionRandomness_>;

/// PRF value used for sampling of error polynomials
using KyberSamplingRandomness = Strong<secure_vector<uint8_t>, struct KyberSamplingRandomness_>;

/// Shared secret value generated during encapsulation and recovered during decapsulation
using KyberSharedSecret = Strong<secure_vector<uint8_t>, struct KyberSharedSecret_>;

/// Public key in serialized form (t || rho)
using KyberSerializedPublicKey = Strong<std::vector<uint8_t>, struct KyberSerializedPublicKey_>;

// Hash value of the serialized public key
using KyberHashedPublicKey = Strong<std::vector<uint8_t>, struct KyberHashedPublicKey_>;

// Compressed and serialized ciphertext value
using KyberCompressedCiphertext = Strong<std::vector<uint8_t>, struct KyberCompressedCiphertext_>;

// Hash of the compressed and serialized ciphertext value
// TODO: Remove this once Kyber-R3 is removed
using KyberHashedCiphertext = Strong<std::vector<uint8_t>, struct KyberHashedCiphertext_>;

// Variant value of either a KyberSeedSigma or a KyberEncryptionRandomness
using KyberSigmaOrEncryptionRandomness =
   std::variant<StrongSpan<const KyberSeedSigma>, StrongSpan<const KyberEncryptionRandomness>>;

}  // namespace Botan

#endif
