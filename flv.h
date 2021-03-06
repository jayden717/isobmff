#ifndef FLV_H_
#define FLV_H_

#include <istream>
#include <ostream>
#include <vector>
#include <stdint.h>

namespace flv {

const static uint8_t TAG_TYPE_AUDIO = 8;
const static uint8_t TAG_TYPE_VIDEO = 9;
const static uint8_t TAG_TYPE_SCRIPT = 18;

const static uint8_t VCODEC_H263 = 2;
const static uint8_t VCODEC_VP6 = 4;
const static uint8_t VCODEC_VP6A = 5;
const static uint8_t VCODEC_AVC = 7;

const static uint8_t ACODEC_ADPCM = 1;
const static uint8_t ACODEC_MP3 = 2;
const static uint8_t ACODEC_PCM = 3;
const static uint8_t ACODEC_NELLYMOSER = 4;
const static uint8_t ACODEC_NELLYMOSER_16K = 5;
const static uint8_t ACODEC_NELLYMOSER_8K = 6;
const static uint8_t ACODEC_AAC = 10;
const static uint8_t ACODEC_SPEEX = 11;
const static uint8_t ACODEC_MP3_8K = 14;

const static uint8_t SOUND_RATE_5K = 0;
const static uint8_t SOUND_RATE_11K = 1;
const static uint8_t SOUND_RATE_22K = 2;
const static uint8_t SOUND_RATE_44K = 3;

const static uint8_t SOUND_SAMPLE_SIZE_8 = 0;
const static uint8_t SOUND_SAMPLE_SIZE_16 = 1;

const static uint8_t TYPE_FLAG_VIDEO = 1;
const static uint8_t TYPE_FLAG_AUDIO = 4;

struct FLVHeader {
    char signature[3];
    uint8_t version;
    uint8_t type_flags;
    uint32_t data_offset;
};

struct FLVTagHeader {
    uint8_t type;
    uint32_t size; // 24bit data size
    uint32_t timestamp; // 24bit + 8bit;
    uint32_t stream_id; // 24bit
    // data...
};

// utils
static inline uint32_t read32(std::istream &is) {
    uint8_t buf[4];
    is.read((char*)&buf[0],sizeof(buf));
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

static inline uint32_t read24(std::istream &is) {
    uint8_t buf[3];
    is.read((char*)&buf[0],sizeof(buf));
    return (buf[0] << 16) | (buf[1] << 8) | buf[2];
}
static inline uint8_t read8(std::istream &is) {
    uint8_t buf[1];
    is.read((char*)&buf[0],sizeof(buf));
    return buf[0];
}
static inline void write8(std::ostream &is, uint8_t d) {
    char buf[] = {d};
    is.write(buf, 1);
}
static inline void write24(std::ostream &is, uint32_t d) {
    char buf[] = {d >> 16, d >> 8, d};
    is.write(buf, 3);
}
static inline void write32(std::ostream &is, uint32_t d) {
    char buf[] = {d >> 24, d >> 16, d >> 8, d};
    is.write(buf, 4);
}
static inline void write64(std::ostream &os, uint64_t d) {
    write32(os, d >> 32);
    write32(os, d);
}

inline static void parse(FLVHeader &h, std::istream &is) {
    is.read(h.signature, 3);
    h.version = read8(is);
    h.type_flags = read8(is);
    h.data_offset = read32(is);
}

inline static void parse(FLVTagHeader &th, std::istream &is) {
    th.type = read8(is);
    th.size = read24(is);
    th.timestamp = read24(is);
    th.timestamp |= (read8(is) << 24);
    th.stream_id = read24(is);
}

inline static int skip_data(FLVTagHeader &th, std::istream &is) {
    is.seekg(th.size,  std::ios_base::cur); // skip
    return th.size;
}

template <typename T>
inline static void write_video(std::ostream &os, FLVTagHeader &th, const T &buf, uint8_t codec, int timeOffset, bool key, bool header = false) {
    th.size = buf.size() + (codec == VCODEC_AVC? 5 : 1);
    os << th;
    write8(os, (key? 0x10 : 0x20) | codec);
    if (codec == VCODEC_AVC) {
        write8(os, header? 0x00 : 0x01);
        write24(os, timeOffset);
    }
    os.write((char*)&buf[0], buf.size());
}

inline static uint8_t audio_format(uint8_t codec, uint8_t channels, uint8_t sound_rate) {
	uint8_t sample_size = SOUND_SAMPLE_SIZE_16; // TODO: sample size.
    return (codec << 4) | (sound_rate << 2) | (sample_size << 1) | (channels==2?1:0);
}

template <typename T>
inline static void write_audio(std::ostream &os, FLVTagHeader &th, const T &buf, uint8_t aformat, bool header = false) {
    th.size = buf.size() + ((aformat>>4) == ACODEC_AAC? 2 : 1);
    os << th;
    write8(os, aformat);
    if ((aformat>>4) == ACODEC_AAC) {
        write8(os, header? 0x00 : 0x01);
    }
    os.write((char*)&buf[0], buf.size());
}

static inline std::ostream& operator<<(std::ostream &os, const FLVHeader& fh) {
    os.write(fh.signature, 3);
    write8(os, fh.version);
    write8(os, fh.type_flags);
    write32(os, fh.data_offset);
    return os;
}

static inline std::ostream& operator<<(std::ostream &os, const FLVTagHeader& th) {
    write8(os, th.type);
    write24(os, th.size);
    write24(os, th.timestamp);
    write8(os, th.timestamp >> 24);
    write24(os, th.stream_id);
    return os;
}

} // namespace flv

#endif
