#include <KLVTranslators.h>

using namespace KLV;

bool KLV::parse_klv(GstByteReader *reader, KLVItem *klv, KEY_FORMATS key_fromat, LENGTH_FORMATS len_format) {
    klv->key_format = key_fromat;
    klv->len_format = len_format;
    guint8 byte;

    guint pos = 0;
    gsize size = 0;
    guint8 *temp = NULL;

    // parse key
    if (key_fromat == KEY_FORMATS::_BER_OID) {
        pos = gst_byte_reader_get_pos(reader);
        size = 0;
        while (true) {
            if (!gst_byte_reader_get_uint8(reader, &byte)) {
                // std::cout << "failed to parse ber-oid encoded key" << std::endl;
                return false;
            }
            size++;
            if (!(byte & 0x80)) break;
        }
        gst_byte_reader_set_pos(reader, pos);
        if (!gst_byte_reader_dup_data(reader, size, &temp)) {
            // std::cout << "failed to store ber-oid encoded key" << std::endl;
            return false;
        }
        klv->key = g_bytes_new_take(temp, size);
    } else if (((gint)key_fromat) > 0) {
        size = (gsize)key_fromat;
        if (!gst_byte_reader_dup_data(reader, size, &temp)) {
            // std::cout << "failed to parse static length encoded key: " << size << " bytes" << std::endl;
            return false;
        }
        klv->key = g_bytes_new_take(temp, size);
    } else {
        // std::cout << "unknown key_format" << std::endl;
        return false;
    }

    // parse length
    if (len_format == LENGTH_FORMATS::_BER) {
        // pos = gst_byte_reader_get_pos(reader);
        if (!gst_byte_reader_peek_uint8(reader, &byte)) {
            // std::cout << "failed to read length" << std::endl;
            return false;
        }
        size = (byte & 0x80 ? byte & 0x7f : 0) + 1;
        if (!gst_byte_reader_dup_data(reader, size, &temp)) {
            // std::cout << "failed to parse ber encoded length: " << size << " bytes" << std::endl;
            return false;
        }
        klv->length = g_bytes_new_take(temp, size);
    } else {
        size = (gsize)len_format;
        if (!gst_byte_reader_dup_data(reader, size, &temp)) {
            // std::cout << "failed to parse static length: " << size << " bytes" << std::endl;
            return false;
        }
        klv->length = g_bytes_new_take(temp, size);
    }

    // parse value
    if (!klv->get_length_content(&size)) {
        // std::cout << "failed to parse length size in bytes" << std::endl;
        return false;
    };
    if (!gst_byte_reader_dup_data(reader, size, &temp)) {
        // std::cout << "failed to parse value: " << size << " bytes" << std::endl;
        return false;
    }
    klv->value = g_bytes_new_take(temp, size);
    return true;
}

bool KLV::gst_byte_reader_get_variable_uint32_be(GstByteReader *reader, guint32 *res, guint8 *res_byte_len) {
    guint32 res_actual = 0;
    guint8 res_byte_len_actual = 1;
    guint8 curr = 0;
    for (res_byte_len_actual = 1; res_byte_len_actual <= 4; res_byte_len_actual++) {
        if (!gst_byte_reader_get_uint8(reader, &curr)) {
            res_byte_len_actual--;
            break;
        }
        res_actual = (res_actual << 8) | curr;
    }

    *res = res_actual;
    if (res_byte_len != NULL)
        *res_byte_len = res_byte_len_actual;
    return true;
}

bool KLV::gst_byte_reader_get_variable_uint64_be(GstByteReader *reader, guint64 *res, guint8 *res_byte_len) {
    guint64 res_actual = 0;
    guint8 res_byte_len_actual = 1;
    guint8 curr = 0;
    for (res_byte_len_actual = 1; res_byte_len_actual <= 8; res_byte_len_actual++) {
        if (!gst_byte_reader_get_uint8(reader, &curr)) {
            res_byte_len_actual--;
            break;
        }
        res_actual = (res_actual << 8) | curr;
    }

    *res = res_actual;
    if (res_byte_len != NULL)
        *res_byte_len = res_byte_len_actual;
    return true;
}

bool KLV::gst_byte_reader_get_variable_int32_be(GstByteReader *reader, gint32 *res, guint8 *res_byte_len) {
    guint32 res_actual = 0;
    guint8 res_byte_len_actual = 0;
    if (!gst_byte_reader_get_variable_uint32_be(reader, &res_actual, &res_byte_len_actual)) {
        return false;
    }
    if (res_byte_len_actual > 4) {
        return false;
    }
    bool is_negative = 1 & (res_actual >> (8 * res_byte_len_actual));
    if (is_negative) {
        guint32 mask = -1 - ((1 << (res_byte_len_actual * 8)) - 1);
        res_actual |= mask;
    }
    *res = res_actual;
    if (res_byte_len != NULL)
        *res_byte_len = res_byte_len_actual;
    return true;
}

bool KLV::gst_byte_reader_get_variable_int64_be(GstByteReader *reader, gint64 *res, guint8 *res_byte_len) {
    guint64 res_actual = 0;
    guint8 res_byte_len_actual = 0;
    if (!gst_byte_reader_get_variable_uint64_be(reader, &res_actual, &res_byte_len_actual)) {
        return false;
    }
    if (res_byte_len_actual > 8) {
        return false;
    }
    bool is_negative = 1 & (res_actual >> (8 * res_byte_len_actual));
    if (is_negative) {
        guint64 mask = -1 - ((1 << (res_byte_len_actual * 8)) - 1);
        res_actual |= mask;
    }
    *res = res_actual;
    if (res_byte_len != NULL)
        *res_byte_len = res_byte_len_actual;
    return true;
}

bool KLV::get_uint8_from_klv_value(KLVItem *item, guint8 *store) {
    if (item->value == NULL) return false;
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint8 res = 0;
    if (!gst_byte_reader_get_uint8(&reader, &res)) {
        return false;
    }
    (*store) = res;
    return true;
}

bool KLV::get_int8_from_klv_value(KLVItem *item, gint8 *store) {
    if (item->value == NULL) return false;
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    gint8 res = 0;
    if (!gst_byte_reader_get_int8(&reader, &res)) {
        return false;
    }
    (*store) = res;
    return true;
}

