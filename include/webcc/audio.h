#pragma once
#include "webcc.h"

namespace webcc::audio {
    enum OpCode {
        OP_CREATE_AUDIO = 0x3b,
        OP_PLAY = 0x3c,
        OP_PAUSE = 0x3d,
        OP_SET_VOLUME = 0x3e,
        OP_SET_LOOP = 0x3f,
        OP_GET_CURRENT_TIME = 0x40,
        OP_GET_DURATION = 0x41
    };

    extern "C" int32_t webcc_audio_create_audio(const char* src, uint32_t src_len);
    inline int32_t create_audio(const char* src){
        ::webcc::flush();
        return webcc_audio_create_audio(src, webcc::strlen(src));
    }

    inline void play(int32_t handle){
        push_command((uint8_t)OP_PLAY);
        push_data<int32_t>(handle);
    }

    inline void pause(int32_t handle){
        push_command((uint8_t)OP_PAUSE);
        push_data<int32_t>(handle);
    }

    inline void set_volume(int32_t handle, float vol){
        push_command((uint8_t)OP_SET_VOLUME);
        push_data<int32_t>(handle);
        push_data<float>(vol);
    }

    inline void set_loop(int32_t handle, uint8_t loop){
        push_command((uint8_t)OP_SET_LOOP);
        push_data<int32_t>(handle);
        push_data<uint8_t>(loop);
    }

    extern "C" float webcc_audio_get_current_time(int32_t handle);
    inline float get_current_time(int32_t handle){
        ::webcc::flush();
        return webcc_audio_get_current_time(handle);
    }

    extern "C" float webcc_audio_get_duration(int32_t handle);
    inline float get_duration(int32_t handle){
        ::webcc::flush();
        return webcc_audio_get_duration(handle);
    }

} // namespace webcc::audio

