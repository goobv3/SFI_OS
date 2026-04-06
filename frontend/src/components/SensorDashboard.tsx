import { Thermometer, Droplets, Sun, Wind, Activity, Settings, Zap, ArrowUp, ArrowDown } from 'lucide-react';
import { useEffect, useState, useRef } from 'react';
import { smartFarmApi } from '../api/client';
import HistoryChart from './HistoryChart';
import SensorGauge from './SensorGauge';
import SensorConfigModal from './SensorConfigModal';
import { useLanguage } from '../i18n/LanguageContext';

interface SensorDashboardProps {
    houseId: string;
    refreshTrigger?: number;
}

/** 센서 타입별 아이콘 + 색상 매핑 */
function getIconAndColor(type: string) {
    const t2 = type.toLowerCase();
    if (t2.includes('temp'))                      return { icon: Thermometer, colorClass: 'text-neon-orange',  borderColor: '#ffaa00', gaugeColor: '#ffaa00' };
    if (t2.includes('hum'))                       return { icon: Droplets,    colorClass: 'text-neon-blue',    borderColor: '#66fcf1', gaugeColor: '#66fcf1' };
    if (t2.includes('co2'))                       return { icon: Wind,        colorClass: 'text-neon-purple',  borderColor: '#b94fff', gaugeColor: '#b94fff' };
    if (t2.includes('solar') || t2.includes('rad')) return { icon: Sun,       colorClass: 'text-yellow-400',   borderColor: '#facc15', gaugeColor: '#facc15' };
    return                                               { icon: Activity,    colorClass: 'text-neon-teal',    borderColor: '#00d4aa', gaugeColor: '#00d4aa' };
}

function getTimeAgo(dateStr?: string) {
    if (!dateStr) return '방금 전';
    const seconds = Math.floor((new Date().getTime() - new Date(dateStr).getTime()) / 1000);
    if (seconds < 60) return `방금 전`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return `${minutes}분 전`;
    const hours = Math.floor(minutes / 60);
    if (hours < 24) return `${hours}시간 전`;
    return `${Math.floor(hours / 24)}일 전`;
}

