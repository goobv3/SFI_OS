import { useState, useEffect, useCallback } from 'react';
import { Plus, Trash2, Pencil, Zap, ChevronRight, X, ToggleLeft, ToggleRight } from 'lucide-react';
import { smartFarmApi } from '../api/client';
import { useLanguage } from '../i18n/LanguageContext';

// ─── 타입 ───────────────────────────────────────────────────────
interface AutoRule {
    id: number;
    name: string;
    house_id: string;
    trigger_sensor_id: string;
    sensor_alias: string;
    condition_type: 'GT' | 'LT' | 'GTE' | 'LTE';
    threshold_value: string;
    actuator_id: string;
    actuator_alias: string;
    action_command: string;
    is_enabled: string;   // "1" | "0" from DB
    cooldown_minutes: string;
    last_triggered_at: string;
}

interface AutomationPanelProps {
    houseId: string;
    houseName?: string;
}

// ─── 상수 ───────────────────────────────────────────────────────
const CONDITION_LABELS: Record<string, string> = {
    GT: '>', LT: '<', GTE: '≥', LTE: '≤',
};
const CONDITION_OPTIONS: { value: string; label: string }[] = [
    { value: 'GT',  label: '> (초과)' },
    { value: 'LT',  label: '< (미만)' },
    { value: 'GTE', label: '≥ (이상)' },
    { value: 'LTE', label: '≤ (이하)' },
];
const COMMAND_PRESETS = ['ON', 'OFF', 'OPEN', 'CLOSE', 'AUTO', 'MANUAL', 'START', 'STOP'];

// ─── 룰 카드 컴포넌트 ─────────────────────────────────────────────
function RuleCard({
    rule,
    onToggle,
    onEdit,
    onDelete,
}: {
    rule: AutoRule;
    onToggle: (id: number, enabled: boolean) => void;
    onEdit: (rule: AutoRule) => void;
    onDelete: (id: number) => void;
}) {
    const enabled = rule.is_enabled === '1' || rule.is_enabled === 'true';
    const condLabel = CONDITION_LABELS[rule.condition_type] ?? rule.condition_type;

    return (
        <div
            className={`relative rounded-xl border transition-all duration-200 hover-lift overflow-hidden ${
                enabled
                    ? 'bg-surface-card border-cyber-border/20 hover:border-neon-blue/40'
                    : 'bg-surface-card/60 border-gray-700/30 opacity-60'
            }`}
        >
            {/* 활성 표시 바 */}
            {enabled && (
                <div className="absolute left-0 top-0 bottom-0 w-[2px] bg-neon-blue shadow-[0_0_6px_#66fcf1]" />
            )}

            <div className="p-4 pl-5">
                {/* 헤더 행 */}
                <div className="flex items-center justify-between mb-3">
                    <div className="flex items-center gap-2">
                        <Zap className={`w-3.5 h-3.5 ${enabled ? 'text-neon-blue' : 'text-gray-600'}`} />
                        <span className="text-sm font-semibold text-white truncate max-w-[180px]">
                            {rule.name || `Rule #${rule.id}`}
                        </span>
                    </div>
                    <div className="flex items-center gap-1">
                        <button
                            onClick={() => onToggle(rule.id, !enabled)}
                            title={enabled ? '비활성화' : '활성화'}
                            className="p-1 rounded-lg hover:bg-white/10 transition"
                        >
                            {enabled
                                ? <ToggleRight className="w-5 h-5 text-neon-blue" />
                                : <ToggleLeft  className="w-5 h-5 text-gray-500" />
                            }
                        </button>
                        <button
                            onClick={() => onEdit(rule)}
                            className="p-1 rounded-lg hover:bg-white/10 transition text-gray-400 hover:text-white"
                        >
                            <Pencil className="w-3.5 h-3.5" />
                        </button>
                        <button
                            onClick={() => onDelete(rule.id)}
                            className="p-1 rounded-lg hover:bg-red-500/20 transition text-gray-400 hover:text-red-400"
                        >
                            <Trash2 className="w-3.5 h-3.5" />
                        </button>
                    </div>
                </div>

                {/* 룰 시각화: IF sensor OP threshold → actuator command */}
                <div className="flex flex-wrap items-center gap-1.5 text-xs">
                    <span className="text-gray-500 font-semibold uppercase tracking-widest">IF</span>
                    <span className="px-2 py-0.5 rounded-lg bg-neon-blue/10 border border-neon-blue/30 text-neon-blue font-mono font-semibold">
                        {rule.sensor_alias || rule.trigger_sensor_id}
                    </span>
                    <span className="px-2 py-0.5 rounded-lg bg-neon-orange/10 border border-neon-orange/30 text-neon-orange font-bold text-base leading-none">
                        {condLabel}
                    </span>
                    <span className="px-2 py-0.5 rounded-lg bg-white/5 border border-gray-600 text-white font-mono font-bold">
                        {Number(rule.threshold_value).toFixed(1)}
                    </span>
                    <ChevronRight className="w-3.5 h-3.5 text-gray-500" />
                    <span className="text-gray-500 font-semibold uppercase tracking-widest">DO</span>
                    <span className="px-2 py-0.5 rounded-lg bg-neon-green/10 border border-neon-green/30 text-neon-green font-mono font-semibold">
                        {rule.actuator_alias || rule.actuator_id}
                    </span>
                    <span className="px-2 py-0.5 rounded-lg bg-neon-green/5 border border-neon-green/20 text-neon-green font-bold uppercase tracking-wider">
                        {rule.action_command}
                    </span>
                </div>

                {/* 쿨다운 + 마지막 실행 */}
                <div className="flex items-center gap-3 mt-3 text-[10px] text-gray-600">
                    <span>쿨다운 {rule.cooldown_minutes}분</span>
                    {rule.last_triggered_at && (
                        <span>마지막 실행: {rule.last_triggered_at.slice(0, 16).replace('T', ' ')}</span>
                    )}
                </div>
            </div>
        </div>
    );
}

