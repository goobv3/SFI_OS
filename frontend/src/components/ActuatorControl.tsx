import { Settings2, ShieldAlert, Loader2, Check } from 'lucide-react';
import { useState, useEffect } from 'react';
import { smartFarmApi } from '../api/client';
import { useLanguage } from '../i18n/LanguageContext';

interface ActuatorControlProps { houseId: string; }

export default function ActuatorControl({ houseId }: ActuatorControlProps) {
    const { t } = useLanguage();
    const [actuators, setActuators] = useState<any[]>([]);
    const [loading, setLoading] = useState(false);
    const [opStateMap, setOpStateMap] = useState<Record<string, 'sending' | 'success' | 'error' | null>>({});

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
        
        setOpStateMap(prev => ({ ...prev, [id]: 'sending' }));
        
        try {
            const result = await smartFarmApi.sendControlCommand(id, command);
            if (result.status === 'success') {
                setActuators(prev => prev.map(act => act.actuator_id === id ? { ...act, status: command } : act));
                setOpStateMap(prev => ({ ...prev, [id]: 'success' }));
                setTimeout(() => setOpStateMap(prev => ({ ...prev, [id]: null })), 800);
            } else if (result.status === 'blocked') {
                setOpStateMap(prev => ({ ...prev, [id]: 'error' }));
                setTimeout(() => setOpStateMap(prev => ({ ...prev, [id]: null })), 800);
                alert(`🚨 ${result.reason}`);
            }
        } catch (error: any) { 
            setOpStateMap(prev => ({ ...prev, [id]: 'error' }));
            setTimeout(() => setOpStateMap(prev => ({ ...prev, [id]: null })), 800);
            alert(`API Error: ${error.message}`); 
        }
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
                const isAuto = act.isAuto ?? true;
                const opState = opStateMap[act.actuator_id];
                const isSending = opState === 'sending';
                const isSuccess = opState === 'success';
                const isError = opState === 'error';

                return (
                    <div key={act.actuator_id} className="bg-[#12141a] rounded-lg border border-cyber-border/30 p-3 relative overflow-hidden flex flex-col justify-between">
                        
                        <style>{`
                            @keyframes shake {
                                0%, 100% { transform: translateX(0); }
                                25% { transform: translateX(-4px); }
                                75% { transform: translateX(4px); }
                            }
                            .animate-shake { animation: shake 0.3s ease-in-out; border-color: rgb(239 68 68) !important; }
                        `}</style>
                        
                        <div className="flex justify-between items-start mb-4">
                            <div className="flex items-center gap-2">
                                <div className={`w-2 h-2 rounded-full ${isActive ? 'bg-neon-orange shadow-[0_0_8px_#ff6b6b]' : 'bg-gray-700'} transition-all duration-300`} />
                                <div>
                                    <h4 className="font-semibold text-gray-200 text-xs tracking-wide">{act.alias}</h4>
                                    <span className="text-[10px] text-gray-500 uppercase">{act.type}</span>
                                </div>
                            </div>
                            {act.manual_lock && (
                                <div className="flex items-center text-red-400 animate-pulse text-[10px] sm:text-xs bg-red-400/10 px-1 py-0.5 rounded">
                                    <ShieldAlert className="w-3 h-3 mr-1" />
                                    {t('locked')}
                                </div>
                            )}
                        </div>

                        <div className="flex items-center justify-between gap-3 mt-2">
                            <button
                                onClick={() => toggleAutoMode(act.actuator_id, isAuto)}
                                className={`flex flex-1 items-center justify-center gap-1 py-1.5 px-2 rounded-md border text-[10px] sm:text-xs font-medium transition-all ${isAuto ? 'bg-neon-blue/10 border-neon-blue/40 text-neon-blue shadow-[0_0_10px_rgba(102,252,241,0.2)]' : 'bg-[#1a1c23] border-gray-600 text-gray-400'}`}
                            >
                                <Settings2 className="w-3 h-3" />
                                {isAuto ? t('auto') : t('manual')}
                            </button>

                            {/* 토글 스위치 */}
                            <button
                                onClick={() => toggleCommand(act.actuator_id, act.status)}
                                disabled={isAuto || act.manual_lock || isSending}
                                className={`
                                    relative flex items-center justify-center w-14 h-7 rounded-full p-1 transition-all duration-300 shrink-0 border
                                    ${isAuto || act.manual_lock ? 'opacity-50 cursor-not-allowed grayscale border-transparent bg-gray-800' : 
                                        isActive ? 'bg-neon-orange/20 border-neon-orange/50' : 'bg-gray-800 border-gray-600'}
                                    ${isError ? 'animate-shake' : ''}
                                `}
                            >
                                {isSending ? (
                                    <Loader2 className={`w-4 h-4 animate-spin ${isActive ? 'text-neon-orange' : 'text-gray-400'}`} />
                                ) : isSuccess ? (
                                    <Check className="w-4 h-4 text-neon-green/90 animate-fade-in-up" />
                                ) : (
                                    <>
                                        <span className={`absolute left-1.5 text-[9px] font-bold text-gray-400 transition-opacity ${isActive ? 'opacity-0' : 'opacity-100'}`}>OFF</span>
                                        <span className={`absolute right-1.5 text-[9px] font-bold text-neon-orange transition-opacity ${isActive ? 'opacity-100' : 'opacity-0'}`}>ON</span>
                                        
                                        <div className={`
                                            absolute w-5 h-5 bg-white rounded-full transition-all duration-300 ease-[cubic-bezier(0.4,0,0.2,1)] shadow-sm
                                            ${isActive ? 'transform translate-x-[14px] bg-neon-orange shadow-[0_0_8px_#ff6b6b]' : 'transform translate-x-[-14px] bg-gray-400'}
                                        `} />
                                    </>
                                )}
                            </button>
                        </div>
                    </div>
                );
            })}
        </div>
    );
}
