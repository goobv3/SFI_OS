CREATE DATABASE IF NOT EXISTS smartfarm;
USE smartfarm;

-- 0. HOUSE MANAGEMENT
CREATE TABLE IF NOT EXISTS houses (
    house_id VARCHAR(50) PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    display_order INT DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 1. SENSOR MANAGEMENT
CREATE TABLE IF NOT EXISTS sensor_metadata (
    sensor_id VARCHAR(50) PRIMARY KEY,
    house_id VARCHAR(50),
    alias VARCHAR(100), 
    type VARCHAR(50),   
    unit VARCHAR(10),
    calibration_offset FLOAT DEFAULT 0.0,
    display_order INT DEFAULT 0,
    is_active BOOLEAN DEFAULT TRUE,
    warn_high FLOAT NULL,
    warn_low FLOAT NULL,
    crit_high FLOAT NULL,
    crit_low FLOAT NULL,
    registered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (house_id) REFERENCES houses(house_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS sensors (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    sensor_id VARCHAR(50) NOT NULL,
    value FLOAT NOT NULL,
    FOREIGN KEY (sensor_id) REFERENCES sensor_metadata(sensor_id) ON DELETE CASCADE
);

-- 2. WEATHER STATION (Global, Not tied to specific house)
CREATE TABLE IF NOT EXISTS weather_data (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    source VARCHAR(20) DEFAULT 'KMA',
    forecast_offset INT DEFAULT 0,
    wind_speed FLOAT,
    wind_direction VARCHAR(10),
    rainfall FLOAT,
    solar_radiation FLOAT,
    temperature FLOAT,
    humidity FLOAT
);

-- 3. ALARMS MANAGEMENT
CREATE TABLE IF NOT EXISTS alarms (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    sensor_id VARCHAR(50) NOT NULL,
    level VARCHAR(20) NOT NULL,
    message TEXT,
    is_acknowledged BOOLEAN DEFAULT FALSE,
    acknowledged_at DATETIME NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (sensor_id) REFERENCES sensor_metadata(sensor_id) ON DELETE CASCADE
);

-- INBOX for unknown devices (Auto-Discovery feature)
CREATE TABLE IF NOT EXISTS unregistered_devices (
    device_id VARCHAR(50) PRIMARY KEY,
    device_type VARCHAR(20) DEFAULT 'sensor',
    first_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    last_value FLOAT
);

-- 3. ACTUATOR & DEVICE MANAGEMENT
CREATE TABLE IF NOT EXISTS actuator_metadata (
    actuator_id VARCHAR(50) PRIMARY KEY,
    house_id VARCHAR(50),
    alias VARCHAR(100),
    type VARCHAR(50), 
    registered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (house_id) REFERENCES houses(house_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS actuator_status (
    actuator_id VARCHAR(50) PRIMARY KEY,
    status VARCHAR(20) NOT NULL, 
    last_updated DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    target_value FLOAT, 
    manual_lock BOOLEAN DEFAULT FALSE,
    FOREIGN KEY (actuator_id) REFERENCES actuator_metadata(actuator_id) ON DELETE CASCADE
);

-- 4. CONTROL LOGS & FEEDBACK
CREATE TABLE IF NOT EXISTS control_logs (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    actuator_id VARCHAR(50) NOT NULL,
    command VARCHAR(50),
    source VARCHAR(50), 
    priority INT,       
    result VARCHAR(20), 
    message TEXT,       
    FOREIGN KEY (actuator_id) REFERENCES actuator_metadata(actuator_id) ON DELETE CASCADE
);

-- 5. INITIAL DUMMY DATA FOR TESTING
-- Insert House
INSERT IGNORE INTO houses (house_id, name) VALUES ('HOUSE_1', 'House 1');

-- Insert Sensors linked to HOUSE_1
INSERT IGNORE INTO sensor_metadata (sensor_id, house_id, alias, type, unit) VALUES
('TEMP_01', 'HOUSE_1', 'House 1 Temp', 'temperature', 'C'),
('HUM_01', 'HOUSE_1', 'House 1 Hum', 'humidity', '%');

-- Insert Actuators linked to HOUSE_1
INSERT IGNORE INTO actuator_metadata (actuator_id, house_id, alias, type) VALUES
('ROOF_WINDOW_1', 'HOUSE_1', 'House 1 Roof Window', 'ROOF_WINDOW'),
('HEATER_1', 'HOUSE_1', 'House 1 Heater', 'HEATER'),
('COOLER_1', 'HOUSE_1', 'House 1 Cooler', 'COOLER');

-- Init Actuator Status
INSERT IGNORE INTO actuator_status (actuator_id, status) VALUES
('ROOF_WINDOW_1', 'Off'),
('HEATER_1', 'Off'),
('COOLER_1', 'Off');
