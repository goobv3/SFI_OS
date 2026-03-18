import asyncio
import httpx
from datetime import datetime, timedelta

SERVICE_KEY = "3b1b77976ce0ca80247d4cd707834cff148c26d257a9b03826dda6829b75f0ad"
# Icheon Baeksa-myeon Grid from my calculation/search
NX, NY = 70, 129 

async def test_forecast():
    kst_now = datetime.now() + timedelta(hours=9)
    if kst_now.minute < 45:
        base_time_dt = kst_now - timedelta(hours=1)
    else:
        base_time_dt = kst_now
    
    base_date = base_time_dt.strftime("%Y%m%d")
    base_time = base_time_dt.strftime("%H00")
    
    url = "http://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getUltraSrtFcst"
    params = {
        "serviceKey": SERVICE_KEY,
        "numOfRows": 100,
        "pageNo": 1,
        "dataType": "JSON",
        "base_date": base_date,
        "base_time": base_time,
        "nx": NX,
        "ny": NY
    }
    
    print(f"Requesting: {url} with params {params}")
    async with httpx.AsyncClient() as client:
        try:
            res = await client.get(url, params=params, timeout=15.0)
            print(f"Status Code: {res.status_code}")
            if res.status_code == 200:
                print("Response Body (partial):")
                print(res.text[:500])
                data = res.json()
                if data['response']['header']['resultCode'] == '00':
                    print("API Call Successful!")
                else:
                    print(f"API Error: {data['response']['header']['resultMsg']}")
        except Exception as e:
            print(f"Error: {e}")

if __name__ == "__main__":
    asyncio.run(test_forecast())
