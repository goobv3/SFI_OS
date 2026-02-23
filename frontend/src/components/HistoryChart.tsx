import { useState, useEffect } from 'react';
import {
    LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer
} from 'recharts';
import { smartFarmApi } from '../api/client';
import { X } from 'lucide-react';

interface HistoryChartProps {
    sensorId: string;
    sensorName: string;
    onClose: () => void;
}

export default function HistoryChart({ sensorId, sensorName, onClose }: HistoryChartProps) {
    const [data, setData] = useState<{ time: string, avg_value: number }[]>([]);
    const [period, setPeriod] = useState<'daily' | 'monthly' | 'yearly'>('daily');
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        let isMounted = true;
        setLoading(true);
        smartFarmApi.getSensorHistory(sensorId, period).then(res => {
            if (isMounted) {
                setData(res);
                setLoading(false);
            }
        }).catch(err => {
            console.error(err);
            if (isMounted) setLoading(false);
        });
        return () => { isMounted = false; }
    }, [sensorId, period]);

    return (
        <div className="fixed inset-0 bg-black/80 flex items-center justify-center z-[100] p-4">
            <div className="bg-cyber-surface border border-cyber-border/50 rounded-xl max-w-4xl w-full p-6 relative shadow-2xl">
                <button onClick={onClose} className="absolute top-4 right-4 text-gray-400 hover:text-white transition-colors">
                    <X className="w-6 h-6" />
                </button>

                <h3 className="text-xl font-bold text-white mb-6 tracking-wide border-l-4 border-neon-blue pl-3">
                    {sensorName} <span className="text-neon-blue font-light">History</span>
                </h3>

                <div className="flex items-center gap-2 mb-6">
                    {(['daily', 'monthly', 'yearly'] as const).map(p => (
                        <button
                            key={p}
                            onClick={() => setPeriod(p)}
                            className={`px-4 py-1.5 rounded-full text-sm font-medium transition-colors border ${period === p
                                ? 'bg-neon-blue/20 border-neon-blue/50 text-neon-blue shadow-[0_0_10px_rgba(102,252,241,0.2)]'
                                : 'bg-transparent border-gray-600 text-gray-400 hover:text-gray-200'
                                }`}
                        >
                            {p.charAt(0).toUpperCase() + p.slice(1)}
                        </button>
                    ))}
                </div>

                <div className="h-80 w-full min-h-[300px]">
                    {loading ? (
                        <div className="w-full h-full flex items-center justify-center text-neon-blue animate-pulse">Loading data...</div>
                    ) : data.length === 0 ? (
                        <div className="w-full h-full flex items-center justify-center text-gray-500">No data available for this period.</div>
                    ) : (
                        <ResponsiveContainer width="100%" height="100%">
                            <LineChart data={data} margin={{ top: 5, right: 20, bottom: 5, left: 0 }}>
                                <CartesianGrid strokeDasharray="3 3" stroke="#333" />
                                <XAxis dataKey="time" stroke="#888" tick={{ fill: '#888', fontSize: 12 }} />
                                <YAxis stroke="#888" tick={{ fill: '#888', fontSize: 12 }} />
                                <Tooltip
                                    contentStyle={{ backgroundColor: '#1a1c23', borderColor: '#45a29e', color: '#fff' }}
                                    itemStyle={{ color: '#66fcf1' }}
                                    formatter={(value: number | undefined) => [value ? value.toFixed(2) : '0.00', 'Avg']}
                                />
                                <Line
                                    type="monotone"
                                    dataKey="avg_value"
                                    name="Value"
                                    stroke="#66fcf1"
                                    strokeWidth={3}
                                    dot={{ fill: '#1a1c23', stroke: '#66fcf1', strokeWidth: 2, r: 4 }}
                                    activeDot={{ r: 6, fill: '#66fcf1', stroke: '#fff' }}
                                />
                            </LineChart>
                        </ResponsiveContainer>
                    )}
                </div>
            </div>
        </div>
    );
}
