/**
 * @file JwtUtils.cpp
 * @brief JwtUtils 클래스 구현 — OpenSSL 기반 JWT HS256 + PBKDF2-SHA256
 *
 * ▶ 의존 OpenSSL 함수 목록
 *   - HMAC()              : HMAC-SHA256 계산 (hmacSha256에서 사용)
 *   - PKCS5_PBKDF2_HMAC() : RFC 2898 기반 키 파생 함수 (hashPassword에서 사용)
 *   - BIO_*               : Base64 인코딩/디코딩 (OpenSSL BIO 체인 방식)
 *   - EVP_sha256()        : SHA-256 다이제스트 선택자
 *
 * ▶ 보안 설계 포인트
 *   1. HMAC-SHA256 서명으로 토큰 위변조 방지
 *   2. Base64URL 인코딩으로 URL/쿠키 안전 전송
 *   3. exp(만료) 클레임으로 토큰 유효기간 제한
 *   4. PBKDF2로 레인보우 테이블 공격 방어
 *   5. 솔트로 동일 비밀번호의 해시값 차별화
 */
#include "JwtUtils.h"
#include <openssl/hmac.h>    // HMAC()
#include <openssl/evp.h>     // EVP_sha256(), PKCS5_PBKDF2_HMAC()
#include <openssl/sha.h>     // SHA256 상수
#include <openssl/bio.h>     // BIO (Base64 체인)
#include <openssl/buffer.h>  // BUF_MEM
#include <nlohmann/json.hpp> // JWT Payload JSON 직렬화
#include <cstdlib>   // getenv
#include <ctime>     // time()
#include <cstring>   // strlen
#include <sstream>   // ostringstream
#include <iomanip>   // hex, setw, setfill
#include <stdexcept> // exception
#include <algorithm> // remove (Base64 처리)

namespace Core {

// ─────────────────────────────────────────────────────────────────────────────
// getSecret — JWT 서명 키 조회
//
// 환경변수 JWT_SECRET이 설정되면 해당 값을, 없으면 기본값을 사용합니다.
// ⚠️ 운영 환경에서는 반드시 JWT_SECRET 환경변수를 강력한 랜덤 키로 설정하세요!
// ─────────────────────────────────────────────────────────────────────────────
std::string JwtUtils::getSecret() {
    const char* env = std::getenv("JWT_SECRET");
    return env ? std::string(env) : "sf_secret_2026"; // 기본값: 개발용 (운영 불가)
}

// ─────────────────────────────────────────────────────────────────────────────
// hmacSha256 — OpenSSL HMAC-SHA256 바이너리 계산
//
// OpenSSL HMAC() 함수를 직접 사용하여 외부 JWT 라이브러리 없이 구현합니다.
// 반환값은 32바이트 바이너리 문자열 (std::string으로 래핑됨)
// ─────────────────────────────────────────────────────────────────────────────
std::string JwtUtils::hmacSha256(const std::string& key, const std::string& data) {
    unsigned char digest[EVP_MAX_MD_SIZE]; // 최대 해시 크기(64바이트) 버퍼
    unsigned int  digest_len = 0;

    // HMAC-SHA256 계산
    // 파라미터: 해시함수, 키, 키길이, 데이터, 데이터길이, 결과버퍼, 결과길이포인터
    HMAC(EVP_sha256(),
         key.data(),  static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()),
         static_cast<int>(data.size()),
         digest, &digest_len);

    // 바이너리 결과를 std::string으로 래핑하여 반환
    return std::string(reinterpret_cast<char*>(digest), digest_len);
}

