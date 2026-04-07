import axios from 'axios';

// ---------------------------------------------------------
// 프론트엔드 API 통신 관리자 (API Client)
// ---------------------------------------------------------
// 이 파일은 리액트 화면(프론트엔드)이 데이터가 필요할 때마다 
// 파이썬 서버(백엔드)로 요청사항을 보내고 받아오는 '우체부' 역할을 합니다.
// ---------------------------------------------------------

// 백엔드 서버의 기본 주소입니다. 모든 통신은 이 주소로 시작합니다.
const API_BASE_URL = '/api';

// axios(통신 라이브러리)를 이용해 기본적으로 사용할 편지지(설정)를 만듭니다.
const apiClient = axios.create({
    baseURL: API_BASE_URL,
    headers: {
        'Content-Type': 'application/json', // 데이터를 JSON(글자) 형태로 보낸다고 알려줌
    },
});

// 요청 인터셉터: 로컬 스토리지에 토큰이 있으면 Authorization 헤더 추가
apiClient.interceptors.request.use(
    (config) => {
        const token = localStorage.getItem('access_token');
        if (token) {
            config.headers['Authorization'] = `Bearer ${token}`;
        }
        return config;
    },
    (error) => Promise.reject(error)
);

// 응답 인터셉터: 401 Unauthorized 시 인증 정보 초기화 및 로그인 이동
apiClient.interceptors.response.use(
    (response) => response,
    (error) => {
        if (error.response && error.response.status === 401) {
            localStorage.removeItem('access_token');
            // 이미 로그인 페이지면 루프 방지, 아니면 리다이렉트
            // 하지만 App.tsx에서 상태로 관리할 것이므로 
            // 강제 리로드하여 최상단 AuthProvider가 에러를 잡게 하거나
            // 이벤트를 발생시킬 수 있습니다.
            window.location.reload(); 
        }
        return Promise.reject(error);
    }
);

