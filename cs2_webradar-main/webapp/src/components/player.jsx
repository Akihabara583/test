import { useRef } from "react";
import { getRadarPosition, playerColors } from "../utilities/utilities";


let playerRotations = [];
const calculatePlayerRotation = (playerData) => {
  const playerViewAngle = 270 - playerData.m_eye_angle;
  const idx = playerData.m_idx;

  playerRotations[idx] = (playerRotations[idx] || 0) % 360;
  playerRotations[idx] +=
    ((playerViewAngle - playerRotations[idx] + 540) % 360) - 180;

  return playerRotations[idx];
};

const Player = ({ playerData, mapData, localTeam, settings }) => {
  const lastKnownPosition = useRef(null);
  const radarPosition = getRadarPosition(mapData, playerData.m_position) || { x: 0, y: 0 };
  const invalidPosition = radarPosition.x <= 0 && radarPosition.y <= 0;
  const playerRotation = calculatePlayerRotation(playerData);
  const isLocalPlayer = Boolean(playerData.m_is_local);
  const isEnemy = Boolean(playerData.m_is_enemy);
  const baseColor = playerData.m_team == localTeam ? playerColors[playerData.m_color] : `#ff4f4f`;

  // Keep markers proportional to the radar itself, including browser zoom.
  const scaledSize = (isLocalPlayer ? 2.1 : 1.6) * settings.dotSize;

  if (playerData.m_is_dead && !lastKnownPosition.current)
    lastKnownPosition.current = radarPosition;
  else if (!playerData.m_is_dead)
    lastKnownPosition.current = null;

  const effectivePosition = playerData.m_is_dead
    ? lastKnownPosition.current || { x: 0, y: 0 }
    : radarPosition;

  return (
    <div
      className={`absolute origin-center rounded-[100%] left-0 top-0`}
      style={{
        width: `${scaledSize}%`,
        height: `${scaledSize}%`,
        left: `${effectivePosition.x * 100}%`,
        top: `${effectivePosition.y * 100}%`,
        transform: `translate(-50%, -50%)`,
        transition: `left 100ms linear, top 100ms linear`,
        zIndex: `${(playerData.m_is_dead && `0`) || `1`}`,
        WebkitMask: `${(playerData.m_is_dead && `url('./assets/icons/icon-enemy-death_png.png') no-repeat center / contain`) || `none`}`,
      }}
    >
      {/* Rotating container for player elements */}
      <div
        style={{
          transform: `rotate(${(playerData.m_is_dead && `0`) || playerRotation}deg)`,
          width: `100%`,
          height: `100%`,
          transition: `transform 100ms linear`,
          opacity: `${(playerData.m_is_dead && `0.8`) || (invalidPosition && `0`) || `1`}`,
          filter: `${(isEnemy && !playerData.m_is_dead && `drop-shadow(0 0 5px rgba(255, 79, 79, 0.85))`) || `none`}`,
        }}
      >
        {/* Rounded player marker with clean outline */}
        <div
          className={`relative w-full h-full rounded-full`}
          style={{
            backgroundColor: `${baseColor}`,
            opacity: `${(playerData.m_is_dead && `0.8`) || (invalidPosition && `0`) || `1`}`,
            boxShadow: `${
              (isLocalPlayer && `0 0 0 2px rgba(141, 220, 255, 0.95), 0 0 10px rgba(141, 220, 255, 0.7)`) ||
              (isEnemy && `0 0 0 1.5px rgba(255, 120, 120, 0.95), 0 0 9px rgba(255, 79, 79, 0.65)`) ||
              `0 0 0 1px rgba(177, 208, 231, 0.8)`
            }`,
          }}
        >
          {!playerData.m_is_dead && (
            <>
              <div
                className="absolute rounded-full"
                style={{
                  left: `50%`,
                  top: `50%`,
                  width: `34%`,
                  height: `34%`,
                  transform: `translate(-50%, -50%)`,
                  backgroundColor: `rgba(7, 20, 30, 0.45)`,
                }}
              />
              <div
                className="absolute"
                style={{
                  left: `50%`,
                  top: `-14%`,
                  width: `0`,
                  height: `0`,
                  transform: `translateX(-50%)`,
                  borderLeft: `0.2rem solid transparent`,
                  borderRight: `0.2rem solid transparent`,
                  borderBottom: `0.33rem solid ${baseColor}`,
                  filter: `drop-shadow(0 0 2px rgba(7, 20, 30, 0.8))`,
                }}
              />
            </>
          )}
        </div>
      </div>
    </div>
  );
};

export default Player;
