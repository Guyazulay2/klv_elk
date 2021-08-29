#include <KLVParser.h>

using namespace KLV;

std::unordered_map<guint64, std::string> KLVParser::klv_tags;
std::unordered_map<guint64, std::function<bool(KLVItem *, KLVStorage *)>> KLVParser::klv_translators;

KLVParser::KLVParser() {
    init_klv_tags();
    init_klv_translators();
}

KLVParser::~KLVParser() {
}

bool KLVParser::parse(GBytes *bytes) {
    gsize size;
    const guint8 *data = (const guint8 *)g_bytes_get_data(bytes, &size);
    return parse(data, size);
}

bool KLVParser::parse(const guint8 *data, guint size) {
    GstByteReader reader;
    gst_byte_reader_init(&reader, data, size);
    return parse(&reader);
}

bool KLVParser::verify_checksum(GstByteReader *reader) {
    bool checksum_is_valid = false;
    guint16 calculated = 0;
    guint16 actual = 0;
    if (!_verify_checksum(reader, &checksum_is_valid, &calculated, &actual)) {
        if (verbose) {
            std::cout << "[WARN] failed to calculate checksum, ignoring packet" << std::endl;
        }
        return false;
    }
    if (!checksum_is_valid) {
        if (verbose) {
            std::cout << "[WARN] checksum is invalid, calculated: 0x" << std::hex << std::setfill('0') << std::setw(4) << calculated << ", actual: 0x" << std::hex << std::setfill('0') << std::setw(4) << actual << std::endl;
        }
        return false;
    }
    return true;
}

bool KLVParser::parse(GstByteReader *reader) {
    try {
        this->klv = std::make_shared<KLVItem>();
        if (strict) {
            if (!verify_checksum(reader)) {
                return false;
            }
        }
        if (!parse_klv(reader, klv.get(), KEY_FORMATS::_16, LENGTH_FORMATS::_BER)) {
            return false;
        }
        if (!translate_stanag(this->klv.get())) {
            return false;
        }
        return true;
    } catch (std::exception &e) {
        if (verbose) {
            std::cout << "[ERROR] " << e.what() << std::endl;
        }
        return false;
    }
    return false;
}

bool KLVParser::_verify_checksum(GstByteReader *reader, bool *is_valid, guint16 *calculated, guint16 *actual) {
    guint16 checksum_calculated = 0;
    guint pos = gst_byte_reader_get_pos(reader);
    guint8 curr = 0;
    for (guint8 i = 0; gst_byte_reader_get_remaining(reader) > 2; i++) {
        if (!gst_byte_reader_get_uint8(reader, &curr)) {
            gst_byte_reader_set_pos(reader, pos);
            return false;
        }
        checksum_calculated += (curr << (8 * ((i + 1) % 2)));
    }
    guint16 checksum_actual = 0;
    if (!gst_byte_reader_get_uint16_be(reader, &checksum_actual)) {
        return false;
    }
    // std::cout << std::bitset<16>{checksum_calculated} << ", " << std::bitset<16>{checksum_actual} << std::endl;
    gst_byte_reader_set_pos(reader, pos);
    if (calculated != NULL) {
        *calculated = checksum_calculated;
    }
    if (actual != NULL) {
        *actual = checksum_actual;
    }
    if (is_valid != NULL) {
        *is_valid = checksum_calculated == checksum_actual;
    }
    return true;
}

// GBytes *KLVParser::encode() {
//     return NULL;
// }

json KLVParser::to_json() {
    if (this->klv == NULL) return nullptr;
    return this->klv->to_json();
}

bool KLVParser::assign_key_content(KLVItem *item) {
    guint64 value;
    if (!item->get_key_content(&value)) {
        return false;
    }
    auto pair = this->klv_tags.find(value);
    if (pair != this->klv_tags.end()) {
        item->key_parsed = pair->second;
        return true;
    }
    return false;
}

bool KLVParser::assign_value_content(KLVItem *item) {
    guint64 value;
    if (!item->get_key_content(&value)) {
        return false;
    }
    auto pair = this->klv_translators.find(value);
    if (pair != this->klv_translators.end()) {
        return pair->second(item, &this->storage);
    }
    return false;
}

bool KLVParser::assign_key_and_value_content(KLVItem *item) {
    guint64 value;
    if (!item->get_key_content(&value)) {
        return false;
    }
    bool done = true;
    auto pair_key = this->klv_tags.find(value);
    if (pair_key != this->klv_tags.end()) {
        item->key_parsed = pair_key->second;
    } else {
        done = false;
    }
    auto pair_value = this->klv_translators.find(value);
    if (pair_value != this->klv_translators.end()) {
        done &= pair_value->second(item, &this->storage);
    } else {
        done = false;
    }
    return done;
}

