#include "client_signature.h"

void BasicCodecave::call_virtual_protect() noexcept {
    DWORD old;
    VirtualProtect(this->data, sizeof(this->data), PAGE_EXECUTE_READWRITE, &old);
}
BasicCodecave::BasicCodecave() noexcept {
    this->call_virtual_protect();
}
BasicCodecave::BasicCodecave(const unsigned char *data, unsigned int length) noexcept {
    this->call_virtual_protect();
    memcpy(this->data, data, length);
}
BasicCodecave::BasicCodecave(const BasicCodecave &other) noexcept {
    this->call_virtual_protect();
    memcpy(this->data, other.data, sizeof(BasicCodecave::data));
}

void write_jmp_call(void *call_instruction, void *before_function, void *after_function, BasicCodecave &codecave) noexcept {
    auto *call_instruction_char = static_cast<unsigned char *>(call_instruction);
    auto call_instruction_end = reinterpret_cast<int>(call_instruction_char + 5);

    DWORD prota, protb;

    size_t nops = 0;
    size_t whereami = 0;

    if(before_function) {
        codecave.data[whereami + 0] = 0x60;
        codecave.data[whereami + 1] = 0xE8;
        *reinterpret_cast<int *>(codecave.data + whereami + 2) = reinterpret_cast<int>(before_function) - reinterpret_cast<int>(codecave.data + whereami + 2 + 4);
        codecave.data[whereami + 6] = 0x61;
        whereami += 7;
    }

    switch(*call_instruction_char) {
        // call <address>
        case 0xE8:
            codecave.data[whereami + 0] = 0xE8;
            *reinterpret_cast<int *>(codecave.data + whereami + 1) = *reinterpret_cast<int *>(call_instruction_char + 1) + call_instruction_end - reinterpret_cast<int>(codecave.data + whereami + 1 + 4);
            nops = 5;
            whereami += 5;
            break;

        case 0x81:
            switch(call_instruction_char[1]) {
                case 0x3D:
                    memcpy(codecave.data + whereami, call_instruction, 10);
                    whereami += 10;
                    nops = 10;
                    break;
                default: std::terminate();
            }
            break;

        // oh shi-
        default: std::terminate();
    }

    if(after_function) {
        codecave.data[whereami + 0] = 0x60;
        codecave.data[whereami + 1] = 0xE8;
        *reinterpret_cast<int *>(codecave.data + whereami + 2) = reinterpret_cast<int>(after_function) - reinterpret_cast<int>(codecave.data + whereami + 2 + 4);
        codecave.data[whereami + 6] = 0x61;
        whereami += 7;
    }

    codecave.data[whereami + 0] = 0xE9;
    *reinterpret_cast<int *>(codecave.data + whereami + 1) = call_instruction_end - reinterpret_cast<int>(codecave.data + whereami + 1 + 4);

    VirtualProtect(call_instruction, 32, PAGE_READWRITE, &prota);

    memset(call_instruction, 0x90, nops);
    *call_instruction_char = 0xE9;
    *reinterpret_cast<int *>(call_instruction_char + 1) = reinterpret_cast<int>(&codecave.data) - call_instruction_end;

    VirtualProtect(call_instruction, 32, prota, &protb);
}

std::vector<ChimeraSignature> *signatures = nullptr;
std::vector<std::string> *missing_signatures = nullptr;

ChimeraSignature &get_signature(const char *name) noexcept {
    for(size_t i=0;i<signatures->size();i++) {
        auto &sig = (*signatures)[i];
        if(strcmp(sig.name(), name) == 0) {
            return sig;
        }
    }
    char message[256] = {};
    sprintf(message, "Could not find %s signature. There may be a bug with Chimera. Halo must close, now.", name);
    MessageBox(NULL, message, "Missing required signature", MB_OK);
    std::terminate();
}

static bool add_signature(const char *name, const short *signature, size_t signature_size) noexcept {
    try {
        (*signatures).emplace_back(name, signature, signature_size);
        return true;
    }
    catch(std::exception &e) {
        (*missing_signatures).push_back(name);
        return false;
    }
}

#define add_signature_s(name,sig) add_signature(name,sig,sizeof(sig)/sizeof(sig[0]))
#define add_signature_s2(sig) add_signature_s(#sig,sig)

enum CheckIfDone {
    CHECK_IF_DONE_NO,
    CHECK_IF_DONE_YES_TRUE,
    CHECK_IF_DONE_YES_FALSE
};

#define check_result size_t size_before = (*missing_signatures).size(); static CheckIfDone well_is_it_done_or_not = CHECK_IF_DONE_NO; if(well_is_it_done_or_not != CHECK_IF_DONE_NO) return well_is_it_done_or_not == CHECK_IF_DONE_YES_TRUE;
#define set_result if(size_before == (*missing_signatures).size()) { well_is_it_done_or_not = CHECK_IF_DONE_YES_TRUE; return true; } else { well_is_it_done_or_not = CHECK_IF_DONE_YES_FALSE; return false; }

