import React, { useEffect, useState } from 'react';
import { Cloud, Wind, Droplets, Sun, ChevronRight } from 'lucide-react';
import { smartFarmApi } from '../api/client';

// ---------------------------------------------------------
// 실시간 날씨 위젯 컴포넌트 (Weather Widget Component)
// ---------------------------------------------------------
// 이 파일은 메인 화면 좌측에 나타나는 "날씨 전광판" 역할을 합니다.
// 기상청의 날씨(현재/예보)와 우리 농장의 자체 기상대 날씨를 나란히 보여주며,
// 설정된 시간(예: 1시간 후)에 맞추어 자동으로 백엔드에 날씨를 물어보고 화면을 갱신합니다.
// ---------------------------------------------------------

interface WeatherInfo {
    source: string;
    temperature: number | null;
    humidity: number | null;
    wind_speed: number | null;
    wind_direction: string | null;
    rainfall: number | null;
    solar_radiation: number | null;
    timestamp: string;
}

const WeatherWidget: React.FC = () => {
    const [kmaWeather, setKmaWeather] = useState<WeatherInfo | null>(null);
    const [farmWeather, setFarmWeather] = useState<WeatherInfo | null>(null);
    const [isLoading, setIsLoading] = useState(true);
    const [hoursAhead, setHoursAhead] = useState<number>(() => {
        const saved = localStorage.getItem('weather_forecast_hours');
        return saved ? parseInt(saved, 10) : 0;
    });

    const fetchWeather = async () => {
        try {
            const data = await smartFarmApi.getLatestWeather(hoursAhead);
            if (data) {
                setKmaWeather(data.KMA);
                setFarmWeather(data.FARM);
            }
        } catch (error) {
            console.error("Failed to fetch weather data", error);
        } finally {
            setIsLoading(false);
        }
    };

    useEffect(() => {
        setIsLoading(true);
        setKmaWeather(null); // Clear old state so user sees loading instantly
        fetchWeather();
        localStorage.setItem('weather_forecast_hours', hoursAhead.toString());
        const interval = setInterval(fetchWeather, 60000); // 1 minute polling
        return () => clearInterval(interval);
    }, [hoursAhead]);

    const getDisplayTime = (isKma: boolean) => {
        const now = new Date();
        if (isKma && hoursAhead > 0) {
            now.setHours(now.getHours() + hoursAhead);
        }
        return now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    };

    const WeatherCard = ({ title, data, isKma = false }: { title: string, data: WeatherInfo | null, isKma?: boolean }) => {
        if (!data) {
            return (
                <div className={`p-4 rounded-xl border flex flex-col items-center justify-center min-h-[160px] 
          ${isKma ? 'bg-cyber-surface/50 border-cyber-border' : 'bg-neon-blue/5 border-neon-blue/30'}`}>
                    <h4 className="text-gray-400 mb-2">{title}</h4>
                    <span className="text-sm text-gray-500">No Data Available</span>
                </div>
            );
        }

        const tColor = isKma ? 'text-neon-blue' : 'text-neon-green';

        return (
            <div className={`p-4 rounded-xl border flex flex-col relative overflow-hidden transition-all duration-300 hover:shadow-[0_0_15px_rgba(0,0,0,0.5)] 
        ${isKma ? 'bg-cyber-surface border-cyber-border/80' : 'bg-cyber-surface border-neon-green/30'}`}>

                {/* Decorative elements */}
                <div className={`absolute top-0 left-0 w-full h-1 opacity-50 ${isKma ? 'bg-gradient-to-r from-neon-blue to-transparent' : 'bg-gradient-to-r from-neon-green to-transparent'}`} />
                <div className="flex justify-between items-center mb-4">
                    <h4 className={`font-semibold tracking-wider flex items-center gap-3 ${isKma ? 'text-gray-300' : 'text-neon-green glow-text-green'}`}>
                        {title}
                        {isKma && (
                            <select
                                value={hoursAhead}
                                onChange={(e) => setHoursAhead(Number(e.target.value))}
                                className="bg-black/50 border border-neon-blue/30 text-neon-blue text-xs rounded px-2 py-0.5 outline-none cursor-pointer focus:border-neon-blue ml-2"
                            >
                                <option value={0}>Current (Now)</option>
                                <option value={1}>1h Forecast</option>
                                <option value={3}>3h Forecast</option>
                                <option value={6}>6h Forecast</option>
                            </select>
                        )}
                    </h4>
                    <span className="text-xs text-gray-500">
                        {getDisplayTime(isKma)}
                    </span>
                </div>

                <div className="grid grid-cols-2 gap-4">
                    <div className="flex items-center gap-2">
                        <div className={`p-1.5 rounded-lg bg-black/40 ${isKma ? 'text-neon-blue' : 'text-neon-green'}`}>
                            <Cloud className="w-5 h-5" />
                        </div>
                        <div>
                            <div className="text-xs text-gray-400 uppercase">Temp</div>
                            <div className={`text-lg font-bold ${tColor}`}>
                                {data.temperature !== null ? `${data.temperature.toFixed(1)}°C` : '--'}
                            </div>
                        </div>
                    </div>

                    <div className="flex items-center gap-2">
                        <div className="p-1.5 rounded-lg bg-black/40 text-blue-400">
                            <Droplets className="w-5 h-5" />
                        </div>
                        <div>
                            <div className="text-xs text-gray-400 uppercase">Humidity</div>
                            <div className="text-lg font-bold text-gray-200">
                                {data.humidity !== null ? `${data.humidity.toFixed(1)}%` : '--'}
                            </div>
                        </div>
                    </div>

                    <div className="flex items-center gap-2">
                        <div className="p-1.5 rounded-lg bg-black/40 text-gray-300">
                            <Wind className="w-5 h-5" />
                        </div>
                        <div>
                            <div className="text-xs text-gray-400 uppercase">Wind</div>
                            <div className="text-sm font-medium text-gray-200 flex items-center gap-1">
                                {data.wind_speed !== null ? `${data.wind_speed} m/s` : '--'}
                                <span className="text-xs text-gray-500">{data.wind_direction || ''}</span>
                            </div>
                        </div>
                    </div>

                    <div className="flex items-center gap-2">
                        <div className="p-1.5 rounded-lg bg-black/40 text-yellow-500">
                            <Sun className="w-5 h-5" />
                        </div>
                        <div>
                            <div className="text-xs text-gray-400 uppercase">{isKma ? 'Rainfall' : 'Solar Rad'}</div>
                            <div className="text-sm font-medium text-gray-200">
                                {isKma
                                    ? (data.rainfall !== null ? `${data.rainfall} mm` : '--')
                                    : (data.solar_radiation !== null ? `${data.solar_radiation} W/m²` : '--')}
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        );
    };

    return (
        <div className="w-full mb-8">
            <div className="flex items-center justify-between mb-4">
                <div>
                    <h2 className="text-2xl font-bold text-white tracking-wide border-l-4 border-neon-blue pl-4 mb-1">
                        External Weather Intelligence
                    </h2>
                    <p className="text-gray-400 text-sm">Real-time macro environmental data & predictions</p>
                </div>
                {!isLoading && (
                    <button onClick={fetchWeather} className="text-xs text-neon-blue hover:text-white transition flex items-center gap-1 bg-[#1a1c23] border border-cyber-border/40 px-3 py-1.5 rounded-md hover:border-neon-blue">
                        Refresh <ChevronRight className="w-3 h-3" />
                    </button>
                )}
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-6 relative">
                {isLoading && (
                    <div className="absolute inset-0 z-10 flex items-center justify-center bg-black/40 backdrop-blur-sm rounded-xl">
                        <div className="w-8 h-8 border-4 border-neon-blue border-t-transparent rounded-full animate-spin"></div>
                    </div>
                )}
                <WeatherCard title={hoursAhead > 0 ? `KMA Forecast` : "KMA Observation"} data={kmaWeather} isKma={true} />
                <WeatherCard title="Farm Station (자체 기상대)" data={farmWeather} isKma={false} />
            </div>
        </div>
    );
};

export default WeatherWidget;
