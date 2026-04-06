import React, { useEffect, useState } from 'react';
import { smartFarmApi } from '../api/client';
import { Activity, MapPin, Home, AlertTriangle, Thermometer } from 'lucide-react';

interface SiteData {
    site_id: string;
    name: string;
    location: string;
    house_count: number;
    active_alarm_count: number;
}

interface SiteOverviewProps {
    onSiteClick: (siteId: string) => void;
}

const SiteOverview: React.FC<SiteOverviewProps> = ({ onSiteClick }) => {
    const [sites, setSites] = useState<SiteData[]>([]);
    const [overviews, setOverviews] = useState<{ [siteId: string]: any }>({});
    const [isLoading, setIsLoading] = useState(true);

    const fetchData = async () => {
        try {
            const data = await smartFarmApi.getSites();
            setSites(data);

            const overviewPromises = data.map((site: SiteData) => 
                smartFarmApi.getSiteOverview(site.site_id).then(res => ({ id: site.site_id, data: res }))
            );
            const overviewResults = await Promise.all(overviewPromises);
            
            const newOverviews: { [key: string]: any } = {};
            overviewResults.forEach(res => {
                newOverviews[res.id] = res.data;
            });
            setOverviews(newOverviews);
        } catch (e) {
            console.error('Failed to fetch site overview', e);
        } finally {
            setIsLoading(false);
        }
    };

    useEffect(() => {
        fetchData();
        const interval = setInterval(fetchData, 30000); // 30초 주기 갱신
        return () => clearInterval(interval);
    }, []);

    if (isLoading) {
        return (
            <div className="flex h-64 items-center justify-center">
                <Activity className="h-8 w-8 animate-spin text-neon-blue" />
            </div>
        );
    }

    return (
        <div className="animate-fade-in-up space-y-6">
            <div className="flex items-center gap-2">
                <Activity className="h-5 w-5 text-neon-blue" />
                <h2 className="text-lg font-bold tracking-widest text-white">SITE OVERVIEW</h2>
                <span className="ml-2 rounded-full bg-neon-blue/20 px-2 py-0.5 text-xs font-semibold text-neon-blue">
                    {sites.length} Sites
                </span>
            </div>

            <div className="grid grid-cols-1 gap-4 md:grid-cols-2 lg:grid-cols-3">
                {sites.map(site => {
                    const hasAlarm = site.active_alarm_count > 0;
                    const overview = overviews[site.site_id] || {};
                    const avgTemp = overview['temperature'] !== undefined ? overview['temperature'].toFixed(1) : '--';

                    return (
                        <div
                            key={site.site_id}
                            onClick={() => onSiteClick(site.site_id)}
                            className={`group relative cursor-pointer overflow-hidden rounded-2xl border bg-surface-elevated p-5 transition-all duration-300 hover:-translate-y-1 ${
                                hasAlarm 
                                    ? 'border-red-500/80 shadow-[0_0_15px_rgba(239,68,68,0.5)]' 
                                    : 'border-cyber-border/30 hover:border-neon-teal/50 hover:shadow-[0_0_20px_rgba(0,212,170,0.2)]'
                            }`}
                        >
                            {/* 아날로그 노이즈/그라데이션 효과용 백그라운드 요소 */}
                            <div className={`absolute -right-10 -top-10 h-32 w-32 rounded-full blur-3xl transition-opacity ${hasAlarm ? 'bg-red-500/20' : 'bg-neon-teal/10 group-hover:bg-neon-teal/20'}`} />

                            <div className="relative z-10 flex flex-col gap-4">
                                <div className="flex items-start justify-between">
                                    <div>
                                        <h3 className="text-lg font-bold text-white group-hover:text-neon-teal transition-colors">{site.name}</h3>
                                        <div className="mt-1 flex items-center gap-1.5 text-xs text-gray-400">
                                            <MapPin className="h-3.5 w-3.5" />
                                            <span className="truncate">{site.location || '위치 미지정'}</span>
                                        </div>
                                    </div>
                                    {hasAlarm && (
                                        <div className="flex animate-pulse items-center gap-1 rounded bg-red-500/20 px-2 py-1 text-xs font-bold text-red-400 border border-red-500/50">
                                            <AlertTriangle className="h-3.5 w-3.5" />
                                            {site.active_alarm_count}
                                        </div>
                                    )}
                                </div>

                                <div className="grid grid-cols-2 gap-3 border-t border-cyber-border/20 pt-4">
                                    <div className="flex items-center gap-2">
                                        <div className="flex h-8 w-8 items-center justify-center rounded-lg bg-white/5 text-gray-400 group-hover:text-white transition-colors">
                                            <Home className="h-4 w-4" />
                                        </div>
                                        <div className="flex flex-col">
                                            <span className="text-[10px] tracking-wider text-gray-500 uppercase">하우스 동수</span>
                                            <span className="font-semibold text-white">{site.house_count}</span>
                                        </div>
                                    </div>
                                    <div className="flex items-center gap-2">
                                        <div className="flex h-8 w-8 items-center justify-center rounded-lg bg-orange-500/10 text-orange-400 border border-orange-500/20">
                                            <Thermometer className="h-4 w-4" />
                                        </div>
                                        <div className="flex flex-col">
                                            <span className="text-[10px] tracking-wider text-gray-500 uppercase">평균 온도</span>
                                            <span className="font-semibold text-white">{avgTemp}°C</span>
                                        </div>
                                    </div>
                                </div>
                            </div>
                        </div>
                    );
                })}
            </div>
            
            {sites.length === 0 && (
                <div className="flex h-32 flex-col items-center justify-center rounded-xl border border-dashed border-gray-700 bg-cyber-surface/30 text-gray-500">
                    <MapPin className="mb-2 h-6 w-6 opacity-50" />
                    <span className="text-sm">등록된 농장(Site)이 없습니다.</span>
                </div>
            )}
        </div>
    );
};

export default SiteOverview;
