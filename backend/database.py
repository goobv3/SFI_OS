import os
import pymysql
import pymysql.cursors

def get_db_connection():
    try:
        connection = pymysql.connect(
            host=os.getenv("DB_HOST", "localhost"),
            port=int(os.getenv("DB_PORT", 3306)),
            user=os.getenv("DB_USER", "root"),
            password=os.getenv("DB_PASSWORD", "rootsecret"),
            database=os.getenv("DB_NAME", "smartfarm"),
            cursorclass=pymysql.cursors.DictCursor,
            autocommit=True
        )
        return connection
    except Exception as e:
        print(f"Database connection blocked: {e}")
        return None
