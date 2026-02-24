import { useState, useEffect } from 'react';
import HouseSelector from './components/HouseSelector';
import SensorDashboard from './components/SensorDashboard';
import ActuatorControl from './components/ActuatorControl';
import ManageMode from './components/ManageMode';
import AlarmToast from './components/AlarmToast';
import { Activity, Settings } from 'lucide-react';
import { smartFarmApi } from './api/client';

function App() {
  const [houses, setHouses] = useState<any[]>([]);
  const [selectedHouseId, setSelectedHouseId] = useState<string>('');
  const [isManageMode, setIsManageMode] = useState(false);
  const [refreshTrigger, setRefreshTrigger] = useState(0);

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

  useEffect(() => {
    fetchHouses();
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
          <div className="flex items-center gap-6">
            <div className="text-sm text-gray-400">
              System Online <span className="inline-block w-2 h-2 rounded-full bg-neon-green ml-2 animate-pulse"></span>
            </div>
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

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 py-8 flex flex-col lg:flex-row gap-8">

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

      {/* Settings Modal */}
      {isManageMode && (
        <ManageMode onClose={() => setIsManageMode(false)} onUpdate={fetchHouses} />
      )}
    </div>
  );
}

export default App;
