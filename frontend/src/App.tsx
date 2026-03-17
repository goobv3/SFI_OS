import { useState, useEffect, useRef } from 'react';
import SensorDashboard from './components/SensorDashboard';
import ActuatorControl from './components/ActuatorControl';
import ManageMode from './components/ManageMode';
import AlarmToast from './components/AlarmToast';
import WeatherWidget from './components/WeatherWidget';
import QuickAddModal from './components/QuickAddModal';
import { Activity, Settings, Globe, ChevronDown, ChevronRight } from 'lucide-react';
import { smartFarmApi } from './api/client';
import { useLanguage } from './i18n/LanguageContext';

function App() {
  const { t, lang, setLang } = useLanguage();

  const [houses, setHouses] = useState<any[]>([]);
  const [selectedHouseId, setSelectedHouseId] = useState<string>('');
  const [isManageMode, setIsManageMode] = useState(false);
  const [refreshTrigger, setRefreshTrigger] = useState(0);
  const [discoveredDevices, setDiscoveredDevices] = useState<any[]>([]);
  const [showQuickAddModal, setShowQuickAddModal] = useState(false);
  const [dropdownOpen, setDropdownOpen] = useState(false);
  const [weatherExpanded, setWeatherExpanded] = useState(false);
  const dropdownRef = useRef<HTMLDivElement>(null);

  const fetchHouses = async () => {
    try {
      const data = await smartFarmApi.getHouses();
      const housesData = Array.isArray(data) ? data : [];
      setHouses(housesData);
      setRefreshTrigger(prev => prev + 1);
      if (housesData.length > 0 && !selectedHouseId) {
        setSelectedHouseId(housesData[0].house_id);
      } else if (housesData.length === 0) {
        setSelectedHouseId('');
      }
    } catch (e) { console.error(e); }
  };

  const pollDiscoveredDevices = async () => {
    try {
      const data = await smartFarmApi.getDiscoveredDevices();
      setDiscoveredDevices(data);
    } catch (e) { console.error(e); }
  };

  useEffect(() => {
    fetchHouses();
    pollDiscoveredDevices();
    const interval = setInterval(pollDiscoveredDevices, 10000);
    return () => clearInterval(interval);
  }, [isManageMode]);

  // 드롭다운 외부 클릭 시 닫기
  useEffect(() => {
    const handler = (e: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target as Node)) {
        setDropdownOpen(false);
      }
    };
    document.addEventListener('mousedown', handler);
    return () => document.removeEventListener('mousedown', handler);
  }, []);

  const selectedHouse = houses.find(h => h.house_id === selectedHouseId);

  return (
    <div className="min-h-screen bg-cyber-bg text-gray-200 font-sans">

      {/* ── Header ── */}
      <header className="border-b border-cyber-border/30 bg-cyber-surface/50 backdrop-blur-sm sticky top-0 z-50">
        <div className="max-w-screen-xl mx-auto px-4 h-12 flex items-center justify-between gap-4">

          {/* 로고 */}
          <div className="flex items-center gap-2 shrink-0">
            <Activity className="w-5 h-5 text-neon-blue animate-pulse" />
            <h1 className="text-sm font-bold tracking-wider text-white hidden sm:block">
              SMART FARM <span className="text-neon-blue font-light glow-text-blue">OS</span>
            </h1>
          </div>

          {/* ── 재배동 드롭다운 ── */}
          <div ref={dropdownRef} className="relative">
            <button
              onClick={() => setDropdownOpen(v => !v)}
              className="flex items-center gap-2 bg-white/5 border border-neon-blue/40 hover:border-neon-blue text-white px-3 py-1.5 rounded-lg text-sm font-medium transition min-w-[140px] justify-between"
            >
              <span className="truncate">{selectedHouse?.name || t('noLocationSelected')}</span>
              <ChevronDown className={`w-4 h-4 text-neon-blue transition-transform shrink-0 ${dropdownOpen ? 'rotate-180' : ''}`} />
            </button>

            {dropdownOpen && (
              <div className="absolute top-full mt-1 left-0 bg-[#1a1c24] border border-cyber-border/60 rounded-lg shadow-xl z-50 min-w-[160px] overflow-hidden">
                {houses.length === 0 && (
                  <div className="px-4 py-3 text-xs text-gray-500">{t('noHouses')}</div>
                )}
                {houses.map(h => (
                  <button
                    key={h.house_id}
                    onClick={() => { setSelectedHouseId(h.house_id); setDropdownOpen(false); }}
                    className={`w-full flex items-center gap-2 px-4 py-2.5 text-sm text-left transition hover:bg-white/5
                      ${h.house_id === selectedHouseId ? 'text-neon-blue bg-neon-blue/10 font-semibold' : 'text-gray-300'}`}
                  >
                    <ChevronRight className={`w-3 h-3 shrink-0 ${h.house_id === selectedHouseId ? 'text-neon-blue' : 'text-transparent'}`} />
                    {h.name}
                  </button>
                ))}
              </div>
            )}
          </div>

          {/* 우측 버튼들 */}
          <div className="flex items-center gap-2 ml-auto">
            {/* 시스템 상태 */}
            <div className="hidden md:flex text-xs text-gray-400 items-center gap-1">
              {t('systemOnline')} <span className="inline-block w-1.5 h-1.5 rounded-full bg-neon-green animate-pulse"></span>
            </div>

            {/* 감지 기기 배지 */}
            {discoveredDevices.length > 0 && (
              <button
                onClick={() => setShowQuickAddModal(true)}
                className="flex items-center gap-1 bg-amber-500/20 border border-amber-500/50 text-amber-400 px-2 py-1 rounded-lg text-xs font-bold animate-pulse hover:bg-amber-500/30 transition"
              >
                <Activity className="w-3.5 h-3.5" />
                {discoveredDevices.length} {t('new')}
              </button>
            )}

            {/* 언어 토글 */}
            <button
              onClick={() => setLang(lang === 'ko' ? 'en' : 'ko')}
              title={lang === 'ko' ? 'Switch to English' : '한국어로 전환'}
              className="text-gray-400 hover:text-white transition flex items-center gap-1 bg-white/5 px-2 py-1 rounded-lg border border-gray-700 hover:border-gray-500 text-xs font-semibold"
            >
              <Globe className="w-3.5 h-3.5" />
              {lang === 'ko' ? 'KO' : 'EN'}
            </button>

            {/* 설정 */}
            <button
              onClick={() => setIsManageMode(true)}
              className="text-gray-400 hover:text-white transition group flex items-center gap-1 bg-white/5 px-2 py-1 rounded-lg border border-gray-700 hover:border-gray-500 text-xs font-semibold"
            >
              <Settings className="w-3.5 h-3.5 group-hover:rotate-45 transition-transform" />
              {t('settings')}
            </button>
          </div>
        </div>
      </header>

      {/* ── Main ── */}
      <main className="max-w-screen-xl mx-auto px-4 py-3 space-y-3">

        {/* ── 날씨 (접기/펼치기) ── */}
        <div className="bg-cyber-surface/60 rounded-lg border border-cyber-border/20">
          <button
            className="w-full flex items-center justify-between px-4 py-2 text-left"
            onClick={() => setWeatherExpanded(v => !v)}
          >
            <span className="text-xs font-semibold text-gray-400 uppercase tracking-widest flex items-center gap-2">
              🌤 {t('externalWeather')}
            </span>
            <ChevronDown className={`w-4 h-4 text-gray-500 transition-transform ${weatherExpanded ? 'rotate-180' : ''}`} />
          </button>
          {weatherExpanded && (
            <div className="px-4 pb-4 pt-1 border-t border-cyber-border/10">
              <WeatherWidget compact />
            </div>
          )}
        </div>

        {/* ── 대시보드 콘텐츠 ── */}
        {selectedHouseId ? (
          <>
            {/* 재배동 제목 */}
            <div className="flex items-center gap-3">
              <div className="h-5 w-1 bg-neon-blue rounded-full shadow-[0_0_8px_#66fcf1]" />
              <h2 className="text-base font-bold text-white tracking-wide">
                {selectedHouse?.name} {lang === 'ko' ? '현황' : 'Dashboard'}
              </h2>
              <p className="text-gray-500 text-xs">{t('dashboardSubtitle')}</p>
            </div>

            {/* 센서 */}
            <section className="bg-cyber-surface/60 rounded-lg px-4 py-3 border border-cyber-border/20">
              <h3 className="text-xs font-semibold mb-3 text-gray-400 uppercase tracking-widest">{t('envStatus')}</h3>
              <SensorDashboard houseId={selectedHouseId} refreshTrigger={refreshTrigger} />
            </section>

            {/* 제어 */}
            <section className="bg-cyber-surface/60 rounded-lg px-4 py-3 border border-cyber-border/20">
              <h3 className="text-xs font-semibold mb-3 text-gray-400 uppercase tracking-widest">{t('manualControls')}</h3>
              <ActuatorControl houseId={selectedHouseId} />
            </section>
          </>
        ) : (
          <div className="flex flex-col items-center justify-center py-16 text-gray-600 border border-dashed border-gray-700 rounded-lg">
            <span className="text-sm mb-1">{t('noLocationSelected')}</span>
            <span className="text-xs">{t('noLocationHint')}</span>
          </div>
        )}
      </main>

      <AlarmToast />

      {showQuickAddModal && (
        <QuickAddModal
          houseId={selectedHouseId}
          houseName={selectedHouse?.name || ''}
          devices={discoveredDevices}
          onClose={() => setShowQuickAddModal(false)}
          onComplete={() => {
            setShowQuickAddModal(false);
            setRefreshTrigger(prev => prev + 1);
            pollDiscoveredDevices();
          }}
        />
      )}

      {isManageMode && (
        <ManageMode onClose={() => setIsManageMode(false)} onUpdate={fetchHouses} />
      )}
    </div>
  );
}

export default App;
