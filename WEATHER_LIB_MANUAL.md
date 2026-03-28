# 기상청 날씨 데이터 페치 라이브러리 매뉴얼 (KMA Weather Library Manual) 🌤️

이 문서는 스마트팜 프로젝트에서 추상화된 `weather_lib.py` 라이브러리를 **다른 Python 프로젝트나 타 개발환경에서 어떻게 활용하는지** 설명하는 사용 설명서입니다.

이 라이브러리는 대한민국 공공데이터포털(데이터.go.kr) 기상청 API의 까다로운 기준 시간(Base Time) 계산, 오류 처리, 그리고 풍향 각도 변환 등을 완전히 자동화해주어 개발자가 사용하기 편하도록 Dictionary(사전) 형태의 규격화된 포맷으로 데이터를 내뱉습니다.

---

## 🚀 1. 빠른 시작 (Quick Start)

이 라이브러리는 `aiohttp` 대신 `httpx` 비동기 라이브러리를 사용합니다.
필요 패키지를 먼저 설치하세요:
```bash
pip install httpx
```

### 기본 사용 예제 (Basic Usage Example)

아래의 Python 코드를 다른 프로젝트의 아무 곳에나 복사하여 실행해보세요.

```python
import asyncio
from weather_lib import KMAWeatherFetcher

async def main():
    # 1. 공공데이터포털 기상청 API 키 (Service Key)
    # 다른 프로젝트 사용 시 본인의 API 키로 교체하세요.
    MY_SERVICE_KEY = "발급받은_API_KEY"
    
    # 2. X, Y 좌표 지정 (기본값은 서울 중구 지역: 55, 127)
    fetcher = KMAWeatherFetcher(service_key=MY_SERVICE_KEY, nx=55, ny=127)
    
    # 3. 데이터 가져오기 (실황 + 예보 1~6시간)
    print("날씨 정보를 기상청으로부터 가져오는 중...")
    weather_data = await fetcher.fetch_all()
    
    # --- 결과 출력 ---
    obs = weather_data.get("observation")
    print(f"[현재 날씨 실황]")
    print(f"- 기온 (Temperature): {obs['temperature']} ℃")
    print(f"- 습도 (Humidity):    {obs['humidity']} %")
    print(f"- 풍속 (Wind):        {obs['wind_speed']} m/s ({obs['wind_direction']})")
    
    print(f"\n[향후 1~6시간 예보]")
    for fcst in weather_data.get("forecasts", []):
        offset = fcst['offset_hours']
        temp = fcst['temperature']
        rain = fcst['rainfall']
        print(f" + {offset}시간 뒤: {temp} ℃  (강수량: {rain} mm)")

# 비동기 실행
asyncio.run(main())
```

---

## 📦 2. 데이터 반환 형식 (Data Structure)

`await fetcher.fetch_all()` 을 실행했을 때 반환되는 Dictionary 구조는 다음과 같습니다. 데이터베이스를 사용하지 않는 순수 파이썬 Dictionary이므로, **Django, Flask, FastAPI 어느 환경에서든 자유롭게 변형하여 쓰시면 됩니다!**

```python
{
  "observation": {
    "temperature": 15.5,     # 현재 기온 (℃)
    "humidity": 60.0,        # 현재 습도 (%)
    "wind_speed": 2.5,       # 현재 풍속 (m/s)
    "wind_direction": "NW",  # 현재 풍향 (영문 16방위 변환 완료)
    "rainfall": 0.0          # 1시간 누적 강수량 (mm)
  },
  "forecasts": [
    {
      "offset_hours": 1,     # X시간 뒤의 예보 (1~6 단계)
      "fcstDate": "20260304", 
      "temperature": 16.0,
      "humidity": 55.0,
      "wind_speed": 3.1,
      "wind_direction": "NNW",
      "rainfall": 0.0
    },
    # ... offset_hours 가 2, 3, 4, 5, 6 인 객체들이 이어집니다 ...
  ]
}
```

---

---

## 🌩️ 3. 농장 자체 기상 데이터 기록 (Farm Weather Recording)

프로젝트 내부의 `WeatherManager`는 기상청 데이터뿐만 아니라, **농장 현장의 센서에서 수집된 고유 기상 정보**를 기록할 수 있는 기능을 제공합니다.

### 기상 데이터 기록 (C++ API)
```cpp
void recordFarmWeather(const nlohmann::json& data);
```
- **데이터 저장 위치:** `weather_data` 테이블 (`source='FARM'`)
- **기록 항목:** 기온, 습도, 풍속, 풍향, 일사량, 강수량 등
- **자동 타임스탬프:** 데이터에 타임스탬프가 없으면 기록 시점의 서버 시간을 자동으로 사용합니다.

---

## 🛠 4. 주요 자동화 기능 (기상청 연동)

라이브러리 내부적으로 다음과 같은 짜증나고 복잡한 연산들을 완벽하게 대신 처리해 줍니다. 
1. **타임존(Timezone) 동기화:** 서버가 UTC 기준(예: Docker 환경)에 있더라도 자동으로 한국 표준시(KST)로 변환하여 API를 요청합니다. 
2. **복잡한 Base Time 계산 방어 로직:** 기상청 실황은 매시간 40분 이후에 업데이트되고, 예보는 매시간 45분 이후에 30분 단위 시각으로 업데이트됩니다. 라이브러리가 현재 KST 분(minute)을 측정하여 자동으로 가장 적합한 Base Time 문자열을 만들어 API 404/NoData 에러를 예방합니다.
3. **풍향(VEC) 변환 공식 내장:** 기상청에서 주는 풍향 각도 수치(예: `315`)를 우리가 알아보기 쉬운 `NW(북서풍)` 형식으로 자동 변환해 줍니다.
4. **자정(Midnight) 크로스 타임 정렬 로직:** 예보의 시간이 `22시 -> 23시 -> 00시 -> 01시` 와 같이 자정을 넘어 다음날로 날짜가 바뀔 때, API 배열 순서가 문자로 인해 섞이는 것을 방지하기 위해 날짜와 시간을 실제 Datetime 객체로 병합한 후 시간순 매핑을 수행합니다. 