bool KLVParser::translate_stanag(KLVItem *item) {
    assign_key_content(item);
    if (verbose) {
        std::cout << "[INFO] " << klv->debug() << std::endl;
    }
    GstByteReader sub_reader;
    if (!gst_byte_reader_from_g_bytes(&sub_reader, this->klv->value)) {
        if (verbose) {
            std::cout << "[ERROR] failed to create byte reader for children" << std::endl;
        }
        return false;
    }
    KLVItem *curr = item;
    int total = 0;
    for (guint8 i = 0; gst_byte_reader_get_remaining(&sub_reader) > 0; i++) {
        std::shared_ptr<KLVItem> next = std::make_shared<KLVItem>();
        if (!parse_klv(&sub_reader, next.get(), KEY_FORMATS::_BER_OID, LENGTH_FORMATS::_BER)) {
            if (verbose) {
                std::cout << "[WARN] failed to parse child #" << i << std::endl;
            }
            break;
        }
        assign_key_and_value_content(next.get());
        if (verbose) {
            std::cout << "[INFO] " << next->debug() << std::endl;
        }
        if (i == 0) {
            curr->child = next;
            next->parent = curr;
        } else {
            curr->next = next;
            next->previous = curr;
        }
        curr = next.get();
        total++;
    }
    if (verbose) {
        std::cout << "parsed klv: " << klv->key << ", children: " << total << std::endl;
    }
    return true;
}

