import { getRadarPosition, teamEnum } from "../utilities/utilities";

const Bomb = ({ bombData, mapData, localTeam, settings }) => {
  const radarPosition = getRadarPosition(mapData, bombData);

  // Calculate bomb size based on settings
  // Percentage of the radar, so the bomb scales together with the map.
  const baseSize = 2.8;
  const scaledSize = baseSize * settings.bombSize;

  return (
    <div
      className={`absolute origin-center rounded-[100%] left-0 top-0`}
      style={{
        width: `${scaledSize}%`,
        height: `${scaledSize}%`,
        left: `${radarPosition.x * 100}%`,
        top: `${radarPosition.y * 100}%`,
        transform: `translate(-50%, -50%)`,
        backgroundColor: `${
          (bombData.m_is_defused && `#50904c`) ||
          (localTeam == teamEnum.counterTerrorist && `#6492b4`) ||
          `#c90b0b`
        }`,
        WebkitMask: `url('./assets/icons/c4_sml.png') no-repeat center / contain`,
        opacity: `1`,
        zIndex: `1`,
      }}
    />
  );
};

export default Bomb;
