import { useState } from "react";

const SettingsButton = ({ settings, onSettingsChange }) => {
  const [isOpen, setIsOpen] = useState(false);

  return (
    <div className="z-50">
      <button
        onClick={() => setIsOpen(!isOpen)}
        className="flex items-center gap-1 transition-all rounded-xl"
      >
        <img className={`w-[1.3rem]`} src={`./assets/icons/cog.svg`} />
        <span className="text-radar-primary">Settings</span>
      </button>

      {isOpen && (
        <div className="absolute right-0 mt-2 w-64 bg-radar-panel/90 backdrop-blur-lg rounded-xl p-4 shadow-xl border border-radar-secondary/20">
          <h3 className="text-radar-primary text-lg font-semibold mb-4">Radar Settings</h3>

          <div className="space-y-3">
            <div>
              <span className="text-radar-secondary text-sm">View mode</span>
              <div className="mt-2 grid grid-cols-3 gap-2">
                {[
                  { id: "radar", label: "Radar" },
                  { id: "split", label: "Split" },
                  { id: "models", label: "Models" },
                ].map((mode) => (
                  <button
                    key={mode.id}
                    type="button"
                    onClick={() => onSettingsChange({ ...settings, viewMode: mode.id })}
                    className={`rounded-lg px-2 py-1.5 text-xs transition-colors ${
                      settings.viewMode === mode.id
                        ? `bg-radar-primary/20 text-radar-primary`
                        : `bg-radar-secondary/15 text-radar-secondary hover:bg-radar-secondary/30`
                    }`}
                  >
                    {mode.label}
                  </button>
                ))}
              </div>
            </div>

            <div>
              <div className="flex justify-between items-center mb-2">
                <span className="text-radar-secondary text-sm">Map rotation</span>
                <span className="text-radar-primary text-sm font-mono">
                  {((settings.mapRotation % 360) + 360) % 360}°
                </span>
              </div>
              <button
                type="button"
                onClick={() => onSettingsChange({
                  ...settings,
                  mapRotation: settings.mapRotation + 90,
                })}
                className="w-full rounded-lg bg-radar-secondary/20 px-3 py-2 text-sm text-radar-primary transition-colors hover:bg-radar-secondary/35"
              >
                Rotate map 90°
              </button>
            </div>

            <div>
              <div className="flex justify-between items-center mb-2">
                <span className="text-radar-secondary text-sm">Radar size</span>
                <span className="text-radar-primary text-sm font-mono">{settings.radarSize.toFixed(2)}x</span>
              </div>
              <input
                type="range"
                min="0.75"
                max="1.5"
                step="0.05"
                value={settings.radarSize}
                onChange={(e) => onSettingsChange({ ...settings, radarSize: parseFloat(e.target.value) })}
                className="w-full h-2 rounded-lg appearance-none cursor-pointer accent-radar-primary"
                style={{
                  background: `linear-gradient(to right, #b1d0e7 ${((settings.radarSize - 0.75) / 0.75) * 100}%, rgba(59, 130, 246, 0.2) ${((settings.radarSize - 0.75) / 0.75) * 100}%)`
                }}
              />
            </div>

            <div>
              <div className="flex justify-between items-center mb-2">
                <span className="text-radar-secondary text-sm">Player size</span>
                <span className="text-radar-primary text-sm font-mono">{settings.dotSize}x</span>
              </div>
              <input
                type="range"
                min="1"
                max="4"
                step="0.1"
                value={settings.dotSize}
                onChange={(e) => onSettingsChange({ ...settings, dotSize: parseFloat(e.target.value) })}
                className="w-full h-2 rounded-lg appearance-none cursor-pointer accent-radar-primary"
                style={{
                  background: `linear-gradient(to right, #b1d0e7 ${((settings.dotSize - 1) / 3) * 100}%, rgba(59, 130, 246, 0.2) ${((settings.dotSize - 1) / 3) * 100}%)`
                }}
              />
            </div>

            <div>
              <div className="flex justify-between items-center mb-2">
                <span className="text-radar-secondary text-sm">Bomb size</span>
                <span className="text-radar-primary text-sm font-mono">{settings.bombSize}x</span>
              </div>
              <input
                type="range"
                min="0.5"
                max="2"
                step="0.1"
                value={settings.bombSize}
                onChange={(e) => onSettingsChange({ ...settings, bombSize: parseFloat(e.target.value) })}
                className="w-full h-2 rounded-lg appearance-none cursor-pointer accent-radar-primary"
                style={{
                  background: `linear-gradient(to right, #b1d0e7 ${((settings.bombSize - 0.5) / 1.5) * 100}%, rgba(59, 130, 246, 0.2) ${((settings.bombSize - 0.5) / 1.5) * 100}%)`
                }}
              />
            </div>

          </div>
        </div>
      )}
    </div>
  );
};

export default SettingsButton;