void KLVParser::init_klv_translators() {
    if (klv_translators.size() > 0) return;

    // 0x01 - 1 - checksum
    klv_translators.insert(std::make_pair(0x01, KLV::checksum_translator));
    // 0x02 - 2 - precision_time_stamp
    klv_translators.insert(std::make_pair(0x02, KLV::precision_time_stamp_translator));
    // 0x03 - 3 - mission_id
    klv_translators.insert(std::make_pair(0x03, KLV::mission_id_translator));
    // 0x04 - 4 - platform_tail_number
    klv_translators.insert(std::make_pair(0x04, KLV::platform_tail_number_translator));
    // 0x05 - 5 - platform_heading_angle
    klv_translators.insert(std::make_pair(0x05, KLV::platform_heading_angle_translator));
    // 0x06 - 6 - platform_pitch_angle
    klv_translators.insert(std::make_pair(0x06, KLV::platform_pitch_angle_translator));
    // 0x07 - 7 - platform_roll_angle
    klv_translators.insert(std::make_pair(0x07, KLV::platform_roll_angle_translator));
    // 0x08 - 8 - platform_true_airspeed
    klv_translators.insert(std::make_pair(0x08, KLV::platform_true_airspeed_translator));
    // 0x09 - 9 - platform_indicated_airspeed
    klv_translators.insert(std::make_pair(0x09, KLV::platform_indicated_airspeed_translator));
    // 0x0A - 10 - platform_designation
    klv_translators.insert(std::make_pair(0x0A, KLV::platform_designation_translator));
    // 0x0B - 11 - image_source_sensor
    klv_translators.insert(std::make_pair(0x0B, KLV::image_source_sensor_translator));
    // 0x0C - 12 - image_coordinate_system
    klv_translators.insert(std::make_pair(0x0C, KLV::image_coordinate_system_translator));
    // 0x0D - 13 - sensor_latitude
    klv_translators.insert(std::make_pair(0x0D, KLV::sensor_latitude_translator));
    // 0x0E - 14 - sensor_longitude
    klv_translators.insert(std::make_pair(0x0E, KLV::sensor_longitude_translator));
    // 0x0F - 15 - sensor_true_altitude
    klv_translators.insert(std::make_pair(0x0F, KLV::sensor_true_altitude_translator));
    // 0x10 - 16 - sensor_horizontal_field_of_view
    klv_translators.insert(std::make_pair(0x10, KLV::sensor_horizontal_field_of_view_translator));
    // 0x11 - 17 - sensor_vertical_field_of_view
    klv_translators.insert(std::make_pair(0x11, KLV::sensor_vertical_field_of_view_translator));
    // 0x12 - 18 - sensor_relative_azimuth_angle
    klv_translators.insert(std::make_pair(0x12, KLV::sensor_relative_azimuth_angle_translator));
    // 0x13 - 19 - sensor_relative_elevation_angle
    klv_translators.insert(std::make_pair(0x13, KLV::sensor_relative_elevation_angle_translator));
    // 0x14 - 20 - sensor_relative_roll_angle
    klv_translators.insert(std::make_pair(0x14, KLV::sensor_relative_roll_angle_translator));
    // 0x15 - 21 - slant_range
    klv_translators.insert(std::make_pair(0x15, KLV::slant_range_translator));
    // 0x16 - 22 - target_width
    klv_translators.insert(std::make_pair(0x16, KLV::target_width_translator));
    // 0x17 - 23 - frame_center_latitude
    klv_translators.insert(std::make_pair(0x17, KLV::frame_center_latitude_translator));
    // 0x18 - 24 - frame_center_longitude
    klv_translators.insert(std::make_pair(0x18, KLV::frame_center_longitude_translator));
    // 0x19 - 25 - frame_center_elevation
    klv_translators.insert(std::make_pair(0x19, KLV::frame_center_elevation_translator));
    // 0x1A - 26 - offset_corner_latitude_point_1
    klv_translators.insert(std::make_pair(0x1A, KLV::offset_corner_latitude_point_1_translator));
    // 0x1B - 27 - offset_corner_longitude_point_1
    klv_translators.insert(std::make_pair(0x1B, KLV::offset_corner_longitude_point_1_translator));
    // 0x1C - 28 - offset_corner_latitude_point_2
    klv_translators.insert(std::make_pair(0x1C, KLV::offset_corner_latitude_point_2_translator));
    // 0x1D - 29 - offset_corner_longitude_point_2
    klv_translators.insert(std::make_pair(0x1D, KLV::offset_corner_longitude_point_2_translator));
    // 0x1E - 30 - offset_corner_latitude_point_3
    klv_translators.insert(std::make_pair(0x1E, KLV::offset_corner_latitude_point_3_translator));
    // 0x1F - 31 - offset_corner_longitude_point_3
    klv_translators.insert(std::make_pair(0x1F, KLV::offset_corner_longitude_point_3_translator));
    // 0x20 - 32 - offset_corner_latitude_point_4
    klv_translators.insert(std::make_pair(0x20, KLV::offset_corner_latitude_point_4_translator));
    // 0x21 - 33 - offset_corner_longitude_point_4
    klv_translators.insert(std::make_pair(0x21, KLV::offset_corner_longitude_point_4_translator));
    // 0x22 - 34 - icing_detected
    klv_translators.insert(std::make_pair(0x22, KLV::icing_detected_translator));
    // 0x23 - 35 - wind_direction
    klv_translators.insert(std::make_pair(0x23, KLV::wind_direction_translator));
    // 0x24 - 36 - wind_speed
    klv_translators.insert(std::make_pair(0x24, KLV::wind_speed_translator));
    // 0x25 - 37 - static_pressure
    klv_translators.insert(std::make_pair(0x25, KLV::static_pressure_translator));
    // 0x26 - 38 - density_altitude
    klv_translators.insert(std::make_pair(0x26, KLV::density_altitude_translator));
    // 0x27 - 39 - outside_air_temperature
    klv_translators.insert(std::make_pair(0x27, KLV::outside_air_temperature_translator));
    // 0x28 - 40 - target_location_latitude
    klv_translators.insert(std::make_pair(0x28, KLV::target_location_latitude_translator));
    // 0x29 - 41 - target_location_longitude
    klv_translators.insert(std::make_pair(0x29, KLV::target_location_longitude_translator));
    // 0x2A - 42 - target_location_elevation
    klv_translators.insert(std::make_pair(0x2A, KLV::target_location_elevation_translator));
    // 0x2B - 43 - target_track_gate_width
    klv_translators.insert(std::make_pair(0x2B, KLV::target_track_gate_width_translator));
    // 0x2C - 44 - target_track_gate_height
    klv_translators.insert(std::make_pair(0x2C, KLV::target_track_gate_height_translator));
    // 0x2D - 45 - target_error_estimate_ce90
    klv_translators.insert(std::make_pair(0x2D, KLV::target_error_estimate_ce90_translator));
    // 0x2E - 46 - target_error_estimate_le90
    klv_translators.insert(std::make_pair(0x2E, KLV::target_error_estimate_le90_translator));
    // 0x2F - 47 - generic_flag_data
    klv_translators.insert(std::make_pair(0x2F, KLV::generic_flag_data_translator));
    // 0x30 - 48 - security_local_set
    klv_translators.insert(std::make_pair(0x30, KLV::security_local_set_translator));
    // 0x31 - 49 - differential_pressure
    klv_translators.insert(std::make_pair(0x31, KLV::differential_pressure_translator));
    // 0x32 - 50 - platform_angle_of_attack
    klv_translators.insert(std::make_pair(0x32, KLV::platform_angle_of_attack_translator));
    // 0x33 - 51 - platform_vertical_speed
    klv_translators.insert(std::make_pair(0x33, KLV::platform_vertical_speed_translator));
    // 0x34 - 52 - platform_sideslip_angle
    klv_translators.insert(std::make_pair(0x34, KLV::platform_sideslip_angle_translator));
    // 0x35 - 53 - airfield_barometric_pressure
    klv_translators.insert(std::make_pair(0x35, KLV::airfield_barometric_pressure_translator));
    // 0x36 - 54 - airfield_elevation
    klv_translators.insert(std::make_pair(0x36, KLV::airfield_elevation_translator));
    // 0x37 - 55 - relative_humidity
    klv_translators.insert(std::make_pair(0x37, KLV::relative_humidity_translator));
    // 0x38 - 56 - platform_ground_speed
    klv_translators.insert(std::make_pair(0x38, KLV::platform_ground_speed_translator));
    // 0x39 - 57 - ground_range
    klv_translators.insert(std::make_pair(0x39, KLV::ground_range_translator));
    // 0x3A - 58 - platform_fuel_remaining
    klv_translators.insert(std::make_pair(0x3A, KLV::platform_fuel_remaining_translator));
    // 0x3B - 59 - platform_call_sign
    klv_translators.insert(std::make_pair(0x3B, KLV::platform_call_sign_translator));
    // 0x3C - 60 - weapon_load
    klv_translators.insert(std::make_pair(0x3C, KLV::weapon_load_translator));
    // 0x3D - 61 - weapon_fired
    klv_translators.insert(std::make_pair(0x3D, KLV::weapon_fired_translator));
    // 0x3E - 62 - laser_prf_code
    klv_translators.insert(std::make_pair(0x3E, KLV::laser_prf_code_translator));
    // 0x3F - 63 - sensor_field_of_view_name
    klv_translators.insert(std::make_pair(0x3F, KLV::sensor_field_of_view_name_translator));
    // 0x40 - 64 - platform_magnetic_heading
    klv_translators.insert(std::make_pair(0x40, KLV::platform_magnetic_heading_translator));
    // 0x41 - 65 - uas_datalink_ls_version_number
    klv_translators.insert(std::make_pair(0x41, KLV::uas_datalink_ls_version_number_translator));

    // // 0x42 - 66 - deprecated
    // klv_translators.insert(std::make_pair(0x42, KLV::deprecated_translator));

    // 0x42 - 66 - target_location_covariance_matrix
    klv_translators.insert(std::make_pair(0x42, KLV::target_location_covariance_matrix_translator));
    // 0x43 - 67 - alternate_platform_latitude
    klv_translators.insert(std::make_pair(0x43, KLV::alternate_platform_latitude_translator));
    // 0x44 - 68 - alternate_platform_longitude
    klv_translators.insert(std::make_pair(0x44, KLV::alternate_platform_longitude_translator));
    // 0x45 - 69 - alternate_platform_altitude
    klv_translators.insert(std::make_pair(0x45, KLV::alternate_platform_altitude_translator));
    // 0x46 - 70 - alternate_platform_name
    klv_translators.insert(std::make_pair(0x46, KLV::alternate_platform_name_translator));
    // 0x47 - 71 - alternate_platform_heading
    klv_translators.insert(std::make_pair(0x47, KLV::alternate_platform_heading_translator));
    // 0x48 - 72 - event_start_time_utc
    klv_translators.insert(std::make_pair(0x48, KLV::event_start_time_utc_translator));
    // 0x49 - 73 - rvt_local_set
    klv_translators.insert(std::make_pair(0x49, KLV::rvt_local_set_translator));
    // 0x4A - 74 - vmti_local_set
    klv_translators.insert(std::make_pair(0x4A, KLV::vmti_local_set_translator));
    // 0x4B - 75 - sensor_ellipsoid_height
    klv_translators.insert(std::make_pair(0x4B, KLV::sensor_ellipsoid_height_translator));
    // 0x4C - 76 - alternate_platform_ellipsoid_height
    klv_translators.insert(std::make_pair(0x4C, KLV::alternate_platform_ellipsoid_height_translator));
    // 0x4D - 77 - operational_mode
    klv_translators.insert(std::make_pair(0x4D, KLV::operational_mode_translator));
    // 0x4E - 78 - frame_center_height_above_ellipsoid
    klv_translators.insert(std::make_pair(0x4E, KLV::frame_center_height_above_ellipsoid_translator));
    // 0x4F - 79 - sensor_north_velocity
    klv_translators.insert(std::make_pair(0x4F, KLV::sensor_north_velocity_translator));
    // 0x50 - 80 - sensor_east_velocity
    klv_translators.insert(std::make_pair(0x50, KLV::sensor_east_velocity_translator));
    // 0x51 - 81 - image_horizon_pixel_pack
    klv_translators.insert(std::make_pair(0x51, KLV::image_horizon_pixel_pack_translator));
    // 0x52 - 82 - corner_latitude_point_1
    klv_translators.insert(std::make_pair(0x52, KLV::corner_latitude_point_1_translator));
    // 0x53 - 83 - corner_longitude_point_1
    klv_translators.insert(std::make_pair(0x53, KLV::corner_longitude_point_1_translator));
    // 0x54 - 84 - corner_latitude_point_2
    klv_translators.insert(std::make_pair(0x54, KLV::corner_latitude_point_2_translator));
    // 0x55 - 85 - corner_longitude_point_2
    klv_translators.insert(std::make_pair(0x55, KLV::corner_longitude_point_2_translator));
    // 0x56 - 86 - corner_latitude_point_3
    klv_translators.insert(std::make_pair(0x56, KLV::corner_latitude_point_3_translator));
    // 0x57 - 87 - corner_longitude_point_3
    klv_translators.insert(std::make_pair(0x57, KLV::corner_longitude_point_3_translator));
    // 0x58 - 88 - corner_latitude_point_4
    klv_translators.insert(std::make_pair(0x58, KLV::corner_latitude_point_4_translator));
    // 0x59 - 89 - corner_longitude_point_4
    klv_translators.insert(std::make_pair(0x59, KLV::corner_longitude_point_4_translator));
    // 0x5A - 90 - platform_pitch_angle_full
    klv_translators.insert(std::make_pair(0x5A, KLV::platform_pitch_angle_full_translator));
    // 0x5B - 91 - platform_roll_angle_full
    klv_translators.insert(std::make_pair(0x5B, KLV::platform_roll_angle_full_translator));
    // 0x5C - 92 - platform_angle_of_attack_full
    klv_translators.insert(std::make_pair(0x5C, KLV::platform_angle_of_attack_full_translator));
    // 0x5D - 93 - platform_sideslip_angle_full
    klv_translators.insert(std::make_pair(0x5D, KLV::platform_sideslip_angle_full_translator));
    // 0x5E - 94 - miis_core_identifier
    klv_translators.insert(std::make_pair(0x5E, KLV::miis_core_identifier_translator));
    // 0x5F - 95 - sar_motion_imagery_local_set
    klv_translators.insert(std::make_pair(0x5F, KLV::sar_motion_imagery_local_set_translator));
    // 0x60 - 96 - target_width_extended
    klv_translators.insert(std::make_pair(0x60, KLV::target_width_extended_translator));
    // 0x61 - 97 - range_image_local_set
    klv_translators.insert(std::make_pair(0x61, KLV::range_image_local_set_translator));
    // 0x62 - 98 - geo_registration_local_set
    klv_translators.insert(std::make_pair(0x62, KLV::geo_registration_local_set_translator));
    // 0x63 - 99 - composite_imaging_local_set
    klv_translators.insert(std::make_pair(0x63, KLV::composite_imaging_local_set_translator));
    // 0x64 - 100 - segment_local_set
    klv_translators.insert(std::make_pair(0x64, KLV::segment_local_set_translator));
    // 0x65 - 101 - amend_local_set
    klv_translators.insert(std::make_pair(0x65, KLV::amend_local_set_translator));
    // 0x66 - 102 - sdcc_flp
    klv_translators.insert(std::make_pair(0x66, KLV::sdcc_flp_translator));
    // 0x67 - 103 - density_altitude_extended
    klv_translators.insert(std::make_pair(0x67, KLV::density_altitude_extended_translator));
    // 0x68 - 104 - sensor_ellipsoid_height_extended
    klv_translators.insert(std::make_pair(0x68, KLV::sensor_ellipsoid_height_extended_translator));
    // 0x69 - 105 - alternate_platform_ellipsoid_height_extended
    klv_translators.insert(std::make_pair(0x69, KLV::alternate_platform_ellipsoid_height_extended_translator));
    // 0x6A - 106 - stream_designator
    klv_translators.insert(std::make_pair(0x6A, KLV::stream_designator_translator));
    // 0x6B - 107 - operational_base
    klv_translators.insert(std::make_pair(0x6B, KLV::operational_base_translator));
    // 0x6C - 108 - broadcast_source
    klv_translators.insert(std::make_pair(0x6C, KLV::broadcast_source_translator));
    // 0x6D - 109 - range_to_recovery_location
    klv_translators.insert(std::make_pair(0x6D, KLV::range_to_recovery_location_translator));
    // 0x6E - 110 - time_airborne
    klv_translators.insert(std::make_pair(0x6E, KLV::time_airborne_translator));
    // 0x6F - 111 - propulsion_unit_speed
    klv_translators.insert(std::make_pair(0x6F, KLV::propulsion_unit_speed_translator));
    // 0x70 - 112 - platform_course_angle
    klv_translators.insert(std::make_pair(0x70, KLV::platform_course_angle_translator));
    // 0x71 - 113 - altitude_agl
    klv_translators.insert(std::make_pair(0x71, KLV::altitude_agl_translator));
    // 0x72 - 114 - radar_altimeter
    klv_translators.insert(std::make_pair(0x72, KLV::radar_altimeter_translator));
    // 0x73 - 115 - control_command
    klv_translators.insert(std::make_pair(0x73, KLV::control_command_translator));
    // 0x74 - 116 - control_command_verification_list
    klv_translators.insert(std::make_pair(0x74, KLV::control_command_verification_list_translator));
    // 0x75 - 117 - sensor_azimuth_rate
    klv_translators.insert(std::make_pair(0x75, KLV::sensor_azimuth_rate_translator));
    // 0x76 - 118 - sensor_elevation_rate
    klv_translators.insert(std::make_pair(0x76, KLV::sensor_elevation_rate_translator));
    // 0x77 - 119 - sensor_roll_rate
    klv_translators.insert(std::make_pair(0x77, KLV::sensor_roll_rate_translator));
    // 0x78 - 120 - onboard_mi_storage_percent_full
    klv_translators.insert(std::make_pair(0x78, KLV::onboard_mi_storage_percent_full_translator));
    // 0x79 - 121 - active_wavelength_list
    klv_translators.insert(std::make_pair(0x79, KLV::active_wavelength_list_translator));
    // 0x7A - 122 - country_codes
    klv_translators.insert(std::make_pair(0x7A, KLV::country_codes_translator));
    // 0x7B - 123 - number_of_navsats_in_view
    klv_translators.insert(std::make_pair(0x7B, KLV::number_of_navsats_in_view_translator));
    // 0x7C - 124 - positioning_method_source
    klv_translators.insert(std::make_pair(0x7C, KLV::positioning_method_source_translator));
    // 0x7D - 125 - platform_status
    klv_translators.insert(std::make_pair(0x7D, KLV::platform_status_translator));
    // 0x7E - 126 - sensor_control_mode
    klv_translators.insert(std::make_pair(0x7E, KLV::sensor_control_mode_translator));
    // 0x7F - 127 - sensor_frame_rate_pack
    klv_translators.insert(std::make_pair(0x7F, KLV::sensor_frame_rate_pack_translator));
    // 0x80 - 128 - wavelengths_list
    klv_translators.insert(std::make_pair(0x80, KLV::wavelengths_list_translator));
    // 0x81 - 129 - target_id
    klv_translators.insert(std::make_pair(0x81, KLV::target_id_translator));
    // 0x82 - 130 - airbase_locations
    klv_translators.insert(std::make_pair(0x82, KLV::airbase_locations_translator));
    // 0x83 - 131 - takeoff_time
    klv_translators.insert(std::make_pair(0x83, KLV::takeoff_time_translator));
    // 0x84 - 132 - transmission_frequency
    klv_translators.insert(std::make_pair(0x84, KLV::transmission_frequency_translator));
    // 0x85 - 133 - onboard_mi_storage_capacity
    klv_translators.insert(std::make_pair(0x85, KLV::onboard_mi_storage_capacity_translator));
    // 0x86 - 134 - zoom_percentage
    klv_translators.insert(std::make_pair(0x86, KLV::zoom_percentage_translator));
    // 0x87 - 135 - communications_method
    klv_translators.insert(std::make_pair(0x87, KLV::communications_method_translator));
    // 0x88 - 136 - leap_seconds
    klv_translators.insert(std::make_pair(0x88, KLV::leap_seconds_translator));
    // 0x89 - 137 - correction_offset
    klv_translators.insert(std::make_pair(0x89, KLV::correction_offset_translator));
    // 0x8A - 138 - payload_list
    klv_translators.insert(std::make_pair(0x8A, KLV::payload_list_translator));
    // 0x8B - 139 - active_payloads
    klv_translators.insert(std::make_pair(0x8B, KLV::active_payloads_translator));
    // 0x8C - 140 - weapons_stores
    klv_translators.insert(std::make_pair(0x8C, KLV::weapons_stores_translator));
    // 0x8D - 141 - waypoint_list
    klv_translators.insert(std::make_pair(0x8D, KLV::waypoint_list_translator));
    // 0x8E - 142 - view_domain
    klv_translators.insert(std::make_pair(0x8E, KLV::view_domain_translator));
}