// ─── 룰 편집 모달 ────────────────────────────────────────────────
function RuleModal({
    houseId,
    sensors,
    actuators,
    editingRule,
    onClose,
    onSaved,
}: {
    houseId: string;
    sensors: any[];
    actuators: any[];
    editingRule: AutoRule | null;
    onClose: () => void;
    onSaved: () => void;
}) {
    const { t } = useLanguage();

    const [form, setForm] = useState({
        name:              editingRule?.name              ?? '',
        trigger_sensor_id: editingRule?.trigger_sensor_id ?? '',
        condition_type:    (editingRule?.condition_type   ?? 'GT') as 'GT' | 'LT' | 'GTE' | 'LTE',
        threshold_value:   editingRule?.threshold_value   ?? '25',
        actuator_id:       editingRule?.actuator_id       ?? '',
        action_command:    editingRule?.action_command     ?? 'ON',
        cooldown_minutes:  editingRule?.cooldown_minutes   ?? '5',
    });
    const [saving, setSaving] = useState(false);
    const [error, setError]   = useState('');

    const isEdit = Boolean(editingRule);

    const handleSubmit = async (e: React.FormEvent) => {
        e.preventDefault();
        if (!form.trigger_sensor_id || !form.actuator_id || !form.action_command) {
            setError('센서, 액추에이터, 명령어는 필수입니다.');
            return;
        }
        setSaving(true);
        setError('');
        try {
            if (isEdit && editingRule) {
                await smartFarmApi.updateRule(editingRule.id, {
                    name:              form.name,
                    trigger_sensor_id: form.trigger_sensor_id,
                    condition_type:    form.condition_type,
                    threshold_value:   Number(form.threshold_value),
                    actuator_id:       form.actuator_id,
                    action_command:    form.action_command,
                    cooldown_minutes:  Number(form.cooldown_minutes),
                });
            } else {
                await smartFarmApi.createRule({
                    name:              form.name,
                    house_id:          houseId,
                    trigger_sensor_id: form.trigger_sensor_id,
                    condition_type:    form.condition_type,
                    threshold_value:   Number(form.threshold_value),
                    actuator_id:       form.actuator_id,
                    action_command:    form.action_command,
                    cooldown_minutes:  Number(form.cooldown_minutes),
                });
            }
            onSaved();
        } catch (err: any) {
            setError(err?.message ?? '저장 실패');
        } finally {
            setSaving(false);
        }
    };

    const inputCls = 'w-full bg-cyber-bg border border-cyber-border/30 rounded-lg px-3 py-2 text-sm text-white placeholder-gray-600 focus:outline-none focus:border-neon-blue/60 transition';
    const labelCls = 'block text-xs text-gray-400 mb-1 font-medium';

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-sm p-4">
            <div className="bg-surface-card border border-cyber-border/30 rounded-2xl w-full max-w-lg shadow-2xl animate-fade-in-up">
                {/* 모달 헤더 */}
                <div className="flex items-center justify-between px-6 py-4 border-b border-cyber-border/20">
                    <div className="flex items-center gap-2">
                        <Zap className="w-4 h-4 text-neon-blue" />
                        <h2 className="text-sm font-bold text-white">{isEdit ? t('autoEdit') : t('autoCreate')}</h2>
                    </div>
                    <button onClick={onClose} className="text-gray-500 hover:text-white transition">
                        <X className="w-4 h-4" />
                    </button>
                </div>

                <form onSubmit={handleSubmit} className="px-6 py-4 space-y-4">
                    {/* 룰 이름 */}
                    <div>
                        <label className={labelCls}>{t('autoRuleName')}</label>
                        <input
                            className={inputCls}
                            placeholder="예: 온도 높으면 냉방기 ON"
                            value={form.name}
                            onChange={e => setForm(f => ({ ...f, name: e.target.value }))}
                        />
                    </div>

                    {/* 트리거 센서 + 조건 + 기준값 */}
                    <div className="grid grid-cols-3 gap-3">
                        <div className="col-span-1">
                            <label className={labelCls}>{t('autoSensor')}</label>
                            <select
                                className={inputCls}
                                value={form.trigger_sensor_id}
                                onChange={e => setForm(f => ({ ...f, trigger_sensor_id: e.target.value }))}
                                required
                            >
                                <option value="">-- 선택 --</option>
                                {sensors.map(s => (
                                    <option key={s.sensor_id} value={s.sensor_id}>{s.alias || s.sensor_id}</option>
                                ))}
                            </select>
                        </div>
                        <div className="col-span-1">
                            <label className={labelCls}>{t('autoCondition')}</label>
                            <select
                                className={inputCls}
                                value={form.condition_type}
                                onChange={e => setForm(f => ({ ...f, condition_type: e.target.value as any }))}
                            >
                                {CONDITION_OPTIONS.map(c => (
                                    <option key={c.value} value={c.value}>{c.label}</option>
                                ))}
                            </select>
                        </div>
                        <div className="col-span-1">
                            <label className={labelCls}>{t('autoThreshold')}</label>
                            <input
                                type="number"
                                step="0.1"
                                className={inputCls}
                                value={form.threshold_value}
                                onChange={e => setForm(f => ({ ...f, threshold_value: e.target.value }))}
                                required
                            />
                        </div>
                    </div>

                    {/* 액추에이터 + 명령 */}
                    <div className="grid grid-cols-2 gap-3">
                        <div>
                            <label className={labelCls}>{t('autoActuator')}</label>
                            <select
                                className={inputCls}
                                value={form.actuator_id}
                                onChange={e => setForm(f => ({ ...f, actuator_id: e.target.value }))}
                                required
                            >
                                <option value="">-- 선택 --</option>
                                {actuators.map(a => (
                                    <option key={a.actuator_id} value={a.actuator_id}>{a.alias || a.actuator_id}</option>
                                ))}
                            </select>
                        </div>
                        <div>
                            <label className={labelCls}>{t('autoCommand')}</label>
                            <div className="flex gap-1">
                                <select
                                    className={inputCls}
                                    value={COMMAND_PRESETS.includes(form.action_command) ? form.action_command : '__custom__'}
                                    onChange={e => {
                                        if (e.target.value !== '__custom__')
                                            setForm(f => ({ ...f, action_command: e.target.value }));
                                    }}
                                >
                                    {COMMAND_PRESETS.map(c => <option key={c} value={c}>{c}</option>)}
                                    <option value="__custom__">직접입력</option>
                                </select>
                            </div>
                            <input
                                className={`${inputCls} mt-1`}
                                placeholder="명령 직접 입력"
                                value={form.action_command}
                                onChange={e => setForm(f => ({ ...f, action_command: e.target.value.toUpperCase() }))}
                            />
                        </div>
                    </div>

                    {/* 쿨다운 */}
                    <div className="w-32">
                        <label className={labelCls}>{t('autoCooldown')}</label>
                        <input
                            type="number"
                            min={0}
                            className={inputCls}
                            value={form.cooldown_minutes}
                            onChange={e => setForm(f => ({ ...f, cooldown_minutes: e.target.value }))}
                        />
                    </div>

                    {/* 에러 */}
                    {error && (
                        <p className="text-red-400 text-xs">{error}</p>
                    )}

                    {/* 룰 미리보기 */}
                    {form.trigger_sensor_id && form.actuator_id && (
                        <div className="rounded-lg bg-cyber-bg/60 border border-cyber-border/20 px-4 py-2.5 flex flex-wrap items-center gap-1.5 text-xs">
                            <span className="text-gray-500 font-semibold">IF</span>
                            <span className="text-neon-blue font-mono">
                                {sensors.find(s => s.sensor_id === form.trigger_sensor_id)?.alias ?? form.trigger_sensor_id}
                            </span>
                            <span className="text-neon-orange font-bold text-sm">
                                {CONDITION_LABELS[form.condition_type]}
                            </span>
                            <span className="text-white font-mono">{form.threshold_value}</span>
                            <ChevronRight className="w-3 h-3 text-gray-500" />
                            <span className="text-gray-500 font-semibold">DO</span>
                            <span className="text-neon-green font-mono">
                                {actuators.find(a => a.actuator_id === form.actuator_id)?.alias ?? form.actuator_id}
                            </span>
                            <span className="text-neon-green font-bold uppercase">{form.action_command}</span>
                        </div>
                    )}

                    {/* 저장 */}
                    <div className="flex justify-end gap-2 pt-1">
                        <button
                            type="button"
                            onClick={onClose}
                            className="px-4 py-2 text-xs text-gray-400 hover:text-white bg-white/5 border border-gray-700 rounded-lg transition"
                        >
                            {t('cancel')}
                        </button>
                        <button
                            type="submit"
                            disabled={saving}
                            className="px-4 py-2 text-xs font-bold text-cyber-bg bg-neon-blue rounded-lg hover:bg-neon-blue/90 transition disabled:opacity-50"
                        >
                            {saving ? t('autoSaving') : (isEdit ? t('save') : t('autoCreate'))}
                        </button>
                    </div>
                </form>
            </div>
        </div>
    );
}

