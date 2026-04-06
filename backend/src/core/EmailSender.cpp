#include "EmailSender.h"
#include <curl/curl.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <cstring>

namespace Core {
namespace EmailSender {

struct upload_status {
  int lines_read;
  std::string payload;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;

  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }

  size_t len = upload_ctx->payload.length() - upload_ctx->lines_read;
  if(len > 0) {
    if(len > size * nmemb) len = size * nmemb;
    memcpy(ptr, upload_ctx->payload.c_str() + upload_ctx->lines_read, len);
    upload_ctx->lines_read += len;
    return len;
  }
  return 0;
}

std::string getCurrentTimeStr() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool sendAlert(const std::string& sensor_alias, const std::string& level, double value, const std::string& message) {
    const char* smtp_host = std::getenv("SMTP_HOST");
    const char* smtp_port = std::getenv("SMTP_PORT");
    const char* smtp_user = std::getenv("SMTP_USER");
    const char* smtp_pass = std::getenv("SMTP_PASS");
    const char* alert_email = std::getenv("ALERT_EMAIL");

    if (!smtp_host || !smtp_port || !smtp_user || !smtp_pass || !alert_email) {
        std::cerr << "[EmailSender] Missing SMTP environment variables. Skipping email alert." << std::endl;
        return false;
    }

    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;

    curl = curl_easy_init();
    if (curl) {
        std::stringstream url;
        url << "smtp://" << smtp_host << ":" << smtp_port;
        curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());

        curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_user);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_pass);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, smtp_user);
        recipients = curl_slist_append(recipients, alert_email);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        std::stringstream email_body;
        email_body << "To: " << alert_email << "\r\n"
                   << "From: " << smtp_user << "\r\n"
                   << "Subject: =?utf-8?B?";
                   
        // 간단한 텍스트 렌더링으로 헤더 포함
        email_body.str("");
        email_body << "To: " << alert_email << "\r\n"
                   << "From: " << smtp_user << "\r\n"
                   << "Subject: [스마트팜 경보] " << sensor_alias << " " << level << " 감지\r\n"
                   << "Content-Type: text/plain; charset=UTF-8\r\n\r\n"
                   << "스마트팜 시스템에서 위험 수위 알림이 발생했습니다.\r\n\r\n"
                   << "- 발생기기: " << sensor_alias << "\r\n"
                   << "- 알람등급: " << level << "\r\n"
                   << "- 현재수치: " << value << "\r\n"
                   << "- 세부내용: " << message << "\r\n"
                   << "- 발생시각: " << getCurrentTimeStr() << "\r\n\r\n"
                   << "시스템을 확인해 주시기 바랍니다.\r\n";

        struct upload_status upload_ctx;
        upload_ctx.lines_read = 0;
        upload_ctx.payload = email_body.str();

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "[EmailSender] curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "[EmailSender] Alert email successfully sent to " << alert_email << std::endl;
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }
    return (res == CURLE_OK);
}

}
}
