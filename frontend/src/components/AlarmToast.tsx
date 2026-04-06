import { useEffect, useState } from 'react';
import { smartFarmApi } from '../api/client';
import { AlertTriangle, AlertOctagon } from 'lucide-react';
import { useLanguage } from '../i18n/LanguageContext';

export default function AlarmToast() {
    const { t } = useLanguage();
    const [alarms, setAlarms] = useState<any[]>([]);

    const fetchAlarms = async () => {
        try { const data = await smartFarmApi.getAlarms(); setAlarms(data); }
        catch (error) { console.error('Failed to fetch alarms', error); }
    };

    useEffect(() => {
        fetchAlarms();
        const intervalId = setInterval(fetchAlarms, 10000);
        return () => clearInterval(intervalId);
    }, []);

    const handleAcknowledge = async (alarmId: number) => {
        // 먼저 닫히는 애니메이션을 위해 닫힘 상태로 마킹할 수도 있지만,
        // 여기선 즉시 DOM에서 제거되도록 filter 처리 후 API 전송
        try {
            await smartFarmApi.acknowledgeAlarm(alarmId);
            setAlarms(prev => prev.filter(a => a.id !== alarmId));
        } catch (error) { console.error('Failed to acknowledge alarm', error); }
    };

    if (alarms.length === 0) return null;

    // 최대 3개까지만 렌더링하도록 자르기 (가장 최근 순)
    const displayAlarms = alarms.slice(0, 3);

    return (
        <div className="fixed bottom-4 right-4 z-[100] flex flex-col gap-3 max-w-sm w-full">
            <style>{`
                @keyframes slideInRight { from { transform: translateX(110%); opacity: 0; } to { transform: translateX(0); opacity: 1; } }
                .animate-slide-in { animation: slideInRight 0.4s cubic-bezier(0.16, 1, 0.3, 1) forwards; }
            `}</style>
            
            {displayAlarms.map(alarm => {
                const isCritical = alarm.level === 'Critical';
                const borderColor = isCritical ? 'border-red-500' : 'border-amber-500';
                const bgColor = isCritical ? 'bg-red-900/80' : 'bg-amber-900/80';
                const iconColor = isCritical ? 'text-red-400' : 'text-amber-400';
                const Icon = isCritical ? AlertOctagon : AlertTriangle;

                return (
                    <div
                        key={alarm.id}
                        className={`relative overflow-hidden backdrop-blur-md border ${borderColor} ${bgColor} rounded-xl p-4 shadow-2xl animate-slide-in ${isCritical ? 'shadow-[0_0_20px_rgba(239,68,68,0.4)]' : ''}`}
                    >
                        <div className={`absolute -inset-4 bg-gradient-to-r from-transparent via-${isCritical ? 'red' : 'amber'}-500/10 to-transparent blur-xl ${isCritical ? 'animate-pulse opacity-70' : 'opacity-30'}`} />
                        <div className="relative z-10 flex items-start gap-3">
                            <Icon className={`w-6 h-6 shrink-0 mt-0.5 ${iconColor}`} />
                            <div className="flex-1">
                                <h4 className={`font-bold text-sm uppercase tracking-wide ${iconColor}`}>
                                    {isCritical ? t('danger') : t('warning')} {t('alarmNotice')} — {alarm.alias}
                                </h4>
                                <p className="text-gray-200 text-sm mt-1 mb-3">{alarm.message}</p>
                                <div className="flex justify-between items-center text-xs text-gray-400">
                                    <span>{new Date(alarm.created_at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' })}</span>
                                    <button
                                        onClick={() => handleAcknowledge(alarm.id)}
                                        className={`px-3 py-1.5 rounded bg-black/40 hover:bg-black/60 border ${borderColor} text-white font-medium transition-colors`}
                                    >
                                        {t('acknowledge')}
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
