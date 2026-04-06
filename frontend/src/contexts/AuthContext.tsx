import React, { createContext, useContext, useState, useEffect } from 'react';
import apiClient from '../api/client';

interface User {
    username: string;
    role: string;
}

interface AuthContextType {
    isAuthenticated: boolean;
    user: User | null;
    login: (token: string, username: string, role: string) => void;
    logout: () => void;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);

export const AuthProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
    const [user, setUser] = useState<User | null>(null);
    const [isAuthenticated, setIsAuthenticated] = useState<boolean>(false);
    const [isLoading, setIsLoading] = useState(true);

    useEffect(() => {
        // 앱 초기 로드 시 토큰 확인 및 검증
        const token = localStorage.getItem('access_token');
        if (token) {
            // me 엔드포인트를 통해 토큰 유효성 및 사용자 정보 확인
            apiClient.get('/auth/me')
                .then(res => {
                    setUser({ username: res.data.username, role: res.data.role });
                    setIsAuthenticated(true);
                })
                .catch(() => {
                    localStorage.removeItem('access_token');
                    setUser(null);
                    setIsAuthenticated(false);
                })
                .finally(() => {
                    setIsLoading(false);
                });
        } else {
            setIsLoading(false);
        }
    }, []);

    const login = (token: string, username: string, role: string) => {
        localStorage.setItem('access_token', token);
        setUser({ username, role });
        setIsAuthenticated(true);
    };

    const logout = () => {
        localStorage.removeItem('access_token');
        setUser(null);
        setIsAuthenticated(false);
    };

    // 로딩 중일 때는 아무것도 렌더링하지 않거나, 로딩 스피너를 표시할 수 있습니다.
    if (isLoading) {
        return <div className="flex h-screen items-center justify-center bg-cyber-bg text-neon-blue font-bold">LOADING OS...</div>;
    }

    return (
        <AuthContext.Provider value={{ isAuthenticated, user, login, logout }}>
            {children}
        </AuthContext.Provider>
    );
};

export const useAuth = () => {
    const context = useContext(AuthContext);
    if (!context) {
        throw new Error('useAuth must be used within an AuthProvider');
    }
    return context;
};
