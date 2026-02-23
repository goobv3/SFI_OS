import { Home, ChevronRight } from 'lucide-react';
import clsx from 'clsx';

interface HouseSelectorProps {
    houses: string[];
    selectedHouse: string;
    onSelectHouse: (house: string) => void;
}

export default function HouseSelector({ houses, selectedHouse, onSelectHouse }: HouseSelectorProps) {
    return (
        <div className="bg-cyber-surface/80 rounded-xl border border-cyber-border/30 overflow-hidden shadow-lg backdrop-blur-sm">
            <div className="p-4 border-b border-cyber-border/30 bg-black/20">
                <h2 className="text-sm font-bold text-gray-400 uppercase tracking-widest flex items-center gap-2">
                    <Home className="w-4 h-4 text-neon-blue" />
                    Locations
                </h2>
            </div>
            <nav className="p-2 space-y-1">
                {houses.map((house) => {
                    const isSelected = house === selectedHouse;
                    return (
                        <button
                            key={house}
                            onClick={() => onSelectHouse(house)}
                            className={clsx(
                                "w-full flex items-center justify-between px-4 py-3 rounded-lg text-left transition-all duration-300",
                                isSelected
                                    ? "bg-neon-blue/10 border border-neon-blue/50 text-white shadow-[0_0_15px_rgba(102,252,241,0.15)]"
                                    : "text-gray-400 hover:bg-white/5 hover:text-gray-200 border border-transparent"
                            )}
                        >
                            <span className="font-medium tracking-wide">{house}</span>
                            {isSelected && <ChevronRight className="w-4 h-4 text-neon-blue animate-pulse" />}
                        </button>
                    );
                })}
            </nav>
        </div>
    );
}
