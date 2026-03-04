import { PieChart, Pie, Cell, ResponsiveContainer } from 'recharts';

// ---------------------------------------------------------
// 센서 도넛 차트 (Sensor Gauge Component)
// ---------------------------------------------------------
// 이 파일은 대시보드에서 각 센서(온도, 습도 등)의 값을 
// 시각적인 둥근 원형 게이지바(도넛 차트)로 예쁘게 그려주는 "디스플레이 부품"입니다.
// 경고(Warning)치나 위험(Critical)치를 넘어가면 게이지 색상이 빨간색이나 주황색으로 실시간으로 변합니다.
// ---------------------------------------------------------

interface GaugeProps {
    value: number; // 현재 센서 측정 값
    min?: number;  // 게이지의 최솟값
    max?: number;  // 게이지의 최댓값
    warnLow?: number | null; // 노란색 경고로 바뀔 하한선
    warnHigh?: number | null; // 노란색 경고로 바뀔 상한선
    critLow?: number | null;  // 빨간색 위험으로 바뀔 하한선
    critHigh?: number | null; // 빨간색 위험으로 바뀔 상한선
    unit: string;
    title: string;
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
    title
}: GaugeProps) {
    // Determine status color
    let statusColor = '#00f0ff'; // Neon Blue (Normal)
    let statusText = '정상';

    if (critHigh !== null && critHigh !== undefined && value >= critHigh) {
        statusColor = '#EF4444'; // Red
        statusText = '위험 (High)';
    } else if (critLow !== null && critLow !== undefined && value <= critLow) {
        statusColor = '#EF4444'; // Red
        statusText = '위험 (Low)';
    } else if (warnHigh !== null && warnHigh !== undefined && value >= warnHigh) {
        statusColor = '#F59E0B'; // Amber
        statusText = '경고 (High)';
    } else if (warnLow !== null && warnLow !== undefined && value <= warnLow) {
        statusColor = '#F59E0B'; // Amber
        statusText = '경고 (Low)';
    }

    // Clamp value for chart rendering
    const clampedValue = Math.min(Math.max(value, min), max);
    const percentage = ((clampedValue - min) / (max - min)) * 100;

    const data = [
        { name: 'Value', value: percentage, color: statusColor },
        { name: 'Remainder', value: 100 - percentage, color: '#1f2937' } // gray-800
    ];

    const buildThresholdData = () => {
        const segments = [];
        let currentStart = min;

        // Ensure we don't go out of bounds
        const safeCritLow = critLow !== null && critLow !== undefined ? Math.max(min, critLow) : null;
        const safeWarnLow = warnLow !== null && warnLow !== undefined ? Math.max(min, warnLow) : null;
        const safeWarnHigh = warnHigh !== null && warnHigh !== undefined ? Math.min(max, warnHigh) : null;
        const safeCritHigh = critHigh !== null && critHigh !== undefined ? Math.min(max, critHigh) : null;

        // Zone 1: Critical Low
        if (safeCritLow !== null && safeCritLow > currentStart) {
            segments.push({ value: safeCritLow - currentStart, color: '#EF4444' }); // Red
            currentStart = safeCritLow;
        }

        // Zone 2: Warning Low
        if (safeWarnLow !== null && safeWarnLow > currentStart) {
            segments.push({ value: safeWarnLow - currentStart, color: '#F59E0B' }); // Amber
            currentStart = safeWarnLow;
        }

        // Zone 3: Normal
        let normalEnd = max;
        if (safeWarnHigh !== null) normalEnd = safeWarnHigh;
        else if (safeCritHigh !== null) normalEnd = safeCritHigh;

        if (normalEnd > currentStart) {
            segments.push({ value: normalEnd - currentStart, color: '#00f0ff' }); // Neon Blue
            currentStart = normalEnd;
        }

        // Zone 4: Warning High
        if (safeWarnHigh !== null) {
            let warnEnd = max;
            if (safeCritHigh !== null) warnEnd = safeCritHigh;
            if (warnEnd > currentStart) {
                segments.push({ value: warnEnd - currentStart, color: '#F59E0B' }); // Amber
                currentStart = warnEnd;
            }
        }

        // Zone 5: Critical High
        if (safeCritHigh !== null && max > currentStart) {
            segments.push({ value: max - currentStart, color: '#EF4444' }); // Red
            currentStart = max;
        }

        // Ensure we fill up to max if thresholds don't
        if (currentStart < max) {
            segments.push({ value: max - currentStart, color: '#00f0ff' });
        }

        return segments;
    };

    const thresholdData = buildThresholdData();

    return (
        <div className="flex flex-col items-center justify-center relative w-full h-[150px]">
            {/* Title at the top */}
            <div className="absolute top-0 w-full flex justify-center z-10 pointer-events-none">
                <span className="text-[11px] font-bold text-gray-500 tracking-widest uppercase truncate px-2">{title}</span>
            </div>

            <div className="w-full h-full mt-2">
                <ResponsiveContainer width="100%" height="100%">
                    <PieChart>
                        {/* Background Threshold Arc */}
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
                                <Cell key={`bg-cell-${index}`} fill={entry.color} opacity={0.3} />
                            ))}
                        </Pie>

                        {/* Foreground Value Arc */}
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
                                <Cell key={`cell-${index}`} fill={entry.color} style={{ filter: `drop-shadow(0 0 6px ${entry.color}80)` }} />
                            ))}
                        </Pie>
                    </PieChart>
                </ResponsiveContainer>
            </div>

            {/* Value placed securely in the hollow center of the gauge */}
            <div className="absolute flex flex-col items-center pointer-events-none" style={{ bottom: '10%' }}>
                <div className="flex items-baseline gap-1">
                    <span
                        className={`text-2xl font-bold tracking-tighter ${statusColor === '#EF4444' ? 'text-red-400 animate-pulse drop-shadow-[0_0_8px_rgba(239,68,68,0.8)]' :
                            statusColor === '#F59E0B' ? 'text-amber-400 drop-shadow-[0_0_8px_rgba(245,158,11,0.6)]' :
                                'text-neon-blue drop-shadow-[0_0_8px_rgba(0,240,255,0.6)]'
                            }`}
                    >
                        {value.toFixed(1)}
                    </span>
                    <span className="text-sm text-gray-500 font-medium">{unit}</span>
                </div>
                <span className={`text-[10px] font-bold uppercase tracking-widest leading-none mt-1 ${statusColor === '#EF4444' ? 'text-red-500' :
                    statusColor === '#F59E0B' ? 'text-amber-500' :
                        'text-neon-blue/70'
                    }`}>
                    {statusText}
                </span>
            </div>

            {/* Min/Max ticks at the bottom corners of the arc */}
            <div className="absolute w-full px-2 flex justify-between text-xs text-gray-400 font-mono pointer-events-none" style={{ bottom: '15%' }}>
                <span>{min}</span>
                <span>{max}</span>
            </div>
        </div>
    );
}
