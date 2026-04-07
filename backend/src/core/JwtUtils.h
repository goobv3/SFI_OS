/**
 * @file JwtUtils.h
 * @brief JWT HS256 토큰 생성/검증 및 PBKDF2-SHA256 비밀번호 해싱 유틸리티
 *
 * ▶ 역할
 *   - 외부 JWT 라이브러리 없이 OpenSSL의 HMAC-SHA256, PKCS5_PBKDF2_HMAC 함수만으로 구현합니다.
 *   - 로그인 API(/api/auth/login)에서 JWT 토큰을 발급하고,
 *     인증이 필요한 모든 API에서 requireAuth()를 통해 토큰을 검증합니다.
 *   - 비밀번호는 PBKDF2-SHA256(10,000회 반복) + 솔트 방식으로 안전하게 저장됩니다.
 *
 * ▶ JWT 토큰 구조 (HS256)
 *   Header:  {"alg":"HS256","typ":"JWT"}
 *   Payload: {"sub":"<username>","role":"<role>","iat":<issued_at>,"exp":<expiry>}
 *   Signature: HMAC-SHA256(Base64URL(header) + "." + Base64URL(payload), JWT_SECRET)
 *   토큰 = Base64URL(header) + "." + Base64URL(payload) + "." + Base64URL(signature)
 *
 * ▶ 비밀번호 해싱 방식
 *   - 알고리즘: PBKDF2-SHA256
 *   - 반복 횟수: 10,000회 (iterations 파라미터로 조정 가능)
 *   - 출력 형식: 64자 소문자 hexadecimal 문자열 (32바이트)
 *   - 저장 위치: DB users 테이블의 password_hash + password_salt 컬럼
 *
 * ▶ 환경변수
 *   JWT_SECRET      : HMAC-SHA256 서명 키 (기본값: "sf_secret_2026") — 운영 시 반드시 변경!
 *   JWT_EXPIRY_HOURS: 토큰 유효 시간(시간 단위, 기본값: 24)
 *
 * ▶ 의존성
 *   - OpenSSL (libssl, libcrypto): HMAC, PKCS5_PBKDF2_HMAC, BIO/BUF_MEM
 *   - nlohmann/json: JWT Payload JSON 직렬화/역직렬화
 *
 * ▶ 커스터마이징 가이드
 *   1. 알고리즘 변경(RS256 등): hmacSha256()을 RSA 개인키 서명으로 교체
 *   2. 토큰 갱신(Refresh Token): generateToken()으로 refresh token 발급 후 별도 저장
 *   3. 반복 횟수 증가: hashPassword()의 iterations 기본값 상향 (보안 강도 향상, 속도 감소)
 */
#pragma once
#include <string>

namespace Core {

/**
 * @struct TokenPayload
 * @brief JWT 검증(verifyToken) 결과를 담는 구조체
 *
 * valid가 false인 경우 나머지 필드는 기본값(빈 문자열, 0)입니다.
 */
struct TokenPayload {
    bool        valid    = false; ///< 서명 유효 AND 만료 미도달 시 true
    std::string username;         ///< JWT sub 클레임 (사용자명)
    std::string role;             ///< JWT role 클레임 (admin / operator / viewer)
    long long   exp      = 0;     ///< 만료 Unix 타임스탬프 (초)
};

/**
 * @class JwtUtils
 * @brief JWT HS256 생성/검증 + PBKDF2-SHA256 비밀번호 해싱 유틸리티 (정적 메서드만 보유)
 *
 * 인스턴스화 없이 정적 메서드로만 사용합니다.
 *
 * @code
 * // 토큰 발급
 * auto token = Core::JwtUtils::generateToken("admin", "admin");
 *
 * // 토큰 검증
 * auto payload = Core::JwtUtils::verifyToken(token);
 * if (payload.valid) { ... }
 *
 * // 비밀번호 해싱 및 검증
 * auto hash = Core::JwtUtils::hashPassword("password", "salt");
 * bool ok   = Core::JwtUtils::verifyPassword("password", "salt", hash);
 * @endcode
 */
class JwtUtils {
public:
    // ── JWT 토큰 관련 ──────────────────────────────────────────────────────────

