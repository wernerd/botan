/*
* (C) 2024 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PCURVES_INSTANCE_H_
#define BOTAN_PCURVES_INSTANCE_H_

#include <botan/build.h>
#include <memory>

namespace Botan {

class BigInt;

}

namespace Botan::PCurve {

class PrimeOrderCurve;

class PCurveInstance final {
   public:
#if defined(BOTAN_HAS_PCURVES_SECP192R1)
      static std::shared_ptr<const PrimeOrderCurve> secp192r1();
#endif

#if defined(BOTAN_HAS_PCURVES_SECP224R1)
      static std::shared_ptr<const PrimeOrderCurve> secp224r1();
#endif

#if defined(BOTAN_HAS_PCURVES_SECP256R1)
      static std::shared_ptr<const PrimeOrderCurve> secp256r1();
#endif

#if defined(BOTAN_HAS_PCURVES_SECP384R1)
      static std::shared_ptr<const PrimeOrderCurve> secp384r1();
#endif

#if defined(BOTAN_HAS_PCURVES_SECP521R1)
      static std::shared_ptr<const PrimeOrderCurve> secp521r1();
#endif

#if defined(BOTAN_HAS_PCURVES_SECP256K1)
      static std::shared_ptr<const PrimeOrderCurve> secp256k1();
#endif

#if defined(BOTAN_HAS_PCURVES_BRAINPOOL256R1)
      static std::shared_ptr<const PrimeOrderCurve> brainpool256r1();
#endif

#if defined(BOTAN_HAS_PCURVES_BRAINPOOL384R1)
      static std::shared_ptr<const PrimeOrderCurve> brainpool384r1();
#endif

#if defined(BOTAN_HAS_PCURVES_BRAINPOOL512R1)
      static std::shared_ptr<const PrimeOrderCurve> brainpool512r1();
#endif

#if defined(BOTAN_HAS_PCURVES_FRP256V1)
      static std::shared_ptr<const PrimeOrderCurve> frp256v1();
#endif

#if defined(BOTAN_HAS_PCURVES_SM2P256V1)
      static std::shared_ptr<const PrimeOrderCurve> sm2p256v1();
#endif

#if defined(BOTAN_HAS_PCURVES_NUMSP512D1)
      static std::shared_ptr<const PrimeOrderCurve> numsp512d1();
#endif

#if defined(BOTAN_HAS_PCURVES_GENERIC)
      static std::shared_ptr<const PrimeOrderCurve> from_params(const BigInt& p,
                                                                const BigInt& a,
                                                                const BigInt& b,
                                                                const BigInt& base_x,
                                                                const BigInt& base_y,
                                                                const BigInt& order);
#endif
};

}  // namespace Botan::PCurve

#endif