bool find_required_signatures() noexcept {
    check_result

    const short enable_console_sig[] = { 0xA0, -1, -1, -1, -1, 0x84, 0xC0, 0x74, 0x28, 0xA0, -1, -1, -1, -1, 0x84, 0xC0, 0x75, 0x1F, 0x57, 0xBF };
    add_signature_s2(enable_console_sig);

    const short console_is_out_sig[] = { 0x38, 0x1D, -1, -1, -1, -1, 0x75, 0x05, 0xE8, -1, -1, -1, -1, 0xE8, -1, -1, -1, -1, 0xE8, -1, -1, -1, -1, 0x84, 0xC0, 0x74, 0x0D, 0x66, 0x39, 0x1D };
    add_signature_s2(console_is_out_sig);

    const short console_block_error_sig[] = { 0x68, -1, -1, -1, -1, 0x53, 0xE8, -1, -1, -1, -1, 0x83, 0xC4, 0x0C, 0x5E, 0x8A, 0xC3, 0x5B, 0x81 };
    add_signature_s2(console_block_error_sig);

    const short console_call_sig[] = { 0xE8, -1, -1, -1, -1, 0x83, 0xC4, 0x04, 0x88, 0x1D, -1, -1, -1, -1, 0x66, 0x89, 0x1D };
    add_signature_s2(console_call_sig);

    const short console_out_sig[] = { 0x83, 0xEC, 0x10, 0x57, 0x8B, 0xF8, 0xA0, -1, -1, -1, -1, 0x84, 0xC0, 0xC7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x80, 0x3F };
    add_signature_s2(console_out_sig);

    const short on_tick_sig[] = { 0xE8, -1, -1, -1, -1, 0xA1, -1, -1, -1, -1, 0x8B, 0x50, 0x14, 0x8B, 0x48, 0x0C };
    add_signature_s2(on_tick_sig);

    const short player_id_sig[] = { 0x8B, 0x35, -1, -1, -1, -1, 0x8B, 0x7C, 0x24, 0x20, 0x83, 0x7C, 0xBE, 0x04, 0xFF, 0x74 };
    add_signature_s2(player_id_sig);

    const short object_table_sig[] = { 0x8B,0x0D,-1,-1,-1,-1,0x8B,0x51,0x34,0x25,0xFF,0xFF,0x00,0x00,0x8D };
    add_signature_s2(object_table_sig);

    const short player_table_sig[] = { 0xA1,-1,-1,-1,-1,0x89,0x44,0x24,0x48,0x35 };
    add_signature_s2(player_table_sig);

    const short path_sig[] = { 0x68, -1, -1, -1, -1, 0xE8, -1, -1, -1, -1, 0x83, 0xC4, 0x04, 0x88, 0x1D, -1, -1, -1, -1, 0xC6, 0x05, -1, -1, -1, -1, 0x01, 0xC6, 0x05 };
    add_signature_s2(path_sig);

    const short descope_fix_sig[] = { 0xE8, -1, -1, -1, -1, 0x83, 0xC4, 0x04, 0x80, 0x7D, 0x20, 0x01, 0x0F, 0x85 };
    add_signature_s2(descope_fix_sig);

    const short player_name_sig[] = { 0xBE, -1, -1, -1, -1, 0x8D, 0x7C, 0x24, 0x10, 0xF3, 0xA5, 0x8B, 0x42, 0x34 };
    add_signature_s2(player_name_sig);

    const short resolution_sig[] = { 0x75, 0x0A, 0x66, 0xA1, -1, -1, -1, -1, 0x66, 0x89, 0x42, 0x04, 0x83, 0xC4, 0x10, 0xC3 };
    add_signature_s2(resolution_sig);

    const short tick_counter_sig[] = { 0xA1, -1, -1, -1, -1, 0x8B, 0x50, 0x14, 0x8B, 0x48, 0x0C, 0x83, 0xC4, 0x04, 0x42, 0x41, 0x4E, 0x4F };
    add_signature_s2(tick_counter_sig);

    const short map_header_sig[] = { 0x81, 0x3D, -1, -1, -1, -1, 0x64, 0x61, 0x65, 0x68, 0x8B, 0x3D, -1, -1, -1, -1, 0x0F, 0x85 };
    add_signature_s2(map_header_sig);

    const short on_map_preload_sig[] = { 0x81, 0x3D, -1, -1, -1, -1, 0x64, 0x61, 0x65, 0x68, 0x8B, 0x3D, -1, -1, -1, -1, 0x0F, 0x85 };
    add_signature_s2(on_map_preload_sig);

    const short on_frame_sig[] = { 0xE8, -1, -1, -1, -1, 0xA1, -1, -1, -1, -1, 0x83, 0xC4, 0x14, 0x5F, 0x48 };
    add_signature_s2(on_frame_sig);

    const short on_camera_sig[] = { 0xE8, -1, -1, -1, -1, 0x8B, 0x45, 0xEC, 0x8B, 0x4D, 0xF0, 0x40, 0x81, 0xC6 };
    add_signature_s2(on_camera_sig);

    const short server_type_sig[] = { 0x0F, 0xBF, 0x2D, -1, -1, -1, -1, 0xE8, -1, -1, -1, -1, 0x39, 0x1D, -1, -1, -1, -1, 0x75, 0x05 };
    add_signature_s2(server_type_sig);

    const short particle_table_sig[] = { 0x8B,0x2D,-1,-1,-1,-1,0x83,0xCA,0xFF,0x8B,0xFD,0xE8,-1,-1,-1,-1,0x8B,0xF8,0x83,0xFF,0xFF,0x0F,0x84,0x10,0x06,0x00,0x00 };
    add_signature_s2(particle_table_sig);

    const short light_table_sig[] = { 0x8B, 0x0D, -1, -1, -1, -1, 0x8B, 0x51, 0x34, 0x56, 0x8B, 0xF0, 0x81, 0xE6, 0xFF, 0xFF, 0x00, 0x00, 0x6B, 0xF6, 0x7C };
    add_signature_s2(light_table_sig);

    const short effect_table_sig[] = { 0xA1, -1, -1, -1, -1, 0x8B, 0x15, -1, -1, -1, -1, 0x53, 0x8B, 0x5C, 0x24, 0x24, 0x81, 0xE3, 0xFF, 0xFF, 0x00, 0x00 };
    add_signature_s2(effect_table_sig);

    const short decal_table_sig[] = { 0xA1, -1, -1, -1, -1, 0x8A, 0x48, 0x24, 0x83, 0xEC, 0x10, 0x84, 0xC9, 0x74, 0x48, 0x89, 0x04, 0x24, 0x57, 0x35, 0x72, 0x65, 0x74, 0x69 };
    add_signature_s2(decal_table_sig);

    const short create_object_sig[] = { 0x56, 0x83, 0xCE, 0xFF, 0x85, 0xC9, 0x57 };
    add_signature_s2(create_object_sig);

    const short create_object_query_sig[] = { 0x53, 0x8B, 0x5C, 0x24, 0x0C, 0x56, 0x8B, 0xF0, 0x33, 0xC0 };
    add_signature_s2(create_object_query_sig);

    const short delete_object_sig[] = { 0x8B, 0xF8, 0x25, 0xFF, 0xFF, 0x00, 0x00, 0x8D, 0x04, 0x40, 0x8B, 0x44, 0x82, 0x08, 0x8B, 0x40, 0x04 };
    add_signature_s2(delete_object_sig);

    const short current_gametype_sig[] = { 0x83, 0x3D, -1, -1, -1, -1, 0x04, 0x8B, 0x4F, 0x6C, 0x89, 0x4C, 0x24, 0x34, 0x75 };
    add_signature_s2(current_gametype_sig);

    const short hs_globals_sig[] = { 0x05,-1,-1,-1,-1,0x8B,0x0D,-1,-1,-1,-1,0x8B,0x51,0x34,0x25,0xFF,0xFF,0x00,0x00 };
    add_signature_s2(hs_globals_sig);

    const short hud_message_sig[] = { 0x66, 0x3D, 0xFF, 0xFF, 0x74, -1, 0x8B, 0x15, -1, -1, -1, -1, 0x56, 0x57, 0x0F, 0xBF, 0xF8, 0x69, 0xFF, 0x60, 0x04, 0x00, 0x00, 0x03, 0xFA, 0x6A, 0x00 };
    add_signature_s2(hud_message_sig);

    const short execute_script_sig[] = { 0x81, 0xEC, 0x0C, 0x08, 0x00, 0x00, 0x53, 0x55, 0x8B, 0xAC, 0x24, 0x18, 0x08, 0x00, 0x00, 0x68, 0x00, 0x04, 0x00, 0x00 };
    add_signature_s2(execute_script_sig);

    const short on_rcon_message_sig[] = { 0xE8, -1, -1, -1, -1, 0x83, 0xC4, 0x08, 0x83, 0xC4, 0x58, 0xC3 };
    add_signature_s2(on_rcon_message_sig);

    const short on_map_load_sig[] = { 0xE8, -1, -1, -1, -1, 0xE8, -1, -1, -1, -1, 0xA1, -1, -1, -1, -1, 0x33, 0xD2, 0x8B, 0xC8, 0x89, 0x11 };
    add_signature_s2(on_map_load_sig);

    set_result
}