bool KLV::get_uint16_from_klv_value(KLVItem *item, guint16 *store) {
    if (item->value == NULL) return false;
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint16 res = 0;
    if (!gst_byte_reader_get_uint16_be(&reader, &res)) {
        return false;
    }
    (*store) = res;
    return true;
}

bool KLV::get_int16_from_klv_value(KLVItem *item, gint16 *store) {
    if (item->value == NULL) return false;
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    gint16 res = 0;
    if (!gst_byte_reader_get_int16_be(&reader, &res)) {
        return false;
    }
    (*store) = res;
    return true;
}

bool KLV::get_uint32_from_klv_value(KLVItem *item, guint32 *store) {
    if (item->value == NULL) return false;
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint32 res = 0;
    if (!gst_byte_reader_get_uint32_be(&reader, &res)) {
        return false;
    }
    (*store) = res;
    return true;
}

bool KLV::get_int32_from_klv_value(KLVItem *item, gint32 *store) {
    if (item->value == NULL) return false;
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    gint32 res = 0;
    if (!gst_byte_reader_get_int32_be(&reader, &res)) {
        return false;
    }
    (*store) = res;
    return true;
}

bool KLV::get_uint64_from_klv_value(KLVItem *item, guint64 *store) {
    if (item->value == NULL) return false;
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint64 res = 0;
    if (!gst_byte_reader_get_uint64_be(&reader, &res)) {
        return false;
    }
    (*store) = res;
    return true;
}

bool KLV::uint8_translator(KLVItem *item) {
    guint8 res = 0;
    if (!get_uint8_from_klv_value(item, &res)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_UINT);
    g_value_set_uint(&item->value_parsed, res);
    return true;
}

bool KLV::uint16_translator(KLVItem *item) {
    guint16 res = 0;
    if (!get_uint16_from_klv_value(item, &res)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_UINT);
    g_value_set_uint(&item->value_parsed, res);
    return true;
}

bool KLV::uint64_translator(KLVItem *item) {
    guint64 res = 0;
    if (!get_uint64_from_klv_value(item, &res)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_UINT64);
    g_value_set_uint64(&item->value_parsed, res);
    return true;
}

bool KLV::string_translator(KLVItem *item) {
    if (item->value == NULL) return false;
    gsize size = 0;
    const gchar *data = (const gchar *)g_bytes_get_data(item->value, &size);
    GString *str = g_string_new_len(data, size);
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    g_value_set_string(&item->value_parsed, str->str);
    g_string_free(str, true);
    return true;
}

std::string timestamp_from_uint64(guint64 time) {
    using time_point = std::chrono::system_clock::time_point;
    time_point uptime_timepoint{std::chrono::duration_cast<time_point::duration>(std::chrono::microseconds(time))};
    std::time_t t = std::chrono::system_clock::to_time_t(uptime_timepoint);
    auto tm = *std::localtime(&t);

    std::stringstream s;
    // s << std::put_time(&tm, "yyyy-MM-dd'T'HH:mm:ss'.'SSSZ");
    s << std::put_time(&tm, "%d-%m-%Y-T%H:%M:%S.%z%Z");
    return s.str();
}

bool KLV::timestamp_translator(KLVItem *item) {
    guint64 res = 0;
    if (!get_uint64_from_klv_value(item, &res)) {
        return false;
    }
    std::string str = timestamp_from_uint64(res);
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    g_value_set_string(&item->value_parsed, str.c_str());
    return true;
}

#define _IN_RANGE(val, min, max) (min < val && val < max)

gfloat KLV::uint8_deg_to_float(guint8 val, gfloat min, gfloat max, gfloat offset) {
    guint8 range = -1;
    gfloat normalizer = (max - min) / range;
    return (((gfloat)val) * normalizer) + offset;
}

gdouble KLV::int32_deg_to_double(gint32 val, gdouble min, gdouble max, gdouble offset) {
    guint32 range = -1;
    gdouble normalizer = (max - min) / range;
    return (((gdouble)val) * normalizer) + offset;
}

