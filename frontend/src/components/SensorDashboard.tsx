import { Thermometer, Droplets, Sun, Wind, Activity } from 'lucide-react';
import { useEffect, useState } from 'react';
import { smartFarmApi } from '../api/client';
import HistoryChart from './HistoryChart';

interface SensorDashboardProps {
    houseId: string;
}

export default function SensorDashboard({ houseId }: SensorDashboardProps) {
    const [sensors, setSensors] = useState<any[]>([]);
    const [loading, setLoading] = useState(false);

    // History Modal State
    const [activeSensorHistory, setActiveSensorHistory] = useState<{ id: string, name: string } | null>(null);

    const fetchSensors = async () => {
        setLoading(true);
        try {
            const data = await smartFarmApi.getHouseDevices(houseId);
            // 백엔드에서 value(최신값)도 함께 내려준다고 가정하거나, 현재는 메타데이터를 기반으로 생성
            // TODO: 실시간 읽기값(value)는 추가 API가 필요하지만, 데모 목적으로 랜더 스켈레톤만 동기화
            setSensors(data.sensors.map((s: any) => ({
                ...s,
                // 임의의 Mock value 처리 (실제로는 GET /sensors/latest 등을 통해 얻어야함)
                value: s.type.includes('temp') ? 22 + Math.random() * 5 :
                    s.type.includes('hum') ? 50 + Math.random() * 20 :
                        Math.random() * 100
            })));
        } catch (e) {
            console.error(e);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        if (houseId) fetchSensors();
    }, [houseId]);

    const getIconAndColor = (type: string) => {
        const t = type.toLowerCase();
        if (t.includes('temp')) return { icon: Thermometer, color: 'text-neon-orange', glow: 'glow-text-orange' };
        if (t.includes('hum')) return { icon: Droplets, color: 'text-neon-blue', glow: 'glow-text-blue' };
        if (t.includes('solar') || t.includes('rad')) return { icon: Sun, color: 'text-yellow-400', glow: 'glow-text-yellow' };
        if (t.includes('wind')) return { icon: Wind, color: 'text-gray-300', glow: '' };
        return { icon: Activity, color: 'text-gray-400', glow: '' };
    };

    if (loading) return <div className="text-neon-blue animate-pulse">Loading sensors...</div>;
    if (!sensors.length) return <div className="text-gray-500">No sensors registered in this house.</div>;

    return (
        <>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
                {sensors.map((sensor) => {
                    const { icon: IconNode, color, glow } = getIconAndColor(sensor.type);
                    return (
                        <div
                            key={sensor.sensor_id}
                            onClick={() => setActiveSensorHistory({ id: sensor.sensor_id, name: sensor.alias })}
                            className="relative bg-[#161821] rounded-lg p-5 border border-cyber-border/10 flex flex-col items-center justify-center overflow-hidden hover:border-cyber-border/40 transition-colors group cursor-pointer"
                        >
                            <div className={`absolute -inset-4 bg-gradient-to-r from-transparent via-${color.replace('text-', '')}/5 to-transparent opacity-0 group-hover:opacity-100 blur-xl transition-opacity duration-500`} />

                            <div className="z-10 flex flex-col items-center text-center w-full">
                                <div className="flex w-full justify-between items-start mb-4">
                                    <span className="text-xs font-semibold text-gray-500 tracking-wider uppercase">{sensor.alias}</span>
                                    <IconNode className={`w-5 h-5 ${color}`} />
                                </div>

                                <div className="flex items-baseline gap-1 mt-2">
                                    <span className={`text-3xl font-bold tracking-tight text-white ${glow}`}>
                                        {sensor.value.toFixed(1)}
                                    </span>
                                    <span className="text-sm font-medium text-gray-400">{sensor.unit}</span>
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
        </>
    );
}
