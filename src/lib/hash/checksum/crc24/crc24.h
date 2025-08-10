/*
* CRC24
* (C) 1999-2007 Jack Lloyd
* (C) 2017 [Ribose Inc](https://www.ribose.com). Performed by Krzysztof Kwiatkowski.
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CRC24_H_
#define BOTAN_CRC24_H_

#include <botan/hash.h>

namespace Botan {

/**
* 24-bit cyclic redundancy check
*
* This is the CRC used for checksums in PGP
*/
class CRC24 final : public HashFunction {
   public:
      std::string name() const override { return "CRC24"; }

      size_t output_length() const override { return 3; }

      std::unique_ptr<HashFunction> new_object() const override { return std::make_unique<CRC24>(); }

      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override { m_crc = 0xCE04B7; }

   private:
      void add_data(std::span<const uint8_t> input) override;
      void final_result(std::span<uint8_t> output) override;
      uint32_t m_crc = 0xCE04B7;
};

}  // namespace Botan

#endif