// ─────────────────────────────────────────────────────────────────────────────
// base64UrlEncode — Base64 → Base64URL 변환 (OpenSSL BIO 체인 사용)
//
// 동작:
//   1. BIO_f_base64() + BIO_s_mem() 체인으로 Base64 인코딩
//   2. 표준 Base64 특수문자 변환: '+' → '-', '/' → '_'
//   3. 패딩 '=' 제거 (RFC 7515 §2 요구사항)
//
// BIO_FLAGS_BASE64_NO_NL: 개행문자 없이 단일 라인으로 인코딩
// ─────────────────────────────────────────────────────────────────────────────
std::string JwtUtils::base64UrlEncode(const std::string& input) {
    // Base64 필터 BIO + 메모리 BIO 체인 구성
    BIO* b64  = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem); // b64 → bmem 체인: 인코딩 결과가 메모리에 축적됨

    // 개행 없이 단일 라인으로 인코딩
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    // 데이터 쓰기 및 플러시
    BIO_write(b64, input.data(), static_cast<int>(input.size()));
    BIO_flush(b64);

    // 메모리 BIO에서 결과 읽기
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64); // 체인 전체 메모리 해제

    // Base64 → Base64URL 변환
    for (char& c : result) {
        if (c == '+') c = '-';  // URL 안전 문자로 교체
        else if (c == '/') c = '_'; // URL 안전 문자로 교체
    }
    // 패딩 '=' 제거 (JWT 표준 요구사항)
    while (!result.empty() && result.back() == '=') result.pop_back();

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// base64UrlDecode — Base64URL → 바이너리 디코딩 (OpenSSL BIO 체인 사용)
//
// 동작:
//   1. '-' → '+', '_' → '/' (URL → 표준 Base64 역변환)
//   2. 4의 배수 패딩 '=' 복원
//   3. BIO_new_mem_buf() + BIO_f_base64() 체인으로 디코딩
// ─────────────────────────────────────────────────────────────────────────────
std::string JwtUtils::base64UrlDecode(const std::string& input) {
    // Base64URL → 표준 Base64 역변환
    std::string b64 = input;
    for (char& c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // 4의 배수 패딩 복원 (디코딩에 필요)
    while (b64.size() % 4) b64 += '=';

    // 메모리 버퍼 + Base64 필터 체인으로 디코딩
    BIO* b64bio = BIO_new(BIO_f_base64());
    BIO* bmem   = BIO_new_mem_buf(b64.data(), static_cast<int>(b64.size()));
    b64bio = BIO_push(b64bio, bmem);
    BIO_set_flags(b64bio, BIO_FLAGS_BASE64_NO_NL);

    std::string out(b64.size(), '\0');
    int len = BIO_read(b64bio, out.data(), static_cast<int>(out.size()));
    BIO_free_all(b64bio);

    if (len < 0) return "";
    out.resize(static_cast<size_t>(len)); // 실제 디코딩 길이로 조정
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// generateToken — JWT HS256 토큰 생성
//
// 구조:
//   ① Header:    {"alg":"HS256","typ":"JWT"}  → Base64URL 인코딩
//   ② Payload:   {"sub":username,"role":role,"iat":now,"exp":now+expiry}  → Base64URL 인코딩
//   ③ Signature: HMAC-SHA256(① + "." + ②, JWT_SECRET)  → Base64URL 인코딩
//   결과 토큰:   ① + "." + ② + "." + ③
// ─────────────────────────────────────────────────────────────────────────────
std::string JwtUtils::generateToken(const std::string& username,
                                    const std::string& role,
                                    int expiry_hours) {
    // ── ① Header ──
    nlohmann::json header = {{"alg","HS256"}, {"typ","JWT"}};
    std::string header_enc = base64UrlEncode(header.dump());

    // ── ② Payload ──
    long long now = static_cast<long long>(std::time(nullptr));    // 현재 Unix 타임스탬프
    long long exp = now + static_cast<long long>(expiry_hours) * 3600; // 만료 타임스탬프
    nlohmann::json payload = {
        {"sub",  username}, // 사용자명 (subject)
        {"role", role},     // 역할
        {"iat",  now},      // 발급 시각 (issued at)
        {"exp",  exp}       // 만료 시각 (expiry)
    };
    std::string payload_enc = base64UrlEncode(payload.dump());

    // ── ③ Signature ──
    std::string signing_input = header_enc + "." + payload_enc; // 서명 대상
    std::string sig_raw       = hmacSha256(getSecret(), signing_input); // HMAC 계산
    std::string sig_enc       = base64UrlEncode(sig_raw); // Base64URL 인코딩

    // 최종 JWT = header.payload.signature
    return signing_input + "." + sig_enc;
}

// ─────────────────────────────────────────────────────────────────────────────
// verifyToken — JWT 검증
//
// 검증 단계:
//   1. 점(.) 기준 3부분 분리 → 형식 검사
//   2. 서명 재계산 → 제출된 서명과 비교 (위변조 검사)
//   3. exp 클레임과 현재 시각 비교 → 만료 검사
//   4. sub, role 클레임 추출 → TokenPayload 반환
// ─────────────────────────────────────────────────────────────────────────────
TokenPayload JwtUtils::verifyToken(const std::string& token) {
    TokenPayload result; // 기본값: valid=false

    // 1. 형식 분리: header.payload.signature
    auto dot1 = token.find('.');
    if (dot1 == std::string::npos) return result;
    auto dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) return result;

    std::string header_enc  = token.substr(0, dot1);
    std::string payload_enc = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string sig_given   = token.substr(dot2 + 1); // 제출된 서명

    // 2. 서명 재계산 및 비교
    std::string signing_input = header_enc + "." + payload_enc;
    std::string sig_expected  = base64UrlEncode(hmacSha256(getSecret(), signing_input));

    if (sig_given != sig_expected) return result; // 서명 불일치 → 위변조 의심

    // 3. Payload 파싱 및 만료 검사
    try {
        std::string payload_json = base64UrlDecode(payload_enc);
        auto j = nlohmann::json::parse(payload_json);

        long long exp = j.value("exp", 0LL);
        long long now = static_cast<long long>(std::time(nullptr));
        if (exp < now) return result; // 만료된 토큰

        // 4. 클레임 추출
        result.valid    = true;
        result.username = j.value("sub",  "");
        result.role     = j.value("role", "");
        result.exp      = exp;
    } catch (...) {
        return result; // JSON 파싱 오류 → invalid
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// hashPassword — PBKDF2-SHA256 비밀번호 해싱
//
// PKCS5_PBKDF2_HMAC 파라미터:
//   - password/pass_len : 평문 비밀번호
//   - salt/salt_len     : 솔트 (DB에 별도 저장)
//   - iter              : 반복 횟수 (기본 10,000 — 높을수록 느리지만 안전)
//   - digest            : SHA-256 다이제스트
//   - keylen/out        : 출력 32바이트 버퍼
//
// hex 변환: 각 바이트를 2자리 소문자 hex로 변환 → 64자 문자열
// ─────────────────────────────────────────────────────────────────────────────
std::string JwtUtils::hashPassword(const std::string& password,
                                   const std::string& salt,
                                   int iterations) {
    unsigned char out[32]; // SHA256 출력 = 32바이트

    PKCS5_PBKDF2_HMAC(
        password.c_str(), static_cast<int>(password.size()),      // 비밀번호
        reinterpret_cast<const unsigned char*>(salt.c_str()),
        static_cast<int>(salt.size()),                             // 솔트
        iterations,                                                // 반복 횟수
        EVP_sha256(),                                              // 해시 함수
        static_cast<int>(sizeof(out)),                             // 출력 길이
        out                                                        // 출력 버퍼
    );

    // 바이너리 → 64자 소문자 hex 문자열 변환
    std::ostringstream oss;
    for (unsigned char b : out)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// verifyPassword — 평문 비밀번호와 DB 저장 해시 비교
//
// hashPassword()로 입력 비밀번호를 해싱한 후 stored_hash와 단순 문자열 비교합니다.
// (타이밍 공격 방어가 필요한 경우 CRYPTO_memcmp 사용 권장)
// ─────────────────────────────────────────────────────────────────────────────
bool JwtUtils::verifyPassword(const std::string& password,
                              const std::string& salt,
                              const std::string& stored_hash) {
    std::string computed = hashPassword(password, salt); // 동일 솔트로 재해싱
    return computed == stored_hash; // 결과 비교
}

// ─────────────────────────────────────────────────────────────────────────────
// extractBearerToken — Authorization 헤더에서 JWT 토큰 추출
//
// 입력:  "Bearer eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJhZG1pbiJ9.xxxx"
// 출력:  "eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJhZG1pbiJ9.xxxx"
//
// "Bearer " 프리픽스(7자)가 없으면 빈 문자열 반환 → requireAuth가 invalid 처리
// ─────────────────────────────────────────────────────────────────────────────
std::string JwtUtils::extractBearerToken(const std::string& auth_header) {
    const std::string prefix = "Bearer ";
    if (auth_header.size() > prefix.size() &&
        auth_header.substr(0, prefix.size()) == prefix) {
        return auth_header.substr(prefix.size()); // "Bearer " 이후의 문자열 반환
    }
    return ""; // 형식 불일치
}

} // namespace Core
