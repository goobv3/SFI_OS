/**
 * @file EmailSender.cpp
 * @brief EmailSender 구현 — libcurl SMTP 이메일 전송
 *
 * ▶ libcurl SMTP 동작 원리
 *   1. curl_easy_init()으로 curl 핸들 생성
 *   2. CURLOPT_URL에 "smtp://host:port" 형태의 SMTP 서버 URL 설정
 *   3. CURLOPT_USERNAME / CURLOPT_PASSWORD로 SMTP 인증 정보 설정
 *   4. CURLOPT_USE_SSL=CURLUSESSL_ALL로 TLS 암호화 강제
 *   5. CURLOPT_MAIL_FROM : 발신자 주소
 *   6. CURLOPT_MAIL_RCPT : 수신자 목록 (curl_slist 이용)
 *   7. CURLOPT_READFUNCTION + CURLOPT_READDATA : 이메일 본문 스트리밍 콜백
 *   8. curl_easy_perform()으로 전송 실행
 *   9. curl_slist_free_all() + curl_easy_cleanup()으로 리소스 해제
 *
 * ▶ 이메일 형식 (RFC 5322)
 *   To:      <수신자>
 *   From:    <발신자>
 *   Subject: [스마트팜 경보] <센서명> <등급> 감지
 *   Content-Type: text/plain; charset=UTF-8
 *   (빈 줄)
 *   본문 내용...
 */
#include "EmailSender.h"
#include <curl/curl.h>   // libcurl SMTP API
#include <iostream>
#include <cstdlib>       // getenv
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <cstring>       // memcpy

namespace Core {
namespace EmailSender {

/**
 * @struct upload_status
 * @brief libcurl READFUNCTION 콜백에서 이메일 본문을 스트리밍하기 위한 상태 구조체
 *
 * libcurl은 이메일 본문을 청크 단위로 읽어가며 전송합니다.
 * payload는 전체 이메일 원문, lines_read는 이미 읽은 바이트 수를 나타냅니다.
 */
struct upload_status {
    int         lines_read; ///< 지금까지 전송된 바이트 수 (오프셋)
    std::string payload;    ///< 전체 이메일 원문 (RFC 5322 형식)
};

/**
 * @brief libcurl이 이메일 본문을 청크 단위로 읽기 위해 호출하는 READFUNCTION 콜백
 *
 * @param ptr    libcurl이 데이터를 기록할 버퍼
 * @param size   항상 1 (libcurl 호출 규약)
 * @param nmemb  이번 호출에서 읽어갈 최대 바이트 수
 * @param userp  upload_status 구조체 포인터 (CURLOPT_READDATA로 전달됨)
 * @return 실제로 복사한 바이트 수 (0이면 전송 완료)
 */
static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp) {
    struct upload_status* upload_ctx = (struct upload_status*)userp;

    // 버퍼 크기 0 체크
    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
        return 0;
    }

    // 아직 전송되지 않은 남은 데이터 크기 계산
    size_t len = upload_ctx->payload.length() - upload_ctx->lines_read;
    if (len > 0) {
        if (len > size * nmemb) len = size * nmemb; // 버퍼 크기 초과 방지

        // 미전송 데이터를 libcurl 버퍼로 복사
        memcpy(ptr, upload_ctx->payload.c_str() + upload_ctx->lines_read, len);
        upload_ctx->lines_read += len; // 전송한 바이트 수만큼 오프셋 증가
        return len;
    }
    return 0; // 전송 완료 신호
}

/**
 * @brief 현재 로컬 시각을 "YYYY-MM-DD HH:MM:SS" 형식의 문자열로 반환합니다.
 * @return 포맷된 시각 문자열
 */
std::string getCurrentTimeStr() {
    auto now      = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// sendAlert() — SMTP 이메일 알람 전송
//
// 동작 흐름:
//   1. 환경변수에서 SMTP 설정 읽기 (미설정 시 즉시 반환)
//   2. curl 핸들 초기화 및 SMTP 옵션 설정
//   3. 이메일 본문(RFC 5322 형식) 생성
//   4. payload_source 콜백을 통한 스트리밍 전송
//   5. 리소스 정리 및 결과 반환
// ─────────────────────────────────────────────────────────────────────────────
bool sendAlert(const std::string& sensor_alias,
               const std::string& level,
               double value,
               const std::string& message)
{
    // ── 1. SMTP 환경변수 확인 ──
    const char* smtp_host   = std::getenv("SMTP_HOST");
    const char* smtp_port   = std::getenv("SMTP_PORT");
    const char* smtp_user   = std::getenv("SMTP_USER");
    const char* smtp_pass   = std::getenv("SMTP_PASS");
    const char* alert_email = std::getenv("ALERT_EMAIL");

    // 필수 환경변수 중 하나라도 미설정이면 전송 건너뜀
    if (!smtp_host || !smtp_port || !smtp_user || !smtp_pass || !alert_email) {
        std::cerr << "[EmailSender] Missing SMTP environment variables. Skipping email alert." << std::endl;
        return false;
    }

    CURL*     curl;
    CURLcode  res        = CURLE_OK;
    struct curl_slist* recipients = NULL;

    // ── 2. curl 초기화 및 SMTP 옵션 설정 ──
    curl = curl_easy_init();
    if (curl) {
        // SMTP 서버 URL: "smtp://host:port"
        std::stringstream url;
        url << "smtp://" << smtp_host << ":" << smtp_port;
        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

        // SMTP 인증 정보
        curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_user);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_pass);

        // TLS 암호화 강제 (STARTTLS 포함)
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

        // 발신자 주소
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, smtp_user);

        // 수신자 목록 (단일 수신자, 복수 추가 시 curl_slist_append 반복)
        recipients = curl_slist_append(recipients, alert_email);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // ── 3. RFC 5322 형식 이메일 본문 생성 ──
        // 헤더와 본문은 빈 줄(\r\n\r\n)로 구분됩니다.
        std::stringstream email_body;
        email_body << "To: "                          << alert_email << "\r\n"
                   << "From: "                        << smtp_user   << "\r\n"
                   << "Subject: [스마트팜 경보] "    << sensor_alias << " " << level << " 감지\r\n"
                   << "Content-Type: text/plain; charset=UTF-8\r\n\r\n"  // 헤더/본문 구분자
                   << "스마트팜 시스템에서 위험 수위 알림이 발생했습니다.\r\n\r\n"
                   << "- 발생기기: " << sensor_alias  << "\r\n"
                   << "- 알람등급: " << level          << "\r\n"
                   << "- 현재수치: " << value          << "\r\n"
                   << "- 세부내용: " << message        << "\r\n"
                   << "- 발생시각: " << getCurrentTimeStr() << "\r\n\r\n"
                   << "시스템을 확인해 주시기 바랍니다.\r\n";

        // ── 4. 스트리밍 전송 설정 ──
        struct upload_status upload_ctx;
        upload_ctx.lines_read = 0; // 전송 오프셋 초기화
        upload_ctx.payload    = email_body.str();

        // libcurl이 payload_source 콜백을 반복 호출하여 이메일 본문을 읽어감
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA,     &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD,       1L); // 업로드 모드 활성화

        // ── 5. 전송 실행 ──
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "[EmailSender] curl_easy_perform() failed: "
                      << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "[EmailSender] Alert email successfully sent to " << alert_email << std::endl;
        }

        // ── 리소스 정리 ──
        curl_slist_free_all(recipients); // 수신자 목록 해제
        curl_easy_cleanup(curl);         // curl 핸들 해제
    }

    return (res == CURLE_OK);
}

} // namespace EmailSender
} // namespace Core
