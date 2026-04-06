#pragma once
#include <string>

namespace Core {

/**
 * @struct TokenPayload
 * @brief JWT 검증 결과를 담는 구조체
 */
struct TokenPayload {
    bool        valid    = false;
    std::string username;
    std::string role;
    long long   exp      = 0;
};

/**
 * @class JwtUtils
 * @brief JWT HS256 생성/검증 + PBKDF2-SHA256 비밀번호 해싱 유틸리티
 *
 * 외부 JWT 라이브러리 없이 OpenSSL의 HMAC-SHA256 및 PBKDF2를 사용.
 * JWT_SECRET 환경변수를 읽어 서명 키로 사용합니다 (기본값: "sf_secret_2026").
 */
class JwtUtils {
public:
    // ── JWT ──────────────────────────────────────────────────────
    /**
     * @brief JWT 토큰 생성
     * @param username  사용자명
     * @param role      역할 (admin / operator / viewer)
     * @param expiry_hours 만료 시간 (시간 단위, 기본 24)
     */
    static std::string generateToken(const std::string& username,
                                     const std::string& role,
                                     int expiry_hours = 24);

    /**
     * @brief JWT 토큰 검증
     * @return TokenPayload { valid, username, role, exp }
     */
    static TokenPayload verifyToken(const std::string& token);

    // ── 비밀번호 해싱 ─────────────────────────────────────────────
    /**
     * @brief PBKDF2-SHA256으로 비밀번호 해싱 (hex 문자열 반환)
     * @param password  평문 비밀번호
     * @param salt      솔트 문자열
     * @param iterations  반복 횟수 (기본 10000)
     */
    static std::string hashPassword(const std::string& password,
                                    const std::string& salt,
                                    int iterations = 10000);

    /**
     * @brief 비밀번호 검증
     * @param password   평문 비밀번호
     * @param salt       DB에 저장된 솔트
     * @param stored_hash DB에 저장된 해시 (hex)
     */
    static bool verifyPassword(const std::string& password,
                               const std::string& salt,
                               const std::string& stored_hash);

    // ── Authorization 헤더 파싱 ───────────────────────────────────
    /**
     * @brief "Bearer <token>" 형태의 헤더에서 토큰 문자열 추출
     */
    static std::string extractBearerToken(const std::string& auth_header);

private:
    // HMAC-SHA256 바이너리 계산
    static std::string hmacSha256(const std::string& key, const std::string& data);
    // Base64URL 인코딩/디코딩
    static std::string base64UrlEncode(const std::string& input);
    static std::string base64UrlDecode(const std::string& input);
    // JWT 시크릿 읽기
    static std::string getSecret();
};

} // namespace Core
