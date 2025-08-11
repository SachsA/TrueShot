#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

namespace Net {

struct BitWriter {
    std::vector<uint8_t> buf;
    void write(const void* data, size_t s) {
        const uint8_t* p = (const uint8_t*)data;
        buf.insert(buf.end(), p, p + s);
    }
    template<typename T>
    void writePOD(const T& v) {
        write(&v, sizeof(T));
    }
    void writeString(const std::string& s) {
        uint16_t n = (uint16_t)s.size();
        writePOD(n);
        if(n) write(s.data(), n);
    }
};

struct BitReader {
    const uint8_t* p;
    size_t remaining;
    BitReader(const uint8_t* data, size_t len): p(data), remaining(len) {}
    bool read(void* out, size_t s) {
        if(s > remaining) return false;
        memcpy(out, p, s);
        p += s; remaining -= s;
        return true;
    }
    template<typename T>
    bool readPOD(T& out) {
        return read(&out, sizeof(T));
    }
    bool readString(std::string& s) {
        uint16_t n;
        if(!readPOD(n)) return false;
        if(n==0) { s.clear(); return true; }
        if(n > remaining) return false;
        s.assign((const char*)p, n);
        p += n; remaining -= n;
        return true;
    }
};

} // namespace Net
