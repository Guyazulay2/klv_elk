#pragma once

#include <KLV.h>
#include <gst/base/gstbytereader.h>

#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace KLV {

gboolean gst_byte_reader_from_g_bytes(GstByteReader* reader, GBytes* bytes);
bool gst_byte_reader_get_ber(GstByteReader* reader, guint64* value);
bool gst_byte_reader_get_ber_oid(GstByteReader* reader, guint64* value);
bool gst_byte_reader_get_count(GstByteReader* reader, guint64* value, int size = -1);

bool get_ber(GBytes* bytes, guint64* value);
bool get_ber_oid(GBytes* bytes, guint64* value);
bool get_count(GBytes* bytes, guint64* value, int size = -1);

class KLVItem {
   public:
    KEY_FORMATS key_format = KEY_FORMATS::_;
    LENGTH_FORMATS len_format = LENGTH_FORMATS::_;

    GBytes* key = NULL;
    bool cached_key_content = false;
    guint64 key_content = 0;
    std::string key_parsed;

    GBytes* length = NULL;

    bool cached_length_content = false;
    guint64 length_content = 0;

    GBytes* value = NULL;
    // is owner of GValue, will unref when done;
    GValue value_parsed = G_VALUE_INIT;

    KLVItem* parent;
    KLVItem* previous;
    std::shared_ptr<KLVItem> child;
    std::shared_ptr<KLVItem> next;

    static std::string debug_bytes(GBytes* bytes, gint8 cols = 16);

   public:
    KLVItem();
    virtual ~KLVItem();

    bool _get_key_content(guint64* value);
    bool get_key_content(guint64* value);

    bool _get_length_content(guint64* value);
    bool get_length_content(guint64* value);

    bool get_key_parsed(std::string* res);

    std::string debug();
    json to_json();
};
};  // namespace KLV