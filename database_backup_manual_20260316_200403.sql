/*M!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19-11.8.6-MariaDB, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: smartfarm
-- ------------------------------------------------------
-- Server version	11.8.6-MariaDB-ubu2404

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*M!100616 SET @OLD_NOTE_VERBOSITY=@@NOTE_VERBOSITY, NOTE_VERBOSITY=0 */;

--
-- Table structure for table `actuator_metadata`
--

DROP TABLE IF EXISTS `actuator_metadata`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `actuator_metadata` (
  `actuator_id` varchar(50) NOT NULL,
  `house_id` varchar(50) DEFAULT NULL,
  `alias` varchar(100) DEFAULT NULL,
  `type` varchar(50) DEFAULT NULL,
  `registered_at` datetime DEFAULT current_timestamp(),
  PRIMARY KEY (`actuator_id`),
  KEY `house_id` (`house_id`),
  CONSTRAINT `actuator_metadata_ibfk_1` FOREIGN KEY (`house_id`) REFERENCES `houses` (`house_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `actuator_metadata`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `actuator_metadata` WRITE;
/*!40000 ALTER TABLE `actuator_metadata` DISABLE KEYS */;
INSERT INTO `actuator_metadata` VALUES
('COOLER_1','HOUSE_1','House 1 Cooler','COOLER','2026-03-16 08:10:43'),
('HEATER_1','HOUSE_1','House 1 Heater','HEATER','2026-03-16 08:10:43'),
('ROOF_WINDOW_1','HOUSE_1','House 1 Roof Window','ROOF_WINDOW','2026-03-16 08:10:43');
/*!40000 ALTER TABLE `actuator_metadata` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;

--
-- Table structure for table `actuator_status`
--

DROP TABLE IF EXISTS `actuator_status`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `actuator_status` (
  `actuator_id` varchar(50) NOT NULL,
  `status` varchar(20) NOT NULL,
  `last_updated` datetime DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  `target_value` float DEFAULT NULL,
  `manual_lock` tinyint(1) DEFAULT 0,
  PRIMARY KEY (`actuator_id`),
  CONSTRAINT `actuator_status_ibfk_1` FOREIGN KEY (`actuator_id`) REFERENCES `actuator_metadata` (`actuator_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `actuator_status`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `actuator_status` WRITE;
/*!40000 ALTER TABLE `actuator_status` DISABLE KEYS */;
INSERT INTO `actuator_status` VALUES
('COOLER_1','Off','2026-03-16 08:10:43',NULL,0),
('HEATER_1','Off','2026-03-16 08:10:43',NULL,0),
('ROOF_WINDOW_1','Off','2026-03-16 08:10:43',NULL,0);
/*!40000 ALTER TABLE `actuator_status` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;

--
-- Table structure for table `alarms`
--

DROP TABLE IF EXISTS `alarms`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `alarms` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `sensor_id` varchar(50) NOT NULL,
  `level` varchar(20) NOT NULL,
  `message` text DEFAULT NULL,
  `is_acknowledged` tinyint(1) DEFAULT 0,
  `acknowledged_at` datetime DEFAULT NULL,
  `created_at` datetime DEFAULT current_timestamp(),
  PRIMARY KEY (`id`),
  KEY `sensor_id` (`sensor_id`),
  CONSTRAINT `alarms_ibfk_1` FOREIGN KEY (`sensor_id`) REFERENCES `sensor_metadata` (`sensor_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `alarms`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `alarms` WRITE;
/*!40000 ALTER TABLE `alarms` DISABLE KEYS */;
/*!40000 ALTER TABLE `alarms` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;

--
-- Table structure for table `control_logs`
--

DROP TABLE IF EXISTS `control_logs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `control_logs` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `timestamp` datetime DEFAULT current_timestamp(),
  `actuator_id` varchar(50) NOT NULL,
  `command` varchar(50) DEFAULT NULL,
  `source` varchar(50) DEFAULT NULL,
  `priority` int(11) DEFAULT NULL,
  `result` varchar(20) DEFAULT NULL,
  `message` text DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `actuator_id` (`actuator_id`),
  CONSTRAINT `control_logs_ibfk_1` FOREIGN KEY (`actuator_id`) REFERENCES `actuator_metadata` (`actuator_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `control_logs`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `control_logs` WRITE;
/*!40000 ALTER TABLE `control_logs` DISABLE KEYS */;
/*!40000 ALTER TABLE `control_logs` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;

--
-- Table structure for table `houses`
--

DROP TABLE IF EXISTS `houses`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `houses` (
  `house_id` varchar(50) NOT NULL,
  `name` varchar(100) NOT NULL,
  `display_order` int(11) DEFAULT 0,
  `created_at` datetime DEFAULT current_timestamp(),
  PRIMARY KEY (`house_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `houses`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `houses` WRITE;
/*!40000 ALTER TABLE `houses` DISABLE KEYS */;
INSERT INTO `houses` VALUES
('HOUSE_1','House 1',0,'2026-03-16 08:10:43');
/*!40000 ALTER TABLE `houses` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;

--
-- Table structure for table `sensor_metadata`
--

DROP TABLE IF EXISTS `sensor_metadata`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `sensor_metadata` (
  `sensor_id` varchar(50) NOT NULL,
  `house_id` varchar(50) DEFAULT NULL,
  `alias` varchar(100) DEFAULT NULL,
  `type` varchar(50) DEFAULT NULL,
  `unit` varchar(10) DEFAULT NULL,
  `calibration_offset` float DEFAULT 0,
  `display_order` int(11) DEFAULT 0,
  `is_active` tinyint(1) DEFAULT 1,
  `warn_high` float DEFAULT NULL,
  `warn_low` float DEFAULT NULL,
  `crit_high` float DEFAULT NULL,
  `crit_low` float DEFAULT NULL,
  `registered_at` datetime DEFAULT current_timestamp(),
  PRIMARY KEY (`sensor_id`),
  KEY `house_id` (`house_id`),
  CONSTRAINT `sensor_metadata_ibfk_1` FOREIGN KEY (`house_id`) REFERENCES `houses` (`house_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sensor_metadata`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `sensor_metadata` WRITE;
/*!40000 ALTER TABLE `sensor_metadata` DISABLE KEYS */;
INSERT INTO `sensor_metadata` VALUES
('HUM_01','HOUSE_1','House 1 Hum','humidity','%',0,0,1,NULL,NULL,NULL,NULL,'2026-03-16 08:10:43'),
('TEMP_01','HOUSE_1','House 1 Temp','temperature','C',0,0,1,NULL,NULL,NULL,NULL,'2026-03-16 08:10:43');
/*!40000 ALTER TABLE `sensor_metadata` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;

--
-- Table structure for table `sensors`
--

DROP TABLE IF EXISTS `sensors`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `sensors` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `timestamp` datetime DEFAULT current_timestamp(),
  `sensor_id` varchar(50) NOT NULL,
  `value` float NOT NULL,
  PRIMARY KEY (`id`),
  KEY `sensor_id` (`sensor_id`),
  CONSTRAINT `sensors_ibfk_1` FOREIGN KEY (`sensor_id`) REFERENCES `sensor_metadata` (`sensor_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `sensors`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `sensors` WRITE;
/*!40000 ALTER TABLE `sensors` DISABLE KEYS */;
/*!40000 ALTER TABLE `sensors` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;

--
-- Table structure for table `unregistered_devices`
--

DROP TABLE IF EXISTS `unregistered_devices`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `unregistered_devices` (
  `device_id` varchar(50) NOT NULL,
  `device_type` varchar(20) DEFAULT 'sensor',
  `first_seen` timestamp NULL DEFAULT current_timestamp(),
  `last_seen` timestamp NULL DEFAULT current_timestamp() ON UPDATE current_timestamp(),
  `last_value` float DEFAULT NULL,
  PRIMARY KEY (`device_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `unregistered_devices`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `unregistered_devices` WRITE;
/*!40000 ALTER TABLE `unregistered_devices` DISABLE KEYS */;
/*!40000 ALTER TABLE `unregistered_devices` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;

--
-- Table structure for table `weather_data`
--

DROP TABLE IF EXISTS `weather_data`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `weather_data` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `timestamp` datetime DEFAULT current_timestamp(),
  `source` varchar(20) DEFAULT 'KMA',
  `forecast_offset` int(11) DEFAULT 0,
  `wind_speed` float DEFAULT NULL,
  `wind_direction` varchar(10) DEFAULT NULL,
  `rainfall` float DEFAULT NULL,
  `solar_radiation` float DEFAULT NULL,
  `temperature` float DEFAULT NULL,
  `humidity` float DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `weather_data`
--

SET @OLD_AUTOCOMMIT=@@AUTOCOMMIT, @@AUTOCOMMIT=0;
LOCK TABLES `weather_data` WRITE;
/*!40000 ALTER TABLE `weather_data` DISABLE KEYS */;
/*!40000 ALTER TABLE `weather_data` ENABLE KEYS */;
UNLOCK TABLES;
COMMIT;
SET AUTOCOMMIT=@OLD_AUTOCOMMIT;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*M!100616 SET NOTE_VERBOSITY=@OLD_NOTE_VERBOSITY */;

-- Dump completed on 2026-03-16 11:04:05
