/*
* (C) 2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECC_HASH_TO_CURVE_H_
#define BOTAN_ECC_HASH_TO_CURVE_H_

#include <botan/point_gfp.h>

namespace Botan {

class EC_Group;

/**
* Hash an input onto an elliptic curve point using the
* Shallue-Woestijne-Ulas method.
*
* This method requires that the ECC group have (a*b) != 0
* which excludes certain groups including secp256k1
*/
PointGFp BOTAN_PUBLIC_API(2,10)
   hash_to_curve_swu(const EC_Group& group,
                     const std::string& hash_fn,
                     const uint8_t input[],
                     size_t input_len,
                     const uint8_t domain_sep[],
                     size_t domain_sep_len);


}

#endif
