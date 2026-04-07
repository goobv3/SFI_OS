import React, { useState, useEffect, useRef } from 'react';
import { 
  Server, Cpu, Database, Activity, 
  Terminal, RefreshCw, Clock, HardDrive,
  Globe
} from 'lucide-react';
import { smartFarmApi } from '../api/client';

/**
 * @file SystemMonitor.tsx
 * @brief 서버 리소스 상태 및 실시간 로그를 시각화하는 통합 모니터링 컴포넌트
 */

const SystemMonitor: React.FC = () => {
    const [status, setStatus] = useState<any>(null);
    const [logs, setLogs] = useState<string[]>([]);
    const [isLoading, setIsLoading] = useState(true);
    const logContainerRef = useRef<HTMLDivElement>(null);

    const fetchData = async () => {
        try {
            const [statusData, logsData] = await Promise.all([
                smartFarmApi.getSystemStatus(),
                smartFarmApi.getSystemLogs(100)
            ]);
            setStatus(statusData);
            setLogs(logsData);
        } catch (error) {
            console.error('Failed to fetch system monitoring data:', error);
        } finally {
            setIsLoading(false);
        }
    };

    // 5초마다 데이터 갱신
    useEffect(() => {
        fetchData();
        const interval = setInterval(fetchData, 5000);
        return () => clearInterval(interval);
    }, []);

    // 로그 최하단 자동 스크롤
    useEffect(() => {
        if (logContainerRef.current) {
            logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
        }
    }, [logs]);

    if (isLoading && !status) {
        return (
            <div className="flex flex-col items-center justify-center py-20 text-gray-500 animate-pulse">
                <RefreshCw className="w-8 h-8 animate-spin mb-4 text-neon-blue" />
                <span className="text-sm font-medium tracking-widest">CONNECTING TO BACKEND KERNEL...</span>
            </div>
        );
    }

    return (
        <div className="space-y-6 animate-fade-in-up">
            
            {/* ── 1. 실시간 서버 메시지 (로그 터미널) ── */}
            <div className="bg-[#0d1117] rounded-xl border border-cyber-border/30 overflow-hidden shadow-2xl">
                <div className="bg-surface-elevated/80 px-4 py-2 border-b border-cyber-border/20 flex items-center justify-between">
                    <div className="flex items-center gap-2">
                        <Terminal className="w-4 h-4 text-neon-blue" />
                        <span className="text-xs font-bold text-gray-300 tracking-wider">LIVE SERVER CONSOLE</span>
                    </div>
                    <div className="flex gap-1.5">
                        <div className="w-2.5 h-2.5 rounded-full bg-red-500/50" />
                        <div className="w-2.5 h-2.5 rounded-full bg-amber-500/50" />
                        <div className="w-2.5 h-2.5 rounded-full bg-green-500/50" />
                    </div>
                </div>
                <div 
                    ref={logContainerRef}
                    className="h-64 overflow-y-auto p-4 font-mono text-[11px] leading-relaxed scrollbar-thin scrollbar-thumb-gray-800"
                >
                    {logs.map((line, idx) => (
                        <div key={idx} className="flex gap-3 mb-0.5 group">
                            <span className="text-gray-600 shrink-0 select-none w-6 text-right">{idx + 1}</span>
                            <span className={`
                                ${line.includes('[Error]') || line.includes('ERR') ? 'text-neon-red' : 
                                  line.includes('[MQTT]') ? 'text-neon-teal' :
                                  line.includes('[Router]') ? 'text-neon-purple' :
                                  'text-gray-400 group-hover:text-gray-200'}
                            `}>
                                {line}
                            </span>
                        </div>
                    ))}
                    {logs.length === 0 && <p className="text-gray-600 italic">No logs available...</p>}
                </div>
            </div>

            {/* ── 2. 리소스 현황 및 서비스 상태 ── */}
            <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
                
                {/* 2-1. CPU & RAM & Disk Gauges */}
                <div className="lg:col-span-2 grid grid-cols-1 md:grid-cols-3 gap-4">
                    {[
                        { label: 'CPU Usage', val: status?.resources?.cpu, icon: <Cpu className="w-4 h-4" />, color: 'neon-blue' },
                        { label: 'RAM Usage', val: status?.resources?.ram, icon: <Activity className="w-4 h-4" />, color: 'neon-purple' },
                        { label: 'Disk Usage', val: status?.resources?.disk, icon: <HardDrive className="w-4 h-4" />, color: 'neon-orange' }
                    ].map((item, idx) => (
                        <div key={idx} className="bg-surface-card rounded-xl p-5 border border-cyber-border/10 hover-lift">
                            <div className="flex items-center justify-between mb-4">
                                <div className={`p-2 rounded-lg bg-${item.color}/10 text-${item.color}`}>
                                    {item.icon}
                                </div>
                                <span className={`text-xl font-bold text-white shadow-sm`}>{Math.round(item.val || 0)}%</span>
                            </div>
                            <h4 className="text-xs font-semibold text-gray-500 uppercase mb-3">{item.label}</h4>
                            <div className="h-1.5 w-full bg-gray-800 rounded-full overflow-hidden">
                                <div 
                                    className={`h-full bg-${item.color} shadow-[0_0_8px] transition-all duration-1000`} 
                                    style={{ width: `${item.val || 0}%`, boxShadow: `0 0 10px var(--color-${item.color})` }}
                                />
                            </div>
                        </div>
                    ))}
                </div>

                {/* 2-2. 서비스 상태 및 세부 정보 */}
                <div className="bg-surface-card rounded-xl p-5 border border-cyber-border/10 space-y-5">
                    <h4 className="text-xs font-bold text-neon-blue uppercase tracking-widest border-b border-cyber-border/20 pb-3 flex items-center gap-2">
                        <Server className="w-4 h-4" /> CORE SERVICES
                    </h4>
                    
                    <div className="space-y-4">
                        <div className="flex items-center justify-between">
                            <div className="flex items-center gap-3">
                                <Database className="w-4 h-4 text-gray-400" />
                                <span className="text-sm font-medium text-gray-300">MariaDB (SQL)</span>
                            </div>
                            <StatusBadge status={status?.services?.mariadb} />
                        </div>
                        <div className="flex items-center justify-between">
                            <div className="flex items-center gap-3">
                                <Globe className="w-4 h-4 text-gray-400" />
                                <span className="text-sm font-medium text-gray-300">MQTT Broker</span>
                            </div>
                            <StatusBadge status={status?.services?.mosquitto} />
                        </div>
                    </div>

                    <div className="pt-4 border-t border-cyber-border/10 flex items-center justify-between text-[11px]">
                        <div className="flex items-center gap-2 text-gray-500">
                            <Clock className="w-3.5 h-3.5" />
                            <span>SERVER START TIME</span>
                        </div>
                        <span className="text-gray-300 font-mono">{status?.uptime?.start_time || 'N/A'}</span>
                    </div>
                </div>
            </div>
        </div>
    );
};

const StatusBadge: React.FC<{ status: string }> = ({ status }) => {
    const isOnline = status === 'online';
    return (
        <div className={`flex items-center gap-1.5 px-2.5 py-1 rounded-full text-[10px] font-bold uppercase transition-all
            ${isOnline ? 'bg-neon-green/10 text-neon-green border border-neon-green/30' : 'bg-neon-red/10 text-neon-red border border-neon-red/30'}
        `}>
            <span className={`w-1.5 h-1.5 rounded-full ${isOnline ? 'bg-neon-green animate-pulse' : 'bg-neon-red'}`} />
            {status}
        </div>
    );
};

export default SystemMonitor;
