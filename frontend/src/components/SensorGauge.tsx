import { PieChart, Pie, Cell, ResponsiveContainer } from 'recharts';
import { useLanguage } from '../i18n/LanguageContext';

// ---------------------------------------------------------
// 센서 도넛 차트 (Sensor Gauge Component)
// ---------------------------------------------------------

interface GaugeProps {
    value: number;
    min?: number;
    max?: number;
    warnLow?: number | null;
    warnHigh?: number | null;
    critLow?: number | null;
    critHigh?: number | null;
    unit: string;
    title: string;
    /** 센서 타입별 기본 색상 (optional, 전달 시 Normal 상태 색에 반영) */
    accentColor?: string;
}

export default function SensorGauge({
    value,
    min = 0,
    max = 100,
    warnLow,
    warnHigh,
    critLow,
    critHigh,
    unit,
    title,
    accentColor = '#00f0ff',
}: GaugeProps) {
    const { t } = useLanguage();

    // ── 상태 판별 ──
    let statusColor = accentColor;
    let statusText = t('statusNormal');

    if (critHigh !== null && critHigh !== undefined && value >= critHigh) {
        statusColor = '#EF4444';
        statusText = t('statusCritHigh');
    } else if (critLow !== null && critLow !== undefined && value <= critLow) {
        statusColor = '#EF4444';
        statusText = t('statusCritLow');
    } else if (warnHigh !== null && warnHigh !== undefined && value >= warnHigh) {
        statusColor = '#F59E0B';
        statusText = t('statusWarnHigh');
    } else if (warnLow !== null && warnLow !== undefined && value <= warnLow) {
        statusColor = '#F59E0B';
        statusText = t('statusWarnLow');
    }

    const isCrit = statusColor === '#EF4444';
    const isWarn = statusColor === '#F59E0B';

    // ── 게이지 퍼센트 ──
    const clampedValue = Math.min(Math.max(value, min), max);
    const percentage = ((clampedValue - min) / (max - min)) * 100;

    const data = [
        { name: 'Value',     value: percentage,       color: statusColor },
        { name: 'Remainder', value: 100 - percentage, color: '#1f2937' },
    ];

    // ── 임계치 배경 아크 ──
    const buildThresholdData = () => {
        const segments: { value: number; color: string }[] = [];
        let cur = min;

        const safeCritLow  = critLow  != null ? Math.max(min, critLow)  : null;
        const safeWarnLow  = warnLow  != null ? Math.max(min, warnLow)  : null;
        const safeWarnHigh = warnHigh != null ? Math.min(max, warnHigh) : null;
        const safeCritHigh = critHigh != null ? Math.min(max, critHigh) : null;

        if (safeCritLow !== null && safeCritLow > cur) {
            segments.push({ value: safeCritLow - cur, color: '#EF4444' });
            cur = safeCritLow;
        }
        if (safeWarnLow !== null && safeWarnLow > cur) {
            segments.push({ value: safeWarnLow - cur, color: '#F59E0B' });
            cur = safeWarnLow;
        }

        let normalEnd = max;
        if (safeWarnHigh !== null) normalEnd = safeWarnHigh;
        else if (safeCritHigh !== null) normalEnd = safeCritHigh;

        if (normalEnd > cur) {
            segments.push({ value: normalEnd - cur, color: accentColor });
            cur = normalEnd;
        }

        if (safeWarnHigh !== null) {
            let warnEnd = max;
            if (safeCritHigh !== null) warnEnd = safeCritHigh;
            if (warnEnd > cur) {
                segments.push({ value: warnEnd - cur, color: '#F59E0B' });
                cur = warnEnd;
            }
        }

        if (safeCritHigh !== null && max > cur) {
            segments.push({ value: max - cur, color: '#EF4444' });
            cur = max;
        }

        if (cur < max) segments.push({ value: max - cur, color: accentColor });

        return segments;
    };

    const thresholdData = buildThresholdData();

    return (
        <div className="flex flex-col items-center justify-center relative w-full h-[130px]">
            {/* 타이틀 */}
            <div className="absolute top-0 w-full flex justify-center z-10 pointer-events-none">
                <span className="text-[11px] font-bold text-gray-500 tracking-widest uppercase truncate px-2">{title}</span>
            </div>

            <div className="w-full h-full mt-2">
                <ResponsiveContainer width="100%" height="100%">
                    <PieChart>
                        {/* 배경 임계치 아크 – opacity 0.15 (더 은은하게) */}
                        <Pie
                            data={thresholdData}
                            cx="50%"
                            cy="75%"
                            startAngle={180}
                            endAngle={0}
                            innerRadius="90%"
                            outerRadius="95%"
                            dataKey="value"
                            stroke="none"
                            cornerRadius={0}
                            paddingAngle={0}
                            isAnimationActive={false}
                        >
                            {thresholdData.map((entry, index) => (
                                <Cell key={`bg-cell-${index}`} fill={entry.color} opacity={0.15} />
                            ))}
                        </Pie>

                        {/* 전경 값 아크 */}
                        <Pie
                            data={data}
                            cx="50%"
                            cy="75%"
                            startAngle={180}
                            endAngle={0}
                            innerRadius="70%"
                            outerRadius="85%"
                            dataKey="value"
                            stroke="none"
                            cornerRadius={4}
                            paddingAngle={2}
                            isAnimationActive={true}
                        >
                            {data.map((entry, index) => (
                                <Cell
                                    key={`cell-${index}`}
                                    fill={entry.color}
                                    style={{ filter: `drop-shadow(0 0 6px ${entry.color}80)` }}
                                />
                            ))}
                        </Pie>
                    </PieChart>
                </ResponsiveContainer>
            </div>

            {/* 중앙 수치 – text-2xl로 확장 */}
            <div className="absolute flex flex-col items-center pointer-events-none" style={{ bottom: '10%' }}>
                <div className="flex items-baseline gap-0.5">
                    <span
                        className={`text-2xl font-bold tracking-tighter ${
                            isCrit ? 'text-red-400 animate-pulse drop-shadow-[0_0_8px_rgba(239,68,68,0.8)]' :
                            isWarn ? 'text-amber-400 drop-shadow-[0_0_8px_rgba(245,158,11,0.6)]' :
                                     'drop-shadow-[0_0_8px_rgba(0,240,255,0.6)]'
                        }`}
                        style={!isCrit && !isWarn ? { color: accentColor } : undefined}
                    >
                        {value.toFixed(1)}
                    </span>
                    <span className="text-xs text-gray-500 font-medium">{unit}</span>
                </div>
                <span
                    className={`text-[10px] font-bold uppercase tracking-widest leading-none mt-1 ${
                        isCrit ? 'text-red-500' :
                        isWarn ? 'text-amber-500' :
                                 'opacity-80'
                    }`}
                    style={!isCrit && !isWarn ? { color: accentColor } : undefined}
                >
                    {statusText}
                </span>
            </div>

            {/* Min / Max 눈금 */}
            <div className="absolute w-full px-2 flex justify-between text-xs text-gray-400 font-mono pointer-events-none" style={{ bottom: '15%' }}>
                <span>{min}</span>
                <span>{max}</span>
            </div>
        </div>
    );
}