bool KLV::uint8_deg_to_float_translator(KLVItem *item, gfloat min, gfloat max, gfloat offset) {
    guint16 res = 0;
    if (!get_uint16_from_klv_value(item, &res)) {
        return false;
    }
    guint8 range = -1;
    gfloat normalizer = (max - min) / range;
    gfloat angle = (((gfloat)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_FLOAT);
    g_value_set_float(&item->value_parsed, angle);
    return true;
}

bool KLV::uint16_deg_to_float_translator(KLVItem *item, gfloat min, gfloat max, gfloat offset) {
    guint16 res = 0;
    if (!get_uint16_from_klv_value(item, &res)) {
        return false;
    }
    guint16 range = -1;
    gfloat normalizer = (max - min) / range;
    gfloat angle = (((gfloat)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_FLOAT);
    g_value_set_float(&item->value_parsed, angle);
    return true;
}

bool KLV::uint16_deg_to_float_translator_reserved(KLVItem *item, gfloat min, gfloat max, gfloat offset, const gchar *reserved) {
    guint16 res = 0;
    if (!get_uint16_from_klv_value(item, &res)) {
        return false;
    }
    guint16 range = -2;
    gfloat normalizer = (max - min) / range;
    gfloat angle = (((gfloat)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    if (_IN_RANGE(angle, min, max)) {
        g_value_init(&item->value_parsed, G_TYPE_FLOAT);
        g_value_set_float(&item->value_parsed, angle);
    } else {
        g_value_init(&item->value_parsed, G_TYPE_STRING);
        g_value_set_string(&item->value_parsed, reserved);
    }
    return true;
}

bool KLV::int16_deg_to_float_translator(KLVItem *item, gfloat min, gfloat max, gfloat offset) {
    gint16 res = 0;
    if (!get_int16_from_klv_value(item, &res)) {
        return false;
    }
    guint16 range = -1;
    gfloat normalizer = (max - min) / range;
    gfloat angle = (((gfloat)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_FLOAT);
    g_value_set_float(&item->value_parsed, angle);
    return true;
}

bool KLV::int16_deg_to_float_translator_reserved(KLVItem *item, gfloat min, gfloat max, gfloat offset, const gchar *reserved) {
    gint16 res = 0;
    if (!get_int16_from_klv_value(item, &res)) {
        return false;
    }
    guint16 range = -2;
    gfloat normalizer = (max - min) / range;
    gfloat angle = (((gfloat)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    if (_IN_RANGE(angle, min, max)) {
        g_value_init(&item->value_parsed, G_TYPE_FLOAT);
        g_value_set_float(&item->value_parsed, angle);
    } else {
        g_value_init(&item->value_parsed, G_TYPE_STRING);
        g_value_set_string(&item->value_parsed, reserved);
    }
    return true;
}

bool KLV::uint32_deg_to_double_translator(KLVItem *item, gdouble min, gdouble max, gdouble offset) {
    guint32 res = 0;
    if (!get_uint32_from_klv_value(item, &res)) {
        return false;
    }
    guint32 range = -1;
    gdouble normalizer = (max - min) / range;
    gdouble angle = (((gdouble)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_DOUBLE);
    g_value_set_double(&item->value_parsed, angle);
    return true;
}

bool KLV::uint32_deg_to_double_translator_reserved(KLVItem *item, gdouble min, gdouble max, gdouble offset, const gchar *reserved) {
    guint32 res = 0;
    if (!get_uint32_from_klv_value(item, &res)) {
        return false;
    }
    guint32 range = -2;
    gdouble normalizer = (max - min) / range;
    gdouble angle = (((gdouble)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    if (_IN_RANGE(angle, min, max)) {
        g_value_init(&item->value_parsed, G_TYPE_DOUBLE);
        g_value_set_double(&item->value_parsed, angle);
    } else {
        g_value_init(&item->value_parsed, G_TYPE_STRING);
        g_value_set_string(&item->value_parsed, reserved);
    }
    return true;
}

bool KLV::int32_deg_to_double_translator(KLVItem *item, gdouble min, gdouble max, gdouble offset) {
    gint32 res = 0;
    if (!get_int32_from_klv_value(item, &res)) {
        return false;
    }
    guint32 range = -1;
    gdouble normalizer = (max - min) / range;
    gdouble angle = (((gdouble)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_DOUBLE);
    g_value_set_double(&item->value_parsed, angle);
    return true;
}

bool KLV::int32_deg_to_double_translator_reserved(KLVItem *item, gdouble min, gdouble max, gdouble offset, const gchar *reserved) {
    gint32 res = 0;
    if (!get_int32_from_klv_value(item, &res)) {
        return false;
    }
    guint32 range = -2;
    gdouble normalizer = (max - min) / range;
    gdouble angle = (((gdouble)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    if (_IN_RANGE(angle, min, max)) {
        g_value_init(&item->value_parsed, G_TYPE_DOUBLE);
        g_value_set_double(&item->value_parsed, angle);
    } else {
        g_value_init(&item->value_parsed, G_TYPE_STRING);
        g_value_set_string(&item->value_parsed, reserved);
    }
    return true;
}

bool KLV::variable_deg_to_float_translator(KLVItem *item, gfloat min, gfloat max, gfloat offset) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint32 res;
    guint8 res_byte_len;
    if (gst_byte_reader_get_variable_uint32_be(&reader, &res, &res_byte_len)) {
        return false;
    }
    guint32 range = res_byte_len >= 4 ? -1 : (1 << (res_byte_len * 8)) - 1;
    gfloat normalizer = (max - min) / range;
    gfloat angle = (((gfloat)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_FLOAT);
    g_value_set_float(&item->value_parsed, angle);
    return true;
}

bool KLV::variable_deg_to_float_translator_reserved(KLVItem *item, gfloat min, gfloat max, gfloat offset, const gchar *reserved) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint32 res;
    guint8 res_byte_len;
    if (gst_byte_reader_get_variable_uint32_be(&reader, &res, &res_byte_len)) {
        return false;
    }
    guint32 range = res_byte_len >= 4 ? -2 : (1 << (res_byte_len * 8)) - 2;
    gfloat normalizer = (max - min) / range;
    gfloat angle = (((gfloat)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    if (_IN_RANGE(angle, min, max)) {
        g_value_init(&item->value_parsed, G_TYPE_FLOAT);
        g_value_set_double(&item->value_parsed, angle);
    } else {
        g_value_init(&item->value_parsed, G_TYPE_STRING);
        g_value_set_string(&item->value_parsed, reserved);
    }
    return true;
}

bool KLV::variable_deg_to_double_translator(KLVItem *item, gdouble min, gdouble max, gdouble offset) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint64 res;
    guint8 res_byte_len;
    if (gst_byte_reader_get_variable_uint64_be(&reader, &res, &res_byte_len)) {
        return false;
    }
    guint64 range = res_byte_len >= 8 ? -1 : (1 << (res_byte_len * 8)) - 1;
    gdouble normalizer = (max - min) / range;
    gdouble angle = (((gdouble)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_DOUBLE);
    g_value_set_double(&item->value_parsed, angle);
    return true;
}

bool KLV::variable_deg_to_double_translator_reserved(KLVItem *item, gdouble min, gdouble max, gdouble offset, const gchar *reserved) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint64 res;
    guint8 res_byte_len;
    if (gst_byte_reader_get_variable_uint64_be(&reader, &res, &res_byte_len)) {
        return false;
    }
    guint64 range = res_byte_len >= 8 ? -2 : (1 << (res_byte_len * 8)) - 2;
    gdouble normalizer = (max - min) / range;
    gdouble angle = (((gdouble)res) * normalizer) + offset;
    g_value_unset(&item->value_parsed);
    if (_IN_RANGE(angle, min, max)) {
        g_value_init(&item->value_parsed, G_TYPE_DOUBLE);
        g_value_set_double(&item->value_parsed, angle);
    } else {
        g_value_init(&item->value_parsed, G_TYPE_STRING);
        g_value_set_string(&item->value_parsed, reserved);
    }
    return true;
}

bool KLV::general_klv_assigner(KLVItem *item) {
    guint64 key_content;
    if (!item->get_key_content(&key_content)) {
        return false;
    }
    item->key_parsed = std::to_string(key_content);
    return string_translator(item);
}

bool KLV::general_klv_translator(KLVItem *item) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    KLVItem *curr = item;  // = std::shared_ptr<KLVItem>(item);
    for (guint8 i = 0; gst_byte_reader_get_remaining(&reader) > 0; i++) {
        std::shared_ptr<KLVItem> next = std::make_shared<KLVItem>();
        // if (!parse_klv(&sub_reader, next.get(), KEY_FORMATS::_BER_OID, LENGTH_FORMATS::_BER)) {
        if (!parse_klv(&reader, next.get(), KEY_FORMATS::_1, LENGTH_FORMATS::_1)) {
            // std::cout << "[WARN] failed to parse nested child #" << i << std::endl;
            break;
        }
        if (!general_klv_assigner(next.get())) {
            // std::cout << "[WARN] failed to assign nested child #" << i << std::endl;
            break;
        }
        // std::cout << "[INFO] " << next->debug() << std::endl;
        if (i == 0) {
            curr->child = next;
            next->parent = curr;
        } else {
            curr->next = next;
            next->previous = curr;
        }
        curr = next.get();
    }
    return true;
}

bool KLV::checksum_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_translator(item);
}

bool KLV::precision_time_stamp_translator(KLVItem *item, KLVStorage *storage) {
    return timestamp_translator(item);
}

bool KLV::mission_id_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::platform_tail_number_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::platform_heading_angle_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 360);
}

bool KLV::platform_pitch_angle_translator(KLVItem *item, KLVStorage *storage) {
    return int16_deg_to_float_translator_reserved(item, -20, 20, 0, "Out of Range");
}

bool KLV::platform_roll_angle_translator(KLVItem *item, KLVStorage *storage) {
    return int16_deg_to_float_translator_reserved(item, -50, 50, 0, "Out of Range");
}

bool KLV::platform_true_airspeed_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_translator(item);
}

bool KLV::platform_indicated_airspeed_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_translator(item);
}

bool KLV::platform_designation_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::image_source_sensor_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::image_coordinate_system_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::sensor_latitude_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "Reserved");
}

bool KLV::sensor_longitude_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "Reserved");
}

bool KLV::sensor_true_altitude_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::sensor_horizontal_field_of_view_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 180);
}

bool KLV::sensor_vertical_field_of_view_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 180);
}