export default function SensorDashboard({ houseId, refreshTrigger }: SensorDashboardProps) {
    const { t } = useLanguage();
    const [sensors, setSensors] = useState<any[]>([]);
    const [loading, setLoading] = useState(false);
    const [activeSensorHistory, setActiveSensorHistory] = useState<{ id: string, name: string } | null>(null);
    const [activeConfigSensor, setActiveConfigSensor] = useState<any | null>(null);
    const prevValuesRef = useRef<Record<string, number>>({});

    const fetchSensors = async () => {
        setLoading(true);
        try {
            const data = await smartFarmApi.getHouseDevices(houseId);
            setSensors(data.sensors.map((s: any) => {
                const newVal = s.value ?? (s.type.includes('temp') ? 22 + Math.random() * 5 :
                    s.type.includes('hum') ? 50 + Math.random() * 20 :
                        Math.random() * 100);
                
                // 이전 값 저장 로직 추가
                const prevVal = prevValuesRef.current[s.sensor_id];
                prevValuesRef.current[s.sensor_id] = newVal;

                return {
                    ...s,
                    value: newVal,
                    prevValue: prevVal,
                    last_updated: s.last_updated || new Date().toISOString()
                };
            }));
        } catch (e) {
            console.error(e);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        if (houseId) {
            fetchSensors();
            const interval = setInterval(fetchSensors, 30000);
            return () => clearInterval(interval);
        }
    }, [houseId, refreshTrigger]);

    // 로딩 시 Skeleton UI
    if (loading && sensors.length === 0) {
        return (
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-3">
                {[1, 2, 3, 4].map(idx => (
                    <div key={idx} className="min-h-[180px] bg-surface-elevated animate-pulse rounded-xl border border-cyber-border/15" />
                ))}
            </div>
        );
    }
    if (!sensors.length) return <div className="text-gray-500 text-xs">{t('noSensors')}</div>;

    return (
        <>
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-3">
                {sensors.map((sensor) => {
                    const { icon: IconNode, colorClass, borderColor } = getIconAndColor(sensor.type);
                    const isActive = sensor.is_active !== false && sensor.is_active !== '0';

                    return (
                        <div
                            key={sensor.sensor_id}
                            onClick={() => isActive && setActiveSensorHistory({ id: sensor.sensor_id, name: sensor.alias })}
                            style={{ '--hover-border': borderColor } as React.CSSProperties}
                            className={`
                                relative flex flex-col min-h-[180px]
                                bg-surface-card rounded-xl border border-cyber-border/15
                                overflow-hidden hover-lift animate-fade-in-up
                                hover:border-[var(--hover-border)]/50 transition-colors
                                ${isActive ? 'cursor-pointer' : 'grayscale opacity-40'}
                                group
                            `}
                        >
                            {/* 비활성 오버레이 */}
                            {!isActive && (
                                <div className="absolute inset-0 bg-black/50 z-20 flex items-center justify-center backdrop-blur-[1px]">
                                    <button
                                        className="text-red-400 font-bold border border-red-500/50 bg-red-500/10 px-3 py-1 rounded-lg text-xs tracking-widest uppercase hover:bg-red-500/20 hover:scale-105 transition-all pointer-events-auto"
                                        onClick={(e) => { e.stopPropagation(); setActiveConfigSensor(sensor); }}
                                    >
                                        {t('paused')}
                                    </button>
                                </div>
                            )}

                            {/* 호버 시 글로우 배경 */}
                            <div
                                className="absolute inset-0 opacity-0 group-hover:opacity-100 transition-opacity duration-500 pointer-events-none"
                                style={{ background: `radial-gradient(ellipse at top, ${borderColor}0a 0%, transparent 70%)` }}
                            />

                            <div className="relative z-10 flex flex-col h-full p-3">
                                {/* Row 1: 아이콘 + alias + 설정 버튼 */}
                                <div className="flex items-center gap-2 mb-2">
                                    <IconNode className={`w-4 h-4 shrink-0 ${colorClass}`} />
                                    <span className="flex-1 text-xs font-semibold text-gray-200 truncate">{sensor.alias}</span>
                                    
                                    {/* 증감 표시 */}
                                    {sensor.prevValue !== undefined && sensor.prevValue !== sensor.value && (
                                        <div className={`flex items-center text-[10px] font-bold ${sensor.value > sensor.prevValue ? 'text-red-400' : 'text-blue-400'}`}>
                                            {sensor.value > sensor.prevValue ? <ArrowUp className="w-3 h-3" /> : <ArrowDown className="w-3 h-3" />}
                                            {Math.abs(sensor.value - sensor.prevValue).toFixed(1)}
                                        </div>
                                    )}

                                    <button
                                        className="p-1 hover:bg-white/10 rounded-lg transition-colors text-gray-500 hover:text-white"
                                        onClick={(e) => { e.stopPropagation(); setActiveConfigSensor(sensor); }}
                                        title="센서 설정"
                                    >
                                        <Settings className="w-3.5 h-3.5" />
                                    </button>
                                </div>

                                {/* Row 2: SensorGauge (flex-1로 확장) */}
                                <div className="flex-1 min-h-0">
                                    <SensorGauge
                                        value={sensor.value}
                                        unit={sensor.unit}
                                        title={sensor.alias}
                                        warnLow={sensor.warn_low}
                                        warnHigh={sensor.warn_high}
                                        critLow={sensor.crit_low}
                                        critHigh={sensor.crit_high}
                                        min={sensor.type.includes('temp') ? -10 : 0}
                                        max={sensor.type.includes('temp') ? 50 : 100}
                                        accentColor={getIconAndColor(sensor.type).gaugeColor}
                                    />
                                </div>

                                {/* Row 3: 마지막 업데이트 + 상태 뱃지 */}
                                <div className="flex items-center justify-between mt-2 pt-2 border-t border-white/5">
                                    <span className="text-[10px] text-gray-600">{getTimeAgo(sensor.last_updated)}</span>
                                    <span
                                        className="text-[10px] font-bold px-1.5 py-0.5 rounded-full"
                                        style={{
                                            color: borderColor,
                                            backgroundColor: `${borderColor}18`,
                                            border: `1px solid ${borderColor}40`,
                                        }}
                                    >
                                        <Zap className="w-2.5 h-2.5 inline mr-0.5 -mt-px" />
                                        LIVE
                                    </span>
                                </div>
                            </div>
                        </div>
                    );
                })}
            </div>

            {activeSensorHistory && (
                <HistoryChart
                    sensors={sensors.map(s => ({ id: s.sensor_id, name: s.alias, type: s.type }))}
                    initialSensorId={activeSensorHistory.id}
                    onClose={() => setActiveSensorHistory(null)}
                />
            )}

            {activeConfigSensor && (
                <SensorConfigModal
                    sensor={activeConfigSensor}
                    onClose={() => setActiveConfigSensor(null)}
                    onSave={() => { setActiveConfigSensor(null); fetchSensors(); }}
                />
            )}
        </>
    );
}
