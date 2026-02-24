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

    // Default to last 3 hours
    const defaultEnd = new Date();
    const defaultStart = new Date(defaultEnd.getTime() - (3 * 60 * 60 * 1000));

    const [startTime, setStartTime] = useState(defaultStart.toISOString().slice(0, 16));
    const [endTime, setEndTime] = useState(defaultEnd.toISOString().slice(0, 16));
    const [activePreset, setActivePreset] = useState<string>('3h');

    // Display Toggles (Show Only Initial Sensor vs All)
    const [displayMode, setDisplayMode] = useState<'single' | 'both'>('single');

    const handlePreset = (hours: number, label: string) => {
        const end = new Date();
        const start = new Date(end.getTime() - (hours * 60 * 60 * 1000));
        setStartTime(start.toISOString().slice(0, 16));
        setEndTime(end.toISOString().slice(0, 16));
        setActivePreset(label);
    };

    const fetchData = () => {
        setLoading(true);
        const startIso = new Date(startTime).toISOString();
        const endIso = new Date(endTime).toISOString();

        const targets = displayMode === 'single' ? [initialSensorId] : sensors.map(s => s.id);

        smartFarmApi.getSensorHistoryByRange(targets, startIso, endIso)
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
    }, [displayMode]); // Reload data when mode changes

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
            <div className="bg-cyber-surface border border-cyber-border/50 rounded-xl max-w-5xl w-full p-6 relative shadow-2xl flex flex-col max-h-[90vh]">
                <button onClick={onClose} className="absolute top-4 right-4 text-gray-400 hover:text-white transition-colors">
                    <X className="w-6 h-6" />
                </button>

                <div className="flex justify-between items-center mb-4 flex-shrink-0">
                    <h3 className="text-xl font-bold text-white tracking-wide border-l-4 border-neon-blue pl-3">
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
                <div className="flex flex-wrap items-center gap-4 mb-6 bg-cyber-bg/50 p-4 rounded-lg border border-gray-800 flex-shrink-0">
                    <div className="flex items-center gap-2">
                        <Calendar className="w-4 h-4 text-neon-blue" />
                        <input
                            type="datetime-local"
                            value={startTime}
                            onChange={(e) => { setStartTime(e.target.value); setActivePreset(''); }}
                            className="bg-cyber-surface border border-gray-700 rounded px-2 py-1 text-sm text-gray-200 focus:border-neon-blue outline-none"
                        />
                        <span className="text-gray-500">-</span>
                        <input
                            type="datetime-local"
                            value={endTime}
                            onChange={(e) => { setEndTime(e.target.value); setActivePreset(''); }}
                            className="bg-cyber-surface border border-gray-700 rounded px-2 py-1 text-sm text-gray-200 focus:border-neon-blue outline-none"
                        />
                        <button
                            onClick={fetchData}
                            className="ml-2 px-4 py-1.5 bg-neon-blue/20 text-neon-blue border border-neon-blue/50 rounded hover:bg-neon-blue/30 transition-colors text-sm font-medium"
                        >
                            Apply
                        </button>
                    </div>

                    {/* Presets */}
                    <div className="flex items-center gap-2 border-l border-gray-700 pl-4">
                        {[
                            { label: '3h', hours: 3 },
                            { label: '24h', hours: 24 },
                            { label: '7d', hours: 24 * 7 },
                            { label: '30d', hours: 24 * 30 }
                        ].map(p => (
                            <button
                                key={p.label}
                                onClick={() => handlePreset(p.hours, p.label)}
                                className={`px-3 py-1 rounded text-xs font-medium transition-colors border ${activePreset === p.label
                                    ? 'bg-neon-purple/20 border-neon-purple text-neon-purple'
                                    : 'bg-transparent border-gray-700 text-gray-400 hover:text-gray-200 hover:border-gray-500'
                                    }`}
                            >
                                {p.label}
                            </button>
                        ))}
                    </div>
                </div>

                {/* Chart Area */}
                <div className="flex-1 w-full min-h-[300px]">
                    {loading ? (
                        <div className="w-full h-full flex flex-col items-center justify-center gap-4">
                            <div className="w-8 h-8 border-2 border-neon-blue border-t-transparent rounded-full animate-spin"></div>
                            <div className="text-neon-blue animate-pulse text-sm">Loading telemetry...</div>
                        </div>
                    ) : data.length === 0 ? (
                        <div className="w-full h-full flex items-center justify-center text-gray-500">No data available for this range.</div>
                    ) : (
                        <ResponsiveContainer width="100%" height="100%">
                            <LineChart data={data} margin={{ top: 5, right: 20, bottom: 25, left: 0 }}>
                                <CartesianGrid strokeDasharray="3 3" stroke="#2a2c33" vertical={false} />
                                <XAxis
                                    dataKey="time"
                                    stroke="#888"
                                    tick={{ fill: '#888', fontSize: 11 }}
                                    dy={10}
                                    minTickGap={30}
                                />
                                <YAxis
                                    stroke="#888"
                                    tick={{ fill: '#888', fontSize: 11 }}
                                    domain={['auto', 'auto']}
                                />
                                <Tooltip
                                    contentStyle={{ backgroundColor: 'rgba(26, 28, 35, 0.95)', border: '1px solid #45a29e', borderRadius: '8px', color: '#fff', boxShadow: '0 4px 6px -1px rgba(0, 0, 0, 0.5)' }}
                                    itemStyle={{ color: '#fff' }}
                                />
                                <Legend verticalAlign="top" height={36} />

                                {displayMode === 'single' ? (
                                    <Line
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
                    )}
                </div>
            </div>
        </div>
    );
}
