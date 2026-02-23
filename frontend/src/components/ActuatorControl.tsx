import { Power, Settings2, ShieldAlert } from 'lucide-react';
import { useState, useEffect } from 'react';
import { smartFarmApi } from '../api/client';

interface ActuatorControlProps {
    houseId: string;
}

export default function ActuatorControl({ houseId }: ActuatorControlProps) {
    const [actuators, setActuators] = useState<any[]>([]);
    const [loading, setLoading] = useState(false);

    const fetchActuators = async () => {
        setLoading(true);
        try {
            const data = await smartFarmApi.getHouseDevices(houseId);
            setActuators(data.actuators);
        } catch (e) {
            console.error(e);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        if (houseId) fetchActuators();
    }, [houseId]);

    const toggleCommand = async (id: string, currentStatus: string) => {
        const isActivating = currentStatus === 'Off' || currentStatus === 'Closed';
        const command = isActivating ? 'On' : 'Off';

        try {
            const result = await smartFarmApi.sendControlCommand(id, command);

            if (result.status === 'success') {
                setActuators(prev =>
                    prev.map(act => act.actuator_id === id ? { ...act, status: command } : act)
                );
            } else if (result.status === 'blocked') {
                alert(`🚨 Blocked: ${result.reason}`);
            }
        } catch (error: any) {
            alert(`API Error: ${error.message}`);
        }
    };

    // Auto Mode UI Toggle (Frontend logic only for now, as DB doesn't have it natively configured per actuator yet)
    const toggleAutoMode = (id: string, isAuto: boolean) => {
        // TODO: This should be synchronized with actual backend settings later
        setActuators(prev =>
            prev.map(act => act.actuator_id === id ? { ...act, isAuto: !isAuto } : act)
        );
    };

    if (loading) return <div className="text-neon-orange animate-pulse">Loading controls...</div>;
    if (!actuators.length) return <div className="text-gray-500">No actuators registered in this house.</div>;

    return (
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            {actuators.map((act) => {
                // Determine active state visually
                const isActive = act.status === 'On' || act.status === 'Open';
                const buttonColor = isActive ? 'bg-neon-orange/20 text-neon-orange border-neon-orange shadow-[0_0_15px_rgba(255,107,107,0.3)]' : 'bg-[#1a1c23] border-gray-700 text-gray-500';

                // Note: we inject a fake 'isAuto' state for UI demonstration if it doesn't exist
                const isAuto = act.isAuto ?? true;

                return (
                    <div key={act.actuator_id} className="bg-[#12141a] rounded-xl border border-cyber-border/30 p-5 relative overflow-hidden flex flex-col justify-between">

                        {/* Header: Status LED & Name */}
                        <div className="flex justify-between items-start mb-6">
                            <div className="flex items-center gap-3">
                                <div className={`w-3 h-3 rounded-full ${isActive ? 'bg-neon-orange shadow-[0_0_8px_#ff6b6b]' : 'bg-gray-700'} transition-all duration-300`} />
                                <div>
                                    <h4 className="font-semibold text-gray-200 tracking-wide">{act.alias}</h4>
                                    <span className="text-xs text-gray-500 uppercase tracking-widest">{act.type} / ID: {act.actuator_id}</span>
                                </div>
                            </div>
                            {act.manual_lock && (
                                <div className="flex items-center text-red-400 animate-pulse text-xs bg-red-400/10 px-2 py-1 rounded">
                                    <ShieldAlert className="w-4 h-4 mr-1" />
                                    LOCKED
                                </div>
                            )}
                        </div>

                        {/* Controls */}
                        <div className="flex items-center justify-between gap-4 mt-auto">

                            {/* Auto / Manual Toggle */}
                            <button
                                onClick={() => toggleAutoMode(act.actuator_id, isAuto)}
                                className={`flex flex-1 items-center justify-center gap-2 py-2 px-3 rounded-lg border text-sm font-medium transition-all ${isAuto ? 'bg-neon-blue/10 border-neon-blue/40 text-neon-blue' : 'bg-[#1a1c23] border-gray-600 text-gray-400'}`}
                            >
                                <Settings2 className="w-4 h-4" />
                                {isAuto ? 'AUTO' : 'MANUAL'}
                            </button>

                            {/* Power Toggle Button */}
                            <button
                                onClick={() => toggleCommand(act.actuator_id, act.status)}
                                disabled={isAuto || act.manual_lock}
                                className={`flex-1 flex items-center justify-center gap-2 py-2 px-3 rounded-lg border text-sm font-bold uppercase transition-all duration-300 ${buttonColor} ${isAuto || act.manual_lock ? 'opacity-50 cursor-not-allowed grayscale' : 'hover:bg-neon-orange/30'}`}
                            >
                                <Power className="w-5 h-5" />
                                {isActive ? 'Running' : 'Stopped'}
                            </button>

                        </div>
                    </div>
                );
            })}
        </div>
    );
}