bool find_interpolation_signatures() noexcept {
    check_result

    const short vehicle_camera_sig[] = { 0x8D, 0x42, 0x5C, 0x8B, 0x18, 0x8B, 0xCF, 0x89, 0x19, 0x8B, 0x58, 0x04, 0x89, 0x59, 0x04, 0x8B, 0x40, 0x08, 0x89, 0x41, 0x08, 0x66, 0x8B, 0x9A, 0xB4, 0x00, 0x00, 0x00, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x8A, 0xCB, 0xD3, 0xE0, 0xA8, 0x03, 0x74, 0x71, 0x66, 0x8B, 0x86 };
    add_signature_s2(vehicle_camera_sig);

    const short camera_coord_sig[] = { 0xD9, 0x1D, -1, -1, -1, -1, 0xD9, 0x05, -1, -1, -1, -1, 0xD8, 0x44, 0x24, 0x08, 0xD9, 0x1D, -1, -1, -1, -1, 0xD9, 0x05, -1, -1, -1, -1, 0xD8, 0x44, 0x24, 0x0C };
    add_signature_s2(camera_coord_sig);

    const short camera_tick_rate_sig[] = { 0xD8, 0x0D, -1, -1, -1, -1, 0x83, 0xC4, 0x0C, 0xD8, 0x15, -1, -1, -1, -1, 0xDF, 0xE0, 0xF6, 0xC4, 0x41, 0x75, 0x0A, 0xDD, 0xD8, 0xD9, 0x05 };
    add_signature_s2(camera_tick_rate_sig);

    const short fp_interp_cam_sig[] = { 0x74, 0x30, 0xD9, 0x05, -1, -1, -1, -1, 0xD8, 0x44, 0x24, 0x04 };
    add_signature_s2(fp_interp_cam_sig);

    const short fp_interp_sig[] = { 0xE8, -1, -1, -1, -1, 0x83, 0xC4, 0x10, 0x5F, 0x5E, 0x5B, 0x83, 0xC4, 0x18, 0xC3 };
    add_signature_s2(fp_interp_sig);

    const short on_camera_get_sig[] = { 0xE8, -1, -1, -1, -1, 0x83, 0xC4, 0x04, 0xE8, -1, -1, -1, -1, 0x0F, 0xB6, 0x0D, -1, -1, -1, -1, 0x89, 0x4C, 0x24, 0x20 };
    add_signature_s2(on_camera_get_sig);

    const short do_reset_sig[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0xB8, 0x10, 0x28, 0x00, 0x00, 0xE8 };
    add_signature_s2(do_reset_sig);

    const short do_reset_particle_sig[] = { 0xE8, -1, -1, -1, -1, 0x83, 0xC4, 0x04, 0x57, 0x8B, 0x7C, 0x24, 0x14, 0x57, 0xE8 };
    add_signature_s2(do_reset_particle_sig);

    const short camera_type_sig[] = { 0x81, 0xC1, -1, -1, -1, -1, 0x8B, 0x41, 0x08, 0x3D, -1, -1, -1, -1, 0x75, 0x1D, 0xD9, 0x05 };
    add_signature_s2(camera_type_sig);

    const short antenna_table_sig[] = { 0x8B, 0x15, -1, -1, -1, -1, 0x8B, 0xC7, 0x25, 0xFF, 0xFF, 0x00, 0x00, 0xC1, 0xE0, 0x05, 0x55, 0x8B, 0x6C, 0x08, 0x14, 0x89, 0x6C, 0x24, 0x28 };
    add_signature_s2(antenna_table_sig);

    const short flag_table_sig[] = { 0x8B, 0x3D, -1, -1, -1, -1, 0x83, 0xC4, 0x0C, 0x8D, 0x4E, 0x01, 0x83, 0xCB, 0xFF, 0x66, 0x85, 0xC9, 0x7C, 0x31 };
    add_signature_s2(flag_table_sig);

    const short tick_rate_sig[] = { 0xD8, 0x0D, -1, -1, -1, -1, 0x83, 0xEC, 0x08, 0xD9, 0x5C, 0x24, 0x08, 0xD9, 0x44, 0x24, 0x14, 0xD8, 0x41, 0x1C };
    add_signature_s2(tick_rate_sig);

    const short game_speed_sig[] = { 0xA1, -1, -1, -1, -1, 0x66, 0x89, 0x58, 0x10, 0x66, 0x8B, 0x0D, -1, -1, -1, -1, 0x66, 0x83, 0xF9, 0x01, 0x74, 0x1E, 0x66, 0x83, 0xF9, 0x02 };
    add_signature_s2(game_speed_sig);

    const short zoomed_in_sig[] = { 0x8B, 0x0D, -1, -1, -1, -1, 0x8B, 0x41, 0x0C, 0xEB, 0x0C, 0x0F, 0xBF, 0xC8, 0x49, 0x89, 0x0C, 0x9E, 0xEB, 0x06 };
    add_signature_s2(zoomed_in_sig);

    set_result
}

