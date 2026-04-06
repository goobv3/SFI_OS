import { useState, useEffect } from 'react';
import {
    LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, Legend
} from 'recharts';
import { smartFarmApi } from '../api/client';
import { X, Calendar } from 'lucide-react';

interface HistoryChartProps {
    sensors: { id: string, name: string, type: string }[];
    initialSensorId: string;
    onClose: () => void;
}

export default function HistoryChart({ sensors, initialSensorId, onClose }: HistoryChartProps) {
    const [data, setData] = useState<{ time: string, [key: string]: any }[]>([]);
    const [loading, setLoading] = useState(true);

    const formatLocal = (date: Date) => {
        const pad = (n: number) => n.toString().padStart(2, '0');
        return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}T${pad(date.getHours())}:${pad(date.getMinutes())}`;
    };

    // Default to last 3 hours
    const defaultEnd = new Date();
    const defaultStart = new Date(defaultEnd.getTime() - (3 * 60 * 60 * 1000));

    // Time ranges passed to the backend
    const [appliedStart, setAppliedStart] = useState(formatLocal(defaultStart));
    const [appliedEnd, setAppliedEnd] = useState(formatLocal(defaultEnd));
    const [activePreset, setActivePreset] = useState<string>('3시간');

    // Display Toggles
    const [displayMode, setDisplayMode] = useState<'single' | 'both'>('single');
    const [showPicker, setShowPicker] = useState(false);

    // Temp state for modal
    const [tempStart, setTempStart] = useState("");
    const [tempEnd, setTempEnd] = useState("");

    const handlePreset = (hours: number, label: string) => {
        const end = new Date();
        const start = new Date(end.getTime() - (hours * 60 * 60 * 1000));
        setAppliedStart(formatLocal(start));
        setAppliedEnd(formatLocal(end));
        setActivePreset(label);
    };

    const fetchData = () => {
        setLoading(true);
        const startIso = new Date(appliedStart).toISOString();
        const endIso = new Date(appliedEnd).toISOString();

        const diffMs = new Date(appliedEnd).getTime() - new Date(appliedStart).getTime();
        const daysDiff = diffMs / (1000 * 60 * 60 * 24);
        
        let resolution = 'auto';
        if (daysDiff > 90) resolution = 'daily';
        else if (daysDiff > 7) resolution = 'hourly';
        else resolution = 'raw';

        const targets = displayMode === 'single' ? [initialSensorId] : sensors.map(s => s.id);

        smartFarmApi.getSensorHistoryByRange(targets, startIso, endIso, resolution)
            .then(res => {
                setData(res);
                setLoading(false);
            })
            .catch(err => {
                console.error(err);
                setLoading(false);
            });
    };

    useEffect(() => {
        fetchData();
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [displayMode, appliedStart, appliedEnd]);

    const openPicker = () => {
        setTempStart(appliedStart);
        setTempEnd(appliedEnd);
        setShowPicker(true);
    };

    const confirmPicker = () => {
        setAppliedStart(tempStart);
        setAppliedEnd(tempEnd);
        setActivePreset('');
        setShowPicker(false);
    };

    const getLineColor = (index: number, type: string) => {
        if (type.includes('temp')) return '#ff6b6b';
        if (type.includes('hum')) return '#4dabf7';
        const colors = ['#66fcf1', '#c5f6fa', '#fcc419', '#b197fc'];
        return colors[index % colors.length];
    };

    const initialSensor = sensors.find(s => s.id === initialSensorId);
    const titleName = displayMode === 'single' ? initialSensor?.name : 'House (All Sensors)';

    return (
        <div className="fixed inset-0 bg-black/80 flex items-center justify-center z-[100] p-4">
            <div className="bg-[#1a1c23] border border-gray-700 rounded-xl max-w-4xl w-full p-4 relative shadow-2xl flex flex-col max-h-[80vh] overflow-y-auto overflow-x-hidden">
                <button onClick={onClose} className="absolute top-3 right-3 text-gray-400 hover:text-white transition-colors z-50 bg-[#1a1c23]/80 p-1 rounded-full">
                    <X className="w-5 h-5" />
                </button>

                <div className="flex justify-between items-center mb-3 flex-shrink-0">
                    <h3 className="text-base font-bold text-white tracking-wide border-l-4 border-neon-blue pl-3">
                        {titleName} <span className="text-neon-blue font-light">History</span>
                    </h3>

                    {/* Display Mode Toggles */}
                    <div className="flex bg-cyber-bg border border-gray-700 rounded overflow-hidden">
                        <button
                            onClick={() => setDisplayMode('single')}
                            className={`px-4 py-1.5 text-xs font-medium transition-colors ${displayMode === 'single' ? 'bg-neon-blue text-black' : 'text-gray-400 hover:text-white hover:bg-gray-800'}`}
                        >
                            {initialSensor?.name} Only
                        </button>
                        <button
                            onClick={() => setDisplayMode('both')}
                            className={`px-4 py-1.5 text-xs font-medium transition-colors ${displayMode === 'both' ? 'bg-neon-blue text-black' : 'text-gray-400 hover:text-white hover:bg-gray-800'}`}
                        >
                            Temp & Humidity (All)
                        </button>
                    </div>
                </div>

                {/* Range Picker Controls */}
                <div className="flex flex-wrap items-center gap-3 mb-4 bg-cyber-bg/50 p-3 rounded-lg border border-gray-800 flex-shrink-0 relative">
                    <div className="flex items-center gap-2">
                        <Calendar className="w-4 h-4 text-neon-blue" />
                        <button
                            onClick={openPicker}
                            className="bg-cyber-surface border border-gray-700 hover:border-neon-blue transition-colors rounded px-4 py-1.5 text-sm text-gray-200 focus:outline-none"
                        >
                            {appliedStart.replace('T', ' ')} ~ {appliedEnd.replace('T', ' ')}
                        </button>
                    </div>

                    {showPicker && (
                        <div className="absolute top-16 left-4 bg-[#1a1c23] border border-gray-700 shadow-2xl rounded-lg p-5 z-50 flex flex-col gap-4">
                            <h4 className="text-white text-sm font-semibold border-b border-gray-700 pb-2">사용자 정의 기간 설정</h4>

                            <div className="flex flex-col gap-3">
                                <div className="flex items-center gap-3">
                                    <span className="text-gray-400 text-xs w-8">시작</span>
                                    <input type="date" value={tempStart.split('T')[0]}
                                        onChange={e => setTempStart(`${e.target.value}T${tempStart.split('T')[1]}`)}
                                        className="bg-cyber-surface border border-gray-700 rounded px-2 py-1 text-sm text-white" />
                                    <select value={tempStart.split('T')[1].split(':')[0]}
                                        onChange={e => setTempStart(`${tempStart.split('T')[0]}T${e.target.value}:${tempStart.split('T')[1].split(':')[1]}`)}
                                        className="bg-cyber-surface border border-gray-700 rounded px-2 py-1 text-sm text-white">
                                        {Array.from({ length: 24 }).map((_, i) => <option key={i} value={i.toString().padStart(2, '0')}>{i.toString().padStart(2, '0')}시</option>)}
                                    </select>
                                    <select value={tempStart.split('T')[1].split(':')[1]}
                                        onChange={e => setTempStart(`${tempStart.split('T')[0]}T${tempStart.split('T')[1].split(':')[0]}:${e.target.value}`)}
                                        className="bg-cyber-surface border border-gray-700 rounded px-2 py-1 text-sm text-white">
                                        {Array.from({ length: 12 }).map((_, i) => <option key={i} value={(i * 5).toString().padStart(2, '0')}>{(i * 5).toString().padStart(2, '0')}분</option>)}
                                    </select>
                                </div>

                                <div className="flex items-center gap-3">
                                    <span className="text-gray-400 text-xs w-8">종료</span>
                                    <input type="date" value={tempEnd.split('T')[0]}
                                        onChange={e => setTempEnd(`${e.target.value}T${tempEnd.split('T')[1]}`)}
                                        className="bg-cyber-surface border border-gray-700 rounded px-2 py-1 text-sm text-white" />
                                    <select value={tempEnd.split('T')[1].split(':')[0]}
                                        onChange={e => setTempEnd(`${tempEnd.split('T')[0]}T${e.target.value}:${tempEnd.split('T')[1].split(':')[1]}`)}
                                        className="bg-cyber-surface border border-gray-700 rounded px-2 py-1 text-sm text-white">
                                        {Array.from({ length: 24 }).map((_, i) => <option key={i} value={i.toString().padStart(2, '0')}>{i.toString().padStart(2, '0')}시</option>)}
                                    </select>
                                    <select value={tempEnd.split('T')[1].split(':')[1]}
                                        onChange={e => setTempEnd(`${tempEnd.split('T')[0]}T${tempEnd.split('T')[1].split(':')[0]}:${e.target.value}`)}
                                        className="bg-cyber-surface border border-gray-700 rounded px-2 py-1 text-sm text-white">
                                        {Array.from({ length: 12 }).map((_, i) => <option key={i} value={(i * 5).toString().padStart(2, '0')}>{(i * 5).toString().padStart(2, '0')}분</option>)}
                                    </select>
                                </div>
                            </div>

                            <div className="flex justify-end gap-2 mt-2">
                                <button onClick={() => setShowPicker(false)} className="px-3 py-1.5 text-xs text-gray-400 hover:text-white transition-colors">취소</button>
                                <button onClick={confirmPicker} className="px-4 py-1.5 bg-neon-blue/20 text-neon-blue border border-neon-blue/50 rounded hover:bg-neon-blue/40 transition-colors text-xs font-medium">적용 확인</button>
                            </div>
                        </div>
                    )}

                    {/* Presets */}
                    <div className="flex items-center gap-2 border-l border-gray-700 pl-4">
                        {[
                            { label: '3시간', hours: 3 },
                            { label: '12시간', hours: 12 },
                            { label: '24시간', hours: 24 },
                            { label: '7일', hours: 24 * 7 },
                            { label: '1개월', hours: 24 * 30 },
                            { label: '1년', hours: 24 * 365 }
                        ].map(p => (
                            <button
                                key={p.label}
                                onClick={() => handlePreset(p.hours, p.label)}
                                className={`relative px-3 py-1 rounded text-xs font-medium transition-all duration-300 border ${activePreset === p.label
                                    ? 'bg-neon-purple/20 border-neon-purple text-neon-purple shadow-[0_0_10px_rgba(157,78,221,0.5)]'
                                    : 'bg-transparent border-gray-700 text-gray-400 hover:text-gray-200 hover:border-gray-500 hover:shadow-[0_0_8px_rgba(255,255,255,0.1)]'
                                    }`}
                            >
                                {p.label}
                            </button>
                        ))}
                    </div>
                </div>

                {/* Chart Area */}
                <div className="flex-1 w-full flex flex-col items-center justify-center p-1 rounded-lg min-h-[280px]">
                    {loading ? (
                        <div className="w-full h-full flex flex-col items-center justify-center gap-4">
                            <div className="w-8 h-8 border-2 border-neon-blue border-t-transparent rounded-full animate-spin"></div>
                            <div className="text-neon-blue animate-pulse text-sm">Loading telemetry...</div>
                        </div>
                    ) : data.length === 0 ? (
                        <div className="w-full h-[350px] flex items-center justify-center text-gray-500">No data available for this range.</div>
                    ) : (
                        <div className="w-full relative h-[300px] pb-4">
                            <ResponsiveContainer width="100%" height="100%">
                                <LineChart data={data} margin={{ top: 10, right: 30, bottom: 30, left: 10 }}>
                                    <CartesianGrid strokeDasharray="3 3" stroke="#2a2c33" vertical={false} />
                                    <XAxis
                                        dataKey="time"
                                        stroke="#888"
                                        tick={{ fill: '#888', fontSize: 11 }}
                                        dy={10}
                                        minTickGap={30}
                                    />
                                    <YAxis
                                        yAxisId="left"
                                        stroke="#888"
                                        tick={{ fill: '#888', fontSize: 11 }}
                                        domain={['auto', 'auto']}
                                    />
                                    {displayMode === 'both' && (
                                        <YAxis
                                            yAxisId="right"
                                            orientation="right"
                                            stroke="#888"
                                            tick={{ fill: '#888', fontSize: 11 }}
                                            domain={['auto', 'auto']}
                                        />
                                    )}
                                    <Tooltip
                                        contentStyle={{ backgroundColor: 'rgba(26, 28, 35, 0.95)', border: '1px solid #45a29e', borderRadius: '8px', color: '#fff', boxShadow: '0 4px 6px -1px rgba(0, 0, 0, 0.5)' }}
                                        itemStyle={{ color: '#fff' }}
                                    />
                                    <Legend verticalAlign="top" height={36} />

                                    {displayMode === 'single' ? (
                                        <Line
                                            yAxisId="left"
                                            type="monotone"
                                            dataKey={initialSensorId}
                                            name={initialSensor?.name}
                                            stroke="#66fcf1"
                                            strokeWidth={2}
                                            dot={false}
                                            activeDot={{ r: 6, fill: '#66fcf1', stroke: '#fff', strokeWidth: 2 }}
                                        />
                                    ) : (
                                        sensors.map((sensor, idx) => (
                                            <Line
                                                key={sensor.id}
                                                yAxisId={sensor.type.includes('hum') ? 'right' : 'left'}
                                                type="monotone"
                                                dataKey={sensor.id}
                                                name={sensor.name}
                                                stroke={getLineColor(idx, sensor.type)}
                                                strokeWidth={2}
                                                dot={false}
                                                activeDot={{ r: 4, fill: getLineColor(idx, sensor.type), stroke: '#fff', strokeWidth: 2 }}
                                            />
                                        ))
                                    )}
                                </LineChart>
                            </ResponsiveContainer>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
}
