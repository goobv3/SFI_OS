import { useState, useEffect } from 'react';
import HouseSelector from './components/HouseSelector';
import SensorDashboard from './components/SensorDashboard';
import ActuatorControl from './components/ActuatorControl';
import ManageMode from './components/ManageMode';
import AlarmToast from './components/AlarmToast';
import WeatherWidget from './components/WeatherWidget';
import QuickAddModal from './components/QuickAddModal';
import { Activity, Settings } from 'lucide-react';
import { smartFarmApi } from './api/client';

// ---------------------------------------------------------
// 프론트엔드 메인 앱 화면 (Main Frontend Application)
// ---------------------------------------------------------
// 이 파일은 스마트팜 웹페이지에 들어왔을 때 가장 먼저 보여지는 "대문"입니다.
// 각종 부품(컴포넌트)들을 화면 좌, 우, 상단에 적절히 뼈대를 잡아 배치하는 역할을 합니다.
// ---------------------------------------------------------

function App() {
  // --- React 상태(State) 변수들 ---
  // 화면이 바뀌어야 할 때마다 데이터를 기억해두는 공간입니다.
  const [houses, setHouses] = useState<any[]>([]); // 데이터베이스에 등록된 1동, 2동 하우스 목록
  const [selectedHouseId, setSelectedHouseId] = useState<string>(''); // 현재 화면에 띄운 하우스 ID (버튼 클릭시 변경됨)
  const [isManageMode, setIsManageMode] = useState(false); // 오른쪽 위 톱니바퀴(Settings)를 눌렀는지 여부
  const [refreshTrigger, setRefreshTrigger] = useState(0); // 데이터를 새로고침할 때 숫자를 올려 화면을 다시 그리게 하는 트리거
  const [discoveredDevices, setDiscoveredDevices] = useState<any[]>([]); // 미등록된 새 기기 목록
  const [showQuickAddModal, setShowQuickAddModal] = useState(false); // 1-Click 등록 모달 표시 여부

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
    
    // 10초마다 자동 스캔 (기존의 수동 스캔 대체)
    const interval = setInterval(() => {
        pollDiscoveredDevices();
    }, 10000);
    
    return () => clearInterval(interval);
  }, [isManageMode]);

  return (
    <div className="min-h-screen bg-cyber-bg text-gray-200 font-sans">
      {/* 1. Header (화면 가장 위쪽 헤더 영역) */}
      <header className="border-b border-cyber-border/30 bg-cyber-surface/50 backdrop-blur-sm sticky top-0 z-50">
        <div className="max-w-7xl mx-auto px-4 h-16 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <Activity className="w-8 h-8 text-neon-blue animate-pulse" />
            <h1 className="text-xl font-bold tracking-wider text-white">
              SMART FARM INTELLIGENCE <span className="text-neon-blue font-light glow-text-blue">OS</span>
            </h1>
          </div>
          <div className="flex items-center gap-6">
            <div className="text-sm text-gray-400">
              System Online <span className="inline-block w-2 h-2 rounded-full bg-neon-green ml-2 animate-pulse"></span>
            </div>
            {/* 설정 모드(모달 창)를 띄우는 톱니바퀴 버튼 */}
            <button
              onClick={() => setIsManageMode(true)}
              className="text-gray-400 hover:text-white transition group flex items-center gap-1 bg-white/5 px-3 py-1.5 rounded-full border border-gray-700 hover:border-gray-500"
            >
              <Settings className="w-4 h-4 group-hover:rotate-45 transition-transform" />
              <span className="text-xs uppercase font-semibold">Settings</span>
            </button>
          </div>
        </div>
      </header>

      {/* 2. Main Content (헤더 아래의 실제 몸통 영역) */}
      <main className="max-w-7xl mx-auto px-4 py-8 flex flex-col lg:flex-row gap-8">

        {/* --- Proactive Auto-Discovery Banner --- */}
        {discoveredDevices.length > 0 && (
            <div className="absolute top-20 left-1/2 -translate-x-1/2 z-40 bg-amber-500/90 border border-amber-400 text-black px-6 py-3 rounded-xl shadow-[0_0_20px_rgba(245,158,11,0.5)] flex items-center gap-4 cursor-pointer hover:scale-105 transition-transform" onClick={() => setShowQuickAddModal(true)}>
                <div className="bg-black/20 p-2 rounded-full animate-pulse">
                    <Activity className="w-6 h-6" />
                </div>
                <div>
                    <h3 className="font-bold text-lg tracking-wide">새로운 기기 {discoveredDevices.length}개가 감지되었습니다!</h3>
                    <p className="text-sm font-medium opacity-80">여기를 클릭하여 현재 하우스({houses.find(h => h.house_id === selectedHouseId)?.name || '선택 안됨'})에 1-Click 등록하세요.</p>
                </div>
            </div>
        )}

        {/* Left Sidebar: House Selector */}
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

        {/* Right Content: Dashboard & Controls */}
        <div className="flex-1 space-y-8">
          <WeatherWidget />

          {selectedHouseId ? (
            <>
              <div className="mb-6">
                <h2 className="text-2xl font-bold text-white tracking-wide border-l-4 border-neon-blue pl-4 mb-2">
                  {houses.find(h => h.house_id === selectedHouseId)?.name} Dashboard
                </h2>
                <p className="text-gray-400 text-sm">Real-time environment monitoring and control.</p>
              </div>

              {/* Sensor Visualization Area */}
              <section className="bg-cyber-surface rounded-xl p-6 border border-cyber-border/20 shadow-lg">
                <h3 className="text-lg font-semibold mb-4 text-gray-300">Environment Status (Click for History)</h3>
                <SensorDashboard houseId={selectedHouseId} refreshTrigger={refreshTrigger} />
              </section>

              {/* Actuator Control Area */}
              <section className="bg-cyber-surface rounded-xl p-6 border border-cyber-border/20 shadow-lg mt-8">
                <h3 className="text-lg font-semibold mb-4 text-gray-300">Manual Controls</h3>
                <ActuatorControl houseId={selectedHouseId} />
              </section>
            </>
          ) : (
            <div className="flex flex-col items-center justify-center p-20 text-gray-500 border border-dashed border-gray-700 rounded-xl">
              <span className="text-lg mb-2">No location selected.</span>
              <span className="text-sm">Click the Settings (⚙️) panel to register a new House.</span>
            </div>
          )}
        </div>
      </main>

      {/* Alarm Notifications */}
      <AlarmToast />

      {/* Quick Add Modal */}
      {showQuickAddModal && (
        <QuickAddModal 
            houseId={selectedHouseId} 
            houseName={houses.find(h => h.house_id === selectedHouseId)?.name || ''}
            devices={discoveredDevices} 
            onClose={() => setShowQuickAddModal(false)}
            onComplete={() => {
                setShowQuickAddModal(false);
                setRefreshTrigger(prev => prev + 1); // reload dashboard
                pollDiscoveredDevices(); // clear banner
            }}
        />
      )}

      {/* Settings Modal */}
      {isManageMode && (
        <ManageMode onClose={() => setIsManageMode(false)} onUpdate={fetchHouses} />
      )}
    </div>
  );
}

export default App;
