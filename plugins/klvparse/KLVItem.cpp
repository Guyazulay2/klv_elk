#include <KLVItem.h>
using namespace KLV;

gboolean KLV::gst_byte_reader_from_g_bytes(GstByteReader* reader, GBytes* bytes) {
    if (bytes == NULL) return false;
    gsize size;
    const guint8* data = (const guint8*)g_bytes_get_data(bytes, &size);
    gst_byte_reader_init(reader, data, size);
    return true;
}

bool KLV::gst_byte_reader_get_ber(GstByteReader* reader, guint64* value) {
    guint8 byte;
    if (!gst_byte_reader_get_uint8(reader, &byte)) {
        return false;
    }
    if (!(byte & 0x80)) {
        (*value) = byte;
        return true;
    }
    guint8 length = byte & 0x7f;
    if (length > 8) {
        // std::cout << "[ERROR] failed to parse ber, size=" << length << " too big for uint64" << std::endl;
        return false;
    }
    guint64 res = 0;
    for (guint i = 0; i < length; i++) {
        if (!gst_byte_reader_get_uint8(reader, &byte)) {
            // std::cout << "[ERROR] failed to parse ber, " << i << "/" << length << std::endl;
            return false;
        }
        res = (res << 8) | byte;
    }
    (*value) = res;
    return true;
}

bool KLV::gst_byte_reader_get_ber_oid(GstByteReader* reader, guint64* value) {
    guint8 byte;
    gsize res = 0;
    for (guint8 i = 0;; i++) {
        if (i > 8) {
            // std::cout << "[ERROR] failed to parse ber-oid, size=" << i << " too big for uint64" << std::endl;
            return false;
        }
        if (!gst_byte_reader_get_uint8(reader, &byte)) {
            break;
        }
        res = res << 7 | (0x7f & byte);
        if (!(byte & 0x80)) break;
    }
    (*value) = res;
    return true;
}

bool KLV::gst_byte_reader_get_count(GstByteReader* reader, guint64* value, int length) {
    if (length > 8) {
        // std::cout << "[ERROR] failed to parse static, size=" << length << " too big for uint64" << std::endl;
        return false;
    }
    guint8 byte;
    gsize res = 0;
    if (length < 0) {
        while (true) {
            if (!gst_byte_reader_get_uint8(reader, &byte)) {
                break;
            }
            res = res << 8 | byte;
        }
        (*value) = res;
        return true;
    } else {
        for (guint8 i = 0; i < length; i++) {
            if (!gst_byte_reader_get_uint8(reader, &byte)) {
                // std::cout << "[ERROR] failed to parse static, " << i << "/" << length << std::endl;
                return false;
            }
            res = res << 8 | byte;
        }
        (*value) = res;
        return true;
    }
}
bool KLV::get_ber(GBytes* bytes, guint64* value) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, bytes)) {
        return false;
    }
    return gst_byte_reader_get_ber(&reader, value);
}

bool KLV::get_ber_oid(GBytes* bytes, guint64* value) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, bytes)) {
        return false;
    }
    return gst_byte_reader_get_ber_oid(&reader, value);
}

bool KLV::get_count(GBytes* bytes, guint64* value, int length) {
    if (length > 8) {
        // std::cout << "[ERROR] failed to parse static, size=" << length << " too big for uint64" << std::endl;
        return false;
    }
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, bytes)) {
        return false;
    }
    return gst_byte_reader_get_count(&reader, value);
}

KLVItem::KLVItem() {
    g_value_unset(&this->value_parsed);
}

KLVItem::~KLVItem() {
    if (this->key != NULL) {
        g_bytes_unref(this->key);
    }
    if (this->length != NULL) {
        g_bytes_unref(this->length);
    }
    if (this->value != NULL) {
        g_bytes_unref(this->value);
    }
    g_value_unset(&this->value_parsed);
}

bool KLVItem::_get_key_content(guint64* value) {
    if (this->key == NULL) return false;
    if (this->key_format == KEY_FORMATS::_BER_OID) return get_ber_oid(this->key, value);
    return get_count(this->key, value, (int)this->key_format);
}

bool KLVItem::get_key_content(guint64* value) {
    if (this->cached_key_content) {
        (*value) = this->key_content;
        return true;
    }
    this->cached_key_content = _get_key_content(value);
    if (this->cached_key_content) this->key_content = *value;
    return this->cached_key_content;
}

bool KLVItem::_get_length_content(guint64* value) {
    if (this->length == NULL) return false;
    if (this->len_format == LENGTH_FORMATS::_BER) return get_ber(this->length, value);
    return get_count(this->length, value, (int)this->len_format);
}

bool KLVItem::get_length_content(guint64* value) {
    if (this->cached_length_content) {
        (*value) = this->length_content;
        return true;
    }
    this->cached_length_content = _get_length_content(value);
    if (this->cached_length_content) this->length_content = *value;
    return this->cached_length_content;
}