// 기능별로 사용할 수 있는 우체부 기능 목록
export const smartFarmApi = {
    // --- [0. 멀티사이트(농장) 관리 통신] ---
    getSites: async () => {
        const response = await apiClient.get('/sites');
        return response.data;
    },
    getSiteOverview: async (siteId: string) => {
        const response = await apiClient.get(`/sites/${siteId}/overview`);
        return response.data;
    },
    getSiteHouses: async (siteId: string) => {
        const response = await apiClient.get(`/sites/${siteId}/houses`);
        return response.data;
    },
    createSite: async (payload: { site_id: string, name: string, location: string }) => {
        const response = await apiClient.post('/sites', payload);
        return response.data;
    },
    updateSite: async (siteId: string, payload: { name: string, location: string, timezone?: string }) => {
        const response = await apiClient.put(`/sites/${siteId}`, payload);
        return response.data;
    },
    deleteSite: async (siteId: string) => {
        const response = await apiClient.delete(`/sites/${siteId}`);
        return response.data;
    },

    // --- [1. 하우스(온실) 관리 통신] ---
    getHouses: async () => {
        const response = await apiClient.get('/houses');
        return response.data; // [{ house_id, name, created_at }, ...]
    },
    createHouse: async (houseId: string, name: string, displayOrder: number = 0) => {
        const response = await apiClient.post('/metadata/houses', { house_id: houseId, name, display_order: displayOrder });
        return response.data;
    },
    updateHouse: async (houseId: string, name: string, displayOrder: number = 0) => {
        const response = await apiClient.put(`/metadata/houses/${houseId}`, { name, display_order: displayOrder });
        return response.data;
    },
    deleteHouse: async (houseId: string) => {
        const response = await apiClient.delete(`/metadata/houses/${houseId}`);
        return response.data;
    },

    // Devices
    getHouseDevices: async (houseId: string) => {
        const response = await apiClient.get(`/houses/${houseId}/devices`);
        return response.data; // { sensors: [], actuators: [] }
    },
    createSensor: async (payload: { sensor_id: string, house_id: string, alias: string, type: string, unit: string, display_order?: number }) => {
        const response = await apiClient.post('/metadata/sensors', payload);
        return response.data;
    },
    updateSensor: async (sensorId: string, payload: { alias: string, type: string, unit: string, display_order?: number, is_active?: boolean, warn_high?: number | null, warn_low?: number | null, crit_high?: number | null, crit_low?: number | null }) => {
        const response = await apiClient.put(`/metadata/sensors/${sensorId}`, payload);
        return response.data;
    },
    deleteSensor: async (sensorId: string) => {
        const response = await apiClient.delete(`/metadata/sensors/${sensorId}`);
        return response.data;
    },
    createActuator: async (payload: { actuator_id: string, house_id: string, alias: string, type: string }) => {
        const response = await apiClient.post('/metadata/actuators', payload);
        return response.data;
    },
    updateActuator: async (actuatorId: string, payload: { alias: string, type: string }) => {
        const response = await apiClient.put(`/metadata/actuators/${actuatorId}`, payload);
        return response.data;
    },
    deleteActuator: async (actuatorId: string) => {
        const response = await apiClient.delete(`/metadata/actuators/${actuatorId}`);
        return response.data;
    },

    // Discovery (Inbox)
    getDiscoveredDevices: async () => {
        const response = await apiClient.get('/discovery');
        return response.data;
    },
    deleteDiscoveredDevice: async (deviceId: string) => {
        const response = await apiClient.delete(`/discovery/${deviceId}`);
        return response.data;
    },

    // Sensor Data & History
    sendSensorData: async (sensorId: string, value: number) => {
        const response = await apiClient.post('/sensors', {
            sensor_id: sensorId,
            value: value,
        });
        return response.data;
    },
    getSensorHistory: async (sensorId: string, period: 'daily' | 'monthly' | 'yearly') => {
        const response = await apiClient.get(`/sensors/${sensorId}/history?period=${period}`);
        return response.data; // [{ time: '...', avg_value: 23.5 }, ...]
    },
    getSensorHistoryByRange: async (sensorIds: string[], startTime: string, endTime: string, resolution: string = 'auto') => {
        const ids = sensorIds.join(',');
        const response = await apiClient.get(`/sensors/${ids}/history?start=${startTime}&end=${endTime}&resolution=${resolution}`);
        return response.data;
    },

    // Alarms
    getAlarms: async () => {
        const response = await apiClient.get('/alarms');
        return response.data; // [{id, sensor_id, level, message, is_acknowledged, created_at, ...}]
    },
    acknowledgeAlarm: async (alarmId: number) => {
        const response = await apiClient.post(`/alarms/${alarmId}/acknowledge`);
        return response.data;
    },

    // Control
    sendControlCommand: async (actuatorId: string, command: string, priority: number = 2) => {
        const response = await apiClient.post('/control', {
            actuator_id: actuatorId,
            command: command,
            source: 'UserDashboard',
            priority: priority,
        });
        return response.data;
    },

    // --- [5. 날씨 정보 통신] ---
    getLatestWeather: async (hours_ahead: number = 0) => {
        // 백엔드에게 "제발 최신 날씨(그리고 hours_ahead 시간 뒤의 예보)를 줘!" 라고 요청합니다.
        const response = await apiClient.get(`/weather/latest?hours_ahead=${hours_ahead}`);
        return response.data; // 서버가 준 날씨 데이터를 반환합니다.
    },

    // --- [6. 자동화 룰 엔진] ---
    getRules: async (houseId?: string) => {
        const query = houseId ? `?house_id=${houseId}` : '';
        const response = await apiClient.get(`/automation/rules${query}`);
        return response.data;
    },
    createRule: async (payload: {
        name?: string;
        house_id: string;
        trigger_sensor_id: string;
        condition_type: 'GT' | 'LT' | 'GTE' | 'LTE';
        threshold_value: number;
        actuator_id: string;
        action_command: string;
        cooldown_minutes?: number;
    }) => {
        const response = await apiClient.post('/automation/rules', payload);
        return response.data;
    },
    updateRule: async (id: number, payload: Partial<{
        name: string;
        trigger_sensor_id: string;
        condition_type: 'GT' | 'LT' | 'GTE' | 'LTE';
        threshold_value: number;
        actuator_id: string;
        action_command: string;
        cooldown_minutes: number;
    }>) => {
        const response = await apiClient.put(`/automation/rules/${id}`, payload);
        return response.data;
    },
    deleteRule: async (id: number) => {
        const response = await apiClient.delete(`/automation/rules/${id}`);
        return response.data;
    },
    toggleRule: async (id: number, enabled: boolean) => {
        const response = await apiClient.patch(`/automation/rules/${id}/toggle`, { is_enabled: enabled });
        return response.data;
    },

    // --- [7. 시스템 모니터링 및 로그] ---
    getSystemStatus: async () => {
        const response = await apiClient.get('/admin/system/status');
        return response.data; // { services: { mariadb, mosquitto }, resources: { cpu, ram, disk }, uptime: { start_time } }
    },
    getSystemLogs: async (lines: number = 50) => {
        const response = await apiClient.get(`/admin/system/logs?lines=${lines}`);
        return response.data; // ["line 1", "line 2", ...]
    },
};

export default apiClient;
