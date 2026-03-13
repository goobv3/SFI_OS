import { Power, Settings2, ShieldAlert } from 'lucide-react';
import { useState, useEffect } from 'react';
import { smartFarmApi } from '../api/client';
import { useLanguage } from '../i18n/LanguageContext';

interface ActuatorControlProps { houseId: string; }

export default function ActuatorControl({ houseId }: ActuatorControlProps) {
    const { t } = useLanguage();
    const [actuators, setActuators] = useState<any[]>([]);
    const [loading, setLoading] = useState(false);

    const fetchActuators = async () => {
        setLoading(true);
        try {
            const data = await smartFarmApi.getHouseDevices(houseId);
            setActuators(data.actuators);
        } catch (e) { console.error(e); }
        finally { setLoading(false); }
    };

    useEffect(() => { if (houseId) fetchActuators(); }, [houseId]);

    const toggleCommand = async (id: string, currentStatus: string) => {
        const isActivating = currentStatus === 'Off' || currentStatus === 'Closed';
        const command = isActivating ? 'On' : 'Off';
        try {
            const result = await smartFarmApi.sendControlCommand(id, command);
            if (result.status === 'success') {
                setActuators(prev => prev.map(act => act.actuator_id === id ? { ...act, status: command } : act));
            } else if (result.status === 'blocked') {
                alert(`🚨 ${result.reason}`);
            }
        } catch (error: any) { alert(`API Error: ${error.message}`); }
    };

    const toggleAutoMode = (id: string, isAuto: boolean) => {
        setActuators(prev => prev.map(act => act.actuator_id === id ? { ...act, isAuto: !isAuto } : act));
    };

    if (loading) return <div className="text-neon-orange animate-pulse text-xs">{t('loadingControls')}</div>;
    if (!actuators.length) return <div className="text-gray-500 text-xs">{t('noActuators')}</div>;

    return (
        <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-2">
            {actuators.map((act) => {
                const isActive = act.status === 'On' || act.status === 'Open';
                const buttonColor = isActive
                    ? 'bg-neon-orange/20 text-neon-orange border-neon-orange shadow-[0_0_15px_rgba(255,107,107,0.3)]'
                    : 'bg-[#1a1c23] border-gray-700 text-gray-500';
                const isAuto = act.isAuto ?? true;

                return (
                    <div key={act.actuator_id} className="bg-[#12141a] rounded-lg border border-cyber-border/30 p-2 relative overflow-hidden flex flex-col justify-between">
                        <div className="flex justify-between items-start mb-4">
                            <div className="flex items-center gap-2">
                                <div className={`w-2 h-2 rounded-full ${isActive ? 'bg-neon-orange shadow-[0_0_8px_#ff6b6b]' : 'bg-gray-700'} transition-all duration-300`} />
                                <div>
                                    <h4 className="font-semibold text-gray-200 text-xs tracking-wide">{act.alias}</h4>
                                    <span className="text-[10px] text-gray-500 uppercase">{act.type}</span>
                                </div>
                            </div>
                            {act.manual_lock && (
                                <div className="flex items-center text-red-400 animate-pulse text-xs bg-red-400/10 px-1 py-0.5 rounded">
                                    <ShieldAlert className="w-3 h-3 mr-1" />
                                    {t('locked')}
                                </div>
                            )}
                        </div>

                        <div className="flex items-center justify-between gap-2 mt-2">
                            <button
                                onClick={() => toggleAutoMode(act.actuator_id, isAuto)}
                                className={`flex flex-1 items-center justify-center gap-1 py-1 px-2 rounded border text-xs font-medium transition-all ${isAuto ? 'bg-neon-blue/10 border-neon-blue/40 text-neon-blue' : 'bg-[#1a1c23] border-gray-600 text-gray-400'}`}
                            >
                                <Settings2 className="w-3 h-3" />
                                {isAuto ? t('auto') : t('manual')}
                            </button>

                            <button
                                onClick={() => toggleCommand(act.actuator_id, act.status)}
                                disabled={isAuto || act.manual_lock}
                                className={`flex-1 flex items-center justify-center gap-1 py-1 px-2 rounded border text-xs font-bold uppercase transition-all duration-300 ${buttonColor} ${isAuto || act.manual_lock ? 'opacity-50 cursor-not-allowed grayscale' : 'hover:bg-neon-orange/30'}`}
                            >
                                <Power className="w-3.5 h-3.5" />
                                {isActive ? t('running') : t('stopped')}
                            </button>
                        </div>
                    </div>
                );
            })}
        </div>
    );
}
