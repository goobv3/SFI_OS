import { useState, useEffect } from 'react';
import SensorDashboard from './components/SensorDashboard';
import ActuatorControl from './components/ActuatorControl';
import ManageMode from './components/ManageMode';
import AlarmToast from './components/AlarmToast';
import WeatherWidget from './components/WeatherWidget';
import QuickAddModal from './components/QuickAddModal';
import AutomationPanel from './components/AutomationPanel';
import {
  Activity, Settings, Globe, LogOut,
  ChevronDown, ChevronRight,
  Thermometer, Cloud, Sliders, Bot
} from 'lucide-react';
import { smartFarmApi } from './api/client';
import { useLanguage } from './i18n/LanguageContext';
import { AuthProvider, useAuth } from './contexts/AuthContext';
import LoginPage from './pages/LoginPage';
import SiteOverview from './components/SiteOverview';
import SystemMonitor from './components/SystemMonitor';

type Tab = 'env' | 'control' | 'weather' | 'automation' | 'system';

function MainApp() {
  const { t, lang, setLang } = useLanguage();
  const { user, logout } = useAuth();
  const isAdmin = user?.role?.toLowerCase() === 'admin' || user?.role?.toLowerCase() === 'operator';

  const [sites, setSites] = useState<any[]>([]);
  const [expandedSites, setExpandedSites] = useState<string[]>([]);
  const [houses, setHouses] = useState<any[]>([]);
  const [viewMode, setViewMode] = useState<'overview' | 'house'>('overview');
  const [selectedHouseId, setSelectedHouseId] = useState<string>('');
  const [isManageMode, setIsManageMode] = useState(false);
  const [refreshTrigger, setRefreshTrigger] = useState(0);
  const [discoveredDevices, setDiscoveredDevices] = useState<any[]>([]);
  const [showQuickAddModal, setShowQuickAddModal] = useState(false);
  const [activeTab, setActiveTab] = useState<Tab>('env');

  const fetchData = async () => {
    try {
      const sitesData = await smartFarmApi.getSites();
      const sData = Array.isArray(sitesData) ? sitesData : [];
      setSites(sData);
      
      const housesData = await smartFarmApi.getHouses();
      const hData = Array.isArray(housesData) ? housesData : [];
      setHouses(hData);

      setRefreshTrigger(prev => prev + 1);

      if (sData.length > 0) {
        setExpandedSites(sData.map(s => s.site_id)); // 모두 열어둠
      }

      if (sData.length === 1 && viewMode === 'overview') {
          // 사이트 1개면 바로 첫 번째 하우스로 포커싱
          if (hData.length > 0) {
              setViewMode('house');
              setSelectedHouseId(hData[0].house_id);
          }
      } else if (hData.length === 0) {
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
    fetchData();
    pollDiscoveredDevices();
    const interval = setInterval(pollDiscoveredDevices, 10000);
    return () => clearInterval(interval);
  }, [isManageMode]);

  const selectedHouse = houses.find(h => h.house_id === selectedHouseId);
  const selectedSite = sites.find(s => s.site_id === selectedHouse?.site_id);

  const tabs: { id: Tab; label: string; icon: React.ReactNode }[] = [
    { id: 'env',        label: t('tabEnv'),        icon: <Thermometer className="w-4 h-4" /> },
    { id: 'control',    label: t('tabControl'),    icon: <Sliders className="w-4 h-4" /> },
    { id: 'weather',    label: t('tabWeather'),    icon: <Cloud className="w-4 h-4" /> },
    { id: 'automation', label: t('tabAutomation'), icon: <Bot className="w-4 h-4" /> },
  ];

  return (
    <div className="flex h-screen overflow-hidden bg-cyber-bg text-gray-200 font-sans">

      {/* ════════════════════════════════
          좌측 사이드바 (md 이상에서 표시)
      ════════════════════════════════ */}
      <aside className="hidden md:flex w-56 shrink-0 flex-col bg-surface-card border-r border-cyber-border/20">

        {/* 로고 */}
        <div className="h-12 flex items-center gap-2 px-4 border-b border-cyber-border/20 shrink-0">
          <Activity className="w-5 h-5 text-neon-blue animate-pulse" />
          <h1 className="text-sm font-bold tracking-wider text-white">
            SMART FARM <span className="text-neon-blue font-light glow-text-blue">OS</span>
          </h1>
        </div>

        {/* 재배동 목록 */}
        <div className="flex-1 overflow-y-auto py-3">
          <p className="px-4 mb-2 text-[10px] font-semibold text-gray-500 uppercase tracking-widest flex justify-between items-center">
            <span>{t('locations')}</span>
            <button onClick={() => setViewMode('overview')} className={`px-2 py-0.5 rounded text-[9px] transition ${viewMode === 'overview' ? 'bg-neon-blue/20 text-neon-blue' : 'hover:bg-white/10'}`}>
              전체 현황
            </button>
          </p>
          
          {sites.length === 0 && houses.length === 0 && (
            <p className="px-4 text-xs text-gray-600">{t('noHouses')}</p>
          )}

          {/* 사이트가 여러 개일 때 2-Depth 트리 구현 */}
          {sites.length > 1 ? (
            sites.map(site => {
              const isExpanded = expandedSites.includes(site.site_id);
              const siteHouses = houses.filter(h => h.site_id === site.site_id);
              return (
                <div key={site.site_id} className="mb-1">
                  <button
                    onClick={() => {
                        setExpandedSites(prev => 
                            prev.includes(site.site_id) 
                            ? prev.filter(id => id !== site.site_id)
                            : [...prev, site.site_id]
                        );
                    }}
                    className="relative w-full flex items-center gap-2 px-4 py-2 text-sm text-left transition-colors font-bold text-gray-300 hover:text-white"
                  >
                    <ChevronDown className={`w-3.5 h-3.5 shrink-0 transition-transform ${isExpanded ? '' : '-rotate-90'}`} />
                    <span className="truncate">{site.name}</span>
                  </button>
                  {isExpanded && (
                    <div className="flex flex-col ml-2">
                        {siteHouses.map(h => {
                            const isActive = viewMode === 'house' && h.house_id === selectedHouseId;
                            return (
                            <button
                                key={h.house_id}
                                onClick={() => {
                                    setViewMode('house');
                                    setSelectedHouseId(h.house_id);
                                }}
                                className={`relative w-full flex items-center gap-2 pl-8 pr-4 py-2 text-sm text-left transition-colors
                                ${isActive
                                    ? 'text-neon-blue bg-neon-blue/8 font-semibold'
                                    : 'text-gray-400 hover:text-gray-200 hover:bg-white/4'
                                }`}
                            >
                                {isActive && <span className="absolute left-6 top-1/2 -translate-y-1/2 w-[2px] h-4 bg-neon-blue rounded-r shadow-[0_0_6px_#66fcf1]" />}
                                <span className="truncate">{h.name}</span>
                            </button>
                            );
                        })}
                    </div>
                  )}
                </div>
              );
            })
          ) : (
            /* 사이트가 1개이거나 없으면 하우스들을 Flat하게 렌더링 */
             houses.map(h => {
                const isActive = viewMode === 'house' && h.house_id === selectedHouseId;
                return (
                  <button
                    key={h.house_id}
                    onClick={() => {
                        setViewMode('house');
                        setSelectedHouseId(h.house_id);
                    }}
                    className={`relative w-full flex items-center gap-2 px-4 py-2.5 text-sm text-left transition-colors
                      ${isActive
                        ? 'text-neon-blue bg-neon-blue/8 font-semibold'
                        : 'text-gray-400 hover:text-gray-200 hover:bg-white/4'
                      }`}
                  >
                    {isActive && (
                      <span className="absolute left-0 top-1/2 -translate-y-1/2 w-[2px] h-5 bg-neon-blue rounded-r shadow-[0_0_6px_#66fcf1]" />
                    )}
                    <ChevronRight className={`w-3 h-3 shrink-0 transition-opacity ${isActive ? 'text-neon-blue opacity-100' : 'opacity-0'}`} />
                    <span className="truncate">{h.name}</span>
                  </button>
                );
              })
          )}
        </div>

        {/* 하단: 날씨 토글 + 관리 메뉴 */}
        <div className="border-t border-cyber-border/20 px-4 py-3 space-y-2 shrink-0">
          <button
            onClick={() => setActiveTab('weather')}
            className="w-full flex items-center justify-between text-xs text-gray-400 hover:text-gray-200 transition py-1"
          >
            <span className="flex items-center gap-1.5">🌤 {t('tabWeather')}</span>
            <ChevronDown className={`w-3.5 h-3.5 transition-transform ${activeTab === 'weather' ? 'rotate-180' : ''}`} />
          </button>

          {/* 시스템 모니터링 (Admin Only) */}
          {isAdmin && (
            <button
              onClick={() => {
                setActiveTab('system');
                setViewMode('overview'); // 특별 뷰는 오버뷰 모드(하우스 미선택)와 조합
              }}
              className={`w-full flex items-center gap-1.5 text-xs transition py-1.5 px-2 rounded-lg border
                ${activeTab === 'system' 
                  ? 'bg-neon-blue/10 border-neon-blue/40 text-neon-blue font-bold shadow-[0_0_8px_rgba(102,252,241,0.2)]' 
                  : 'text-gray-400 hover:text-gray-200 border-transparent hover:bg-white/5'
                }`}
            >
              <Activity className="w-3.5 h-3.5" />
              시스템 모니터링
            </button>
          )}

          <div className="flex items-center gap-1.5 text-xs text-gray-500">
            <span className="inline-block w-1.5 h-1.5 rounded-full bg-neon-green animate-pulse" />
            {t('systemOnline')} (Role: {user?.role || 'None'})
          </div>
        </div>
      </aside>

      {/* ════════════════════════════════
          우측 메인 영역
      ════════════════════════════════ */}
      <div className="flex-1 flex flex-col overflow-hidden pb-14 md:pb-0">

        {/* 슬림 헤더 (h-12) */}
        <header className="h-12 shrink-0 flex items-center justify-between px-4 border-b border-cyber-border/20 bg-surface-card/80 backdrop-blur-sm z-40">
          {/* 브레드크럼 */}
          <div className="flex items-center gap-2 text-sm">
            {/* 모바일 로고 */}
            <Activity className="w-4 h-4 text-neon-blue md:hidden" />
            <span className="text-gray-500 hidden md:inline text-xs">SMART FARM OS</span>
            {viewMode === 'overview' ? (
                <>
                  <ChevronRight className="w-3.5 h-3.5 text-gray-600 hidden md:inline" />
                  <span className="font-semibold text-white text-xs">전체 현황</span>
                </>
            ) : selectedHouse && (
              <>
                <ChevronRight className="w-3.5 h-3.5 text-gray-600 hidden md:inline" />
                <span className="text-gray-400 text-xs">{selectedSite?.name || '사이트 미지정'}</span>
                <ChevronRight className="w-3.5 h-3.5 text-gray-600 hidden md:inline" />
                <span className="font-semibold text-white text-xs">{selectedHouse.name}</span>
              </>
            )}
          </div>

          {/* 우측 액션 버튼들 */}
          <div className="flex items-center gap-2">
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
              <span className="hidden sm:inline">{t('settings')}</span>
            </button>

            {/* 로그아웃 */}
            <button
              onClick={logout}
              title="Logout"
              className="text-red-400 hover:text-red-300 transition group flex items-center gap-1 bg-red-500/10 px-2 py-1 rounded-lg border border-red-500/30 hover:border-red-500/60 text-xs font-semibold ml-2"
            >
              <LogOut className="w-3.5 h-3.5" />
            </button>
          </div>
        </header>

        {/* 탭 바 */}
        <nav className="shrink-0 flex items-center gap-1 px-4 pt-2 pb-0 border-b border-cyber-border/20 bg-surface-card/40">
          {tabs.map(tab => (
            <button
              key={tab.id}
              onClick={() => setActiveTab(tab.id)}
              className={`flex items-center gap-1.5 px-3 py-2 text-xs font-semibold rounded-t-lg transition-colors border-b-2
                ${activeTab === tab.id
                  ? 'text-neon-blue border-neon-blue bg-neon-blue/5'
                  : 'text-gray-500 border-transparent hover:text-gray-300 hover:border-gray-600'
                }`}
            >
              {tab.icon}
              {tab.label}
            </button>
          ))}
        </nav>

        {/* 탭 콘텐츠 (스크롤 영역) */}
        <main className="flex-1 overflow-y-auto p-4 space-y-3">
          
          {/* 전체 대시보드 */}
          {viewMode === 'overview' && activeTab === 'env' && (
              <SiteOverview onSiteClick={(siteId) => {
                  setExpandedSites(prev => prev.includes(siteId) ? prev : [...prev, siteId]);
                  const firstHouseForSite = houses.find(h => h.site_id === siteId);
                  if (firstHouseForSite) {
                      setViewMode('house');
                      setSelectedHouseId(firstHouseForSite.house_id);
                  }
              }} />
          )}

          {/* 재배동 미선택 (house 모드인데 선택안된경우) */}
          {viewMode === 'house' && !selectedHouseId && activeTab !== 'weather' && (
            <div className="flex flex-col items-center justify-center py-16 text-gray-600 border border-dashed border-gray-700 rounded-lg">
              <span className="text-sm mb-1">{t('noLocationSelected')}</span>
              <span className="text-xs">{t('noLocationHint')}</span>
            </div>
          )}

          {/* ── 환경현황 탭 ── */}
          {viewMode === 'house' && activeTab === 'env' && selectedHouseId && (
            <section className="animate-fade-in-up bg-cyber-surface/40 rounded-xl px-4 py-3 border border-cyber-border/20">
              <h3 className="text-xs font-semibold mb-3 text-gray-400 uppercase tracking-widest">{t('envStatus')}</h3>
              <SensorDashboard houseId={selectedHouseId} refreshTrigger={refreshTrigger} />
            </section>
          )}

          {/* ── 제어 탭 ── */}
          {viewMode === 'house' && activeTab === 'control' && selectedHouseId && (
            <section className="animate-fade-in-up bg-cyber-surface/40 rounded-xl px-4 py-3 border border-cyber-border/20">
              <h3 className="text-xs font-semibold mb-3 text-gray-400 uppercase tracking-widest">{t('manualControls')}</h3>
              <ActuatorControl houseId={selectedHouseId} />
            </section>
          )}

          {/* ── 날씨 탭 ── */}
          {activeTab === 'weather' && (
            <section className="animate-fade-in-up bg-cyber-surface/40 rounded-xl px-4 py-3 border border-cyber-border/20">
              <h3 className="text-xs font-semibold mb-3 text-gray-400 uppercase tracking-widest">🌤 {t('externalWeather')}</h3>
              <WeatherWidget compact />
            </section>
          )}

          {/* ── 자동화 탭 ── */}
          {viewMode === 'house' && activeTab === 'automation' && selectedHouseId && (
            <section className="animate-fade-in-up bg-cyber-surface/40 rounded-xl px-4 py-3 border border-cyber-border/20">
              <AutomationPanel houseId={selectedHouseId} houseName={selectedHouse?.name} />
            </section>
          )}
          {viewMode === 'house' && activeTab === 'automation' && !selectedHouseId && (
            <div className="flex flex-col items-center justify-center py-16 text-gray-600 border border-dashed border-gray-700 rounded-lg">
              <span className="text-sm mb-1">{t('noLocationSelected')}</span>
              <span className="text-xs">{t('noLocationHint')}</span>
            </div>
          )}

          {/* ── 시스템 모니터링 탭 (보안 적용) ── */}
          {activeTab === 'system' && (
            isAdmin ? (
              <SystemMonitor />
            ) : (
              <div className="flex flex-col items-center justify-center py-20 text-red-500 bg-red-500/5 border border-red-500/20 rounded-xl">
                <Activity className="w-12 h-8 mb-4 animate-pulse" />
                <h2 className="text-lg font-bold mb-2">접근 권한 없음</h2>
                <p className="text-sm text-gray-500">관리자 또는 오퍼레이터 권한이 필요한 메뉴입니다.</p>
                <button 
                  onClick={() => setActiveTab('env')}
                  className="mt-6 px-4 py-2 bg-red-500/20 hover:bg-red-500/30 border border-red-500/40 rounded-lg text-xs font-bold transition"
                >
                  대시보드로 돌아가기
                </button>
              </div>
            )
          )}
        </main>
      </div>

      {/* ════════════════════════════════
          모바일 하단 탭바 (md 미만)
      ════════════════════════════════ */}
      <nav className="md:hidden fixed bottom-0 inset-x-0 h-14 flex bg-surface-card border-t border-cyber-border/20 z-50">
        {/* 재배동 선택 버튼 */}
        <div className="relative flex-1">
          <select
            value={selectedHouseId}
            onChange={e => setSelectedHouseId(e.target.value)}
            className="absolute inset-0 opacity-0 w-full h-full cursor-pointer"
          >
            {houses.map(h => <option key={h.house_id} value={h.house_id}>{h.name}</option>)}
          </select>
          <div className="flex flex-col items-center justify-center h-full gap-0.5 pointer-events-none">
            <ChevronDown className="w-5 h-5 text-gray-400" />
            <span className="text-[10px] text-gray-500 truncate max-w-[60px]">
              {selectedHouse?.name || '선택'}
            </span>
          </div>
        </div>
        {tabs.map(tab => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`flex-1 flex flex-col items-center justify-center gap-0.5 transition-colors
              ${activeTab === tab.id ? 'text-neon-blue' : 'text-gray-500'}`}
          >
            {tab.icon}
            <span className="text-[10px] font-medium">{tab.label}</span>
          </button>
        ))}
        {/* 설정 버튼 */}
        <button
          onClick={() => setIsManageMode(true)}
          className="flex-1 flex flex-col items-center justify-center gap-0.5 text-gray-500 hover:text-gray-300 transition-colors"
        >
          <Settings className="w-5 h-5" />
          <span className="text-[10px]">{t('settings')}</span>
        </button>
      </nav>

      {/* ════════════════════════════════
          전역 오버레이 컴포넌트
      ════════════════════════════════ */}
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
        <ManageMode onClose={() => setIsManageMode(false)} onUpdate={fetchData} />
      )}
    </div>
  );
}

export default function App() {
  return (
    <AuthProvider>
      <AuthWrapper />
    </AuthProvider>
  );
}

function AuthWrapper() {
  const { isAuthenticated } = useAuth();
  
  if (!isAuthenticated) {
    return <LoginPage />;
  }

  return <MainApp />;
}
