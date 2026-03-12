import { useState, useEffect } from 'react';
import { X, Save, Activity, Edit2 } from 'lucide-react';
import { smartFarmApi } from '../api/client';

interface QuickAddModalProps {
    houseId: string;
    houseName: string;
    devices: any[];
    onClose: () => void;
    onComplete: () => void;
}

export default function QuickAddModal({ houseId, houseName, devices, onClose, onComplete }: QuickAddModalProps) {
    // forms state maps device_id to its parsed form data
    const [forms, setForms] = useState<Record<string, any>>({});
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        // Smart Auto-Parsing Logic
        const initialForms: Record<string, any> = {};
        
        devices.forEach(device => {
            let type = 'temperature';
            let unit = 'C';
            let alias = device.device_id; // Default

            const upperId = device.device_id.toUpperCase();
            
            if (upperId.includes('TEMP')) {
                type = 'temperature';
                unit = 'C';
                alias = '온도 센서';
            } else if (upperId.includes('HUM') || upperId.includes('HUMID')) {
                type = 'humidity';
                unit = '%';
                alias = '습도 센서';
            } else if (upperId.includes('RAD') || upperId.includes('SOLAR')) {
                type = 'solar';
                unit = 'W/m2';
                alias = '일사량 센서';
            } else if (upperId.includes('CO2') || upperId.includes('GAS')) {
                type = 'co2';
                unit = 'ppm';
                alias = 'CO2 센서';
            } else if (upperId.includes('SOIL')) {
                type = 'soil_moisture';
                unit = '%';
                alias = '토양 수분 센서';
            } else if (upperId.includes('WIND')) {
                type = 'wind_speed';
                unit = 'm/s';
                alias = '풍속 센서';
            }

            // 아두이노 다중 센서를 위해 번호 붙여주기 
            // ex: TEMP_02 -> 온도 센서 02
            const numbers = upperId.match(/\d+/);
            if (numbers) {
                alias = `${alias} ${numbers[0]}`;
            }

            initialForms[device.device_id] = {
                house_id: houseId,
                alias,
                type,
                unit
            };
        });
        
        setForms(initialForms);
    }, [devices, houseId]);

    const handleFormChange = (id: string, field: string, value: string) => {
        setForms(prev => ({
            ...prev,
            [id]: { ...prev[id], [field]: value }
        }));
    };

    const handleBatchRegister = async () => {
        if (!houseId) {
            alert("No house selected. Please select a house from the left sidebar first.");
            return;
        }

        setLoading(true);
        try {
            await Promise.all(devices.map(async (device) => {
                const form = forms[device.device_id];
                
                if (device.device_type === 'sensor') {
                    await smartFarmApi.createSensor({
                        sensor_id: device.device_id,
                        house_id: houseId,
                        alias: form.alias,
                        type: form.type,
                        unit: form.unit
                    });
                } else if (device.device_type === 'actuator') {
                    await smartFarmApi.createActuator({
                        actuator_id: device.device_id,
                        house_id: houseId,
                        alias: form.alias,
                        type: form.type === 'temperature' ? 'HEATER' : form.type // fallback
                    });
                }
                // 성공적으로 DB에 넣었으면 미등록 큐에서 즉시 삭제
                await smartFarmApi.deleteDiscoveredDevice(device.device_id);
            }));
            
            setLoading(false);
            onComplete(); // App.tsx 등 부모 컴포넌트에 즉시 렌더링 갱신 요청
            
        } catch (e) {
            console.error("Batch Registration Error: ", e);
            alert("일부 기기 추가에 실패했습니다. 관리자 화면에서 재시도 해주세요.");
            setLoading(false);
        }
    };

    return (
        <div className="fixed inset-0 z-[100] flex flex-col items-center justify-center p-4">
            <div className="absolute inset-0 bg-black/80 backdrop-blur-sm" onClick={onClose} />
            
            <div className="bg-[#12141a] border border-amber-500/40 rounded-2xl w-full max-w-3xl relative shadow-[0_0_50px_rgba(245,158,11,0.15)] flex flex-col overflow-hidden transform transition-all animate-fade-in-up">
                
                {/* Header */}
                <div className="px-6 py-5 border-b border-amber-500/20 bg-gradient-to-r from-amber-500/10 to-transparent flex items-start justify-between">
                    <div className="flex gap-4">
                        <div className="bg-amber-500/20 p-3 rounded-full h-fit flex shrink-0">
                            <Activity className="w-8 h-8 text-amber-500" />
                        </div>
                        <div>
                            <h2 className="text-2xl font-bold text-white tracking-wide">새로운 센서 감지됨!</h2>
                            <p className="text-gray-400 text-sm mt-1">시스템이 자동으로 기기 종류를 파악했습니다. 클릭 한 번으로 <span className="text-neon-blue font-bold">[{houseName}]</span> 에 모두 등록합니다.</p>
                        </div>
                    </div>
                    <button onClick={onClose} className="text-gray-500 hover:text-white transition-colors p-2 shrink-0">
                        <X className="w-6 h-6" />
                    </button>
                </div>

                {/* Content - Inline Editable List */}
                <div className="p-6 overflow-y-auto max-h-[50vh] space-y-3 bg-[#161821]">
                    {devices.map((device, idx) => {
                        const form = forms[device.device_id] || {};
                        return (
                            <div key={device.device_id} className="group flex items-center gap-4 bg-[#1a1c23] border border-gray-700 hover:border-amber-500/50 rounded-lg p-3 transition-colors">
                                <div className="w-10 h-10 rounded bg-gray-800 flex items-center justify-center font-bold text-gray-400 shrink-0">
                                    {idx + 1}
                                </div>
                                <div className="w-1/4 min-w-[120px]">
                                    <div className="text-xs text-gray-500 font-medium mb-1 uppercase">Device ID</div>
                                    <div className="font-mono text-white tracking-wider">{device.device_id}</div>
                                </div>

                                {/* Inline Edit Fields */}
                                <div className="flex-1 min-w-[150px]">
                                    <div className="text-xs text-amber-500 font-medium mb-1 uppercase flex gap-1 items-center">
                                        Alias / Name 
                                        <Edit2 className="w-3 h-3 opacity-0 group-hover:opacity-100 transition-opacity" />
                                    </div>
                                    <input 
                                        type="text" 
                                        value={form.alias || ''} 
                                        onChange={e => handleFormChange(device.device_id, 'alias', e.target.value)}
                                        className="w-full bg-black/50 border border-gray-700 focus:border-neon-blue rounded px-3 py-1.5 text-sm text-white outline-none transition-colors"
                                    />
                                </div>
                                <div className="w-1/4 min-w-[100px]">
                                    <div className="text-xs text-amber-500 font-medium mb-1 uppercase">Type</div>
                                    <select 
                                        value={form.type || ''} 
                                        onChange={e => handleFormChange(device.device_id, 'type', e.target.value)}
                                        className="w-full bg-black/50 border border-gray-700 focus:border-neon-blue rounded px-2 py-1.5 text-sm text-white outline-none appearance-none"
                                    >
                                        <option value="temperature">온도</option>
                                        <option value="humidity">습도</option>
                                        <option value="solar">일사량</option>
                                        <option value="co2">CO2</option>
                                        <option value="soil_moisture">토양수분</option>
                                        <option value="wind_speed">풍속</option>
                                        <option value="custom">기타</option>
                                    </select>
                                </div>
                                <div className="w-20 min-w-[80px]">
                                    <div className="text-xs text-amber-500 font-medium mb-1 uppercase">Unit</div>
                                    <input 
                                        type="text" 
                                        value={form.unit || ''} 
                                        onChange={e => handleFormChange(device.device_id, 'unit', e.target.value)}
                                        className="w-full bg-black/50 border border-gray-700 focus:border-neon-blue rounded px-3 py-1.5 text-sm text-white outline-none"
                                    />
                                </div>
                            </div>
                        );
                    })}
                </div>

                {/* Footer - 1-Click Action */}
                <div className="p-6 border-t border-gray-800 bg-[#12141a] flex justify-end gap-3">
                    <button 
                        onClick={onClose}
                        className="px-6 py-2.5 rounded text-gray-400 hover:text-white hover:bg-white/5 font-semibold transition-colors"
                    >
                        다음에 하기
                    </button>
                    <button 
                        onClick={handleBatchRegister}
                        disabled={loading || !houseId}
                        className={`px-8 py-2.5 rounded font-bold transition-all flex items-center gap-2 shadow-lg 
                            ${loading || !houseId ? 'bg-gray-700 text-gray-500 cursor-not-allowed' : 'bg-neon-blue text-black hover:bg-blue-400 hover:shadow-[0_0_15px_rgba(0,240,255,0.6)] hover:scale-105'}
                        `}
                    >
                        {loading ? '등록 중...' : (
                            <>
                                <Save className="w-5 h-5" /> 
                                모두 자동 등록하기
                            </>
                        )}
                    </button>
                </div>
                
            </div>
        </div>
    );
}
