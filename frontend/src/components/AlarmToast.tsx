import { useEffect, useState } from 'react';
import { smartFarmApi } from '../api/client';
import { AlertTriangle, AlertOctagon } from 'lucide-react';

export default function AlarmToast() {
    const [alarms, setAlarms] = useState<any[]>([]);

    const fetchAlarms = async () => {
        try {
            const data = await smartFarmApi.getAlarms();
            setAlarms(data);
        } catch (error) {
            console.error('Failed to fetch alarms', error);
        }
    };

    useEffect(() => {
        fetchAlarms();
        // Poll for new alarms every 10 seconds
        const intervalId = setInterval(fetchAlarms, 10000);
        return () => clearInterval(intervalId);
    }, []);

    const handleAcknowledge = async (alarmId: number) => {
        try {
            await smartFarmApi.acknowledgeAlarm(alarmId);
            setAlarms(prev => prev.filter(a => a.id !== alarmId));
        } catch (error) {
            console.error('Failed to acknowledge alarm', error);
        }
    };

    if (alarms.length === 0) return null;

    return (
        <div className="fixed bottom-4 right-4 z-50 flex flex-col gap-3 max-w-sm w-full">
            {alarms.map(alarm => {
                const isCritical = alarm.level === 'Critical';
                const borderColor = isCritical ? 'border-red-500' : 'border-amber-500';
                const bgColor = isCritical ? 'bg-red-900/80' : 'bg-amber-900/80';
                const iconColor = isCritical ? 'text-red-400' : 'text-amber-400';
                const Icon = isCritical ? AlertOctagon : AlertTriangle;

                return (
                    <div
                        key={alarm.id}
                        className={`relative overflow-hidden backdrop-blur-md border ${borderColor} ${bgColor} rounded-lg p-4 shadow-lg shadow-black/50 animate-in slide-in-from-right-8`}
                    >
                        <div className={`absolute -inset-4 bg-gradient-to-r from-transparent via-${isCritical ? 'red' : 'amber'}-500/10 to-transparent blur-xl`} />

                        <div className="relative z-10 flex items-start gap-3">
                            <Icon className={`w-6 h-6 shrink-0 mt-0.5 ${iconColor}`} />
                            <div className="flex-1">
                                <h4 className={`font-bold text-sm uppercase tracking-wide ${iconColor}`}>
                                    {isCritical ? '위험' : '경고'} 알림 - {alarm.alias}
                                </h4>
                                <p className="text-gray-200 text-sm mt-1 mb-3">
                                    {alarm.message}
                                </p>
                                <div className="flex justify-between items-center text-xs text-gray-400">
                                    <span>{new Date(alarm.created_at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' })}</span>
                                    <button
                                        onClick={() => handleAcknowledge(alarm.id)}
                                        className={`px-3 py-1.5 rounded bg-black/40 hover:bg-black/60 border ${borderColor} text-white font-medium transition-colors`}
                                    >
                                        확인
                                    </button>
                                </div>
                            </div>
                        </div>
                    </div>
                );
            })}
        </div>
    );
}
