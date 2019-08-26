/*
* (C) 2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECC_HASH_TO_CURVE_H_
#define BOTAN_ECC_HASH_TO_CURVE_H_

#include <botan/types.h>
#include <botan/point_gfp.h>
#include <string>

namespace Botan {

class EC_Group;

/**
* Hash an input onto an elliptic curve point using the
* simplified Shallue-Woestijne-Ulas method.
*
* This method requires that the ECC group have (a*b) != 0
* which excludes certain groups including secp256k1
*/
PointGFp BOTAN_PUBLIC_API(2,12)
   hash_to_curve_sswu(const EC_Group& group,
                      const std::string& hash_fn,
                      const uint8_t input[],
                      size_t input_len,
                      const uint8_t domain_sep[],
                      size_t domain_sep_len);

/**
* Remove this later ...
*/
PointGFp BOTAN_TEST_API map_to_curve_sswu(const EC_Group& group, const BigInt& u);

/**
* Hash to an integer value
*/
BigInt BOTAN_TEST_API hash_to_base(const EC_Group& group,
                                   const std::string& hash_fn,
                                   const uint8_t input[], size_t input_len,
                                   const uint8_t domain_sep[], size_t domain_sep_len,
                                   uint8_t ctr,
                                   size_t k = 128);


}

#endif