std::string KLVItem::debug() {
    std::stringstream s;
    // s << "(" << std::endl
    //   << g_bytes_get_size(this->key) << ": " << debug_bytes(this->key, -1) << ", " << std::endl
    //   << g_bytes_get_size(this->length) << ": " << debug_bytes(this->length, -1) << ", " << std::endl
    //   << g_bytes_get_size(this->value) << ": " << debug_bytes(this->value, -1) << std::endl
    //   << ")";
    s << "(";
    guint64 value;
    if (this->key_parsed != "") {
        guint64 key;
        if (this->get_key_content(&key)) {
            s << std::setfill(' ') << std::setw(3) << key << ": ";
        }
        s << this->key_parsed;
    } else {
        if (this->get_key_content(&value)) {
            s << value;
        } else {
            s << "~";
        }
    }
    s << ", ";
    if (this->get_length_content(&value)) {
        s << "len:" << value;
    } else {
        s << "~";
    }
    if (G_IS_VALUE(&this->value_parsed)) {
        s << ", ";
        if (G_VALUE_HOLDS_STRING(&this->value_parsed)) {
            s << "\"" << g_value_get_string(&this->value_parsed) << "\"";
        } else if (G_VALUE_HOLDS_BOOLEAN(&this->value_parsed)) {
            s << g_value_get_boolean(&this->value_parsed);
        } else if (G_VALUE_HOLDS_INT64(&this->value_parsed)) {
            s << g_value_get_int64(&this->value_parsed);
        } else if (G_VALUE_HOLDS_INT(&this->value_parsed)) {
            s << g_value_get_int(&this->value_parsed);
        } else if (G_VALUE_HOLDS_UINT(&this->value_parsed)) {
            s << g_value_get_uint(&this->value_parsed);
        } else if (G_VALUE_HOLDS_UINT64(&this->value_parsed)) {
            s << g_value_get_uint64(&this->value_parsed);
        } else if (G_VALUE_HOLDS_DOUBLE(&this->value_parsed)) {
            s << g_value_get_double(&this->value_parsed);
        } else if (G_VALUE_HOLDS_FLOAT(&this->value_parsed)) {
            s << g_value_get_float(&this->value_parsed);
        } else if (G_VALUE_HOLDS_VARIANT(&this->value_parsed)) {
            s << "variant (not implemented)";
            // s << g_value_get_variant(&this->value_parsed);
        } else if (G_VALUE_HOLDS_BOXED(&this->value_parsed)) {
            s << "boxed (not implemented)";
            // s << g_value_get_boxed(&this->value_parsed);
        }
        s << ")";
    }
    // else {
    //     if (this->get_length_content(&value)) {
    //         s << "len:" << value;
    //     } else {
    //         s << "~";
    //     }
    //     s << ")";
    // }
    return s.str();
}

std::string KLVItem::debug_bytes(GBytes* bytes, gint8 cols) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, bytes)) {
        return "";
    }
    std::stringstream s;
    guint8 byte;
    for (guint8 i = 0;; i++) {
        if (!gst_byte_reader_get_uint8(&reader, &byte)) {
            if (cols > 0) s << "\n";
            break;
        }
        if (cols > 0 && i % cols == 0)
            s << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)byte << std::dec << "\n";
        else {
            if (cols > 0 && i % cols == 1) s << i << ": ";
            s << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)byte << std::dec << ", ";
        }
    }
    return s.str();
}

bool KLVItem::get_key_parsed(std::string* res) {
    std::string key_parsed = this->key_parsed;
    if (key_parsed != "") {
        *res = key_parsed;
        return true;
    }
    guint64 key;
    if (this->get_key_content(&key)) {
        *res = key;
        return true;
    }
    if (this->key == NULL) {
        return false;
    }
    gsize size;
    const gchar* data = (const gchar*)g_bytes_get_data(this->key, &size);
    GString* str = g_string_new_len(data, size);
    *res = str->str;
    g_string_free(str, true);
    return true;
}

json KLVItem::to_json() {
    if (child != NULL) {
        json j = json::object();
        for (KLVItem* curr = child.get(); curr != NULL; curr = curr->next.get()) {
            std::string key;
            if (!curr->get_key_parsed(&key)) {
                continue;
            }
            j[key] = curr->to_json();
        }
        return j;
    }
    if (G_IS_VALUE(&this->value_parsed)) {
        if (G_VALUE_HOLDS_STRING(&this->value_parsed)) {
            return g_value_get_string(&this->value_parsed);
        } else if (G_VALUE_HOLDS_BOOLEAN(&this->value_parsed)) {
            return g_value_get_boolean(&this->value_parsed);
        } else if (G_VALUE_HOLDS_INT64(&this->value_parsed)) {
            return g_value_get_int64(&this->value_parsed);
        } else if (G_VALUE_HOLDS_INT(&this->value_parsed)) {
            return g_value_get_int(&this->value_parsed);
        } else if (G_VALUE_HOLDS_UINT(&this->value_parsed)) {
            return g_value_get_uint(&this->value_parsed);
        } else if (G_VALUE_HOLDS_UINT64(&this->value_parsed)) {
            return g_value_get_uint64(&this->value_parsed);
        } else if (G_VALUE_HOLDS_DOUBLE(&this->value_parsed)) {
            return g_value_get_double(&this->value_parsed);
        } else if (G_VALUE_HOLDS_FLOAT(&this->value_parsed)) {
            return g_value_get_float(&this->value_parsed);
        } else if (G_VALUE_HOLDS_VARIANT(&this->value_parsed)) {
            return "variant (not implemented)";
            // return g_value_get_variant(&this->value_parsed);
        } else if (G_VALUE_HOLDS_BOXED(&this->value_parsed)) {
            return "boxed (not implemented)";
            // return g_value_get_boxed(&this->value_parsed);
        }
    }
    return nullptr;
}