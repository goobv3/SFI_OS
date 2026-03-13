// ---------------------------------------------------------
// i18n 언어 컨텍스트 (Language Context)
// ---------------------------------------------------------
// 브라우저의 navigator.language 에서 자동으로 언어를 감지합니다.
// 한국어(ko) 가 설정되어 있으면 한국어, 아니면 기본적으로 영어를 사용합니다.
// useLanguage() 훅을 통해 모든 컴포넌트에서 t('key') 로 번역 문자열에 접근할 수 있습니다.
// ---------------------------------------------------------

import React, { createContext, useContext, useState } from 'react';
import translations from './translations';
import type { TranslationKey } from './translations';

type Lang = 'ko' | 'en';

interface LangContextType {
  lang: Lang;
  setLang: (l: Lang) => void;
  t: (key: TranslationKey) => string;
}

const LangContext = createContext<LangContextType>({
  lang: 'ko',
  setLang: () => {},
  t: (key) => key,
});

export function LanguageProvider({ children }: { children: React.ReactNode }) {
  const detectLang = (): Lang => {
    // 1) localStorage에 저장된 설정이 있으면 우선 사용
    const saved = localStorage.getItem('sf_lang') as Lang | null;
    if (saved === 'ko' || saved === 'en') return saved;
    // 2) 브라우저 시스템 언어 감지
    const nav = navigator.language || '';
    return nav.toLowerCase().startsWith('ko') ? 'ko' : 'en';
  };

  const [lang, setLangState] = useState<Lang>(detectLang);

  const setLang = (l: Lang) => {
    setLangState(l);
    localStorage.setItem('sf_lang', l);
  };

  const t = (key: TranslationKey): string => {
    return translations[lang][key] ?? key;
  };

  return (
    <LangContext.Provider value={{ lang, setLang, t }}>
      {children}
    </LangContext.Provider>
  );
}

// 모든 컴포넌트에서 임포트해서 사용하는 훅
export function useLanguage() {
  return useContext(LangContext);
}