bool KLV::sensor_relative_azimuth_angle_translator(KLVItem *item, KLVStorage *storage) {
    return uint32_deg_to_double_translator(item, 0, 360);
}

bool KLV::sensor_relative_elevation_angle_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "Reserved");
}

bool KLV::sensor_relative_roll_angle_translator(KLVItem *item, KLVStorage *storage) {
    return uint32_deg_to_double_translator(item, 0, 360);
}

bool KLV::slant_range_translator(KLVItem *item, KLVStorage *storage) {
    return uint32_deg_to_double_translator(item, 0, 5e6);  // 5,000,000
}

bool KLV::target_width_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 1e4);  // 10,000
}

bool KLV::frame_center_latitude_translator(KLVItem *item, KLVStorage *storage) {
    bool res = int32_deg_to_double_translator_reserved(item, -90, 90, 0, "N/A (Off-Earth)");
    if (res && G_VALUE_HOLDS_DOUBLE(&item->value_parsed)) {
        storage->set(23, &item->value_parsed);
    }
    return res;
    // return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "N/A (Off-Earth)");
}

bool KLV::frame_center_longitude_translator(KLVItem *item, KLVStorage *storage) {
    bool res = int32_deg_to_double_translator_reserved(item, -180, 180, 0, "N/A (Off-Earth)");
    if (res && G_VALUE_HOLDS_DOUBLE(&item->value_parsed)) {
        storage->set(24, &item->value_parsed);
    }
    return res;
    // return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "N/A (Off-Earth)");
}

bool KLV::frame_center_elevation_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::offset_corner_latitude_point_1_translator(KLVItem *item, KLVStorage *storage) {
    GValue item_23 = G_VALUE_INIT;
    if (!storage->get(23, &item_23)) {
        return false;
    }
    if (!G_VALUE_HOLDS_DOUBLE(&item_23)) {
        return false;
    }
    gdouble item_23_val = g_value_get_double(&item_23);
    return int16_deg_to_float_translator_reserved(item, -0.075, 0.075, item_23_val, "N/A (Off-Earth)");
}

bool KLV::offset_corner_longitude_point_1_translator(KLVItem *item, KLVStorage *storage) {
    GValue item_24 = G_VALUE_INIT;
    if (!storage->get(24, &item_24)) {
        return false;
    }
    if (!G_VALUE_HOLDS_DOUBLE(&item_24)) {
        return false;
    }
    gdouble item_24_val = g_value_get_double(&item_24);
    return int16_deg_to_float_translator_reserved(item, -0.075, 0.075, item_24_val, "N/A (Off-Earth)");
}

bool KLV::offset_corner_latitude_point_2_translator(KLVItem *item, KLVStorage *storage) {
    GValue item_23 = G_VALUE_INIT;
    if (!storage->get(23, &item_23)) {
        return false;
    }
    if (!G_VALUE_HOLDS_DOUBLE(&item_23)) {
        return false;
    }
    gdouble item_23_val = g_value_get_double(&item_23);
    return int16_deg_to_float_translator_reserved(item, -0.075, 0.075, item_23_val, "N/A (Off-Earth)");
}

bool KLV::offset_corner_longitude_point_2_translator(KLVItem *item, KLVStorage *storage) {
    GValue item_24 = G_VALUE_INIT;
    if (!storage->get(24, &item_24)) {
        return false;
    }
    if (!G_VALUE_HOLDS_DOUBLE(&item_24)) {
        return false;
    }
    gdouble item_24_val = g_value_get_double(&item_24);
    return int16_deg_to_float_translator_reserved(item, -0.075, 0.075, item_24_val, "N/A (Off-Earth)");
}

bool KLV::offset_corner_latitude_point_3_translator(KLVItem *item, KLVStorage *storage) {
    GValue item_23 = G_VALUE_INIT;
    if (!storage->get(23, &item_23)) {
        return false;
    }
    if (!G_VALUE_HOLDS_DOUBLE(&item_23)) {
        return false;
    }
    gdouble item_23_val = g_value_get_double(&item_23);
    return int16_deg_to_float_translator_reserved(item, -0.075, 0.075, item_23_val, "N/A (Off-Earth)");
}