void KLVParser::init_klv_tags() {
    if (klv_tags.size() > 0) return;

    klv_tags.insert(std::make_pair(0x01, "checksum"));                         // 1
    klv_tags.insert(std::make_pair(0x02, "precision_time_stamp"));             // 2
    klv_tags.insert(std::make_pair(0x03, "mission_id"));                       // 3
    klv_tags.insert(std::make_pair(0x04, "platform_tail_number"));             // 4
    klv_tags.insert(std::make_pair(0x05, "platform_heading_angle"));           // 5
    klv_tags.insert(std::make_pair(0x06, "platform_pitch_angle"));             // 6
    klv_tags.insert(std::make_pair(0x07, "platform_roll_angle"));              // 7
    klv_tags.insert(std::make_pair(0x08, "platform_true_airspeed"));           // 8
    klv_tags.insert(std::make_pair(0x09, "platform_indicated_airspeed"));      // 9
    klv_tags.insert(std::make_pair(0x0A, "platform_designation"));             // 10
    klv_tags.insert(std::make_pair(0x0B, "image_source_sensor"));              // 11
    klv_tags.insert(std::make_pair(0x0C, "image_coordinate_system"));          // 12
    klv_tags.insert(std::make_pair(0x0D, "sensor_latitude"));                  // 13
    klv_tags.insert(std::make_pair(0x0E, "sensor_longitude"));                 // 14
    klv_tags.insert(std::make_pair(0x0F, "sensor_true_altitude"));             // 15
    klv_tags.insert(std::make_pair(0x10, "sensor_horizontal_field_of_view"));  // 16
    klv_tags.insert(std::make_pair(0x11, "sensor_vertical_field_of_view"));    // 17
    klv_tags.insert(std::make_pair(0x12, "sensor_relative_azimuth_angle"));    // 18
    klv_tags.insert(std::make_pair(0x13, "sensor_relative_elevation_angle"));  // 19
    klv_tags.insert(std::make_pair(0x14, "sensor_relative_roll_angle"));       // 20
    klv_tags.insert(std::make_pair(0x15, "slant_range"));                      // 21
    klv_tags.insert(std::make_pair(0x16, "target_width"));                     // 22
    klv_tags.insert(std::make_pair(0x17, "frame_center_latitude"));            // 23
    klv_tags.insert(std::make_pair(0x18, "frame_center_longitude"));           // 24
    klv_tags.insert(std::make_pair(0x19, "frame_center_elevation"));           // 25
    klv_tags.insert(std::make_pair(0x1A, "offset_corner_latitude_point_1"));   // 26
    klv_tags.insert(std::make_pair(0x1B, "offset_corner_longitude_point_1"));  // 27
    klv_tags.insert(std::make_pair(0x1C, "offset_corner_latitude_point_2"));   // 28
    klv_tags.insert(std::make_pair(0x1D, "offset_corner_longitude_point_2"));  // 29
    klv_tags.insert(std::make_pair(0x1E, "offset_corner_latitude_point_3"));   // 30
    klv_tags.insert(std::make_pair(0x1F, "offset_corner_longitude_point_3"));  // 31
    klv_tags.insert(std::make_pair(0x20, "offset_corner_latitude_point_4"));   // 32
    klv_tags.insert(std::make_pair(0x21, "offset_corner_longitude_point_4"));  // 33
    klv_tags.insert(std::make_pair(0x22, "icing_detected"));                   // 34
    klv_tags.insert(std::make_pair(0x23, "wind_direction"));                   // 35
    klv_tags.insert(std::make_pair(0x24, "wind_speed"));                       // 36
    klv_tags.insert(std::make_pair(0x25, "static_pressure"));                  // 37
    klv_tags.insert(std::make_pair(0x26, "density_altitude"));                 // 38
    klv_tags.insert(std::make_pair(0x27, "outside_air_temperature"));          // 39
    klv_tags.insert(std::make_pair(0x28, "target_location_latitude"));         // 40
    klv_tags.insert(std::make_pair(0x29, "target_location_longitude"));        // 41
    klv_tags.insert(std::make_pair(0x2A, "target_location_elevation"));        // 42
    klv_tags.insert(std::make_pair(0x2B, "target_track_gate_width"));          // 43
    klv_tags.insert(std::make_pair(0x2C, "target_track_gate_height"));         // 44
    klv_tags.insert(std::make_pair(0x2D, "target_error_estimate_ce90"));       // 45
    klv_tags.insert(std::make_pair(0x2E, "target_error_estimate_le90"));       // 46
    klv_tags.insert(std::make_pair(0x2F, "generic_flag_data"));                // 47
    klv_tags.insert(std::make_pair(0x30, "security_local_set"));               // 48
    klv_tags.insert(std::make_pair(0x31, "differential_pressure"));            // 49
    klv_tags.insert(std::make_pair(0x32, "platform_angle_of_attack"));         // 50
    klv_tags.insert(std::make_pair(0x33, "platform_vertical_speed"));          // 51
    klv_tags.insert(std::make_pair(0x34, "platform_sideslip_angle"));          // 52
    klv_tags.insert(std::make_pair(0x35, "airfield_barometric_pressure"));     // 53
    klv_tags.insert(std::make_pair(0x36, "airfield_elevation"));               // 54
    klv_tags.insert(std::make_pair(0x37, "relative_humidity"));                // 55
    klv_tags.insert(std::make_pair(0x38, "platform_ground_speed"));            // 56
    klv_tags.insert(std::make_pair(0x39, "ground_range"));                     // 57
    klv_tags.insert(std::make_pair(0x3A, "platform_fuel_remaining"));          // 58
    klv_tags.insert(std::make_pair(0x3B, "platform_call_sign"));               // 59
    klv_tags.insert(std::make_pair(0x3C, "weapon_load"));                      // 60
    klv_tags.insert(std::make_pair(0x3D, "weapon_fired"));                     // 61
    klv_tags.insert(std::make_pair(0x3E, "laser_prf_code"));                   // 62
    klv_tags.insert(std::make_pair(0x3F, "sensor_field_of_view_name"));        // 63
    klv_tags.insert(std::make_pair(0x40, "platform_magnetic_heading"));        // 64
    klv_tags.insert(std::make_pair(0x41, "uas_datalink_ls_version_number"));   // 65

    // //////////////// ? deprecated ? ///////////////////
    // klv_tags.insert(std::make_pair(0x42, "deprecated"));                                 // 66
    klv_tags.insert(std::make_pair(0x42, "target_location_covariance_matrix"));             // 66
    klv_tags.insert(std::make_pair(0x43, "alternate_platform_latitude"));                   // 67
    klv_tags.insert(std::make_pair(0x44, "alternate_platform_longitude"));                  // 68
    klv_tags.insert(std::make_pair(0x45, "alternate_platform_altitude"));                   // 69
    klv_tags.insert(std::make_pair(0x46, "alternate_platform_name"));                       // 70
    klv_tags.insert(std::make_pair(0x47, "alternate_platform_heading"));                    // 71
    klv_tags.insert(std::make_pair(0x48, "event_start_time_utc"));                          // 72
    klv_tags.insert(std::make_pair(0x49, "rvt_local_set"));                                 // 73
    klv_tags.insert(std::make_pair(0x4A, "vmti_local_set"));                                // 74
    klv_tags.insert(std::make_pair(0x4B, "sensor_ellipsoid_height"));                       // 75
    klv_tags.insert(std::make_pair(0x4C, "alternate_platform_ellipsoid_height"));           // 76
    klv_tags.insert(std::make_pair(0x4D, "operational_mode"));                              // 77
    klv_tags.insert(std::make_pair(0x4E, "frame_center_height_above_ellipsoid"));           // 78
    klv_tags.insert(std::make_pair(0x4F, "sensor_north_velocity"));                         // 79
    klv_tags.insert(std::make_pair(0x50, "sensor_east_velocity"));                          // 80
    klv_tags.insert(std::make_pair(0x51, "image_horizon_pixel_pack"));                      // 81
    klv_tags.insert(std::make_pair(0x52, "corner_latitude_point_1"));                       // 82
    klv_tags.insert(std::make_pair(0x53, "corner_longitude_point_1"));                      // 83
    klv_tags.insert(std::make_pair(0x54, "corner_latitude_point_2"));                       // 84
    klv_tags.insert(std::make_pair(0x55, "corner_longitude_point_2"));                      // 85
    klv_tags.insert(std::make_pair(0x56, "corner_latitude_point_3"));                       // 86
    klv_tags.insert(std::make_pair(0x57, "corner_longitude_point_3"));                      // 87
    klv_tags.insert(std::make_pair(0x58, "corner_latitude_point_4"));                       // 88
    klv_tags.insert(std::make_pair(0x59, "corner_longitude_point_4"));                      // 89
    klv_tags.insert(std::make_pair(0x5A, "platform_pitch_angle_full"));                     // 90
    klv_tags.insert(std::make_pair(0x5B, "platform_roll_angle_full"));                      // 91
    klv_tags.insert(std::make_pair(0x5C, "platform_angle_of_attack_full"));                 // 92
    klv_tags.insert(std::make_pair(0x5D, "platform_sideslip_angle_full"));                  // 93
    klv_tags.insert(std::make_pair(0x5E, "miis_core_identifier"));                          // 94
    klv_tags.insert(std::make_pair(0x5F, "sar_motion_imagery_local_set"));                  // 95
    klv_tags.insert(std::make_pair(0x60, "target_width_extended"));                         // 96
    klv_tags.insert(std::make_pair(0x61, "range_image_local_set"));                         // 97
    klv_tags.insert(std::make_pair(0x62, "geo_registration_local_set"));                    // 98
    klv_tags.insert(std::make_pair(0x63, "composite_imaging_local_set"));                   // 99
    klv_tags.insert(std::make_pair(0x64, "segment_local_set"));                             // 100
    klv_tags.insert(std::make_pair(0x65, "amend_local_set"));                               // 101
    klv_tags.insert(std::make_pair(0x66, "sdcc_flp"));                                      // 102
    klv_tags.insert(std::make_pair(0x67, "density_altitude_extended"));                     // 103
    klv_tags.insert(std::make_pair(0x68, "sensor_ellipsoid_height_extended"));              // 104
    klv_tags.insert(std::make_pair(0x69, "alternate_platform_ellipsoid_height_extended"));  // 105
    klv_tags.insert(std::make_pair(0x6A, "stream_designator"));                             // 106
    klv_tags.insert(std::make_pair(0x6B, "operational_base"));                              // 107
    klv_tags.insert(std::make_pair(0x6C, "broadcast_source"));                              // 108
    klv_tags.insert(std::make_pair(0x6D, "range_to_recovery_location"));                    // 109
    klv_tags.insert(std::make_pair(0x6E, "time_airborne"));                                 // 110
    klv_tags.insert(std::make_pair(0x6F, "propulsion_unit_speed"));                         // 111
    klv_tags.insert(std::make_pair(0x70, "platform_course_angle"));                         // 112
    klv_tags.insert(std::make_pair(0x71, "altitude_agl"));                                  // 113
    klv_tags.insert(std::make_pair(0x72, "radar_altimeter"));                               // 114
    klv_tags.insert(std::make_pair(0x73, "control_command"));                               // 115
    klv_tags.insert(std::make_pair(0x74, "control_command_verification_list"));             // 116
    klv_tags.insert(std::make_pair(0x75, "sensor_azimuth_rate"));                           // 117
    klv_tags.insert(std::make_pair(0x76, "sensor_elevation_rate"));                         // 118
    klv_tags.insert(std::make_pair(0x77, "sensor_roll_rate"));                              // 119
    klv_tags.insert(std::make_pair(0x78, "onboard_mi_storage_percent_full"));               // 120
    klv_tags.insert(std::make_pair(0x79, "active_wavelength_list"));                        // 121
    klv_tags.insert(std::make_pair(0x7A, "country_codes"));                                 // 122
    klv_tags.insert(std::make_pair(0x7B, "number_of_navsats_in_view"));                     // 123
    klv_tags.insert(std::make_pair(0x7C, "positioning_method_source"));                     // 124
    klv_tags.insert(std::make_pair(0x7D, "platform_status"));                               // 125
    klv_tags.insert(std::make_pair(0x7E, "sensor_control_mode"));                           // 126
    klv_tags.insert(std::make_pair(0x7F, "sensor_frame_rate_pack"));                        // 127
    klv_tags.insert(std::make_pair(0x80, "wavelengths_list"));                              // 128
    klv_tags.insert(std::make_pair(0x81, "target_id"));                                     // 129
    klv_tags.insert(std::make_pair(0x82, "airbase_locations"));                             // 130
    klv_tags.insert(std::make_pair(0x83, "takeoff_time"));                                  // 131
    klv_tags.insert(std::make_pair(0x84, "transmission_frequency"));                        // 132
    klv_tags.insert(std::make_pair(0x85, "onboard_mi_storage_capacity"));                   // 133
    klv_tags.insert(std::make_pair(0x86, "zoom_percentage"));                               // 134
    klv_tags.insert(std::make_pair(0x87, "communications_method"));                         // 135
    klv_tags.insert(std::make_pair(0x88, "leap_seconds"));                                  // 136
    klv_tags.insert(std::make_pair(0x89, "correction_offset"));                             // 137
    klv_tags.insert(std::make_pair(0x8A, "payload_list"));                                  // 138
    klv_tags.insert(std::make_pair(0x8B, "active_payloads"));                               // 139
    klv_tags.insert(std::make_pair(0x8C, "weapons_stores"));                                // 140
    klv_tags.insert(std::make_pair(0x8D, "waypoint_list"));                                 // 141
    klv_tags.insert(std::make_pair(0x8E, "view_domain"));                                   // 142
}
