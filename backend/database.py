import os
import pymysql
import pymysql.cursors

# ---------------------------------------------------------
# 데이터베이스 연결 모듈 (Database Connection Module)
# ---------------------------------------------------------
# 이 파일은 스마트팜 백엔드 서버(Python)가 정보를 저장하고 읽어올 
# MariaDB(데이터베이스)와 안전하게 연결되도록 다리 역할을 합니다.
# ---------------------------------------------------------

def get_db_connection():
    """
    데이터베이스와 통신할 수 있는 '연결 통로(Connection)'를 생성하여 반환합니다.
    """
    try:
        # pymysql 라이브러리를 사용해 DB 연결을 시도합니다.
        # os.getenv()는 환경변수(Docker나 OS에서 설정한 값)를 가져오는 함수입니다.
        connection = pymysql.connect(
            host=os.getenv("DB_HOST", "localhost"),        # DB가 설치된 컴퓨터 주소
            port=int(os.getenv("DB_PORT", 3306)),          # DB 접속 포트 번호
            user=os.getenv("DB_USER", "root"),             # DB 접속 아이디
            password=os.getenv("DB_PASSWORD", "rootsecret"), # DB 접속 비밀번호
            database=os.getenv("DB_NAME", "smartfarm"),    # 사용할 데이터베이스 구역 이름
            cursorclass=pymysql.cursors.DictCursor,        # 결과를 보기 쉬운 딕셔너리(사전) 형태로 가져오기
            autocommit=True                                # 데이터 변경 시 즉시 저장(자동 커밋) 설정
        )
        return connection # 성공적으로 연결되면 통로 객체를 반환합니다.
    except Exception as e:
        # 만약 비밀번호가 틀리거나 DB가 꺼져있어서 에러가 나면 원인을 화면에 출력합니다.
        print(f"Database connection blocked(데이터베이스 연결 실패): {e}")
        return None

