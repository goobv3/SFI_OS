import { Thermometer, Droplets, Sun, Wind, Activity, Settings } from 'lucide-react';
import { useEffect, useState } from 'react';
import { smartFarmApi } from '../api/client';
import HistoryChart from './HistoryChart';
import SensorGauge from './SensorGauge';
import SensorConfigModal from './SensorConfigModal';
import { useLanguage } from '../i18n/LanguageContext';

interface SensorDashboardProps {
    houseId: string;
    refreshTrigger?: number;
}

export default function SensorDashboard({ houseId, refreshTrigger }: SensorDashboardProps) {
    const { t } = useLanguage();
    const [sensors, setSensors] = useState<any[]>([]);
    const [loading, setLoading] = useState(false);
    const [activeSensorHistory, setActiveSensorHistory] = useState<{ id: string, name: string } | null>(null);
    const [activeConfigSensor, setActiveConfigSensor] = useState<any | null>(null);

    const fetchSensors = async () => {
        setLoading(true);
        try {
            const data = await smartFarmApi.getHouseDevices(houseId);
            setSensors(data.sensors.map((s: any) => ({
                ...s,
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
    }, [houseId, refreshTrigger]);

    const getIconAndColor = (type: string) => {
        const t2 = type.toLowerCase();
        if (t2.includes('temp')) return { icon: Thermometer, color: 'text-neon-orange', glow: 'glow-text-orange' };
        if (t2.includes('hum')) return { icon: Droplets, color: 'text-neon-blue', glow: 'glow-text-blue' };
        if (t2.includes('solar') || t2.includes('rad')) return { icon: Sun, color: 'text-yellow-400', glow: 'glow-text-yellow' };
        if (t2.includes('wind')) return { icon: Wind, color: 'text-gray-300', glow: '' };
        return { icon: Activity, color: 'text-gray-400', glow: '' };
    };

    if (loading) return <div className="text-neon-blue animate-pulse">{t('loadingSensors')}</div>;
    if (!sensors.length) return <div className="text-gray-500">{t('noSensors')}</div>;

    return (
        <>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
                {sensors.map((sensor) => {
                    const { icon: IconNode, color } = getIconAndColor(sensor.type);
                    return (
                        <div
                            key={sensor.sensor_id}
                            onClick={() => setActiveSensorHistory({ id: sensor.sensor_id, name: sensor.alias })}
                            className="relative bg-[#161821] rounded-lg p-5 border border-cyber-border/10 flex flex-col items-center justify-center overflow-hidden hover:border-cyber-border/40 transition-colors group cursor-pointer"
                        >
                            <div className={`absolute -inset-4 bg-gradient-to-r from-transparent via-${color.replace('text-', '')}/5 to-transparent opacity-0 group-hover:opacity-100 blur-xl transition-opacity duration-500`} />

                            {!sensor.is_active && (
                                <div className="absolute inset-0 bg-black/60 z-20 flex items-center justify-center backdrop-blur-[1px] pointer-events-none">
                                    <button
                                        className="text-red-500 font-bold border border-red-500/50 bg-red-500/10 px-3 py-1 rounded text-sm tracking-widest uppercase pointer-events-auto hover:bg-red-500/20 hover:scale-105 transition-all"
                                        onClick={(e) => { e.stopPropagation(); setActiveConfigSensor(sensor); }}
                                    >
                                        {t('paused')}
                                    </button>
                                </div>
                            )}

                            <div className={`z-10 flex flex-col items-center w-full ${!sensor.is_active ? 'opacity-30' : ''}`}>
                                <div className="flex w-full justify-between items-center mb-1">
                                    <IconNode className={`w-5 h-5 ${color}`} />
                                    <button
                                        className="p-1 hover:bg-white/10 rounded transition-colors text-gray-500 hover:text-white"
                                        onClick={(e) => { e.stopPropagation(); setActiveConfigSensor(sensor); }}
                                    >
                                        <Settings className="w-4 h-4" />
                                    </button>
                                </div>
                                <div className="relative w-full">
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
                                    />
                                    {!sensor.is_active && <div className="absolute inset-0 z-30 pointer-events-none" />}
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
