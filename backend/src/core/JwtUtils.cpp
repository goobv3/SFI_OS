#include "JwtUtils.h"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

namespace Core {

// ─────────────────────────────────────────────────────────────────
// 내부 헬퍼: JWT 시크릿
// ─────────────────────────────────────────────────────────────────
std::string JwtUtils::getSecret() {
    const char* env = std::getenv("JWT_SECRET");
    return env ? std::string(env) : "sf_secret_2026";
}

// ─────────────────────────────────────────────────────────────────
// 내부 헬퍼: HMAC-SHA256 (바이너리)
// ─────────────────────────────────────────────────────────────────
std::string JwtUtils::hmacSha256(const std::string& key, const std::string& data) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;

    HMAC(EVP_sha256(),
         key.data(),  static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()),
         static_cast<int>(data.size()),
         digest, &digest_len);

    return std::string(reinterpret_cast<char*>(digest), digest_len);
}

// ─────────────────────────────────────────────────────────────────
// 내부 헬퍼: Base64URL 인코딩/디코딩
// ─────────────────────────────────────────────────────────────────
std::string JwtUtils::base64UrlEncode(const std::string& input) {
    BIO* b64  = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input.data(), static_cast<int>(input.size()));
    BIO_flush(b64);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);

    // Base64 → Base64URL: '+' → '-', '/' → '_', 패딩 '=' 제거
    for (char& c : result) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!result.empty() && result.back() == '=') result.pop_back();
    return result;
}

std::string JwtUtils::base64UrlDecode(const std::string& input) {
    // Base64URL → Base64
    std::string b64 = input;
    for (char& c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // 패딩 복원
    while (b64.size() % 4) b64 += '=';

    BIO* b64bio  = BIO_new(BIO_f_base64());
    BIO* bmem    = BIO_new_mem_buf(b64.data(), static_cast<int>(b64.size()));
    b64bio = BIO_push(b64bio, bmem);
    BIO_set_flags(b64bio, BIO_FLAGS_BASE64_NO_NL);

    std::string out(b64.size(), '\0');
    int len = BIO_read(b64bio, out.data(), static_cast<int>(out.size()));
    BIO_free_all(b64bio);

    if (len < 0) return "";
    out.resize(static_cast<size_t>(len));
    return out;
}

// ─────────────────────────────────────────────────────────────────
// generateToken
// ─────────────────────────────────────────────────────────────────
std::string JwtUtils::generateToken(const std::string& username,
                                    const std::string& role,
                                    int expiry_hours) {
    // ── Header ──
    nlohmann::json header = {{"alg","HS256"},{"typ","JWT"}};
    std::string header_enc = base64UrlEncode(header.dump());

    // ── Payload ──
    long long now = static_cast<long long>(std::time(nullptr));
    long long exp = now + static_cast<long long>(expiry_hours) * 3600;
    nlohmann::json payload = {
        {"sub", username},
        {"role", role},
        {"iat", now},
        {"exp", exp}
    };
    std::string payload_enc = base64UrlEncode(payload.dump());

    // ── Signature ──
    std::string signing_input = header_enc + "." + payload_enc;
    std::string sig_raw       = hmacSha256(getSecret(), signing_input);
    std::string sig_enc       = base64UrlEncode(sig_raw);

    return signing_input + "." + sig_enc;
}

// ─────────────────────────────────────────────────────────────────
// verifyToken
// ─────────────────────────────────────────────────────────────────
TokenPayload JwtUtils::verifyToken(const std::string& token) {
    TokenPayload result;

    // 형식 분리
    auto dot1 = token.find('.');
    if (dot1 == std::string::npos) return result;
    auto dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) return result;

    std::string header_enc  = token.substr(0, dot1);
    std::string payload_enc = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string sig_given   = token.substr(dot2 + 1);

    // 서명 검증
    std::string signing_input = header_enc + "." + payload_enc;
    std::string sig_expected  = base64UrlEncode(hmacSha256(getSecret(), signing_input));

    if (sig_given != sig_expected) return result; // 서명 불일치

    // Payload 파싱
    try {
        std::string payload_json = base64UrlDecode(payload_enc);
        auto j = nlohmann::json::parse(payload_json);

        long long exp = j.value("exp", 0LL);
        long long now = static_cast<long long>(std::time(nullptr));
        if (exp < now) return result; // 만료

        result.valid    = true;
        result.username = j.value("sub", "");
        result.role     = j.value("role", "");
        result.exp      = exp;
    } catch (...) {
        return result;
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────
// hashPassword: PBKDF2-SHA256, hex 출력
// ─────────────────────────────────────────────────────────────────
std::string JwtUtils::hashPassword(const std::string& password,
                                   const std::string& salt,
                                   int iterations) {
    unsigned char out[32]; // SHA256 = 32 bytes
    PKCS5_PBKDF2_HMAC(
        password.c_str(), static_cast<int>(password.size()),
        reinterpret_cast<const unsigned char*>(salt.c_str()),
        static_cast<int>(salt.size()),
        iterations,
        EVP_sha256(),
        static_cast<int>(sizeof(out)),
        out
    );

    std::ostringstream oss;
    for (unsigned char b : out)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────
// verifyPassword
// ─────────────────────────────────────────────────────────────────
bool JwtUtils::verifyPassword(const std::string& password,
                              const std::string& salt,
                              const std::string& stored_hash) {
    std::string computed = hashPassword(password, salt);
    return computed == stored_hash;
}

// ─────────────────────────────────────────────────────────────────
// extractBearerToken
// ─────────────────────────────────────────────────────────────────
std::string JwtUtils::extractBearerToken(const std::string& auth_header) {
    const std::string prefix = "Bearer ";
    if (auth_header.size() > prefix.size() &&
        auth_header.substr(0, prefix.size()) == prefix) {
        return auth_header.substr(prefix.size());
    }
    return "";
}

} // namespace Core
