#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace zqv {

inline unsigned hex_nibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
    throw std::runtime_error("invalid hexadecimal character");
}

inline std::vector<std::uint8_t> from_hex(const std::string& text) {
    if (text.size() % 2 != 0) throw std::runtime_error("odd hexadecimal length");
    std::vector<std::uint8_t> out(text.size() / 2);
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<std::uint8_t>((hex_nibble(text[i * 2]) << 4) | hex_nibble(text[i * 2 + 1]));
    }
    return out;
}

template <std::size_t N>
inline std::array<std::uint8_t, N> array_from_hex(const std::string& text) {
    const auto bytes = from_hex(text);
    if (bytes.size() != N) throw std::runtime_error("unexpected hexadecimal size");
    std::array<std::uint8_t, N> out{};
    std::copy(bytes.begin(), bytes.end(), out.begin());
    return out;
}

inline std::string to_hex(const std::uint8_t* data, std::size_t size) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out(size * 2, '0');
    for (std::size_t i = 0; i < size; ++i) {
        out[i * 2] = digits[data[i] >> 4];
        out[i * 2 + 1] = digits[data[i] & 0x0f];
    }
    return out;
}

template <typename Container>
inline std::string to_hex(const Container& data) {
    return to_hex(reinterpret_cast<const std::uint8_t*>(data.data()), data.size());
}

inline bool has_zqvx_pow_prefix(const std::vector<std::uint8_t>& blob) {
    static constexpr std::array<std::uint8_t, 8> prefix{{'Z', 'Q', 'V', 'X', 'P', 'O', 'W', 1}};
    return blob.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), blob.begin());
}

inline std::size_t canonical_nonce_offset(const std::vector<std::uint8_t>& blob) {
    std::size_t offset = 0;
    for (int field = 0; field < 3; ++field) {
        int count = 0;
        while (true) {
            if (offset >= blob.size() || ++count > 10) throw std::runtime_error("invalid CryptoNote header varint");
            const auto byte = blob[offset++];
            if ((byte & 0x80) == 0) break;
        }
    }
    offset += 32;
    if (offset + 4 > blob.size()) throw std::runtime_error("block blob is too short for nonce");
    return offset;
}

inline Endpoint parse_endpoint(const std::string& value) {
    const auto pos = value.rfind(':');
    if (pos == std::string::npos || pos == 0 || pos + 1 == value.size()) {
        throw std::runtime_error("endpoint must use HOST:PORT");
    }
    return {value.substr(0, pos), value.substr(pos + 1)};
}

} // namespace zqv
