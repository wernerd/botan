/*
* DataSink
* (C) 1999-2007 Jack Lloyd
*     2017 Philippe Lieser
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DATA_SINK_H_
#define BOTAN_DATA_SINK_H_

#include <botan/filter.h>
#include <iosfwd>
#include <memory>

namespace Botan {

/**
* This class represents abstract data sink objects.
*/
class BOTAN_PUBLIC_API(2, 0) DataSink : public Filter {
   public:
      bool attachable() override { return false; }
};

/**
* This class represents a data sink which writes its output to a stream.
*/
class BOTAN_PUBLIC_API(2, 0) DataSink_Stream final : public DataSink {
   public:
      /**
      * Construct a DataSink_Stream from a stream.
      * @param stream the stream to write to
      * @param name identifier
      */
      BOTAN_FUTURE_EXPLICIT DataSink_Stream(std::ostream& stream, std::string_view name = "<std::ostream>");

#if defined(BOTAN_TARGET_OS_HAS_FILESYSTEM)

      /**
      * Construct a DataSink_Stream from a filesystem path name.
      * @param pathname the name of the file to open a stream to
      * @param use_binary indicates whether to treat the file
      * as a binary file or not
      */
      BOTAN_FUTURE_EXPLICIT DataSink_Stream(std::string_view pathname, bool use_binary = false);
#endif

      DataSink_Stream(const DataSink_Stream& other) = delete;
      DataSink_Stream(DataSink_Stream&& other) = delete;
      DataSink_Stream& operator=(const DataSink_Stream& other) = delete;
      DataSink_Stream& operator=(DataSink_Stream&& other) = delete;

      std::string name() const override { return m_identifier; }

      void write(const uint8_t buf[], size_t len) override;

      void end_msg() override;

      ~DataSink_Stream() override;

   private:
      const std::string m_identifier;

      // May be null, if m_sink was an external reference
      std::unique_ptr<std::ostream> m_sink_memory;
      std::ostream& m_sink;
};

}  // namespace Botan

#endif