bool find_uncap_cinematic_signatures() noexcept {
    check_result

    const short uncap_cinematic_sig[] = {0x74, 0x04, 0xB3, 0x01, 0xEB, 0x02, 0x32, 0xDB, 0x8B, 0x2D};
    add_signature_s2(uncap_cinematic_sig);

    const short fps_throttle_sig[] = { 0xDD, 0x05, -1, -1, -1, -1, 0xDC, 0x64, 0x24, 0x10, 0xDC, 0x1D, -1, -1, -1, -1, 0xDF, 0xE0, 0xF6, 0xC4, 0x41, 0x75, 0x04, 0x6A, 0x0A };
    add_signature_s2(fps_throttle_sig);

    const short do_throttle_fps_sig[] = { 0xA0, -1, -1, -1, -1, 0x84, 0xC0, 0x75, 0x0C, 0xA1, -1, -1, -1, -1, 0x8A, 0x48, 0x09, 0x84, 0xC9, -1, 0x04, 0xB3, 0x01 };
    add_signature_s2(do_throttle_fps_sig);

    set_result
}

bool find_magnetism_signatures() noexcept {
    check_result

    const short player_magnetism_enabled_sig[] = {0xA0, -1, -1, -1, -1, 0x83, 0xC4, 0x10, 0x84, 0xC0, 0x0F, 0x84};
    add_signature_s2(player_magnetism_enabled_sig);

    const short magnetism_sig[] = {0x84,   -1, 0x0F, 0x84,   -1,   -1,   -1,   -1, 0x8A, -1, -1, 0x84, -1, 0x8A};
    add_signature_s2(magnetism_sig);

    const short gamepad_horizontal_0_sig[] = { 0xD9, 0x1D, -1, -1, -1, -1, 0xEB, 0x22, 0x81, 0xFD, 0xFF, 0x7F, 0x00, 0x00, 0x74, 0x1A, 0x8A, 0x85, -1, -1, -1, -1, 0x8A, 0x4C, 0x24, 0x13, 0x3A, 0xC1 };
    const short gamepad_horizontal_1_sig[] = { 0xD9, 0x1D, -1, -1, -1, -1, 0xE9, 0x86, 0x00, 0x00, 0x00, 0x85, 0xC9, 0xC6, 0x05, -1, -1, -1, -1, 0x01, 0xD9, 0x04, 0x9D };
    add_signature_s2(gamepad_horizontal_0_sig);
    add_signature_s2(gamepad_horizontal_1_sig);

    const short gamepad_vertical_0_sig[] = { 0xD9, 0x1D, -1, -1, -1, -1, 0xEB, 0x54, 0x85, 0xC9, 0xC6, 0x05, -1, -1, -1, -1, 0x01, 0xD9, 0x04, 0x9D, -1, -1, -1, -1, 0xD9, 0x1D, -1, -1, -1, -1, 0x7D, 0x02, 0xF7, 0xD9, 0x89, 0x4C, 0x24, 0x28 };
    const short gamepad_vertical_1_sig[] = { 0xD9, 0x1D, -1, -1, -1, -1, 0xE9, 0xD6, 0x00, 0x00, 0x00, 0x83, 0xFD, 0x1A, 0x0F, 0x8F, 0xAB, 0x00, 0x00, 0x00, 0x0F, 0x84, 0x73, 0x00, 0x00, 0x00, 0x8B, 0xC5, 0x83, 0xE8, 0x18 };
    add_signature_s2(gamepad_vertical_0_sig);
    add_signature_s2(gamepad_vertical_1_sig);

    const short mouse_horizontal_0_sig[] = { 0xD9, 0x1D, -1, -1, -1, -1, 0x83, 0xC4, 0x08, 0xEB, 0x43, 0x3B, 0xF9, 0xC6, 0x05, -1, -1, -1, -1, 0x00, 0x8B, 0xC7, 0x7D, 0x02 };
    const short mouse_horizontal_1_sig[] = { 0xD9, 0x1D, -1, -1, -1, -1, 0x83, 0xC4, 0x08, 0xEB, 0x1A, 0x8A, 0x85, -1, -1, -1, -1, 0x8A, 0x4C, 0x24, 0x13, 0x3A, 0xC1 };
    add_signature_s2(mouse_horizontal_0_sig);
    add_signature_s2(mouse_horizontal_1_sig);

    const short joybutt_sig[] = { 0x83, 0xB8, -1, -1, -1, -1, 0xFF, 0x0F, 0x95, 0xC0, 0x84, 0xC0 };
    add_signature_s2(joybutt_sig);

    const short movement_info_sig[] = {0xD8, 0x05, -1, -1, -1, -1, 0xD8, 0x1D, -1, -1, -1, -1, 0xDF, 0xE0, 0xF6, 0xC4, 0x05, 0x7A, 0x11, 0xD9, 0x05}; // 0x006AD498
    add_signature_s2(movement_info_sig);

    set_result
}

bool find_auto_center_signature() noexcept {
    check_result

    const short auto_center_sig[] = { 0x8B, 0x51, 0x40, 0xDF, 0xE0, 0x89, 0x54, 0x24, 0x10, 0xF6, 0xC4, 0x44, 0x7A, 0x16, 0xD9, 0x05, -1, -1, -1, -1, 0xD9, 0x41, 0x44, 0xDA, 0xE9, 0xDF, 0xE0, 0xF6, 0xC4, 0x44, 0x0f, 0x8B, 0xF1, 0x00, 0x00, 0x00 };
    add_signature_s2(auto_center_sig);

    set_result
}

bool find_widescreen_scope_signature() noexcept {
    check_result

    const short widescreen_scope_sig[] = {0xC7, 0x44, 0x24, 0x38, 0x00, 0x00, 0x20, 0x44, 0xC7, 0x44, 0x24, 0x3C, 0x00, 0x00, 0xF0, 0x43, 0x7D, 0x06};
    add_signature_s2(widescreen_scope_sig);

    set_result
}

