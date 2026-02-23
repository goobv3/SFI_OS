import axios from 'axios';

const API_BASE_URL = 'http://localhost:8000/api';

const apiClient = axios.create({
    baseURL: API_BASE_URL,
    headers: {
        'Content-Type': 'application/json',
    },
});

export const smartFarmApi = {
    // Houses
    getHouses: async () => {
        const response = await apiClient.get('/houses');
        return response.data; // [{ house_id, name, created_at }, ...]
    },
    createHouse: async (houseId: string, name: string) => {
        const response = await apiClient.post('/houses', { house_id: houseId, name });
        return response.data;
    },
    updateHouse: async (houseId: string, name: string) => {
        const response = await apiClient.put(`/houses/${houseId}`, { name });
        return response.data;
    },
    deleteHouse: async (houseId: string) => {
        const response = await apiClient.delete(`/houses/${houseId}`);
        return response.data;
    },

    // Devices
    getHouseDevices: async (houseId: string) => {
        const response = await apiClient.get(`/houses/${houseId}/devices`);
        return response.data; // { sensors: [], actuators: [] }
    },
    createSensor: async (payload: { sensor_id: string, house_id: string, alias: string, type: string, unit: string }) => {
        const response = await apiClient.post('/metadata/sensors', payload);
        return response.data;
    },
    updateSensor: async (sensorId: string, payload: { alias: string, type: string, unit: string }) => {
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
};

export default apiClient;
