/**
 * @file EmailSender.h
 * @brief libcurl 기반 SMTP 이메일 알림 전송 모듈
 *
 * ▶ 역할
 *   - 스마트팜 시스템에서 CRITICAL 등급 알람이 발생했을 때 이메일로 관리자에게 알립니다.
 *   - SensorManager::checkAlarms()에서 CRITICAL_HIGH 또는 CRITICAL_LOW 알람 시 호출됩니다.
 *
 * ▶ 환경변수 (필수 — 미설정 시 이메일 전송 건너뜀)
 *   SMTP_HOST   : SMTP 서버 호스트 (예: smtp.gmail.com)
 *   SMTP_PORT   : SMTP 포트 (예: 587 for STARTTLS, 465 for SSL)
 *   SMTP_USER   : 발신자 이메일 주소 (예: farm@example.com)
 *   SMTP_PASS   : SMTP 인증 비밀번호 또는 앱 비밀번호
 *   ALERT_EMAIL : 수신자 이메일 주소 (예: admin@example.com)
 *
 * ▶ 의존성
 *   - libcurl (curl/curl.h): SMTP 과 SMTP over SSL/TLS를 지원
 *
 * ▶ 커스터마이징 가이드
 *   1. 다수 수신자 지원: curl_slist_append()를 여러 번 호출하여 recipients 목록 확장
 *   2. HTML 이메일: Content-Type을 "text/html; charset=UTF-8"로 변경
 *   3. 특정 알람등급 필터링: sendAlert()에서 level 파라미터 조건 추가
 *   4. 슬랙/텔레그램 알림: libcurl HTTP POST로 Webhook URL 호출 방식으로 확장 가능
 */
#pragma once

#include <string>

namespace Core {
namespace EmailSender {

/**
 * @brief SMTP를 통해 알람 이메일을 전송합니다.
 * @param sensor_alias 경보가 발생한 센서의 별칭 (예: "온실1 온도센서")
 * @param level        알람 등급 문자열 (예: "CRITICAL_HIGH", "CRITICAL_LOW")
 * @param value        경보 발생 시 측정값 (예: 45.3)
 * @param message      상세 설명 (예: "Critical High limit exceeded")
 * @return true  이메일 전송 성공
 * @return false SMTP 환경변수 미설정 또는 전송 오류
 *
 * @note 환경변수 SMTP_HOST, SMTP_PORT, SMTP_USER, SMTP_PASS, ALERT_EMAIL이
 *       모두 설정되어 있어야 실제 전송이 이루어집니다.
 *
 * @example
 * @code
 * // SensorManager::checkAlarms()에서 사용 예시
 * Core::EmailSender::sendAlert("온실1 온도센서", "CRITICAL_HIGH", 52.3, "Critical High limit exceeded");
 * @endcode
 */
bool sendAlert(const std::string& sensor_alias,
               const std::string& level,
               double value,
               const std::string& message);

} // namespace EmailSender
} // namespace Core