bool find_zoom_blur_signatures() noexcept {
    check_result

    const short zoom_blur_1_sig[] = {0xD9, 0x42, 0x48, 0xD9, 0x42, 0x44, 0xDA, 0xE9, 0xDF, 0xE0, 0xF6, 0xC4, 0x44};
    add_signature_s2(zoom_blur_1_sig);

    const short zoom_blur_2_sig[] = {0xD9, 0x42, 0x50, 0xD8, 0x15, -1, -1, -1, -1, 0xDF, 0xE0};
    add_signature_s2(zoom_blur_2_sig);

    const short zoom_blur_3_sig[] = {0xF6, 0xC4, 0x41, 0x75, 0x0D, 0xD9, 0x5C, 0x24, 0x18, 0x66, 0xC7, 0x44, 0x24};
    add_signature_s2(zoom_blur_3_sig);

    const short zoom_blur_4_sig[] = {0xBD, 0x02, 0x00, 0x00, 0x00, 0x89, 0x44, 0x24, 0x18, 0x0F, 0xBF, 0xC5, 0x8D};
    add_signature_s2(zoom_blur_4_sig);

    set_result
}

bool find_anisotropic_filtering_signature() noexcept {
    check_result

    const short af_is_enabled_sig[] = { 0xA1, -1, -1, -1, -1, 0x85, 0xC0, 0xBE, 0x02, 0x00, 0x00, 0x00, 0x8B, 0xFE };
    add_signature_s2(af_is_enabled_sig);

    set_result
}

bool find_debug_signatures() noexcept {
    check_result

    const short visible_object_count_sig[] = { 0x66, 0xA3, -1, -1, -1, -1, 0x8B, 0x15, -1, -1, -1, -1, 0x68, -1, -1, -1, -1, 0xB9, -1, -1, -1, -1, 0x2B, 0xCA, 0x68 };
    add_signature_s2(visible_object_count_sig);

    const short bsp_poly_count_sig[] = { 0x66, 0xFF, 0x05, -1, -1, -1, -1, 0x8B, 0x47, 0x18, 0x83, 0xC6, 0x04, 0x45, 0x0F, 0xBF, 0xCD, 0x3B, 0xC8 };
    add_signature_s2(bsp_poly_count_sig);

    const short wireframe_sig[] = { 0x8A, 0x15, -1, -1, -1, -1, 0xA1, -1, -1, -1, -1, 0x8B, 0x08, 0x83, 0xC4, 0x08, 0xF6, 0xDA };
    add_signature_s2(wireframe_sig);

    set_result
}

bool find_loading_screen_signatures() noexcept {
    check_result

    const short loading_screen_1_sig[] = {0xC7, 0x05, -1, -1, -1, -1, 0x08, 0x00, 0x00, 0x00, 0x88, 0x9E, 0xEC, 0x0A, 0x00, 0x00, 0xE9};
    add_signature_s2(loading_screen_1_sig);

    const short loading_screen_2_sig[] = {0xC7, 0x05, -1, -1, -1, -1, 0x08, 0x00, 0x00, 0x00, 0x33, 0xC0, 0x88, 0x83, 0x22, 0x0F, 0x00};
    add_signature_s2(loading_screen_2_sig);

    const short loading_screen_3_sig[] = {0xC7, 0x05, -1, -1, -1, -1, 0x08, 0x00, 0x00, 0x00, 0x8D, 0x83, 0x14, 0x0B, 0x00, 0x00, 0x8B};
    add_signature_s2(loading_screen_3_sig);

    set_result
}

bool find_multitexture_overlay_signature() noexcept {
    check_result

    const short multitexture_overlay_sig[] = {0x8B, 0x45, 0x58, 0x83, 0xC4, 0x1C, 0x33, 0xFF, 0x85, 0xC0, 0x0F, 0x8E};
    add_signature_s2(multitexture_overlay_sig);

    set_result
}

bool find_set_resolution_signatures() noexcept {
    check_result

    const short change_resolution_query_sig[] = {0x55, 0x8B, 0x6C, 0x24, 0x08, 0x56, 0x8B, 0xF0, 0x85, 0xF6, 0x57, 0x8B, 0xFD, 0xB9, 0x0E, 0x00, 0x00, 0x00};
    add_signature_s2(change_resolution_query_sig);

    const short change_resolution_sig[] = {0x0F, 0xB6, 0x15, -1, -1, -1, -1, 0xA1, -1, -1, -1, -1, 0x8B, 0x08, 0x83, 0xEC, 0x18, 0x53, 0x56, 0x57, 0x52, 0x50};
    add_signature_s2(change_resolution_sig);

    const short change_window_size_sig[] = {0x83, 0xEC, 0x20, 0x53, 0x55, 0x8B, 0x2D, -1, -1, -1, -1, 0x56, 0x8B, 0x35, -1, -1, -1, -1, 0x57, 0x8B, 0xF8, 0x8D};
    add_signature_s2(change_window_size_sig);

    const short block_auto_resolution_change_sig[] = {0xE8, 0xD3, 0xFB, 0xFF, 0xFF, 0x8B, 0xF5, 0xE8, 0x1C, 0xFE, 0xFF, 0xFF};
    add_signature_s2(block_auto_resolution_change_sig);

    set_result
}

