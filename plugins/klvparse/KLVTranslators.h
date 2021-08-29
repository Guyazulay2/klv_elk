#pragma once
#include <KLVItem.h>
#include <KLVStorage.h>

#include <chrono>
#include <ctime>

namespace KLV {
using namespace KLV;

bool parse_klv(GstByteReader *reader, KLVItem *item, KEY_FORMATS key_fromat, LENGTH_FORMATS len_format = LENGTH_FORMATS::_BER);

// bool max_number_from_byte_len(guint8 len, guint64 *max);
bool gst_byte_reader_get_variable_uint32_be(GstByteReader *reader, guint32 *res, guint8 *res_byte_len = NULL);
bool gst_byte_reader_get_variable_uint64_be(GstByteReader *reader, guint64 *res, guint8 *res_byte_len = NULL);

bool gst_byte_reader_get_variable_int32_be(GstByteReader *reader, gint32 *res, guint8 *res_byte_len = NULL);
bool gst_byte_reader_get_variable_int64_be(GstByteReader *reader, gint64 *res, guint8 *res_byte_len = NULL);

bool get_uint8_from_klv_value(KLVItem *item, guint8 *store);
bool get_int8_from_klv_value(KLVItem *item, gint8 *store);

bool get_uint16_from_klv_value(KLVItem *item, guint16 *store);
bool get_int16_from_klv_value(KLVItem *item, gint16 *store);

bool get_uint32_from_klv_value(KLVItem *item, guint32 *store);
bool get_int32_from_klv_value(KLVItem *item, gint32 *store);

bool get_uint64_from_klv_value(KLVItem *item, guint64 *store);

bool uint8_translator(KLVItem *item);
bool uint16_translator(KLVItem *item);
bool uint64_translator(KLVItem *item);

bool string_translator(KLVItem *item);

bool timestamp_translator(KLVItem *item);

gfloat uint8_deg_to_float(guint8 val, gfloat min = 0, gfloat max = 0, gfloat offset = 0);
gdouble int32_deg_to_double(gint32 val, gdouble min = 0, gdouble max = 0, gdouble offset = 0);

bool uint8_deg_to_float_translator(KLVItem *item, gfloat min = 0, gfloat max = 0, gfloat offset = 0);
bool uint16_deg_to_float_translator(KLVItem *item, gfloat min = 0, gfloat max = 0, gfloat offset = 0);
bool uint16_deg_to_float_translator_reserved(KLVItem *item, gfloat min = 0, gfloat max = 0, gfloat offset = 0, const gchar *reserved = "Reserved");
bool int16_deg_to_float_translator(KLVItem *item, gfloat min = 0, gfloat max = 0, gfloat offset = 0);
bool int16_deg_to_float_translator_reserved(KLVItem *item, gfloat min = 0, gfloat max = 0, gfloat offset = 0, const gchar *reserved = "Reserved");
bool uint32_deg_to_double_translator(KLVItem *item, gdouble min = 0, gdouble max = 0, gdouble offset = 0);
bool uint32_deg_to_double_translator_reserved(KLVItem *item, gdouble min = 0, gdouble max = 0, gdouble offset = 0, const gchar *reserved = "Reserved");

bool int32_deg_to_double_translator(KLVItem *item, gdouble min = 0, gdouble max = 0, gdouble offset = 0);
bool int32_deg_to_double_translator_reserved(KLVItem *item, gdouble min = 0, gdouble max = 0, gdouble offset = 0, const gchar *reserved = "Reserved");

bool variable_deg_to_float_translator(KLVItem *item, gfloat min = 0, gfloat max = 0, gfloat offset = 0);
bool variable_deg_to_float_translator_reserved(KLVItem *item, gfloat min = 0, gfloat max = 0, gfloat offset = 0, const gchar *reserved = "Reserved");

bool variable_deg_to_double_translator(KLVItem *item, gdouble min = 0, gdouble max = 0, gdouble offset = 0);
bool variable_deg_to_double_translator_reserved(KLVItem *item, gdouble min = 0, gdouble max = 0, gdouble offset = 0, const gchar *reserved = "Reserved");

bool general_klv_assigner(KLVItem *item);
bool general_klv_translator(KLVItem *item);

/**
 *  0x01 - 1 - checksum
 */
bool checksum_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x02 - 2 - precision_time_stamp
 */
bool precision_time_stamp_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x03 - 3 - mission_id
 */
bool mission_id_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x04 - 4 - platform_tail_number
 */
bool platform_tail_number_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x05 - 5 - platform_heading_angle
 */
bool platform_heading_angle_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x06 - 6 - platform_pitch_angle
 */
bool platform_pitch_angle_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x07 - 7 - platform_roll_angle
 */
bool platform_roll_angle_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x08 - 8 - platform_true_airspeed
 */
bool platform_true_airspeed_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x09 - 9 - platform_indicated_airspeed
 */
bool platform_indicated_airspeed_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x0A - 10 - platform_designation
 */
bool platform_designation_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x0B - 11 - image_source_sensor
 */
bool image_source_sensor_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x0C - 12 - image_coordinate_system
 */
bool image_coordinate_system_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x0D - 13 - sensor_latitude
 */
bool sensor_latitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x0E - 14 - sensor_longitude
 */
bool sensor_longitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x0F - 15 - sensor_true_altitude
 */
bool sensor_true_altitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x10 - 16 - sensor_horizontal_field_of_view
 */
bool sensor_horizontal_field_of_view_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x11 - 17 - sensor_vertical_field_of_view
 */
bool sensor_vertical_field_of_view_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x12 - 18 - sensor_relative_azimuth_angle
 */
bool sensor_relative_azimuth_angle_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x13 - 19 - sensor_relative_elevation_angle
 */
bool sensor_relative_elevation_angle_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x14 - 20 - sensor_relative_roll_angle
 */
bool sensor_relative_roll_angle_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x15 - 21 - slant_range
 */
bool slant_range_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x16 - 22 - target_width
 */
bool target_width_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x17 - 23 - frame_center_latitude
 */
bool frame_center_latitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x18 - 24 - frame_center_longitude
 */
bool frame_center_longitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x19 - 25 - frame_center_elevation
 */
bool frame_center_elevation_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x1A - 26 - offset_corner_latitude_point_1
 */
bool offset_corner_latitude_point_1_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x1B - 27 - offset_corner_longitude_point_1
 */
bool offset_corner_longitude_point_1_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x1C - 28 - offset_corner_latitude_point_2
 */
bool offset_corner_latitude_point_2_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x1D - 29 - offset_corner_longitude_point_2
 */
bool offset_corner_longitude_point_2_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x1E - 30 - offset_corner_latitude_point_3
 */
bool offset_corner_latitude_point_3_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x1F - 31 - offset_corner_longitude_point_3
 */
bool offset_corner_longitude_point_3_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x20 - 32 - offset_corner_latitude_point_4
 */
bool offset_corner_latitude_point_4_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x21 - 33 - offset_corner_longitude_point_4
 */
bool offset_corner_longitude_point_4_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x22 - 34 - icing_detected
 */
bool icing_detected_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x23 - 35 - wind_direction
 */
bool wind_direction_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x24 - 36 - wind_speed
 */
bool wind_speed_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x25 - 37 - static_pressure
 */
bool static_pressure_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x26 - 38 - density_altitude
 */
bool density_altitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x27 - 39 - outside_air_temperature
 */
bool outside_air_temperature_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x28 - 40 - target_location_latitude
 */
bool target_location_latitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x29 - 41 - target_location_longitude
 */
bool target_location_longitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x2A - 42 - target_location_elevation
 */
bool target_location_elevation_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x2B - 43 - target_track_gate_width
 */
bool target_track_gate_width_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x2C - 44 - target_track_gate_height
 */
bool target_track_gate_height_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x2D - 45 - target_error_estimate_ce90
 */
bool target_error_estimate_ce90_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x2E - 46 - target_error_estimate_le90
 */
bool target_error_estimate_le90_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x2F - 47 - generic_flag_data
 */
bool generic_flag_data_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x30 - 48 - security_local_set
 */
bool security_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x31 - 49 - differential_pressure
 */
bool differential_pressure_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x32 - 50 - platform_angle_of_attack
 */
bool platform_angle_of_attack_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x33 - 51 - platform_vertical_speed
 */
bool platform_vertical_speed_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x34 - 52 - platform_sideslip_angle
 */
bool platform_sideslip_angle_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x35 - 53 - airfield_barometric_pressure
 */
bool airfield_barometric_pressure_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x36 - 54 - airfield_elevation
 */
bool airfield_elevation_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x37 - 55 - relative_humidity
 */
bool relative_humidity_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x38 - 56 - platform_ground_speed
 */
bool platform_ground_speed_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x39 - 57 - ground_range
 */
bool ground_range_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x3A - 58 - platform_fuel_remaining
 */
bool platform_fuel_remaining_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x3B - 59 - platform_call_sign
 */
bool platform_call_sign_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x3C - 60 - weapon_load
 */
bool weapon_load_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x3D - 61 - weapon_fired
 */
bool weapon_fired_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x3E - 62 - laser_prf_code
 */
bool laser_prf_code_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x3F - 63 - sensor_field_of_view_name
 */
bool sensor_field_of_view_name_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x40 - 64 - platform_magnetic_heading
 */
bool platform_magnetic_heading_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x41 - 65 - uas_datalink_ls_version_number
 */
bool uas_datalink_ls_version_number_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x42 - 66 - deprecated
 */
bool deprecated_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x42 - 66 - target_location_covariance_matrix
 */
bool target_location_covariance_matrix_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x43 - 67 - alternate_platform_latitude
 */
bool alternate_platform_latitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x44 - 68 - alternate_platform_longitude
 */
bool alternate_platform_longitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x45 - 69 - alternate_platform_altitude
 */
bool alternate_platform_altitude_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x46 - 70 - alternate_platform_name
 */
bool alternate_platform_name_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x47 - 71 - alternate_platform_heading
 */
bool alternate_platform_heading_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x48 - 72 - event_start_time_utc
 */
bool event_start_time_utc_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x49 - 73 - rvt_local_set
 */
bool rvt_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x4A - 74 - vmti_local_set
 */
bool vmti_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x4B - 75 - sensor_ellipsoid_height
 */
bool sensor_ellipsoid_height_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x4C - 76 - alternate_platform_ellipsoid_height
 */
bool alternate_platform_ellipsoid_height_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x4D - 77 - operational_mode
 */
bool operational_mode_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x4E - 78 - frame_center_height_above_ellipsoid
 */
bool frame_center_height_above_ellipsoid_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x4F - 79 - sensor_north_velocity
 */
bool sensor_north_velocity_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x50 - 80 - sensor_east_velocity
 */
bool sensor_east_velocity_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x51 - 81 - image_horizon_pixel_pack
 */
bool image_horizon_pixel_pack_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x52 - 82 - corner_latitude_point_1
 */
bool corner_latitude_point_1_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x53 - 83 - corner_longitude_point_1
 */
bool corner_longitude_point_1_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x54 - 84 - corner_latitude_point_2
 */
bool corner_latitude_point_2_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x55 - 85 - corner_longitude_point_2
 */
bool corner_longitude_point_2_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x56 - 86 - corner_latitude_point_3
 */
bool corner_latitude_point_3_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x57 - 87 - corner_longitude_point_3
 */
bool corner_longitude_point_3_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x58 - 88 - corner_latitude_point_4
 */
bool corner_latitude_point_4_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x59 - 89 - corner_longitude_point_4
 */
bool corner_longitude_point_4_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x5A - 90 - platform_pitch_angle_full
 */
bool platform_pitch_angle_full_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x5B - 91 - platform_roll_angle_full
 */
bool platform_roll_angle_full_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x5C - 92 - platform_angle_of_attack_full
 */
bool platform_angle_of_attack_full_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x5D - 93 - platform_sideslip_angle_full
 */
bool platform_sideslip_angle_full_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x5E - 94 - miis_core_identifier
 */
bool miis_core_identifier_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x5F - 95 - sar_motion_imagery_local_set
 */
bool sar_motion_imagery_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x60 - 96 - target_width_extended
 */
bool target_width_extended_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x61 - 97 - range_image_local_set
 */
bool range_image_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x62 - 98 - geo_registration_local_set
 */
bool geo_registration_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x63 - 99 - composite_imaging_local_set
 */
bool composite_imaging_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x64 - 100 - segment_local_set
 */
bool segment_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x65 - 101 - amend_local_set
 */
bool amend_local_set_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x66 - 102 - sdcc_flp
 */
bool sdcc_flp_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x67 - 103 - density_altitude_extended
 */
bool density_altitude_extended_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x68 - 104 - sensor_ellipsoid_height_extended
 */
bool sensor_ellipsoid_height_extended_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x69 - 105 - alternate_platform_ellipsoid_height_extended
 */
bool alternate_platform_ellipsoid_height_extended_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x6A - 106 - stream_designator
 */
bool stream_designator_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x6B - 107 - operational_base
 */
bool operational_base_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x6C - 108 - broadcast_source
 */
bool broadcast_source_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x6D - 109 - range_to_recovery_location
 */
bool range_to_recovery_location_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x6E - 110 - time_airborne
 */
bool time_airborne_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x6F - 111 - propulsion_unit_speed
 */
bool propulsion_unit_speed_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x70 - 112 - platform_course_angle
 */
bool platform_course_angle_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x71 - 113 - altitude_agl
 */
bool altitude_agl_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x72 - 114 - radar_altimeter
 */
bool radar_altimeter_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x73 - 115 - control_command
 */
bool control_command_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x74 - 116 - control_command_verification_list
 */
bool control_command_verification_list_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x75 - 117 - sensor_azimuth_rate
 */
bool sensor_azimuth_rate_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x76 - 118 - sensor_elevation_rate
 */
bool sensor_elevation_rate_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x77 - 119 - sensor_roll_rate
 */
bool sensor_roll_rate_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x78 - 120 - onboard_mi_storage_percent_full
 */
bool onboard_mi_storage_percent_full_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x79 - 121 - active_wavelength_list
 */
bool active_wavelength_list_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x7A - 122 - country_codes
 */
bool country_codes_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x7B - 123 - number_of_navsats_in_view
 */
bool number_of_navsats_in_view_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x7C - 124 - positioning_method_source
 */
bool positioning_method_source_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x7D - 125 - platform_status
 */
bool platform_status_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x7E - 126 - sensor_control_mode
 */
bool sensor_control_mode_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x7F - 127 - sensor_frame_rate_pack
 */
bool sensor_frame_rate_pack_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x80 - 128 - wavelengths_list
 */
bool wavelengths_list_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x81 - 129 - target_id
 */
bool target_id_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x82 - 130 - airbase_locations
 */
bool airbase_locations_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x83 - 131 - takeoff_time
 */
bool takeoff_time_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x84 - 132 - transmission_frequency
 */
bool transmission_frequency_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x85 - 133 - onboard_mi_storage_capacity
 */
bool onboard_mi_storage_capacity_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x86 - 134 - zoom_percentage
 */
bool zoom_percentage_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x87 - 135 - communications_method
 */
bool communications_method_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x88 - 136 - leap_seconds
 */
bool leap_seconds_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x89 - 137 - correction_offset
 */
bool correction_offset_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x8A - 138 - payload_list
 */
bool payload_list_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x8B - 139 - active_payloads
 */
bool active_payloads_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x8C - 140 - weapons_stores
 */
bool weapons_stores_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x8D - 141 - waypoint_list
 */
bool waypoint_list_translator(KLVItem *item, KLVStorage *storage);

/**
 *  0x8E - 142 - view_domain
 */
bool view_domain_translator(KLVItem *item, KLVStorage *storage);

}  // namespace KLV