    /**
     * @brief JWT HS256 토큰을 생성합니다.
     * @param username     사용자명 (JWT sub 클레임에 저장)
     * @param role         역할 문자열 (admin / operator / viewer)
     * @param expiry_hours 만료까지의 시간 (기본값: 24시간)
     * @return JWT 문자열 (header.payload.signature 형식)
     *
     * @note JWT_SECRET 환경변수가 설정되어 있어야 합니다.
     *       운영 환경에서는 충분히 길고 랜덤한 시크릿을 사용하세요.
     */
    static std::string generateToken(const std::string& username,
                                     const std::string& role,
                                     int expiry_hours = 24);

    /**
     * @brief JWT 토큰을 검증하고 페이로드를 반환합니다.
     * @param token 검증할 JWT 문자열
     * @return TokenPayload { valid, username, role, exp }
     *         서명 불일치 또는 만료 시 valid=false
     *
     * @note 형식 오류(점 구분자 누락), 서명 불일치, 만료 중 하나라도 해당하면 valid=false
     */
    static TokenPayload verifyToken(const std::string& token);

    // ── 비밀번호 해싱 ─────────────────────────────────────────────────────────

    /**
     * @brief PBKDF2-SHA256으로 비밀번호를 해싱하여 hex 문자열로 반환합니다.
     * @param password   평문 비밀번호
     * @param salt       솔트 문자열 (DB에 별도 저장)
     * @param iterations 반복 횟수 (기본값: 10,000 — 높을수록 보안 강도 증가)
     * @return 64자 소문자 hexadecimal 문자열 (32바이트 = SHA256 출력 길이)
     */
    static std::string hashPassword(const std::string& password,
                                    const std::string& salt,
                                    int iterations = 10000);

    /**
     * @brief 입력 비밀번호와 DB 저장 해시를 비교합니다.
     * @param password    평문 비밀번호
     * @param salt        DB에 저장된 솔트
     * @param stored_hash DB에 저장된 해시 (hex 문자열)
     * @return true 일치, false 불일치
     *
     * @note 내부적으로 hashPassword()를 호출하여 결과를 stored_hash와 비교합니다.
     */
    static bool verifyPassword(const std::string& password,
                               const std::string& salt,
                               const std::string& stored_hash);

    // ── Authorization 헤더 파싱 ───────────────────────────────────────────────

    /**
     * @brief "Bearer <token>" 형태의 Authorization 헤더에서 토큰 문자열을 추출합니다.
     * @param auth_header HTTP Authorization 헤더 값 전체
     * @return 추출된 토큰 문자열. "Bearer " 접두사가 없으면 빈 문자열 반환.
     *
     * @example "Bearer eyJhbGc..." → "eyJhbGc..."
     */
    static std::string extractBearerToken(const std::string& auth_header);

private:
    /**
     * @brief OpenSSL HMAC-SHA256으로 바이너리 서명을 계산합니다.
     * @param key  서명 키 (JWT_SECRET)
     * @param data 서명 대상 데이터 (header_enc + "." + payload_enc)
     * @return 32바이트 바이너리 문자열
     */
    static std::string hmacSha256(const std::string& key, const std::string& data);

    /**
     * @brief 입력 바이너리 문자열을 Base64URL 형식으로 인코딩합니다.
     * @note '+' → '-', '/' → '_', 패딩 '=' 제거 (RFC 7515 §2)
     */
    static std::string base64UrlEncode(const std::string& input);

    /**
     * @brief Base64URL 문자열을 디코딩하여 바이너리 문자열로 반환합니다.
     * @note '-' → '+', '_' → '/', 4의 배수 패딩 복원 후 디코딩
     */
    static std::string base64UrlDecode(const std::string& input);

    /**
     * @brief 환경변수 JWT_SECRET에서 서명 키를 읽습니다.
     * @return 서명 키 문자열 (미설정 시 "sf_secret_2026" 반환)
     */
    static std::string getSecret();
};

} // namespace Core