bool find_widescreen_fix_signatures() noexcept {
    check_result

    const short hud_element_widescreen_sig[] = {0xC7, 0x84, 0x24, 0xA8, 0x00, 0x00, 0x00, 0xCD, 0xCC, 0x4C, 0x3B, 0xC7, 0x84, 0x24, 0xAC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xBC, 0x00, 0x00, 0x00, 0x89, 0x88, 0x88, 0xBB, 0xC7, 0x84, 0x24, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xD4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xC7, 0x84, 0x24, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xE4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0xC7, 0x84, 0x24, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0xF4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x89, 0x4C, 0x24, 0x4C, 0x89, 0x54, 0x24, 0x50, 0xC7, 0x44, 0x24, 0x58, 0x00, 0x00, 0x80, 0x3F, 0xD8, 0x05, 0x8C, 0x21, 0x61, 0x00, 0xD8};
    add_signature_s2(hud_element_widescreen_sig);

    const short hud_element_motion_sensor_blip_widescreen_sig[] = {0xC7, 0x44, 0x24, 0x78, 0xCD, 0xCC, 0x4C, 0x3B, 0xC7, 0x44, 0x24, 0x7C, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x84, 0x24, 0x84, 0x00, 0x00, 0x00, 0x33, 0x33, 0x80, 0xBF};
    add_signature_s2(hud_element_motion_sensor_blip_widescreen_sig);

    const short hud_text_widescreen_sig[] = {0xC7, 0x05, 0xC8, 0xCC, 0x67, 0x00, 0xCD, 0xCC, 0x4C, 0x3B, 0xC7, 0x05, 0xCC, 0xCC, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x05, 0xD0, 0xCC, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x05, 0xD4, 0xCC, 0x67, 0x00, 0x33, 0x33, 0x80, 0xBF};
    add_signature_s2(hud_text_widescreen_sig);

    const short hud_text_fix_1_sig[] = {0x7D, 0x0C, 0x8B, 0xDD, 0x2B, 0xD9, 0x89, 0x5C, 0x24, 0x1C, 0x8B, 0xCD, 0x2B, 0xF3, 0x0F, 0xBF, 0xEA};
    add_signature_s2(hud_text_fix_1_sig);

    const short hud_text_fix_2_sig[] = { 0x7D, 0x06, 0x8B, 0xD1, 0x89, 0x54, 0x24, 0x50, 0x33, 0xC9, 0x66, 0x8B, 0x08, 0x66, 0x81, 0xF9, 0x00, 0x80, 0x7E, 0x06, 0x8B, 0xD9, 0x89, 0x5C, 0x24, 0x18, 0x0F, 0xBF, 0x40, 0x04, 0x66, 0x3D, 0xFF, 0x7F, 0x7D, 0x06, 0x8B, 0xF8, 0x89, 0x7C, 0x24, 0x10, 0x85, 0xED, 0x74, 0x44, 0x33, 0xC0, 0x66, 0x8B, 0x45, 0x02, 0x66, 0x3B, 0xC6, 0x7E, 0x06, 0x8B, 0xF0, 0x89, 0x74, 0x24, 0x14, 0x33, 0xC0, 0x66, 0x8B, 0x45, 0x06, 0x66, 0x3B, 0xC2, 0x7D, 0x06, 0x8B, 0xD0, 0x89, 0x54, 0x24, 0x50, 0x33, 0xC0, 0x66, 0x8B, 0x45, 0x00, 0x66, 0x3B, 0xC3, 0x7E, 0x06, 0x8B, 0xD8, 0x89, 0x5C, 0x24, 0x18, 0x33, 0xC0, 0x66, 0x8B, 0x45, 0x04, 0x66, 0x3B, 0xC7, 0x7D, 0x06, 0x8B, 0xF8, 0x89, 0x7C, 0x24, 0x10, 0x66, 0x3B, 0xF2, 0x0F, 0x8D, 0xA6, 0x01, 0x00, 0x00, 0x66, 0x3B, 0xDF, 0x0F, 0x8D, 0x9D, 0x01, 0x00, 0x00, 0xA1, -1, -1, -1, -1, 0x66, 0x8B, 0x15, -1, -1, -1, -1, 0x8B, 0x1D, 0xF4, 0xF6, 0x67, 0x00, 0x8B, 0x4C, 0x24, 0x58, 0x68, -1, -1, -1, -1, 0x50, 0x8D, 0x74, 0x24, 0x30, 0xE8, 0xDE, 0xF9, 0xFF, 0xFF };
    add_signature_s2(hud_text_fix_2_sig);

    const short hud_text_fix_3_sig[] = { 0x7D, 0x06, 0x8B, 0xD1, 0x89, 0x54, 0x24, 0x50, 0x33, 0xC9, 0x66, 0x8B, 0x08, 0x66, 0x81, 0xF9, 0x00, 0x80, 0x7E, 0x06, 0x8B, 0xD9, 0x89, 0x5C, 0x24, 0x18, 0x0F, 0xBF, 0x40, 0x04, 0x66, 0x3D, 0xFF, 0x7F, 0x7D, 0x06, 0x8B, 0xF8, 0x89, 0x7C, 0x24, 0x10, 0x85, 0xED, 0x74, 0x44, 0x33, 0xC0, 0x66, 0x8B, 0x45, 0x02, 0x66, 0x3B, 0xC6, 0x7E, 0x06, 0x8B, 0xF0, 0x89, 0x74, 0x24, 0x14, 0x33, 0xC0, 0x66, 0x8B, 0x45, 0x06, 0x66, 0x3B, 0xC2, 0x7D, 0x06, 0x8B, 0xD0, 0x89, 0x54, 0x24, 0x50, 0x33, 0xC0, 0x66, 0x8B, 0x45, 0x00, 0x66, 0x3B, 0xC3, 0x7E, 0x06, 0x8B, 0xD8, 0x89, 0x5C, 0x24, 0x18, 0x33, 0xC0, 0x66, 0x8B, 0x45, 0x04, 0x66, 0x3B, 0xC7, 0x7D, 0x06, 0x8B, 0xF8, 0x89, 0x7C, 0x24, 0x10, 0x66, 0x3B, 0xF2, 0x0F, 0x8D, 0xA6, 0x01, 0x00, 0x00, 0x66, 0x3B, 0xDF, 0x0F, 0x8D, 0x9D, 0x01, 0x00, 0x00, 0xA1, -1, -1, -1, -1, 0x66, 0x8B, 0x15, -1, -1, -1, -1, 0x8B, 0x1D, 0xF4, 0xF6, 0x67, 0x00, 0x8B, 0x4C, 0x24, 0x58, 0x68, -1, -1, -1, -1, 0x50, 0x8D, 0x74, 0x24, 0x30, 0xE8, 0x5E, 0xF7, 0xFF, 0xFF };
    add_signature_s2(hud_text_fix_3_sig);

    const short console_text_fix_1_sig[] = { 0x66, 0x8B, 0x35, -1, -1, -1, -1, 0x33, 0xC9, 0x66, 0x8B, 0x0D, -1, -1, -1, -1, 0xF7, 0xD8, 0x66, 0xF7, 0xD9, 0xBA, 0xE0, 0x01, 0x00, 0x00, 0x2B, 0xD3, 0x66, 0x03, 0xD0, 0x05, 0xE0, 0x01, 0x00, 0x00, 0x66, 0x03, 0xF1, 0x66, 0x89, 0x54, 0x24, 0x10, 0x8B, 0x54, 0x24, 0x18, 0x66, 0x89, 0x44, 0x24, 0x14, 0x81, 0xC1, 0x80, 0x02, 0x00, 0x00 };
    add_signature_s2(console_text_fix_1_sig);

    const short console_text_fix_2_sig[] = { 0x66, 0x8B, 0x15, -1, -1, -1, -1, 0xD8, 0x4C, 0x24, 0x1C, 0xA1, -1, -1, -1, -1, 0x33, 0xC9, 0x66, 0x8B, 0x0D, -1, -1, -1, -1, 0xD9, 0x5C, 0x24, 0x1C, 0x66, 0xF7, 0xD9, 0x66, 0x03, 0xD1, 0xF7, 0xD8, 0x66, 0x8B, 0xEF, 0x2B, 0xFB, 0x66, 0x03, 0xE8, 0x66, 0x89, 0x54, 0x24, 0x12, 0x8D, 0x14, 0x38, 0x8A, 0x46, 0x0C, 0x81, 0xC1, 0x80, 0x02, 0x00, 0x00, 0x84, 0xC0, 0x66, 0x89, 0x4C, 0x24, 0x16, 0x66, 0x89, 0x54, 0x24, 0x10, 0x66, 0x89, 0x6C, 0x24, 0x14, 0x74, 0x21, 0xA1, -1, -1, -1, -1, 0x66, 0x8B, 0x0D, -1, -1, -1, -1 };
    add_signature_s2(console_text_fix_2_sig);

    const short hud_menu_sig[] = { 0x7E, 0x13, 0x0F, 0xBF, 0xCA, 0x89, 0x4C, 0x24, 0x14, 0xDB, 0x44, 0x24, 0x14, 0xD9, 0x54, 0x24, 0x3C, 0xD9, 0x5C, 0x24, 0x24, 0x66, 0x8B, 0x48, 0x06, 0x66, 0x3B, 0x4C, 0x24, 0x20, 0x7D, 0x13, 0x0F, 0xBF, 0xD1, 0x89, 0x54, 0x24, 0x20, 0xDB, 0x44, 0x24, 0x20, 0xD9, 0x54, 0x24, 0x34, 0xD9, 0x5C, 0x24, 0x2C, 0x66, 0x8B, 0x08 };
    add_signature_s2(hud_menu_sig);

    const short team_icon_ctf_sig[] = { 0x66, 0xC7, 0x44, 0x24, 0x30, 0xC8, 0x01, 0x66, 0xC7, 0x44, 0x24, 0x32, 0x06, 0x00, 0xC7, 0x44, 0x24, 0x4C, 0xCD, 0xCC, 0x0C, 0x3F, 0xC7, 0x44, 0x24, 0x50, 0xCD, 0xCC, 0x0C, 0x3F };
    add_signature_s2(team_icon_ctf_sig);

    const short team_icon_king_sig[] = { 0x66, 0xC7, 0x44, 0x24, 0x30, 0xCA, 0x01, 0xC7, 0x44, 0x24, 0x4C, 0x14, 0xAE, 0x07, 0x3F, 0xC7, 0x44, 0x24, 0x50, 0x14 };
    add_signature_s2(team_icon_king_sig);

    const short team_icon_oddball_sig[] = { 0x66, 0xC7, 0x44, 0x24, 0x30, 0xC8, 0x01, 0x66, 0xC7, 0x44, 0x24, 0x32, 0x07, 0x00, 0xC7, 0x44, 0x24, 0x4C, 0xCD, 0xCC, 0x0C, 0x3F, 0xC7, 0x44, 0x24, 0x50, 0xCD };
    add_signature_s2(team_icon_oddball_sig);

    const short team_icon_race_sig[] = { 0x66, 0xC7, 0x44, 0x24, 0x30, 0xC5, 0x01, 0x66, 0xC7, 0x44, 0x24, 0x32, 0x06, 0x00, 0xC7, 0x44, 0x24, 0x4C, 0x9A, 0x99, 0x19, 0x3F, 0xC7, 0x44, 0x24, 0x50, 0x9A };
    add_signature_s2(team_icon_race_sig);

    const short team_icon_slayer_sig[] = { 0x66, 0xC7, 0x44, 0x24, 0x30, 0xC9, 0x01, 0xC7, 0x44, 0x24, 0x4C, 0x00, 0x00, 0x00, 0x3F, 0xC7, 0x44, 0x24, 0x50, 0x00, 0x00, 0x00, 0x3F };
    add_signature_s2(team_icon_slayer_sig);

    const short team_icon_background_sig[] = { 0x66, 0xC7, 0x44, 0x24, 0x70, 0xBD, 0x01, 0x66, 0xC7, 0x44, 0x24, 0x72, 0x06, 0x00, 0xC7, 0x84, 0x24, 0xF0, 0x00, 0x00, 0x00, 0x9A, 0x99, 0x19, 0x3F, 0xC7, 0x84, 0x24, 0xF4, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3F };
    add_signature_s2(team_icon_background_sig);

    const short letterbox_sig[] = { 0x8B, 0x15, -1, -1, -1, -1, 0x8A, 0x4A, 0x08, 0x83, 0xEC, 0x34, 0x84, 0xC9, 0x53, 0x55, 0x56, 0x57, 0x75, 0x13 };
    add_signature_s2(letterbox_sig);

    const short hud_nav_widescreen_sig[] = { 0xD8, 0x0D, -1, -1, -1, -1, 0xDA, 0x04, 0x24, 0xD9, 0x19, 0x0F, 0xBF, 0x47, 0x2C, 0xD9, 0x41, 0x04, 0x89, 0x04, 0x24 };
    add_signature_s2(hud_nav_widescreen_sig);

    const short cursor_sig[] = { 0x7D, 0x08, 0x89, 0x15, -1, -1, -1, -1, 0xEB, 0x16, 0x3D, 0x80, 0x02, 0x00, 0x00, 0xC7, 0x05, -1, -1, -1, -1, 0x80, 0x02, 0x00, 0x00, 0x7F, 0x05 };
    add_signature_s2(cursor_sig);

    set_result
}

