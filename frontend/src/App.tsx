import { useState, useEffect } from 'react';
import HouseSelector from './components/HouseSelector';
import SensorDashboard from './components/SensorDashboard';
import ActuatorControl from './components/ActuatorControl';
import ManageMode from './components/ManageMode';
import AlarmToast from './components/AlarmToast';
import WeatherWidget from './components/WeatherWidget';
import QuickAddModal from './components/QuickAddModal';
import { Activity, Settings, Globe } from 'lucide-react';
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

  const fetchHouses = async () => {
    try {
      const data = await smartFarmApi.getHouses();
      setHouses(data);
      setRefreshTrigger(prev => prev + 1);
      if (data.length > 0 && !selectedHouseId) {
        setSelectedHouseId(data[0].house_id);
      } else if (data.length === 0) {
        setSelectedHouseId('');
      }
    } catch (e) {
      console.error(e);
    }
  };

  const pollDiscoveredDevices = async () => {
    try {
      const data = await smartFarmApi.getDiscoveredDevices();
      setDiscoveredDevices(data);
    } catch (e) {
      console.error(e);
    }
  };

  useEffect(() => {
    fetchHouses();
    pollDiscoveredDevices();
    const interval = setInterval(() => { pollDiscoveredDevices(); }, 10000);
    return () => clearInterval(interval);
  }, [isManageMode]);

  return (
    <div className="min-h-screen bg-cyber-bg text-gray-200 font-sans">
      {/* Header */}
      <header className="border-b border-cyber-border/30 bg-cyber-surface/50 backdrop-blur-sm sticky top-0 z-50">
        <div className="max-w-7xl mx-auto px-4 h-16 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <Activity className="w-8 h-8 text-neon-blue animate-pulse" />
            <h1 className="text-xl font-bold tracking-wider text-white">
              SMART FARM INTELLIGENCE <span className="text-neon-blue font-light glow-text-blue">OS</span>
            </h1>
          </div>
          <div className="flex items-center gap-4">
            <div className="text-sm text-gray-400">
              {t('systemOnline')} <span className="inline-block w-2 h-2 rounded-full bg-neon-green ml-2 animate-pulse"></span>
            </div>

            {/* 언어 전환 버튼 */}
            <button
              onClick={() => setLang(lang === 'ko' ? 'en' : 'ko')}
              title={lang === 'ko' ? 'Switch to English' : '한국어로 전환'}
              className="text-gray-400 hover:text-white transition flex items-center gap-1.5 bg-white/5 px-3 py-1.5 rounded-full border border-gray-700 hover:border-gray-500"
            >
              <Globe className="w-4 h-4" />
              <span className="text-xs font-semibold">{lang === 'ko' ? 'KO' : 'EN'}</span>
            </button>

            {/* 설정 버튼 */}
            <button
              onClick={() => setIsManageMode(true)}
              className="text-gray-400 hover:text-white transition group flex items-center gap-1 bg-white/5 px-3 py-1.5 rounded-full border border-gray-700 hover:border-gray-500"
            >
              <Settings className="w-4 h-4 group-hover:rotate-45 transition-transform" />
              <span className="text-xs uppercase font-semibold">{t('settings')}</span>
            </button>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 py-8 flex flex-col lg:flex-row gap-8">

        {/* 자동 감지 배너 */}
        {discoveredDevices.length > 0 && (
          <div
            className="absolute top-20 left-1/2 -translate-x-1/2 z-40 bg-amber-500/90 border border-amber-400 text-black px-6 py-3 rounded-xl shadow-[0_0_20px_rgba(245,158,11,0.5)] flex items-center gap-4 cursor-pointer hover:scale-105 transition-transform"
            onClick={() => setShowQuickAddModal(true)}
          >
            <div className="bg-black/20 p-2 rounded-full animate-pulse">
              <Activity className="w-6 h-6" />
            </div>
            <div>
              <h3 className="font-bold text-lg tracking-wide">
                {discoveredDevices.length}{t('devicesDetected')}
              </h3>
              <p className="text-sm font-medium opacity-80">
                {t('clickToRegister')} ({houses.find(h => h.house_id === selectedHouseId)?.name || '-'})
              </p>
            </div>
          </div>
        )}

        {/* 좌측 사이드바: 재배동 선택 */}
        <aside className="w-full lg:w-64 shrink-0">
          <HouseSelector
            houses={houses.map(h => h.name)}
            selectedHouse={houses.find(h => h.house_id === selectedHouseId)?.name || ''}
            onSelectHouse={(name) => {
              const h = houses.find(h => h.name === name);
              if (h) setSelectedHouseId(h.house_id);
            }}
          />
        </aside>

        {/* 우측: 대시보드 */}
        <div className="flex-1 space-y-8">
          <WeatherWidget />

          {selectedHouseId ? (
            <>
              <div className="mb-6">
                <h2 className="text-2xl font-bold text-white tracking-wide border-l-4 border-neon-blue pl-4 mb-2">
                  {houses.find(h => h.house_id === selectedHouseId)?.name} {lang === 'ko' ? '현황' : 'Dashboard'}
                </h2>
                <p className="text-gray-400 text-sm">{t('dashboardSubtitle')}</p>
              </div>

              <section className="bg-cyber-surface rounded-xl p-6 border border-cyber-border/20 shadow-lg">
                <h3 className="text-lg font-semibold mb-4 text-gray-300">{t('envStatus')}</h3>
                <SensorDashboard houseId={selectedHouseId} refreshTrigger={refreshTrigger} />
              </section>

              <section className="bg-cyber-surface rounded-xl p-6 border border-cyber-border/20 shadow-lg mt-8">
                <h3 className="text-lg font-semibold mb-4 text-gray-300">{t('manualControls')}</h3>
                <ActuatorControl houseId={selectedHouseId} />
              </section>
            </>
          ) : (
            <div className="flex flex-col items-center justify-center p-20 text-gray-500 border border-dashed border-gray-700 rounded-xl">
              <span className="text-lg mb-2">{t('noLocationSelected')}</span>
              <span className="text-sm">{t('noLocationHint')}</span>
            </div>
          )}
        </div>
      </main>

      <AlarmToast />

      {showQuickAddModal && (
        <QuickAddModal
          houseId={selectedHouseId}
          houseName={houses.find(h => h.house_id === selectedHouseId)?.name || ''}
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
