// ---------------------------------------------------------
// i18n 번역 파일 (Translation Definitions)
// ---------------------------------------------------------
// 브라우저의 언어 설정에 따라 영어(en) 또는 한국어(ko)로 자동 전환됩니다.
// 새로운 텍스트를 추가할 때는 두 언어 모두 동시에 추가해 주세요.
// ---------------------------------------------------------

export type TranslationKey = keyof typeof translations.ko;

const translations = {
  ko: {
    // === 헤더 ===
    systemOnline:       '시스템 정상',
    settings:           '설정',

    // === 사이드바 ===
    locations:          '재배동 목록',

    // === 메인 대시보드 ===
    dashboardSubtitle:  '실시간 환경 모니터링 및 제어',
    envStatus:          '환경 현황 (클릭 시 이력 확인)',
    manualControls:     '수동 제어',
    noLocationSelected: '재배동이 선택되지 않았습니다.',
    noLocationHint:     '설정(⚙️) 패널에서 새로운 재배동을 등록해 주세요.',

    // === 자동 감지 배너 ===
    devicesDetected:    '개의 새 기기가 감지되었습니다!',
    clickToRegister:    '클릭하여 현재 재배동에 1-Click 등록하세요.',

    // === 센서 대시보드 ===
    loadingSensors:     '센서 데이터 로딩 중...',
    noSensors:          '이 재배동에 등록된 센서가 없습니다.',
    paused:             '일시정지',

    // === 액추에이터 제어 ===
    loadingControls:    '제어 장치 로딩 중...',
    noActuators:        '이 재배동에 등록된 제어 장치가 없습니다.',
    auto:               '자동',
    manual:             '수동',
    running:            '가동 중',
    stopped:            '정지',
    locked:             '잠금',

    // === 날씨 위젯 ===
    kmaWeather:         '기상청 날씨',
    farmWeather:        '자체 기상대',
    noData:             '데이터 없음',
    currentNow:         '현재',
    forecast1h:         '1시간 후 예보',
    forecast3h:         '3시간 후 예보',
    forecast6h:         '6시간 후 예보',

    // === 알람 토스트 ===
    danger:             '위험',
    warning:            '경고',
    alarmNotice:        '알림',
    acknowledge:        '확인',

    // === 센서 설정 모달 ===
    sensorConfig:       '센서 설정',
    alias:              '표시 이름',
    type:               '유형',
    unit:               '단위',
    displayOrder:       '표시 순서',
    active:             '활성',
    warnHigh:           '경고 상한',
    warnLow:            '경고 하한',
    critHigh:           '위험 상한',
    critLow:            '위험 하한',
    save:               '저장',
    cancel:             '취소',
    saving:             '저장 중...',
    failedToSave:       '설정 저장에 실패했습니다.',

    // === 퀵 등록 모달 ===
    quickAddTitle:      '새 기기 일괄 등록',
    quickAddDesc:       '아래 기기들을 현재 재배동에 등록합니다. 값을 수정한 후 등록하세요.',
    sensorType:         '센서 유형',
    sensorAlias:        '이름(별칭)',
    registerAll:        '전체 등록',
    registering:        '등록 중...',
    manualEntry:        '직접 입력',
    firstSeen:          '최초 감지',

    // === 설정 모달 (ManageMode) ===
    manageTitle:        '시스템 설정',
    houseManagement:    '재배동 관리',
    deviceDiscovery:    '기기 자동 감지',
    addHouse:           '재배동 추가',
    houseName:          '재배동 이름',
    scanDevices:        '기기 스캔',
    scanning:           '스캔 중...',
    stopScan:           '스캔 중지',
    close:              '닫기',
    delete:             '삭제',
    edit:               '수정',
    sensors:            '센서',
    actuators:          '제어 장치',
    noHouses:           '등록된 재배동이 없습니다.',
    confirmDelete:      '정말 삭제하시겠습니까?',
  },

  en: {
    // === Header ===
    systemOnline:       'System Online',
    settings:           'Settings',

    // === Sidebar ===
    locations:          'Locations',

    // === Main Dashboard ===
    dashboardSubtitle:  'Real-time environment monitoring and control.',
    envStatus:          'Environment Status (Click for History)',
    manualControls:     'Manual Controls',
    noLocationSelected: 'No location selected.',
    noLocationHint:     'Click the Settings (⚙️) panel to register a new House.',

    // === Discovery Banner ===
    devicesDetected:    'new device(s) detected!',
    clickToRegister:    'Click to 1-Click register to current house.',

    // === Sensor Dashboard ===
    loadingSensors:     'Loading sensors...',
    noSensors:          'No sensors registered in this house.',
    paused:             'Paused',

    // === Actuator Control ===
    loadingControls:    'Loading controls...',
    noActuators:        'No actuators registered in this house.',
    auto:               'AUTO',
    manual:             'MANUAL',
    running:            'Running',
    stopped:            'Stopped',
    locked:             'LOCKED',

    // === Weather Widget ===
    kmaWeather:         'KMA Weather',
    farmWeather:        'Farm Station',
    noData:             'No Data Available',
    currentNow:         'Current (Now)',
    forecast1h:         '1h Forecast',
    forecast3h:         '3h Forecast',
    forecast6h:         '6h Forecast',

    // === Alarm Toast ===
    danger:             'CRITICAL',
    warning:            'WARNING',
    alarmNotice:        'ALERT',
    acknowledge:        'OK',

    // === Sensor Config Modal ===
    sensorConfig:       'Sensor Configuration',
    alias:              'Display Name',
    type:               'Type',
    unit:               'Unit',
    displayOrder:       'Display Order',
    active:             'Active',
    warnHigh:           'Warn High',
    warnLow:            'Warn Low',
    critHigh:           'Crit High',
    critLow:            'Crit Low',
    save:               'Save',
    cancel:             'Cancel',
    saving:             'Saving...',
    failedToSave:       'Failed to save configuration.',

    // === Quick Add Modal ===
    quickAddTitle:      'Register New Devices',
    quickAddDesc:       'Register detected devices to the current house. Review and edit before registering.',
    sensorType:         'Sensor Type',
    sensorAlias:        'Name (Alias)',
    registerAll:        'Register All',
    registering:        'Registering...',
    manualEntry:        'Manual Entry',
    firstSeen:          'First Seen',

    // === Manage Mode Modal ===
    manageTitle:        'System Settings',
    houseManagement:    'House Management',
    deviceDiscovery:    'Device Discovery',
    addHouse:           'Add House',
    houseName:          'House Name',
    scanDevices:        'Scan for Devices',
    scanning:           'Scanning...',
    stopScan:           'Stop Scan',
    close:              'Close',
    delete:             'Delete',
    edit:               'Edit',
    sensors:            'Sensors',
    actuators:          'Actuators',
    noHouses:           'No houses registered.',
    confirmDelete:      'Are you sure you want to delete?',
  }
};

export default translations;
