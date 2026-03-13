import React, { useEffect, useState } from 'react';
import { Cloud, Wind, Droplets, Sun } from 'lucide-react';
import { smartFarmApi } from '../api/client';
import { useLanguage } from '../i18n/LanguageContext';

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

interface WeatherWidgetProps {
    compact?: boolean;
}

const WeatherWidget: React.FC<WeatherWidgetProps> = ({ compact = false }) => {
    const { t } = useLanguage();
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
                <div className={`p-3 rounded-lg border flex flex-col items-center justify-center min-h-[80px]
          ${isKma ? 'bg-cyber-surface/50 border-cyber-border' : 'bg-neon-blue/5 border-neon-blue/30'}`}>
                    <h4 className="text-gray-400 mb-1 text-xs">{title}</h4>
                    <span className="text-xs text-gray-500">{t('noData')}</span>
                </div>
            );
        }

        const tColor = isKma ? 'text-neon-blue' : 'text-neon-green';

        return (
            <div className={`rounded-lg border flex flex-col relative overflow-hidden transition-all duration-300
        ${isKma ? 'bg-cyber-surface border-cyber-border/80' : 'bg-cyber-surface border-neon-green/30'}
        ${compact ? 'p-3' : 'p-4'}`}>

                {/* Decorative top bar */}
                <div className={`absolute top-0 left-0 w-full h-0.5 opacity-50 ${isKma ? 'bg-gradient-to-r from-neon-blue to-transparent' : 'bg-gradient-to-r from-neon-green to-transparent'}`} />
                <div className={`flex justify-between items-center ${compact ? 'mb-2' : 'mb-4'}`}>
                    <h4 className={`font-semibold tracking-wider flex items-center gap-2 text-xs ${isKma ? 'text-gray-300' : 'text-neon-green glow-text-green'}`}>
                        {title}
                        {isKma && (
                            <select
                                value={hoursAhead}
                                onChange={(e) => setHoursAhead(Number(e.target.value))}
                                className="bg-black/50 border border-neon-blue/30 text-neon-blue text-xs rounded px-1.5 py-0.5 outline-none cursor-pointer focus:border-neon-blue ml-1"
                            >
                                <option value={0}>{t('currentNow')}</option>
                                <option value={1}>{t('forecast1h')}</option>
                                <option value={3}>{t('forecast3h')}</option>
                                <option value={6}>{t('forecast6h')}</option>
                            </select>
                        )}
                    </h4>
                    <span className="text-xs text-gray-500">
                        {getDisplayTime(isKma)}
                    </span>
                </div>

                <div className={`grid gap-2 ${compact ? 'grid-cols-4' : 'grid-cols-2 gap-4'}`}>
                    <div className="flex items-center gap-1.5">
                        <div className={`p-1 rounded bg-black/40 ${isKma ? 'text-neon-blue' : 'text-neon-green'}`}>
                            <Cloud className="w-4 h-4" />
                        </div>
                        <div>
                            <div className="text-[10px] text-gray-400 uppercase">Temp</div>
                            <div className={`text-sm font-bold ${tColor}`}>
                                {data.temperature !== null ? `${data.temperature.toFixed(1)}°C` : '--'}
                            </div>
                        </div>
                    </div>
                    <div className="flex items-center gap-1.5">
                        <div className="p-1 rounded bg-black/40 text-blue-400">
                            <Droplets className="w-4 h-4" />
                        </div>
                        <div>
                            <div className="text-[10px] text-gray-400 uppercase">Hum</div>
                            <div className="text-sm font-bold text-gray-200">
                                {data.humidity !== null ? `${data.humidity.toFixed(1)}%` : '--'}
                            </div>
                        </div>
                    </div>
                    <div className="flex items-center gap-1.5">
                        <div className="p-1 rounded bg-black/40 text-gray-300">
                            <Wind className="w-4 h-4" />
                        </div>
                        <div>
                            <div className="text-[10px] text-gray-400 uppercase">Wind</div>
                            <div className="text-xs font-medium text-gray-200">
                                {data.wind_speed !== null ? `${data.wind_speed} m/s` : '--'}
                                <span className="text-gray-500 ml-1">{data.wind_direction || ''}</span>
                            </div>
                        </div>
                    </div>
                    <div className="flex items-center gap-1.5">
                        <div className="p-1 rounded bg-black/40 text-yellow-500">
                            <Sun className="w-4 h-4" />
                        </div>
                        <div>
                            <div className="text-[10px] text-gray-400 uppercase">{isKma ? 'Rain' : 'Solar'}</div>
                            <div className="text-xs font-medium text-gray-200">
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
        <div className="w-full">
            <div className={`grid gap-3 ${compact ? 'grid-cols-1 md:grid-cols-2' : 'grid-cols-1 md:grid-cols-2'} relative`}>
                {isLoading && (
                    <div className="absolute inset-0 z-10 flex items-center justify-center bg-black/40 backdrop-blur-sm rounded-lg">
                        <div className="w-6 h-6 border-4 border-neon-blue border-t-transparent rounded-full animate-spin"></div>
                    </div>
                )}
                <WeatherCard title={hoursAhead > 0 ? `${t('kmaWeather')} ${t('forecast1h').replace('1h', hoursAhead + 'h')}` : t('kmaWeather')} data={kmaWeather} isKma={true} />
                <WeatherCard title={t('farmWeather')} data={farmWeather} isKma={false} />
            </div>
        </div>
    );
};

export default WeatherWidget;
