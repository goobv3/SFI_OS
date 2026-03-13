import { useState, useEffect } from 'react';
import { smartFarmApi } from '../api/client';
import { X, Plus, Trash2, Home, Activity, Power, Settings, Edit2, Save, XCircle, Inbox } from 'lucide-react';
import { useLanguage } from '../i18n/LanguageContext';

interface ManageModeProps {
    onClose: () => void;
    onUpdate: () => void;
}

export default function ManageMode({ onClose, onUpdate }: ManageModeProps) {
    const { t } = useLanguage();
    const [houses, setHouses] = useState<any[]>([]);
    const [newHouseId, setNewHouseId] = useState('');
    const [newHouseName, setNewHouseName] = useState('');

    const [editingHouseId, setEditingHouseId] = useState('');
    const [editHouseName, setEditHouseName] = useState('');
    const [editHouseOrder, setEditHouseOrder] = useState<number | string>(0);

    const [selectedHouseId, setSelectedHouseId] = useState<string>('');
    const [houseDevices, setHouseDevices] = useState<{ sensors: any[], actuators: any[] }>({ sensors: [], actuators: [] });

    // Discovery State
    const [viewMode, setViewMode] = useState<'house_editor' | 'inbox'>('house_editor');
    const [discoveredDevices, setDiscoveredDevices] = useState<any[]>([]);
    const [isScanning, setIsScanning] = useState(false);
    const [scanProgress, setScanProgress] = useState(0);

    // Register from Inbox State
    const [inboxForms, setInboxForms] = useState<Record<string, { house_id: string, alias: string, type: string, unit: string }>>({});

    // New Device Forms
    const [newSensor, setNewSensor] = useState({ id: '', alias: '', type: 'temperature', unit: 'C' });
    const [newActuator, setNewActuator] = useState({ id: '', alias: '', type: 'HEATER' });

    // Edit Device State
    const [editingSensorId, setEditingSensorId] = useState('');
    const [editSensorForm, setEditSensorForm] = useState({ alias: '', type: '', unit: '' });

    const [editingActuatorId, setEditingActuatorId] = useState('');
    const [editActuatorForm, setEditActuatorForm] = useState({ alias: '', type: '' });

    // Drag and Drop State
    const [draggedHouseIndex, setDraggedHouseIndex] = useState<number | null>(null);
    const [dragOverHouseIndex, setDragOverHouseIndex] = useState<number | null>(null);

    const [draggedSensorIndex, setDraggedSensorIndex] = useState<number | null>(null);
    const [dragOverSensorIndex, setDragOverSensorIndex] = useState<number | null>(null);

    // Dirty State (Unsaved Changes)
    const [isDirty, setIsDirty] = useState(false);

    // House Drag & Drop Handlers
    const onHouseDragStart = (e: React.DragEvent, index: number) => {
        setDraggedHouseIndex(index);
        e.dataTransfer.effectAllowed = 'move';
        e.dataTransfer.setData('text/plain', index.toString());
    };

    const onHouseDragEnter = (index: number) => {
        setDragOverHouseIndex(index);
    };

    const onHouseDragEnd = async () => {
        if (draggedHouseIndex === null || dragOverHouseIndex === null || draggedHouseIndex === dragOverHouseIndex) {
            setDraggedHouseIndex(null);
            setDragOverHouseIndex(null);
            return;
        }

        const newHouses = [...houses];
        const draggedItem = newHouses.splice(draggedHouseIndex, 1)[0];
        newHouses.splice(dragOverHouseIndex, 0, draggedItem);

        setHouses(newHouses);
        setDraggedHouseIndex(null);
        setDragOverHouseIndex(null);
        setIsDirty(true);
    };

    // Sensor Drag & Drop Handlers
    const onSensorDragStart = (e: React.DragEvent, index: number) => {
        setDraggedSensorIndex(index);
        e.dataTransfer.effectAllowed = 'move';
        e.dataTransfer.setData('text/plain', index.toString());
    };

    const onSensorDragEnter = (index: number) => {
        setDragOverSensorIndex(index);
    };

    const onSensorDragEnd = async () => {
        if (draggedSensorIndex === null || dragOverSensorIndex === null || draggedSensorIndex === dragOverSensorIndex) {
            setDraggedSensorIndex(null);
            setDragOverSensorIndex(null);
            return;
        }

        const newSensors = [...houseDevices.sensors];
        const draggedItem = newSensors.splice(draggedSensorIndex, 1)[0];
        newSensors.splice(dragOverSensorIndex, 0, draggedItem);

        setHouseDevices(prev => ({ ...prev, sensors: newSensors }));
        setDraggedSensorIndex(null);
        setDragOverSensorIndex(null);
        setIsDirty(true);
    };

    const handleSaveAll = async () => {
        try {
            // Save Houses Order
            await Promise.all(
                houses.map((h, index) =>
                    smartFarmApi.updateHouse(h.house_id, h.name, index)
                )
            );

            // Save Sensors Order (only for the currently selected house, 
            // but this logic supports the user's primary use-case of editing one house at a time)
            if (houseDevices.sensors.length > 0) {
                await Promise.all(
                    houseDevices.sensors.map((s, index) =>
                        smartFarmApi.updateSensor(s.sensor_id, {
                            alias: s.alias,
                            type: s.type,
                            unit: s.unit,
                            display_order: index,
                            is_active: s.is_active,
                            warn_high: s.warn_high,
                            warn_low: s.warn_low,
                            crit_high: s.crit_high,
                            crit_low: s.crit_low
                        })
                    )
                );
            }

            setIsDirty(false);
            onUpdate(); // Re-fetch dashboard
        } catch (e) {
            console.error("Failed to save changes", e);
            alert("Failed to save changes. Please try again.");
        }
    };

    const handleCancel = () => {
        fetchHouses();
        if (selectedHouseId && viewMode === 'house_editor') {
            loadDevices(selectedHouseId);
        }
        setIsDirty(false);
    };

    const fetchHouses = async () => {
        try {
            const data = await smartFarmApi.getHouses();
            setHouses(data);
        } catch (e) {
            console.error(e);
        }
    };

    const fetchDiscovered = async () => {
        try {
            const data = await smartFarmApi.getDiscoveredDevices();
            setDiscoveredDevices(data);
        } catch (e) {
            console.error(e);
        }
    };

    // Auto-Discovery Scanner Logic
    useEffect(() => {
        let fetchInterval: ReturnType<typeof setInterval>;
        let tickInterval: ReturnType<typeof setInterval>;

        if (isScanning) {
            fetchDiscovered();
            setScanProgress(30);

            fetchInterval = setInterval(() => {
                fetchDiscovered();
            }, 3000);

            tickInterval = setInterval(() => {
                setScanProgress((prev) => {
                    const next = prev - 1;
                    if (next <= 0) {
                        setIsScanning(false);
                        return 0;
                    }
                    return next;
                });
            }, 1000);
        }

        return () => {
            clearInterval(fetchInterval);
            clearInterval(tickInterval);
        };
    }, [isScanning]);

    const loadDevices = async (hId: string) => {
        try {
            const data = await smartFarmApi.getHouseDevices(hId);
            setHouseDevices(data);
        } catch (e) {
            console.error(e);
        }
    };

    useEffect(() => {
        fetchHouses();
    }, []);

    useEffect(() => {
        if (selectedHouseId && viewMode === 'house_editor') {
            loadDevices(selectedHouseId);
        } else {
            setHouseDevices({ sensors: [], actuators: [] });
        }
        setEditingSensorId('');
        setEditingActuatorId('');
    }, [selectedHouseId, viewMode]);

    // House Actions
    const handleAddHouse = async () => {
        if (!newHouseId || !newHouseName) return;
        try {
            await smartFarmApi.createHouse(newHouseId, newHouseName);
            setNewHouseId('');
            setNewHouseName('');
            fetchHouses();
            onUpdate();
        } catch (e) {
            alert("Failed to create house");
        }
    };

    const handleUpdateHouse = async (houseId: string) => {
        if (!editHouseName) return;
        try {
            await smartFarmApi.updateHouse(houseId, editHouseName, Number(editHouseOrder) || 0);
            setEditingHouseId('');
            fetchHouses();
            onUpdate();
        } catch (error) {
            alert("Failed to update house name");
        }
    };

    const handleDeleteHouse = async (houseId: string) => {
        if (!window.confirm(`Are you sure you want to delete ${houseId}?`)) return;
        try {
            await smartFarmApi.deleteHouse(houseId);
            if (selectedHouseId === houseId) setSelectedHouseId('');
            fetchHouses();
            onUpdate();
        } catch (e) {
            alert("Failed to delete house");
        }
    };

    // Sensor Actions
    const handleAddSensor = async () => {
        if (!newSensor.id || !newSensor.alias) return alert("Fill required sensor fields");
        try {
            await smartFarmApi.createSensor({
                sensor_id: newSensor.id, house_id: selectedHouseId, alias: newSensor.alias, type: newSensor.type, unit: newSensor.unit
            });
            setNewSensor({ id: '', alias: '', type: 'temperature', unit: 'C' });
            loadDevices(selectedHouseId);
        } catch (error) {
            alert("Failed to add sensor");
        }
    };

    const handleUpdateSensor = async (sensorId: string) => {
        try {
            await smartFarmApi.updateSensor(sensorId, editSensorForm);
            setEditingSensorId('');
            loadDevices(selectedHouseId);
        } catch (error) {
            alert("Failed to update sensor");
        }
    };

    const handleDeleteSensor = async (sensorId: string) => {
        if (!window.confirm(`Delete sensor ${sensorId}?`)) return;
        try {
            await smartFarmApi.deleteSensor(sensorId);
            loadDevices(selectedHouseId);
        } catch (error) {
            alert("Failed to delete sensor");
        }
    };

    // Actuator Actions
    const handleAddActuator = async () => {
        if (!newActuator.id || !newActuator.alias) return alert("Fill required actuator fields");
        try {
            await smartFarmApi.createActuator({
                actuator_id: newActuator.id, house_id: selectedHouseId, alias: newActuator.alias, type: newActuator.type
            });
            setNewActuator({ id: '', alias: '', type: 'HEATER' });
            loadDevices(selectedHouseId);
        } catch (error) {
            alert("Failed to add actuator");
        }
    };

    const handleUpdateActuator = async (actuatorId: string) => {
        try {
            await smartFarmApi.updateActuator(actuatorId, editActuatorForm);
            setEditingActuatorId('');
            loadDevices(selectedHouseId);
        } catch (error) {
            alert("Failed to update actuator");
        }
    };

    const handleDeleteActuator = async (actuatorId: string) => {
        if (!window.confirm(`Delete actuator ${actuatorId}?`)) return;
        try {
            await smartFarmApi.deleteActuator(actuatorId);
            loadDevices(selectedHouseId);
        } catch (error) {
            alert("Failed to delete actuator");
        }
    };

    // Inbox Actions
    const handleInboxFormChange = (deviceId: string, field: string, value: string) => {
        setInboxForms(prev => ({
            ...prev,
            [deviceId]: {
                ...(prev[deviceId] || { house_id: houses[0]?.house_id || '', alias: '', type: 'temperature', unit: 'C' }),
                [field]: value
            }
        }));
    };

    const handleRegisterFromInbox = async (device: any) => {
        const form = inboxForms[device.device_id];
        if (!form || !form.house_id || !form.alias) return alert(t('fillRequired'));

        try {
            if (device.device_type === 'sensor') {
                await smartFarmApi.createSensor({
                    sensor_id: device.device_id,
                    house_id: form.house_id,
                    alias: form.alias,
                    type: form.type,
                    unit: form.unit
                });
            }
            await smartFarmApi.deleteDiscoveredDevice(device.device_id);
            fetchDiscovered();
            alert(`${t('deviceRegistered')}`);
        } catch (e) {
            alert(t('failedToSave'));
        }
    };

    const handleDeleteFromInbox = async (deviceId: string) => {
        if (!window.confirm(`${t('ignoreDevice')} (${deviceId})`)) return;
        await smartFarmApi.deleteDiscoveredDevice(deviceId);
        fetchDiscovered();
    };

    return (
        <div className="fixed inset-0 bg-black/80 flex items-center justify-center z-[100] p-4 text-gray-200">
            <div className="bg-[#12141a] border border-cyber-border/40 rounded-xl max-w-5xl w-full h-[82vh] flex flex-col relative shadow-2xl overflow-hidden">
                {/* Header */}
                <div className="h-12 flex items-center justify-between px-4 border-b border-cyber-border/20 bg-black/40 shrink-0">
                    <div className="flex items-center gap-3">
                        <h2 className="text-base font-bold text-white tracking-widest flex items-center gap-2">
                            <Settings className="w-4 h-4 text-neon-blue" />
                        {t('sysConfig')}
                        </h2>
                        {isDirty && (
                            <div className="flex items-center gap-2 ml-4 animate-fade-in">
                                <span className="text-amber-400 text-xs font-semibold tracking-wide mr-1">{t('unsavedChanges')}</span>
                                <button onClick={handleCancel} className="bg-gray-700 hover:bg-gray-600 text-white text-xs font-bold py-1 px-3 rounded transition">
                                    {t('cancel')}
                                </button>
                                <button onClick={handleSaveAll} className="bg-neon-blue hover:bg-blue-400 text-black shadow-[0_0_10px_rgba(0,240,255,0.4)] text-xs font-bold py-1 px-3 rounded transition flex items-center gap-1">
                                    <Save className="w-3 h-3" /> {t('saveOrdering')}
                                </button>
                            </div>
                        )}
                    </div>
                    <button
                        onClick={() => {
                            if (isDirty) {
                                if (window.confirm(t('unsavedCloseConfirm'))) {
                                    onClose();
                                }
                            } else {
                                onClose();
                            }
                        }}
                        className="text-gray-400 hover:text-white transition-colors"
                    >
                        <X className="w-5 h-5" />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 flex overflow-hidden">
                    {/* Left Panel: Navigation (Houses & Inbox) */}
                    <div className="w-1/3 border-r border-cyber-border/20 bg-[#161821] flex flex-col">

                        {/* Inbox Toggle Button */}
                        <div className="p-3 border-b border-cyber-border/20 bg-[#1c1e28]">
                            <button
                                onClick={() => setViewMode('inbox')}
                                className={`w-full flex items-center justify-between p-2.5 rounded-lg border transition text-sm ${viewMode === 'inbox' ? 'bg-amber-500/20 border-amber-500/50 text-amber-400 font-bold tracking-wide shadow-[0_0_15px_rgba(245,158,11,0.2)]' : 'bg-black/40 border-gray-700 text-gray-400 hover:bg-white/5'}`}
                            >
                                <div className="flex items-center gap-2">
                                    <Inbox className="w-4 h-4" />
                                    <span>{t('discoveryInbox')}</span>
                                </div>
                                {discoveredDevices.length > 0 && (
                                    <span className="bg-amber-500 text-black text-xs font-bold px-2 py-0.5 rounded-full animate-pulse">
                                        {discoveredDevices.length} {t('new')}
                                    </span>
                                )}
                            </button>
                        </div>

                        {/* House Creation */}
                        <div className="p-3 border-b border-cyber-border/10">
                                <h3 className="text-xs font-semibold text-gray-400 uppercase tracking-wider mb-2">{t('addLocation')}</h3>
                            <div className="space-y-2">
                                <input
                                    type="text" placeholder={t('houseIdPlaceholder')}
                                    value={newHouseId} onChange={e => setNewHouseId(e.target.value)}
                                    className="w-full bg-black/50 border border-gray-700 rounded px-2 py-1 text-xs focus:border-neon-blue outline-none"
                                />
                                <input
                                    type="text" placeholder={t('houseNamePlaceholder')}
                                    value={newHouseName} onChange={e => setNewHouseName(e.target.value)}
                                    className="w-full bg-black/50 border border-gray-700 rounded px-3 py-1.5 text-sm focus:border-neon-blue outline-none"
                                />
                                <button onClick={handleAddHouse} className="w-full bg-neon-blue/20 hover:bg-neon-blue/30 text-neon-blue border border-neon-blue/40 text-sm py-1.5 rounded transition">
                                    <Plus className="w-4 h-4 inline-block mr-1" /> {t('create')}
                                </button>
                            </div>
                        </div>

                        {/* House List */}
                        <div className="flex-1 overflow-y-auto p-2 space-y-1">
                            <h3 className="text-xs font-semibold text-gray-500 uppercase tracking-wider px-2 py-1 mb-1">{t('registeredHouses')}</h3>
                            {houses.map((h, index) => (
                                <div
                                    key={h.house_id}
                                    draggable
                                    onDragStart={(e) => onHouseDragStart(e, index)}
                                    onDragEnter={() => onHouseDragEnter(index)}
                                    onDragEnd={onHouseDragEnd}
                                    onDragOver={(e) => e.preventDefault()}
                                    className={`flex flex-col p-3 rounded cursor-pointer transition 
                                        ${selectedHouseId === h.house_id && viewMode === 'house_editor' ? 'bg-neon-blue/10 border border-neon-blue/30 text-neon-blue' : 'hover:bg-white/5 border border-transparent text-gray-300'}
                                        ${dragOverHouseIndex === index ? 'border-t-2 border-t-neon-blue border-dashed' : ''}
                                        ${draggedHouseIndex === index ? 'opacity-50 bg-white/5' : ''}
                                    `}
                                    onClick={() => { setSelectedHouseId(h.house_id); setViewMode('house_editor'); }}
                                >
                                    {editingHouseId === h.house_id ? (
                                        <div className="flex items-center gap-2" onClick={e => e.stopPropagation()}>
                                            <input
                                                autoFocus
                                                value={editHouseName}
                                                onChange={e => setEditHouseName(e.target.value)}
                                                className="flex-[2] bg-black border border-gray-600 rounded px-2 py-1 text-sm outline-none text-white"
                                                placeholder="Name"
                                            />
                                            <input
                                                type="number"
                                                value={editHouseOrder}
                                                onChange={e => setEditHouseOrder(e.target.value)}
                                                className="flex-1 w-16 bg-black border border-gray-600 rounded px-2 py-1 text-sm outline-none text-white"
                                                placeholder="Order"
                                            />
                                            <button onClick={() => handleUpdateHouse(h.house_id)} className="text-green-400 hover:text-green-300"><Save className="w-4 h-4" /></button>
                                            <button onClick={() => setEditingHouseId('')} className="text-gray-400 hover:text-white"><XCircle className="w-4 h-4" /></button>
                                        </div>
                                    ) : (
                                        <div className="flex items-center justify-between">
                                            <div className="flex items-center gap-2 text-sm font-medium">
                                                <Home className="w-4 h-4" /> {h.name}
                                            </div>
                                            <div className="flex gap-2">
                                                <button onClick={(e) => { e.stopPropagation(); setEditingHouseId(h.house_id); setEditHouseName(h.name); setEditHouseOrder(h.display_order ?? 0); }} className="text-gray-500 hover:text-blue-400 transition">
                                                    <Edit2 className="w-4 h-4" />
                                                </button>
                                                <button onClick={(e) => { e.stopPropagation(); handleDeleteHouse(h.house_id); }} className="text-gray-500 hover:text-red-400 transition">
                                                    <Trash2 className="w-4 h-4" />
                                                </button>
                                            </div>
                                        </div>
                                    )}
                                </div>
                            ))}
                        </div>
                    </div>

                    {/* Right Panel: Content View */}
                    <div className="flex-1 bg-[#1a1c23] flex flex-col overflow-y-auto">

                        {/* --- VIEW MODE: DISCOVERY INBOX --- */}
                        {viewMode === 'inbox' && (
                            <div className="p-6 space-y-6 flex flex-col h-full">
                                <div className="border-b border-cyber-border/20 pb-4 flex items-end justify-between">
                                    <div>
                                        <h3 className="text-2xl font-bold tracking-wide text-amber-400 flex items-center gap-2">
                                            <Activity className="w-6 h-6" /> {t('autoDiscovery')}
                                        </h3>
                                        <p className="text-gray-400 text-sm mt-1">{t('autoDiscoveryDesc')}</p>
                                    </div>

                                    <div className="flex gap-3 items-center">
                                        {isScanning && (
                                            <div className="text-amber-500 font-mono text-sm mr-2 flex items-center gap-2 animate-pulse">
                                                {t('scanning')}... {scanProgress}{t('scanningProgress')}
                                            </div>
                                        )}
                                        <button
                                            onClick={() => isScanning ? setIsScanning(false) : setIsScanning(true)}
                                            className={`px-5 py-2 rounded-lg font-bold border transition-all flex items-center gap-2 ${isScanning
                                                ? 'bg-red-500/20 text-red-500 border-red-500/50 hover:bg-red-500/30'
                                                : 'bg-amber-500/20 text-amber-500 border-amber-500/50 hover:bg-amber-500/30'
                                                }`}
                                        >
                                            {isScanning ? <><XCircle className="w-4 h-4" /> {t('stopScan')}</> : <><Activity className="w-4 h-4" /> {t('startScan')}</>}
                                        </button>
                                    </div>
                                </div>

                                {/* Radar Animation Area (when scanning but no device) */}
                                {isScanning && discoveredDevices.length === 0 && (
                                    <div className="flex-1 flex flex-col items-center justify-center p-12 text-amber-500/80">
                                        <div className="relative w-32 h-32 mb-8">
                                            <div className="absolute inset-0 border-4 border-amber-500/30 rounded-full"></div>
                                            <div className="absolute inset-4 border-4 border-amber-500/20 rounded-full"></div>
                                            <div className="absolute inset-8 border-4 border-amber-500/10 rounded-full"></div>
                                            <div className="absolute inset-0 rounded-full border-t-4 border-amber-400 animate-[spin_2s_linear_infinite]"></div>
                                            <Activity className="absolute inset-0 m-auto w-8 h-8 text-amber-400 animate-pulse" />
                                        </div>
                                        <p className="text-lg font-bold animate-pulse">{t('listeningSignal')}</p>
                                        <p className="text-sm mt-2 text-gray-500">{t('listeningHint')}</p>
                                    </div>
                                )}

                                {!isScanning && discoveredDevices.length === 0 && (
                                    <div className="flex-1 flex flex-col items-center justify-center p-12 text-gray-500">
                                        <Inbox className="w-16 h-16 mb-4 opacity-50" />
                                        <p>{t('noDevicesFound')}</p>
                                        <p className="text-sm mt-2">{t('clickStartScan')}</p>
                                    </div>
                                )}

                                {discoveredDevices.length > 0 && (
                                    <div className="space-y-4 overflow-y-auto flex-1 pr-2">
                                        {discoveredDevices.map(device => {
                                            const form = inboxForms[device.device_id] || { house_id: houses[0]?.house_id || '', alias: '', type: 'temperature', unit: 'C' };
                                            return (
                                                <div key={device.device_id} className="bg-[#161821] border border-amber-500/30 rounded-lg p-5 shadow-lg shadow-amber-500/5 transition hover:border-amber-500/60">
                                                    <div className="flex justify-between items-start mb-4">
                                                        <div>
                                                            <div className="flex items-center gap-2">
                                                                <h4 className="font-bold text-lg text-white font-mono">{device.device_id}</h4>
                                                                <span className="bg-gray-800 text-xs px-2 py-0.5 rounded text-gray-300">{device.device_type.toUpperCase()}</span>
                                                            </div>
                                                            <div className="text-sm text-gray-400 mt-1">
                                                                {t('lastSignal')}: {new Date(device.last_seen).toLocaleString()} <span className="text-amber-400 font-bold ml-2">{t('value')}: {device.last_value}</span>
                                                            </div>
                                                        </div>
                                                        <button onClick={() => handleDeleteFromInbox(device.device_id)} className="text-gray-500 hover:text-red-400 p-1">
                                                            <Trash2 className="w-5 h-5" />
                                                        </button>
                                                    </div>

                                                    <div className="flex flex-wrap gap-3 items-end bg-black/40 p-4 rounded-md border border-gray-800">
                                                        <div className="flex-1 min-w-[150px]">
                                                            <label className="block text-xs text-gray-500 mb-1">{t('assignToHouse')}</label>
                                                            <select value={form.house_id} onChange={e => handleInboxFormChange(device.device_id, 'house_id', e.target.value)} className="w-full bg-[#1a1c23] border border-gray-700 rounded px-2 py-1.5 text-sm outline-none focus:border-amber-400">
                                                                {houses.map(h => <option key={h.house_id} value={h.house_id}>{h.name}</option>)}
                                                            </select>
                                                        </div>
                                                        <div className="flex-[1.5] min-w-[150px]">
                                                            <label className="block text-xs text-gray-500 mb-1">{t('aliasName')}</label>
                                                            <input type="text" placeholder="e.g. Main Temp" value={form.alias} onChange={e => handleInboxFormChange(device.device_id, 'alias', e.target.value)} className="w-full bg-[#1a1c23] border border-gray-700 rounded px-2 py-1.5 text-sm outline-none focus:border-amber-400" />
                                                        </div>
                                                        <div className="w-28">
                                                            <label className="block text-xs text-gray-500 mb-1">{t('type')}</label>
                                                            <select value={form.type} onChange={e => handleInboxFormChange(device.device_id, 'type', e.target.value)} className="w-full bg-[#1a1c23] border border-gray-700 rounded px-2 py-1.5 text-sm outline-none focus:border-amber-400">
                                                                <option value="temperature">{t('typeTemp')}</option>
                                                                <option value="humidity">{t('typeHumidity')}</option>
                                                                <option value="solar">{t('typeSolar')}</option>
                                                                <option value="soil_temp">{t('typeSoilTemp')}</option>
                                                            </select>
                                                        </div>
                                                        <div className="w-16">
                                                            <label className="block text-xs text-gray-500 mb-1">{t('unit')}</label>
                                                            <input type="text" placeholder="C, %" value={form.unit} onChange={e => handleInboxFormChange(device.device_id, 'unit', e.target.value)} className="w-full bg-[#1a1c23] border border-gray-700 rounded px-2 py-1.5 text-sm outline-none focus:border-amber-400 text-center" />
                                                        </div>
                                                        <button onClick={() => handleRegisterFromInbox(device)} className="bg-amber-500 hover:bg-amber-400 text-black font-bold py-1.5 px-4 rounded transition">
                                                            {t('register')}
                                                        </button>
                                                    </div>
                                                </div>
                                            )
                                        })}
                                    </div>
                                )}
                            </div>
                        )}

                        {/* --- VIEW MODE: HOUSE EDITOR (MANUAL) --- */}
                        {viewMode === 'house_editor' && selectedHouseId && (
                            <div className="p-6 h-full space-y-8">
                                <h3 className="text-xl font-bold tracking-wide text-neon-blue border-b border-cyber-border/20 pb-3">
                                    {t('deviceEditor')}: {houses.find(h => h.house_id === selectedHouseId)?.name}
                                </h3>

                                {/* Sensors Section */}
                                <section>
                                    <div className="flex items-center justify-between mb-4">
                                        <h4 className="flex items-center gap-2 font-semibold text-neon-green uppercase tracking-wider"><Activity className="w-5 h-5" /> {t('sensorsSection')}</h4>
                                    </div>

                                    {/* Add Sensor Form */}
                                    <div className="bg-[#161821] border border-cyber-border/20 p-4 rounded-lg flex flex-wrap gap-2 items-center mb-4 text-sm">
                                        <span className="text-gray-500 font-medium mr-2">{t('manualAdd')}</span>
                                        <input type="text" placeholder={t('sensorIdPlaceholder')} value={newSensor.id} onChange={e => setNewSensor({ ...newSensor, id: e.target.value })} className="bg-black/50 border border-gray-700 rounded px-2 py-1.5 flex-1 min-w-[120px]" />
                                        <input type="text" placeholder={t('sensorAliasPlaceholder')} value={newSensor.alias} onChange={e => setNewSensor({ ...newSensor, alias: e.target.value })} className="bg-black/50 border border-gray-700 rounded px-2 py-1.5 flex-1 min-w-[120px]" />
                                        <select value={newSensor.type} onChange={e => setNewSensor({ ...newSensor, type: e.target.value })} className="bg-black/50 border border-gray-700 rounded px-2 py-1.5 w-32">
                                            <option value="temperature">{t('typeTemp')}</option>
                                            <option value="humidity">{t('typeHumidity')}</option>
                                            <option value="solar">{t('typeSolar')}</option>
                                        </select>
                                        <input type="text" placeholder="Unit (C, %)" value={newSensor.unit} onChange={e => setNewSensor({ ...newSensor, unit: e.target.value })} className="bg-black/50 border border-gray-700 rounded px-2 py-1.5 w-16 text-center" />
                                        <button onClick={handleAddSensor} className="bg-neon-green/20 hover:bg-neon-green/30 text-neon-green border border-neon-green/40 px-3 py-1.5 rounded transition"><Plus className="w-4 h-4" /></button>
                                    </div>

                                    {/* Sensor List */}
                                    <div className="flex flex-col gap-3">
                                        {houseDevices.sensors.map((s, index) => (
                                            <div
                                                key={s.sensor_id}
                                                draggable
                                                onDragStart={(e) => onSensorDragStart(e, index)}
                                                onDragEnter={() => onSensorDragEnter(index)}
                                                onDragEnd={onSensorDragEnd}
                                                onDragOver={(e) => e.preventDefault()}
                                                className={`bg-black/40 border border-gray-800 p-3 rounded flex flex-col gap-2 text-sm justify-between shadow-sm cursor-grab active:cursor-grabbing transition
                                                    ${dragOverSensorIndex === index ? 'border-t-2 border-t-neon-green border-dashed' : ''}
                                                    ${draggedSensorIndex === index ? 'opacity-50 bg-white/5' : ''}
                                                `}
                                            >
                                                {editingSensorId === s.sensor_id ? (
                                                    <div className="flex flex-col gap-2">
                                                        <input value={editSensorForm.alias} onChange={e => setEditSensorForm({ ...editSensorForm, alias: e.target.value })} className="bg-black border border-gray-600 rounded px-2 py-1 text-sm outline-none" />
                                                        <div className="flex gap-2">
                                                            <input value={editSensorForm.type} onChange={e => setEditSensorForm({ ...editSensorForm, type: e.target.value })} className="bg-black border border-gray-600 rounded px-2 py-1 text-sm outline-none flex-1" />
                                                            <input value={editSensorForm.unit} onChange={e => setEditSensorForm({ ...editSensorForm, unit: e.target.value })} className="bg-black border border-gray-600 rounded px-2 py-1 text-sm outline-none w-16 text-center" />
                                                        </div>
                                                        <div className="flex justify-end gap-2 mt-1">
                                                            <button onClick={() => handleUpdateSensor(s.sensor_id)} className="text-green-400 hover:text-green-300"><Save className="w-4 h-4" /></button>
                                                            <button onClick={() => setEditingSensorId('')} className="text-gray-400 hover:text-white"><XCircle className="w-4 h-4" /></button>
                                                        </div>
                                                    </div>
                                                ) : (
                                                    <>
                                                        <div className="flex justify-between items-start">
                                                            <span className="font-semibold text-white">{s.alias}</span>
                                                            <div className="flex gap-2">
                                                                <button onClick={() => { setEditingSensorId(s.sensor_id); setEditSensorForm({ alias: s.alias, type: s.type, unit: s.unit }); }} className="text-gray-500 hover:text-blue-400 pt-0.5"><Edit2 className="w-3.5 h-3.5" /></button>
                                                                <button onClick={() => handleDeleteSensor(s.sensor_id)} className="text-gray-500 hover:text-red-400 pt-0.5"><Trash2 className="w-3.5 h-3.5" /></button>
                                                            </div>
                                                        </div>
                                                        <span className="text-gray-500 text-xs font-mono">{s.sensor_id} / {s.type} [{s.unit}]</span>
                                                    </>
                                                )}
                                            </div>
                                        ))}
                                        {houseDevices.sensors.length === 0 && <div className="text-gray-600 text-sm py-2">{t('noSensorsReg')}</div>}
                                    </div>
                                </section>

                                <hr className="border-cyber-border/10" />

                                {/* Actuators Section */}
                                <section>
                                    <div className="flex items-center justify-between mb-4">
                                        <h4 className="flex items-center gap-2 font-semibold text-neon-orange uppercase tracking-wider"><Power className="w-5 h-5" /> {t('actuatorsSection')}</h4>
                                    </div>

                                    {/* Add Actuator Form */}
                                    <div className="bg-[#161821] border border-cyber-border/20 p-4 rounded-lg flex flex-wrap gap-2 items-center mb-4 text-sm">
                                        <span className="text-gray-500 font-medium mr-2">{t('manualAdd')}</span>
                                        <input type="text" placeholder={t('actuatorIdPlaceholder')} value={newActuator.id} onChange={e => setNewActuator({ ...newActuator, id: e.target.value })} className="bg-black/50 border border-gray-700 rounded px-2 py-1.5 flex-1 min-w-[120px]" />
                                        <input type="text" placeholder={t('actuatorAliasPlaceholder')} value={newActuator.alias} onChange={e => setNewActuator({ ...newActuator, alias: e.target.value })} className="bg-black/50 border border-gray-700 rounded px-2 py-1.5 flex-1 min-w-[120px]" />
                                        <select value={newActuator.type} onChange={e => setNewActuator({ ...newActuator, type: e.target.value })} className="bg-black/50 border border-gray-700 rounded px-2 py-1.5 w-32">
                                            <option value="ROOF_WINDOW">{t('typeRoofWindow')}</option>
                                            <option value="HEATER">{t('typeHeater')}</option>
                                            <option value="COOLER">{t('typeCooler')}</option>
                                            <option value="PUMP">{t('typePump')}</option>
                                        </select>
                                        <button onClick={handleAddActuator} className="bg-neon-orange/20 hover:bg-neon-orange/30 text-neon-orange border border-neon-orange/40 px-3 py-1.5 rounded transition"><Plus className="w-4 h-4" /></button>
                                    </div>

                                    {/* Actuators List */}
                                    <div className="grid grid-cols-1 lg:grid-cols-2 gap-3">
                                        {houseDevices.actuators.map(a => (
                                            <div key={a.actuator_id} className="bg-black/40 border border-gray-800 p-3 rounded flex flex-col gap-2 text-sm justify-between shadow-sm">
                                                {editingActuatorId === a.actuator_id ? (
                                                    <div className="flex flex-col gap-2">
                                                        <input value={editActuatorForm.alias} onChange={e => setEditActuatorForm({ ...editActuatorForm, alias: e.target.value })} className="bg-black border border-gray-600 rounded px-2 py-1 text-sm outline-none" />
                                                        <input value={editActuatorForm.type} onChange={e => setEditActuatorForm({ ...editActuatorForm, type: e.target.value })} className="bg-black border border-gray-600 rounded px-2 py-1 text-sm outline-none" />
                                                        <div className="flex justify-end gap-2 mt-1">
                                                            <button onClick={() => handleUpdateActuator(a.actuator_id)} className="text-green-400 hover:text-green-300"><Save className="w-4 h-4" /></button>
                                                            <button onClick={() => setEditingActuatorId('')} className="text-gray-400 hover:text-white"><XCircle className="w-4 h-4" /></button>
                                                        </div>
                                                    </div>
                                                ) : (
                                                    <>
                                                        <div className="flex justify-between items-start">
                                                            <span className="font-semibold text-white">{a.alias}</span>
                                                            <div className="flex gap-2">
                                                                <button onClick={() => { setEditingActuatorId(a.actuator_id); setEditActuatorForm({ alias: a.alias, type: a.type }); }} className="text-gray-500 hover:text-blue-400 pt-0.5"><Edit2 className="w-3.5 h-3.5" /></button>
                                                                <button onClick={() => handleDeleteActuator(a.actuator_id)} className="text-gray-500 hover:text-red-400 pt-0.5"><Trash2 className="w-3.5 h-3.5" /></button>
                                                            </div>
                                                        </div>
                                                        <span className="text-gray-500 text-xs font-mono">{a.actuator_id} / {a.type}</span>
                                                    </>
                                                )}
                                            </div>
                                        ))}
                                        {houseDevices.actuators.length === 0 && <div className="text-gray-600 text-sm py-2">{t('noActuatorsReg')}</div>}
                                    </div>
                                </section>

                            </div>
                        )}

                        {/* Empty Selected House Placeholder */}
                        {viewMode === 'house_editor' && !selectedHouseId && (
                            <div className="flex-1 flex flex-col items-center justify-center text-gray-500 p-8 text-center space-y-4">
                                <Settings className="w-16 h-16 text-gray-700 mx-auto" />
                                <p>{t('selectHouseHint')}</p>
                            </div>
                        )}

                    </div>
                </div>
            </div>
        </div>
    );
}