bool KLV::offset_corner_longitude_point_3_translator(KLVItem *item, KLVStorage *storage) {
    GValue item_24 = G_VALUE_INIT;
    if (!storage->get(24, &item_24)) {
        return false;
    }
    if (!G_VALUE_HOLDS_DOUBLE(&item_24)) {
        return false;
    }
    gdouble item_24_val = g_value_get_double(&item_24);
    return int16_deg_to_float_translator_reserved(item, -0.075, 0.075, item_24_val, "N/A (Off-Earth)");
}

bool KLV::offset_corner_latitude_point_4_translator(KLVItem *item, KLVStorage *storage) {
    GValue item_23 = G_VALUE_INIT;
    if (!storage->get(23, &item_23)) {
        return false;
    }
    if (!G_VALUE_HOLDS_DOUBLE(&item_23)) {
        return false;
    }
    gdouble item_23_val = g_value_get_double(&item_23);
    return int16_deg_to_float_translator_reserved(item, -0.075, 0.075, item_23_val, "N/A (Off-Earth)");
}

bool KLV::offset_corner_longitude_point_4_translator(KLVItem *item, KLVStorage *storage) {
    GValue item_24 = G_VALUE_INIT;
    if (!storage->get(24, &item_24)) {
        return false;
    }
    if (!G_VALUE_HOLDS_DOUBLE(&item_24)) {
        return false;
    }
    gdouble item_24_val = g_value_get_double(&item_24);
    return int16_deg_to_float_translator_reserved(item, -0.075, 0.075, item_24_val, "N/A (Off-Earth)");
}

bool KLV::icing_detected_translator(KLVItem *item, KLVStorage *storage) {
    guint8 value;
    if (!get_uint8_from_klv_value(item, &value)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    bool res = true;
    switch (value) {
        case 0:
            g_value_set_string(&item->value_parsed, "Detector off");
            break;
        case 1:
            g_value_set_string(&item->value_parsed, "No icing Detected");
            break;
        case 2:
            g_value_set_string(&item->value_parsed, "Icing Detected");
            break;

        default:
            res = false;
            break;
    }
    return res;
}

bool KLV::wind_direction_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 360);
}

bool KLV::wind_speed_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_deg_to_float_translator(item, 0, 100);
}

bool KLV::static_pressure_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 5000);
}

bool KLV::density_altitude_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::outside_air_temperature_translator(KLVItem *item, KLVStorage *storage) {
    gint8 value;
    if (!get_int8_from_klv_value(item, &value)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_INT);
    g_value_set_int(&item->value_parsed, value);
    return true;
}

bool KLV::target_location_latitude_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "N/A (Off-Earth)");
}

bool KLV::target_location_longitude_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "N/A (Off-Earth)");
}

bool KLV::target_location_elevation_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::target_track_gate_width_translator(KLVItem *item, KLVStorage *storage) {
    guint8 res = 0;
    if (!get_uint8_from_klv_value(item, &res)) {
        return false;
    }
    guint16 doubled = (guint16)res * 2;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_UINT);
    g_value_set_uint(&item->value_parsed, doubled);
    return true;
}

bool KLV::target_track_gate_height_translator(KLVItem *item, KLVStorage *storage) {
    guint8 res = 0;
    if (!get_uint8_from_klv_value(item, &res)) {
        return false;
    }
    guint16 doubled = (guint16)res * 2;
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_UINT);
    g_value_set_uint(&item->value_parsed, doubled);
    return true;
}

bool KLV::target_error_estimate_ce90_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 4095);
}

bool KLV::target_error_estimate_le90_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 4095);
}

bool KLV::generic_flag_data_translator(KLVItem *item, KLVStorage *storage) {
    gint8 value;
    if (!get_int8_from_klv_value(item, &value)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_INT);
    g_value_set_int(&item->value_parsed, value);
    return true;
}

bool KLV::security_local_set_translator(KLVItem *item, KLVStorage *storage) {
    return general_klv_translator(item);
}

bool KLV::differential_pressure_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 5000);
}

bool KLV::platform_angle_of_attack_translator(KLVItem *item, KLVStorage *storage) {
    return int16_deg_to_float_translator_reserved(item, -20, 20, 0, "Out of Range");
}

bool KLV::platform_vertical_speed_translator(KLVItem *item, KLVStorage *storage) {
    return int16_deg_to_float_translator_reserved(item, -180, 180, 0, "Out of Range");
}

bool KLV::platform_sideslip_angle_translator(KLVItem *item, KLVStorage *storage) {
    return int16_deg_to_float_translator_reserved(item, -20, 20, 0, "Out of Range");
}

bool KLV::airfield_barometric_pressure_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 5000);
}

bool KLV::airfield_elevation_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::relative_humidity_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_deg_to_float_translator(item, 0, 100);
}

bool KLV::platform_ground_speed_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_translator(item);
}

bool KLV::ground_range_translator(KLVItem *item, KLVStorage *storage) {
    return uint32_deg_to_double_translator(item, 0, 5e6);  // 5,000,000
}

bool KLV::platform_fuel_remaining_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 1e4);  // 10,000
}

bool KLV::platform_call_sign_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::weapon_load_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_translator(item);
}

bool KLV::weapon_fired_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_translator(item);
}

bool KLV::laser_prf_code_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_translator(item);
}

bool KLV::sensor_field_of_view_name_translator(KLVItem *item, KLVStorage *storage) {
    guint8 value = 0;
    if (!get_uint8_from_klv_value(item, &value)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    switch (value) {
        case 0:
            g_value_set_string(&item->value_parsed, "Ultranarrow");
            break;
        case 1:
            g_value_set_string(&item->value_parsed, "Narrow");
            break;
        case 2:
            g_value_set_string(&item->value_parsed, "Medium");
            break;
        case 3:
            g_value_set_string(&item->value_parsed, "Wide");
            break;
        case 4:
            g_value_set_string(&item->value_parsed, "Ultrawide");
            break;
        case 5:
            g_value_set_string(&item->value_parsed, "Narrow Medium");
            break;
        case 6:
            g_value_set_string(&item->value_parsed, "2x Ultranarrow");
            break;
        case 7:
            g_value_set_string(&item->value_parsed, "4x Ultranarrow");
            break;
        case 8:
            g_value_set_string(&item->value_parsed, "Continuous Zoom");
            break;
        default:
            g_value_set_string(&item->value_parsed, "Reserved - Do not use");
            break;
    }
    return true;
}

bool KLV::platform_magnetic_heading_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 360);
}