bool find_gametype_indicator_sig() noexcept {
    check_result

    const short team_icon_background_name_sig[] = { 0x68, -1, -1, -1, -1, 0xBF, 0x6D, 0x74, 0x69, 0x62, 0x89, 0x74, 0x24, 0x14, 0xE8, -1, -1, -1, -1, 0x6A, 0x00, 0x33, 0xFF };
    add_signature_s2(team_icon_background_name_sig);

    set_result
}

bool find_devmode_sig() noexcept {
    check_result

    const short devmode_sig[] = { 0x8A, 0x0D, -1, -1, -1, -1, 0x80, 0xE2, 0x07, 0x84, 0xC9, 0xB0, 0x01 };
    add_signature_s2(devmode_sig);

    set_result
}

bool find_simple_score_screen_sigs() noexcept {
    check_result

    const short ss_elements_sig[] = { 0x8B, 0x0D, -1, -1, -1, -1, 0x89, 0x44, 0x24, 0x10, 0xA1, -1, -1, -1, -1, 0x89, 0x4C, 0x24, 0x0C, 0x8B, 0x88, 0x40, 0x01, 0x00, 0x00, 0x33, 0xDB, 0x3B, 0xCB, 0x74, 0x08, 0x8B, 0x80, 0x44, 0x01, 0x00, 0x00, 0xEB, 0x02, 0x33, 0xC0, 0x3B, 0xD3, 0x8B, 0x70, 0x1C, 0x66, 0xC7, 0x44, 0x24, 0x14, 0x19, 0x00, 0x66, 0xC7, 0x44, 0x24, 0x16, 0x5A, 0x00, 0x66, 0xC7, 0x44, 0x24, 0x18, 0x2C, 0x01, 0x66, 0xC7, 0x44, 0x24, 0x1A, 0x6D, 0x01, 0x66, 0xC7, 0x44, 0x24, 0x1C, 0xAE, 0x01, 0x66, 0xC7, 0x44, 0x24, 0x1E, 0xEF, 0x01, 0x66, 0xB8, 0x30, 0x02, 0x74, 0x2F };
    add_signature_s2(ss_elements_sig);

    const short ss_score_position_sig[] = { 0x6B, 0xD2, 0x0F, 0x8B, 0x4C, 0x24, 0x2C, 0x83, 0xC2, 0x3B, 0x66, 0x89, 0x54, 0x24, 0x0C };
    add_signature_s2(ss_score_position_sig);

    const short ss_score_background_sig[] = { 0xC7, 0x44, 0x24, 0x24, 0x00, 0x00, 0x00, 0x3E, 0xC7, 0x44, 0x24, 0x28, 0x00, 0x00, 0x00, 0x3E, 0xC7, 0x44, 0x24, 0x2C, 0x00, 0x00, 0x00, 0x3E, 0x66, 0xC7, 0x44, 0x24, 0x3E, 0x0A, 0x00, 0x66, 0xC7, 0x44, 0x24, 0x42, 0x76, 0x02, 0x66, 0xC7, 0x44, 0x24, 0x40, 0x86, 0x01, 0x66, 0xC7, 0x44, 0x24, 0x3C, 0x3C, 0x00, 0xE8, -1, -1, -1, -1, 0x8D };
    add_signature_s2(ss_score_background_sig);

    set_result
}

