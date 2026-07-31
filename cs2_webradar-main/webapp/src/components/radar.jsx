import Player from "./player";
import Bomb from "./bomb";
import { useRef } from "react";

const DefuseProgress = ({ bombData }) => {
  const remaining = Math.max(0, Number(bombData?.m_defuse_time) || 0);
  const isDefusing = Boolean(bombData?.m_is_defusing && !bombData.m_is_defused);
  const durationRef = useRef(5);
  const wasDefusingRef = useRef(false);

  if (isDefusing && !wasDefusingRef.current)
    durationRef.current = remaining > 5 ? 10 : 5;

  wasDefusingRef.current = isDefusing;

  if (!isDefusing)
    return null;

  const duration = durationRef.current;
  const progress = Math.min(1, remaining / duration);
  const color = `hsl(${progress * 120} 78% 45%)`;

  return (
    <div
      className="defuse-progress"
      role="progressbar"
      aria-label="Bomb defuse progress"
      aria-valuemin="0"
      aria-valuemax={duration}
      aria-valuenow={remaining}
    >
      <div
        className="defuse-progress__fill"
        style={{
          width: `${progress * 100}%`,
          backgroundColor: color,
        }}
      />
    </div>
  );
};

const Radar = ({
  playerArray,
  radarImage,
  mapData,
  localTeam,
  bombData,
  settings
}) => {
  return (
    <div
      id="radar"
      className="relative origin-center"
      style={{
        transform: `scale(${settings.radarSize})`,
      }}
    >
      <DefuseProgress bombData={bombData} />

      <div className="radar-content">
        <div
          className="radar-world"
          style={{ transform: `rotate(${settings.mapRotation}deg)` }}
        >
          <img
            className="radar-image"
            src={radarImage}
            alt=""
          />

          {playerArray.map((player) => (
            <Player
              key={player.m_idx}
              playerData={player}
              mapData={mapData}
              localTeam={localTeam}
              settings={settings}
            />
          ))}

          {bombData && (
            <Bomb
              bombData={bombData}
              mapData={mapData}
              localTeam={localTeam}
              settings={settings}
            />
          )}
        </div>
      </div>
    </div>
  );
};

export default Radar;