bool KLV::uas_datalink_ls_version_number_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_translator(item);
}

bool KLV::deprecated_translator(KLVItem *item, KLVStorage *storage) {
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    g_value_set_string(&item->value_parsed, "Deprecated");
    return true;
}

bool KLV::target_location_covariance_matrix_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "Reserved");
}

bool KLV::alternate_platform_latitude_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "Reserved");
}

bool KLV::alternate_platform_longitude_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "Reserved");
}

bool KLV::alternate_platform_altitude_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::alternate_platform_name_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::alternate_platform_heading_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, 0, 360);
}

bool KLV::event_start_time_utc_translator(KLVItem *item, KLVStorage *storage) {
    return timestamp_translator(item);
}

bool KLV::rvt_local_set_translator(KLVItem *item, KLVStorage *storage) {
    return general_klv_translator(item);
}

bool KLV::vmti_local_set_translator(KLVItem *item, KLVStorage *storage) {
    return general_klv_translator(item);
}

bool KLV::sensor_ellipsoid_height_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::alternate_platform_ellipsoid_height_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::operational_mode_translator(KLVItem *item, KLVStorage *storage) {
    guint8 value = 0;
    if (!get_uint8_from_klv_value(item, &value)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    switch (value) {
        case 0:
            g_value_set_string(&item->value_parsed, "Other");
            break;
        case 1:
            g_value_set_string(&item->value_parsed, "Operational");
            break;
        case 2:
            g_value_set_string(&item->value_parsed, "Training");
            break;
        case 3:
            g_value_set_string(&item->value_parsed, "Exercise");
            break;
        case 4:
            g_value_set_string(&item->value_parsed, "Maintenance");
            break;
        case 5:
            g_value_set_string(&item->value_parsed, "Test");
            break;
        default:
            g_value_set_string(&item->value_parsed, "Reserved - Do not use");
            break;
    }
    return true;
}

bool KLV::frame_center_height_above_ellipsoid_translator(KLVItem *item, KLVStorage *storage) {
    return uint16_deg_to_float_translator(item, -900, 19000, -900);
}

bool KLV::sensor_north_velocity_translator(KLVItem *item, KLVStorage *storage) {
    return int16_deg_to_float_translator_reserved(item, -327, 327, 0, "Out of Range");
}

bool KLV::sensor_east_velocity_translator(KLVItem *item, KLVStorage *storage) {
    return int16_deg_to_float_translator_reserved(item, -327, 327, 0, "Out of Range");
}

bool KLV::image_horizon_pixel_pack_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    GList *list = g_list_alloc();
    GVariantBuilder builder = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE("a(dd)"));
    {  // first (x,y)(x,y) each uint8, representing percentage 0->100
        guint8 x = 0;
        guint8 y = 0;
        for (guint8 i = 0; i < 2; i++) {
            if (!gst_byte_reader_get_uint8(&reader, &x)) {
                break;
            }
            if (!gst_byte_reader_get_uint8(&reader, &y)) {
                return false;
            }
            g_variant_builder_add(&builder, "(dd)", uint8_deg_to_float(x, 0, 100), uint8_deg_to_float(y, 0, 100));
        }
    }
    {  // second (x,y)(x,y) are optional each int32 converted to angle;
        // TODO: add reserved, ie out of bounds to int32_deg_to_double;
        gint32 x = 0;
        gint32 y = 0;
        for (guint8 i = 0; i < 2; i++) {
            if (!gst_byte_reader_get_int32_be(&reader, &x)) {
                break;
            }
            if (!gst_byte_reader_get_int32_be(&reader, &y)) {
                return false;
            }
            g_variant_builder_add(&builder, "(dd)", int32_deg_to_double(x, -90, 90), int32_deg_to_double(x, -180, 180));
        }
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_VARIANT);
    g_value_set_variant(&item->value_parsed, g_variant_new("a(dd)", &builder));
    return true;
}

bool KLV::corner_latitude_point_1_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "N/A (Off-Earth)");
}

bool KLV::corner_longitude_point_1_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "N/A (Off-Earth)");
}

bool KLV::corner_latitude_point_2_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "N/A (Off-Earth)");
}

bool KLV::corner_longitude_point_2_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "N/A (Off-Earth)");
}

bool KLV::corner_latitude_point_3_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "N/A (Off-Earth)");
}

bool KLV::corner_longitude_point_3_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "N/A (Off-Earth)");
}

bool KLV::corner_latitude_point_4_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "N/A (Off-Earth)");
}

bool KLV::corner_longitude_point_4_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "N/A (Off-Earth)");
}

bool KLV::platform_pitch_angle_full_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "Out of Range");
}

bool KLV::platform_roll_angle_full_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "Out of Range");
}

bool KLV::platform_angle_of_attack_full_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -90, 90, 0, "Out of Range");
}

bool KLV::platform_sideslip_angle_full_translator(KLVItem *item, KLVStorage *storage) {
    return int32_deg_to_double_translator_reserved(item, -180, 180, 0, "Out of Range");
}

bool KLV::miis_core_identifier_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: does not parse, only sets; need to view MISB ST 1204
    // GstByteReader reader;
    // if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
    //     return false;
    // }
    // std::stringstream s;
    // guint8 val;
    // for (guint i = 0; i < 16; i++) {
    //     if (!gst_byte_reader_get_uint8(&reader, val)) {
    //         break;
    //     }
    // }
    // s << std::setfill('0') << std::setw(2) << key << ": ";
    // std::string str = s.str();
    // g_value_unset(&item->value_parsed);
    // g_value_init(&item->value_parsed, G_TYPE_STRING);
    // g_value_set_string(&item->value_parsed, str.c_str());
    g_value_set_boxed(&item->value_parsed, item->value);
    return true;
}

