import { useState } from 'react';
import { X, Save } from 'lucide-react';
import { smartFarmApi } from '../api/client';
import { useLanguage } from '../i18n/LanguageContext';

interface SensorConfigModalProps {
    sensor: any;
    onClose: () => void;
    onSave: () => void;
}

export default function SensorConfigModal({ sensor, onClose, onSave }: SensorConfigModalProps) {
    const { t } = useLanguage();
    const [formData, setFormData] = useState({
        alias: sensor.alias || '',
        type: sensor.type || '',
        unit: sensor.unit || '',
        display_order: sensor.display_order || 0,
        is_active: sensor.is_active ?? true,
        warn_high: sensor.warn_high ?? '',
        warn_low: sensor.warn_low ?? '',
        crit_high: sensor.crit_high ?? '',
        crit_low: sensor.crit_low ?? ''
    });
    const [loading, setLoading] = useState(false);

    const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const { name, value, type, checked } = e.target;
        setFormData(prev => ({ ...prev, [name]: type === 'checkbox' ? checked : value }));
    };

    const handleSave = async () => {
        setLoading(true);
        try {
            const payload: any = {
                alias: formData.alias,
                type: formData.type,
                unit: formData.unit,
                display_order: Number(formData.display_order),
                is_active: formData.is_active
            };

            // Convert to null if empty string, else float
            const parseFloatOrNull = (val: any) => val === '' ? null : parseFloat(val);

            payload.warn_high = parseFloatOrNull(formData.warn_high);
            payload.warn_low = parseFloatOrNull(formData.warn_low);
            payload.crit_high = parseFloatOrNull(formData.crit_high);
            payload.crit_low = parseFloatOrNull(formData.crit_low);

            await smartFarmApi.updateSensor(sensor.sensor_id, payload);
            onSave();
        } catch (e) {
            console.error("Failed to update sensor config", e);
            alert(t('failedToSave'));
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm p-4">
            <div className="bg-[#1a1c23] border border-gray-700 rounded-xl max-w-md w-full p-6 relative shadow-2xl flex flex-col">
                <button
                    onClick={onClose}
                    className="absolute top-4 right-4 text-gray-400 hover:text-white transition-colors"
                >
                    <X className="w-6 h-6" />
                </button>

                <h2 className="text-xl font-bold text-white mb-6">⚙️ {t('sensorConfig')}</h2>

                <div className="space-y-4 overflow-y-auto max-h-[70vh] pr-2">
                    <div>
                        <label className="block text-xs text-gray-400 mb-1">Sensor ID</label>
                        <input type="text" disabled value={sensor.sensor_id} className="w-full bg-[#0f1115] border border-gray-700 text-gray-500 rounded p-2 text-sm" />
                    </div>

                    <div className="grid grid-cols-2 gap-4">
                        <div>
                            <label className="block text-xs text-gray-400 mb-1">{t('alias')}</label>
                            <input name="alias" value={formData.alias} onChange={handleChange} className="w-full bg-[#161821] border border-gray-600 text-white rounded p-2 text-sm focus:border-neon-blue focus:outline-none" />
                        </div>
                        <div>
                            <label className="block text-xs text-gray-400 mb-1">{t('displayOrder')}</label>
                            <input name="display_order" type="number" value={formData.display_order} onChange={handleChange} className="w-full bg-[#161821] border border-gray-600 text-white rounded p-2 text-sm focus:border-neon-blue focus:outline-none" />
                        </div>
                    </div>

                    {/* Data Collection Toggle */}
                    <div className="flex items-center gap-3 bg-[#111318] border border-gray-700 p-3 rounded mt-2">
                        <label className="relative inline-flex items-center cursor-pointer">
                            <input
                                type="checkbox"
                                name="is_active"
                                className="sr-only peer"
                                checked={formData.is_active}
                                onChange={handleChange}
                            />
                            <div className="w-11 h-6 bg-gray-700 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all peer-checked:bg-neon-blue"></div>
                        </label>
                        <div>
                            <span className="text-sm font-medium text-white block">Data Collection Active</span>
                            <span className="text-xs text-gray-500">If disabled, new sensor data will be ignored and alarms paused.</span>
                        </div>
                    </div>

                    <div className="border-t border-gray-700 pt-4 mt-2">
                        <h3 className="text-sm font-semibold text-neon-orange mb-3">{t('warning')} / {t('danger')} 임계값 설정</h3>
                        <div className="grid grid-cols-2 gap-4">
                            <div>
                                <label className="block text-xs text-red-400 mb-1">{t('critHigh')}</label>
                                <input name="crit_high" type="number" step="0.1" value={formData.crit_high} onChange={handleChange} placeholder="None" className="w-full bg-[#161821] border border-red-900/50 text-white rounded p-2 text-sm focus:border-red-500 focus:outline-none" />
                            </div>
                            <div>
                                <label className="block text-xs text-amber-400 mb-1">{t('warnHigh')}</label>
                                <input name="warn_high" type="number" step="0.1" value={formData.warn_high} onChange={handleChange} placeholder="None" className="w-full bg-[#161821] border border-amber-900/50 text-white rounded p-2 text-sm focus:border-amber-500 focus:outline-none" />
                            </div>
                            <div>
                                <label className="block text-xs text-amber-400 mb-1 mt-2">{t('warnLow')}</label>
                                <input name="warn_low" type="number" step="0.1" value={formData.warn_low} onChange={handleChange} placeholder="None" className="w-full bg-[#161821] border border-amber-900/50 text-white rounded p-2 text-sm focus:border-amber-500 focus:outline-none" />
                            </div>
                            <div>
                                <label className="block text-xs text-red-400 mb-1 mt-2">{t('critLow')}</label>
                                <input name="crit_low" type="number" step="0.1" value={formData.crit_low} onChange={handleChange} placeholder="None" className="w-full bg-[#161821] border border-red-900/50 text-white rounded p-2 text-sm focus:border-red-500 focus:outline-none" />
                            </div>
                        </div>
                    </div>
                </div>

                <div className="mt-6 flex justify-end gap-3">
                    <button onClick={onClose} className="px-4 py-2 rounded text-sm font-medium border border-gray-600 text-gray-300 hover:bg-gray-800 transition-colors">
                        {t('cancel')}
                    </button>
                    <button onClick={handleSave} disabled={loading} className="px-4 py-2 rounded text-sm font-medium bg-neon-blue text-black hover:bg-cyan-400 transition-colors flex items-center gap-2 disabled:opacity-50">
                        {loading ? <div className="w-4 h-4 border-2 border-black border-t-transparent rounded-full animate-spin"></div> : <Save className="w-4 h-4" />}
                        {loading ? t('saving') : t('save')}
                    </button>
                </div>
            </div>
        </div>
    );
}
