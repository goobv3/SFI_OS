import React, { useState } from 'react';
import axios from 'axios';
import { useAuth } from '../contexts/AuthContext';
import { Activity, Lock, User, AlertCircle } from 'lucide-react';

const LoginPage: React.FC = () => {
    const [username, setUsername] = useState('');
    const [password, setPassword] = useState('');
    const [error, setError] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const { login } = useAuth();

    const handleSubmit = async (e: React.FormEvent) => {
        e.preventDefault();
        setError('');
        setIsLoading(true);

        try {
            // axios를 직접 사용하여 로그인 요청 (interceptor 순환 참조 방지 및 에러 핸들링 용이)
            const response = await axios.post('/api/auth/login', {
                username,
                password
            });
            const { access_token, username: resUser, role } = response.data;
            login(access_token, resUser, role);
        } catch (err: any) {
            if (err.response && err.response.data && err.response.data.error) {
                setError(err.response.data.error);
            } else {
                setError('Login failed. Please check your connection.');
            }
        } finally {
            setIsLoading(false);
        }
    };

    return (
        <div className="flex min-h-screen flex-col items-center justify-center bg-cyber-bg font-sans text-gray-200">
            <div className="w-full max-w-sm animate-fade-in-up">
                
                {/* 로고 영역 */}
                <div className="mb-8 flex flex-col items-center">
                    <div className="mb-4 flex h-16 w-16 items-center justify-center rounded-2xl bg-surface-elevated border border-cyber-border/30 shadow-[0_0_15px_rgba(102,252,241,0.2)]">
                        <Activity className="h-8 w-8 text-neon-blue animate-pulse" />
                    </div>
                    <h1 className="text-2xl font-bold tracking-widest text-white">
                        SMART FARM <span className="font-light text-neon-blue glow-text-blue">OS</span>
                    </h1>
                    <p className="mt-2 text-xs uppercase tracking-widest text-gray-500">
                        System Authentication Required
                    </p>
                </div>

                {/* 로그인 폼 */}
                <form onSubmit={handleSubmit} className="overflow-hidden rounded-2xl border border-cyber-border/20 bg-surface-card p-6 shadow-2xl backdrop-blur-sm">
                    {error && (
                        <div className="mb-4 flex items-center gap-2 rounded-lg border border-red-500/50 bg-red-500/10 p-3 text-xs text-red-400">
                            <AlertCircle className="h-4 w-4 shrink-0" />
                            <span>{error}</span>
                        </div>
                    )}

                    <div className="mb-4">
                        <label className="mb-1.5 block text-xs font-semibold text-gray-400 uppercase tracking-wider">
                            Username
                        </label>
                        <div className="relative">
                            <div className="absolute inset-y-0 left-0 flex items-center pl-3 pointer-events-none">
                                <User className="h-4 w-4 text-gray-500" />
                            </div>
                            <input
                                type="text"
                                value={username}
                                onChange={(e) => setUsername(e.target.value)}
                                className="block w-full rounded-lg border border-gray-700 bg-cyber-surface/50 p-2.5 pl-10 text-sm text-white placeholder-gray-500 focus:border-neon-blue focus:outline-none focus:ring-1 focus:ring-neon-blue transition"
                                placeholder="Enter admin username"
                                required
                            />
                        </div>
                    </div>

                    <div className="mb-6">
                        <label className="mb-1.5 block text-xs font-semibold text-gray-400 uppercase tracking-wider">
                            Password
                        </label>
                        <div className="relative">
                            <div className="absolute inset-y-0 left-0 flex items-center pl-3 pointer-events-none">
                                <Lock className="h-4 w-4 text-gray-500" />
                            </div>
                            <input
                                type="password"
                                value={password}
                                onChange={(e) => setPassword(e.target.value)}
                                className="block w-full rounded-lg border border-gray-700 bg-cyber-surface/50 p-2.5 pl-10 text-sm text-white placeholder-gray-500 focus:border-neon-teal focus:outline-none focus:ring-1 focus:ring-neon-teal transition"
                                placeholder="••••••••"
                                required
                            />
                        </div>
                    </div>

                    <button
                        type="submit"
                        disabled={isLoading}
                        className="w-full rounded-lg bg-neon-blue/20 border border-neon-blue/50 p-2.5 text-sm font-bold text-neon-blue hover:bg-neon-blue/30 hover:shadow-[0_0_15px_rgba(102,252,241,0.4)] disabled:opacity-50 disabled:cursor-not-allowed transition-all duration-300"
                    >
                        {isLoading ? 'AUTHENTICATING...' : 'ACCESS GRANTED'}
                    </button>
                </form>

                <div className="mt-8 text-center text-[10px] text-gray-600">
                    <p>© 2026 Antigravity. All rights reserved.</p>
                </div>
            </div>
        </div>
    );
};

export default LoginPage;