bool KLV::sar_motion_imagery_local_set_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: does not parse, only sets; need to view MISB ST 1206
    g_value_set_boxed(&item->value_parsed, item->value);
    return true;
}

bool KLV::target_width_extended_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_double_translator(item, 0, 15e5);  // 1,500,000
}

bool KLV::range_image_local_set_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: does not parse, only sets; need to view MISB ST 1002
    g_value_set_boxed(&item->value_parsed, item->value);
    return true;
}

bool KLV::geo_registration_local_set_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: does not parse, only sets; need to view MISB ST 1601 [19]
    g_value_set_boxed(&item->value_parsed, item->value);
    return true;
}

bool KLV::composite_imaging_local_set_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: does not parse, only sets; need to view MISB ST 1602 [20]
    g_value_set_boxed(&item->value_parsed, item->value);
    return true;
}

bool KLV::segment_local_set_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: does not parse, only sets; need to view MISB ST 1607 [11]
    g_value_set_boxed(&item->value_parsed, item->value);
    return true;
}

bool KLV::amend_local_set_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: does not parse, only sets; need to view MISB ST 1607 [11]
    g_value_set_boxed(&item->value_parsed, item->value);
    return true;
}

bool KLV::sdcc_flp_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: does not parse, only sets; need to view MISB ST 1010 [9]
    g_value_set_boxed(&item->value_parsed, item->value);
    return true;
}

bool KLV::density_altitude_extended_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_double_translator(item, -900, 4e4, -900);  // 40,000
}

bool KLV::sensor_ellipsoid_height_extended_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_double_translator(item, -900, 4e4, -900);  // 40,000
}

bool KLV::alternate_platform_ellipsoid_height_extended_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_double_translator(item, -900, 4e4, -900);  // 40,000
}

bool KLV::stream_designator_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::operational_base_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::broadcast_source_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::range_to_recovery_location_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_float_translator(item, 0, 21000);
}

bool KLV::time_airborne_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint32 res;
    if (gst_byte_reader_get_variable_uint32_be(&reader, &res)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_INT);
    g_value_set_int(&item->value_parsed, res);
    return true;
}

bool KLV::propulsion_unit_speed_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint32 res;
    if (gst_byte_reader_get_variable_uint32_be(&reader, &res)) {
        return false;
    }
    g_value_init(&item->value_parsed, G_TYPE_INT);
    g_value_set_int(&item->value_parsed, res);
    return true;
}

bool KLV::platform_course_angle_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_double_translator(item, 0, 360);
}

bool KLV::altitude_agl_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: check issue with docs - wants float64, but max length is 4;
    return variable_deg_to_double_translator(item, -900, 4e4, -900);     // 40,000
}

bool KLV::radar_altimeter_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: check issue with docs - wants float64, but max length is 4;
    return variable_deg_to_double_translator(item, -900, 4e4, -900);        // 40,000
}

bool KLV::control_command_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint64 command_id = 0;
    if (!gst_byte_reader_get_ber_oid(&reader, &command_id)) {
        return false;
    }
    guint64 string_length = 0;
    if (!gst_byte_reader_get_ber(&reader, &string_length)) {
        return false;
    }
    const guint8 *string_data;
    if (!gst_byte_reader_get_data(&reader, string_length, &string_data)) {
        return false;
    }
    GString *str = g_string_new_len((const gchar *)string_data, string_length);
    guint64 command_time;
    if (!gst_byte_reader_get_uint64_be(&reader, &command_time)) {
        return false;
    }
    std::stringstream s;
    s << command_id << ", " << str->str << ", " << timestamp_from_uint64(command_time);
    g_string_free(str, true);
    std::string sstr = s.str();

    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    g_value_set_string(&item->value_parsed, sstr.c_str());
    return true;
}

bool KLV::control_command_verification_list_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    GVariantBuilder builder = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE("at"));
    guint64 command_id = 0;
    for (guint8 i = 0;; i++) {
        if (!gst_byte_reader_get_ber_oid(&reader, &command_id)) {
            break;
        }
        g_variant_builder_add(&builder, "t", command_id);
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_VARIANT);
    g_value_set_variant(&item->value_parsed, g_variant_new("at", &builder));
    return true;
}

bool KLV::sensor_azimuth_rate_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_float_translator(item, -1000, 1000, -1000);
}

bool KLV::sensor_elevation_rate_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_float_translator(item, -1000, 1000, -1000);
}

bool KLV::sensor_roll_rate_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_float_translator(item, -1000, 1000, -1000);
}

bool KLV::onboard_mi_storage_percent_full_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_float_translator(item, 0, 100);
}

bool KLV::active_wavelength_list_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    GVariantBuilder builder = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE("at"));
    guint64 wavelength_id = 0;
    for (guint8 i = 0;; i++) {
        if (!gst_byte_reader_get_ber_oid(&reader, &wavelength_id)) {
            break;
        }
        g_variant_builder_add(&builder, "t", wavelength_id);
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_VARIANT);
    g_value_set_variant(&item->value_parsed, g_variant_new("at", &builder));
    return true;
}

bool KLV::country_codes_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint64 len = 0;
    guint8 codeing_method = 0;
    const guint8 *data = NULL;
    GString *overflight_country = NULL;
    GString *operator_country = NULL;
    GString *country_of_manufacture = NULL;

    if (!gst_byte_reader_get_ber_oid(&reader, &len)) {
        return false;
    }
    if (len != 1) {
        return false;
    }
    if (!gst_byte_reader_get_uint8(&reader, &codeing_method)) {
        return false;
    }
    if (!gst_byte_reader_get_ber_oid(&reader, &len)) {
        return false;
    }
    if (!gst_byte_reader_get_data(&reader, len, &data)) {
        return false;
    }

    overflight_country = g_string_new_len((const gchar *)data, len);
    if (gst_byte_reader_get_ber_oid(&reader, &len)) {
        if (len > 0) {
            if (!gst_byte_reader_get_data(&reader, len, &data)) {
                if (overflight_country != NULL) g_string_free(overflight_country, true);
                return false;
            }
            operator_country = g_string_new_len((const gchar *)data, len);
        }
        if (gst_byte_reader_get_ber_oid(&reader, &len)) {
            if (len > 0) {
                if (!gst_byte_reader_get_data(&reader, len, &data)) {
                    if (overflight_country != NULL) g_string_free(overflight_country, true);
                    if (operator_country != NULL) g_string_free(operator_country, true);
                    return false;
                }
                country_of_manufacture = g_string_new_len((const gchar *)data, len);
            }
        }
    }

    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_VARIANT);
    g_value_set_variant(&item->value_parsed, g_variant_new("(csmsms)", codeing_method, overflight_country));  // (c, s, s?, s?);
    if (overflight_country != NULL) g_string_free(overflight_country, true);
    if (operator_country != NULL) g_string_free(operator_country, true);
    if (country_of_manufacture != NULL) g_string_free(country_of_manufacture, true);
    return true;
}