bool find_split_screen_hud_sigs() noexcept {
    check_result

    const short ammo_counter_ss_sig[] = {0x66, 0x8B, 0x40, 0x0C, 0x33, 0xC9, 0x66, 0x83, 0x7A, 0x3C, 0x02};
    add_signature_s2(ammo_counter_ss_sig);

    const short hud_text_ss_sig[] = {0x66, 0x8B, 0x47, 0x0C, 0x66, 0x3D, 0x01, 0x00, 0x8B, 0x35};
    add_signature_s2(hud_text_ss_sig);

    const short split_screen_hud_ss_sig[] = {0x66, 0x83, 0x78, 0x0C, 0x01, 0x8B, 0x83, 0xA8, 0x02, 0x00, 0x00};
    add_signature_s2(split_screen_hud_ss_sig);

    const short hac2_workaround_ss_sig[] = {0x83, 0xEC, 0x08, 0x53, 0x55, 0x56, 0x8B, 0xF0, 0x8A, 0x46, 0x0E};
    add_signature_s2(hac2_workaround_ss_sig);

    set_result
}

bool find_mouse_sigs() noexcept {
    check_result

    const short mouse_accel_1_sig[] = { 0x7E, 0x0F, 0x83, 0xC1, 0x10, 0x40, 0x81, 0xF9, -1, -1, -1, -1, 0x7C, 0xF0, 0x5B, 0x59, 0xC3, 0xDD, 0xD8, 0xC1, 0xE0, 0x04 };
    add_signature_s2(mouse_accel_1_sig);

    const short mouse_accel_2_sig[] = { 0xD9, 0x80, -1, -1, -1, -1, 0xD8, 0xA0, -1, -1, -1, -1, 0xDE, 0xC9, 0xD8, 0x80, -1, -1, -1, -1, 0xD9, 0x44, 0x24, 0x0C, 0xD8, 0x88, -1, -1, -1, -1, 0xD8, 0x05, -1, -1, -1, -1, 0xDE, 0xC9, 0xDA, 0x4C, 0x24, 0x10, 0x5B, 0x59, 0xC3 };
    add_signature_s2(mouse_accel_2_sig);

    const short mouse_horiz_1_sig[] = { 0x8B, 0x0D, -1, -1, -1, -1, 0x50, 0x51, 0xE8, 0x13, 0xF0, 0xFF, 0xFF, 0xD9, 0xE0 };
    add_signature_s2(mouse_horiz_1_sig);

    const short mouse_horiz_2_sig[] = { 0xA1, -1, -1, -1, -1, 0x50, 0xE8, 0x3A, 0xF0, 0xFF, 0xFF };
    add_signature_s2(mouse_horiz_2_sig);

    const short mouse_vert_1_sig[] = { 0x8B, 0x0D, -1, -1, -1, -1, 0x50, 0x51, 0xE8, 0x8C, 0xF0, 0xFF, 0xFF, 0xD9, 0x1D };
    add_signature_s2(mouse_vert_1_sig);

    const short mouse_vert_2_sig[] = { 0x8B, 0x15, -1, -1, -1, -1, 0x50, 0x52, 0xE8, 0x62, 0xF0, 0xFF, 0xFF, 0xD9, 0xE0 };
    add_signature_s2(mouse_vert_2_sig);

    set_result
}
    set_result
}
