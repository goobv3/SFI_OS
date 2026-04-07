/**
 * @file Router.h
 * @brief REST API 라우터 헤더 — Crow 프레임워크 기반 라우팅
 *
 * ▶ 역할
 *   - Crow 웹 프레임워크의 App 인스턴스를 인자로 받아 모든 API 엔드포인트를 등록
 *   - 미들웨어(CORSHandler)를 활용하여 다른 도메인이나 포트에서의 API 접근 가능토록 설정
 *   - 기존 FastAPI 프로젝트와 동일한 API 형식을 제공하기 위해 설계됨
 *
 * ▶ 커스터마이징 가이드
 *   1. 신규 API 추가 시 CROW_ROUTE(app, "/api/신규경로") 형식으로 추가 정의
 *   2. 특정 API만 권한 체크(requireAuth)를 해제하려면 람다 내부의 권한 검증 코드 삭제
 */
#pragma once
#include "crow.h"
#include "crow/middlewares/cors.h"

namespace API {
class Router {
public:
    /**
     * @brief REST API 라우트를 조립/등록하는 정적 셋업 메서드
     * @param app Crow 웹 서버 인스턴스 (CORS 미들웨어 적용)
     */
    static void setupRoutes(crow::App<crow::CORSHandler>& app);
};
}