bool KLV::number_of_navsats_in_view_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_translator(item);
}

bool KLV::positioning_method_source_translator(KLVItem *item, KLVStorage *storage) {
    return uint8_translator(item);  // #TODO: this includes flags, might be able to do a dictionary / list
}

bool KLV::platform_status_translator(KLVItem *item, KLVStorage *storage) {
    guint8 value;
    if (!get_uint8_from_klv_value(item, &value)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    switch (value) {
        case 0:
            g_value_set_string(&item->value_parsed, "Active");
            break;
        case 1:
            g_value_set_string(&item->value_parsed, "Pre-flight");
            break;
        case 2:
            g_value_set_string(&item->value_parsed, "Pre-flight-taxiing");
            break;
        case 3:
            g_value_set_string(&item->value_parsed, "Run-up");
            break;
        case 4:
            g_value_set_string(&item->value_parsed, "Take-off");
            break;
        case 5:
            g_value_set_string(&item->value_parsed, "Ingress");
            break;
        case 6:
            g_value_set_string(&item->value_parsed, "Manual operation");
            break;
        case 7:
            g_value_set_string(&item->value_parsed, "Automated-orbit");
            break;
        case 8:
            g_value_set_string(&item->value_parsed, "Transitioning");
            break;
        case 9:
            g_value_set_string(&item->value_parsed, "Egress");
            break;
        case 10:
            g_value_set_string(&item->value_parsed, "Landing");
            break;
        case 11:
            g_value_set_string(&item->value_parsed, "Landed-taxiing");
            break;
        case 12:
            g_value_set_string(&item->value_parsed, "Landed-Parked");
            break;
        default:
            g_value_set_string(&item->value_parsed, "Reserved");
            break;
    }
    return true;
}

bool KLV::sensor_control_mode_translator(KLVItem *item, KLVStorage *storage) {
    guint8 value;
    if (!get_uint8_from_klv_value(item, &value)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_STRING);
    switch (value) {
        case 0:
            g_value_set_string(&item->value_parsed, "Off");
            break;
        case 1:
            g_value_set_string(&item->value_parsed, "Home Position");
            break;
        case 2:
            g_value_set_string(&item->value_parsed, "Uncontrolled");
            break;
        case 3:
            g_value_set_string(&item->value_parsed, "Manual Control");
            break;
        case 4:
            g_value_set_string(&item->value_parsed, "Calibrating");
            break;
        case 5:
            g_value_set_string(&item->value_parsed, "Auto - Holding Position");
            break;
        case 6:
            g_value_set_string(&item->value_parsed, "Auto - Tracking");
            break;
        default:
            g_value_set_string(&item->value_parsed, "Reserved");
            break;
    }
    return true;
}

bool KLV::sensor_frame_rate_pack_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint64 numerator = 0;
    guint64 denomerator = 0;
    if (!gst_byte_reader_get_ber_oid(&reader, &numerator)) {
        return false;
    }
    if (!gst_byte_reader_get_ber_oid(&reader, &denomerator)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_VARIANT);
    g_value_set_variant(&item->value_parsed, g_variant_new("(tt)", numerator, denomerator));  // (c, s, s?, s?);
    return true;
}

bool KLV::wavelengths_list_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: not implemented
    return false;
}

bool KLV::target_id_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::airbase_locations_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: not implemented
    return false;
}

bool KLV::takeoff_time_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: docs say variable length, assuming length is not variable;
    return timestamp_translator(item);
}

bool KLV::transmission_frequency_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_float_translator(item, 1, 99999, 1);
}

bool KLV::onboard_mi_storage_capacity_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    guint32 res;
    if (gst_byte_reader_get_variable_uint32_be(&reader, &res)) {
        return false;
    }
    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_UINT);
    g_value_set_uint(&item->value_parsed, res);
    return true;
}

bool KLV::zoom_percentage_translator(KLVItem *item, KLVStorage *storage) {
    return variable_deg_to_float_translator(item, 0, 100);
}

bool KLV::communications_method_translator(KLVItem *item, KLVStorage *storage) {
    return string_translator(item);
}

bool KLV::leap_seconds_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: could cause issues due to unknown encoding when variable and negative
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    gint32 res = 0;
    if (!gst_byte_reader_get_variable_int32_be(&reader, &res)) {
        return false;
    }

    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_INT);
    g_value_set_int(&item->value_parsed, res);
    return true;
}

bool KLV::correction_offset_translator(KLVItem *item, KLVStorage *storage) {
    GstByteReader reader;
    if (!gst_byte_reader_from_g_bytes(&reader, item->value)) {
        return false;
    }
    gint64 res = 0;
    if (!gst_byte_reader_get_variable_int64_be(&reader, &res)) {
        return false;
    }

    g_value_unset(&item->value_parsed);
    g_value_init(&item->value_parsed, G_TYPE_INT64);
    g_value_set_int64(&item->value_parsed, res);
    return true;
}

bool KLV::payload_list_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: not implemented
    return false;
}

bool KLV::active_payloads_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: not implemented
    return false;
}

bool KLV::weapons_stores_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: not implemented
    return false;
}

bool KLV::waypoint_list_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: not implemented
    return false;
}

bool KLV::view_domain_translator(KLVItem *item, KLVStorage *storage) {  // #TODO: not implemented
    return false;
}
