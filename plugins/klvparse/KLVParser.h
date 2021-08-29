#pragma once

#include <KLVItem.h>
#include <KLVStorage.h>
#include <KLVTranslators.h>
// #include <gst/base/gstbitreader.h>
#include <gst/base/gstbytereader.h>

#include <bitset>
#include <iostream>
#include <sstream>

namespace KLV {
class KLVParser {
   private:
    std::shared_ptr<KLVItem> klv;
    KLVStorage storage;
    bool assign_key_content(KLVItem *item);
    bool assign_value_content(KLVItem *item);
    bool assign_key_and_value_content(KLVItem *item);
    bool translate_stanag(KLVItem *item);

    /**
     * checks if the last uint16 equals the sum of uint16 beforehand.
     * returns true if test was successfull, 
     * if is_valid != NULL, stores if calculated checksum equals stored checksum
     * if calculated != NULL, stores calculated checksum value in *calculated;
     * if actual != NULL, stores actual checksum value in *actual;
     */
    bool _verify_checksum(GstByteReader *reader, bool *is_valid = NULL, guint16 *calculated = NULL, guint16 *actual = NULL);

   public:
    static std::unordered_map<guint64, std::string> klv_tags;
    static std::unordered_map<guint64, std::function<bool(KLVItem *, KLVStorage *)>> klv_translators;

    static void init_klv_tags();
    static void init_klv_translators();

    KLVParser();
    virtual ~KLVParser();

    bool parse(GBytes *bytes);
    bool parse(const guint8 *bytes, guint size);
    bool parse(GstByteReader *reader);

    bool verify_checksum(GstByteReader *reader);

    bool strict = false;
    bool verbose = false;

    // GBytes *encode();
    json to_json();
};
}  // namespace KLV