// ─── 메인 패널 ────────────────────────────────────────────────────
export default function AutomationPanel({ houseId, houseName }: AutomationPanelProps) {
    const { t } = useLanguage();
    const [rules, setRules]         = useState<AutoRule[]>([]);
    const [loading, setLoading]     = useState(false);
    const [showModal, setShowModal] = useState(false);
    const [editingRule, setEditingRule] = useState<AutoRule | null>(null);
    const [sensors, setSensors]     = useState<any[]>([]);
    const [actuators, setActuators] = useState<any[]>([]);

    const fetchRules = useCallback(async () => {
        setLoading(true);
        try {
            const data = await smartFarmApi.getRules(houseId);
            setRules(Array.isArray(data) ? data : []);
        } catch (e) { console.error(e); }
        finally { setLoading(false); }
    }, [houseId]);

    const fetchDevices = useCallback(async () => {
        if (!houseId) return;
        try {
            const data = await smartFarmApi.getHouseDevices(houseId);
            setSensors(data.sensors  ?? []);
            setActuators(data.actuators ?? []);
        } catch (e) { console.error(e); }
    }, [houseId]);

    useEffect(() => {
        fetchRules();
        fetchDevices();
    }, [fetchRules, fetchDevices]);

    const handleToggle = async (id: number, enabled: boolean) => {
        try {
            await smartFarmApi.toggleRule(id, enabled);
            fetchRules();
        } catch (e) { console.error(e); }
    };

    const handleDelete = async (id: number) => {
        if (!window.confirm(t('autoConfirmDelete'))) return;
        try {
            await smartFarmApi.deleteRule(id);
            fetchRules();
        } catch (e) { console.error(e); }
    };

    const handleEdit = (rule: AutoRule) => {
        setEditingRule(rule);
        setShowModal(true);
    };

    const handleAdd = () => {
        setEditingRule(null);
        setShowModal(true);
    };

    const handleSaved = () => {
        setShowModal(false);
        setEditingRule(null);
        fetchRules();
    };

    const enabledCount = rules.filter(r => r.is_enabled === '1' || r.is_enabled === 'true').length;

    return (
        <div className="space-y-4">
            {/* 헤더 */}
            <div className="flex items-center justify-between">
                <div className="flex items-center gap-3">
                    <div className="flex items-center gap-2">
                        <Zap className="w-4 h-4 text-neon-blue" />
                        <h2 className="text-sm font-bold text-white">{t('autoTitle')}</h2>
                        {houseName && <span className="text-xs text-gray-500">— {houseName}</span>}
                    </div>
                    {rules.length > 0 && (
                        <span className="text-[10px] font-semibold px-2 py-0.5 rounded-full bg-neon-blue/10 text-neon-blue border border-neon-blue/30">
                            {enabledCount}/{rules.length} {t('autoEnabled')}
                        </span>
                    )}
                </div>
                <button
                    onClick={handleAdd}
                    className="flex items-center gap-1.5 text-xs font-bold px-3 py-1.5 bg-neon-blue/10 border border-neon-blue/40 text-neon-blue rounded-lg hover:bg-neon-blue/20 transition"
                >
                    <Plus className="w-3.5 h-3.5" />
                    {t('autoAddRule')}
                </button>
            </div>

            {/* 콘텐츠 */}
            {loading ? (
                <p className="text-xs text-neon-blue animate-pulse">{t('autoLoading')}</p>
            ) : rules.length === 0 ? (
                <div className="flex flex-col items-center justify-center py-12 border border-dashed border-gray-700 rounded-xl text-gray-600 gap-2">
                    <Zap className="w-8 h-8 opacity-20" />
                    <p className="text-sm">{t('autoNoRules')}</p>
                    <button
                        onClick={handleAdd}
                        className="mt-1 text-xs text-neon-blue hover:underline"
                    >
                        + {t('autoAddRule')}
                    </button>
                </div>
            ) : (
                <div className="grid grid-cols-1 lg:grid-cols-2 gap-3">
                    {rules.map(rule => (
                        <RuleCard
                            key={rule.id}
                            rule={rule}
                            onToggle={handleToggle}
                            onEdit={handleEdit}
                            onDelete={handleDelete}
                        />
                    ))}
                </div>
            )}

            {/* 룰 추가/편집 모달 */}
            {showModal && (
                <RuleModal
                    houseId={houseId}
                    sensors={sensors}
                    actuators={actuators}
                    editingRule={editingRule}
                    onClose={() => { setShowModal(false); setEditingRule(null); }}
                    onSaved={handleSaved}
                />
            )}
        </div>
    );
}
