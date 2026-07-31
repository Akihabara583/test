import { useState, useEffect } from "react";
import MaskedIcon from "./maskedicon";
import { playerColors, teamEnum } from "../utilities/utilities";

const PlayerCard = ({ playerData, isOnRightSide = false, right = false, emphasizeModel = false }) => {
  const [modelName, setModelName] = useState(playerData.m_model_name);
  const roundedDistance = Number.isFinite(playerData.m_distance_to_local)
    ? Math.round(playerData.m_distance_to_local)
    : null;
  const activeWeapon = playerData?.m_weapons?.m_active;
  const flipped = isOnRightSide || right;

  useEffect(() => {
    if (playerData.m_model_name)
      setModelName(playerData.m_model_name);
  }, [playerData.m_model_name]);

  return (
    <li
      style={{ opacity: `${(playerData.m_is_dead && `0.5`) || `1`}` }}
      className={`flex ${flipped && `flex-row-reverse`} ${emphasizeModel && `rounded-xl bg-radar-panel/45 p-3 border border-radar-secondary/20`}`}
    >
      <div
        className={`flex flex-col gap-[0.375rem] justify-center items-center`}
      >
        <div
          className={`hover:cursor-pointer`}
          onClick={() =>
            window.open(
              `https://steamcommunity.com/profiles/${playerData.m_steam_id}`,
              "_blank",
              "noopener,noreferrer"
            )
          }
        >
          {playerData.m_name}
        </div>
        <div className="text-[11px] text-radar-secondary">
          {playerData.m_is_local
            ? `YOU`
            : playerData.m_is_enemy
              ? `ENEMY`
              : `ALLY`}
          {roundedDistance !== null && ` • ${roundedDistance}u`}
        </div>
        <div
          className={`w-0 h-0 border-solid border-t-[12px] border-r-[8px] border-b-[12px] border-l-[8px]`}
          style={{
            borderColor: `${
              playerColors[playerData.m_color]
            } transparent transparent transparent`,
          }}
        ></div>
        <div
          className={`relative ${flipped && `scale-x-[-1]`}`}
          style={{
            filter: `${(playerData.m_is_enemy && `drop-shadow(0 0 8px rgba(255, 80, 80, 0.65))`) || `drop-shadow(0 0 7px rgba(141, 220, 255, 0.5))`}`,
          }}
        >
          <img
            className={`${emphasizeModel ? `h-[11rem]` : `h-[8rem]`}`}
            src={`./assets/characters/${modelName}.png`}
          ></img>
          <div
            className="absolute inset-0 pointer-events-none"
            style={{
              border: `${playerData.m_is_enemy ? `2px solid rgba(255, 80, 80, 0.9)` : `2px solid rgba(141, 220, 255, 0.85)`}`,
              borderRadius: `0.5rem`,
              boxShadow: `${playerData.m_is_enemy ? `0 0 10px rgba(255, 80, 80, 0.65)` : `0 0 10px rgba(141, 220, 255, 0.55)`}`,
            }}
          />
        </div>
      </div>

      <div
        className={`flex flex-col ${
          flipped && `flex-row-reverse`
        } justify-center gap-2`}
      >
        <span
          className={`${flipped && `flex justify-end`} text-radar-green`}
        >
          ${playerData.m_money}
        </span>

        <div className={`flex ${flipped && `flex-row-reverse`} gap-2`}>
          <div className="flex gap-[4px] items-center">
            <MaskedIcon
              path={`./assets/icons/health.svg`}
              height={16}
              color={`bg-radar-secondary`}
            />
            <span className="text-radar-primary">{playerData.m_health}</span>
          </div>

          <div className="flex gap-[4px] items-center">
            <MaskedIcon
              path={`./assets/icons/${
                (playerData.m_has_helmet && `kevlar_helmet`) || `kevlar`
              }.svg`}
              height={16}
              color={`bg-radar-secondary`}
            />
            <span className="text-radar-primary">{playerData.m_armor}</span>
          </div>

          {activeWeapon && (
            <div
              className="flex gap-[4px] items-center px-2 py-1 rounded-md"
              style={{
                backgroundColor: `rgba(9, 24, 35, 0.75)`,
                boxShadow: `0 0 0 1px rgba(177, 208, 231, 0.25) inset`,
              }}
            >
              <MaskedIcon
                path={`./assets/icons/${activeWeapon}.svg`}
                height={20}
                color={`bg-radar-primary`}
              />
              <span className="text-radar-primary text-[11px] uppercase">
                {activeWeapon}
              </span>
            </div>
          )}
        </div>

        <div className={`flex ${flipped && `flex-row-reverse`} gap-3`}>
          {playerData.m_weapons && playerData.m_weapons.m_primary && (
            <MaskedIcon
              path={`./assets/icons/${playerData.m_weapons.m_primary}.svg`}
              height={28}
              color={`${
                (playerData.m_weapons.m_active ==
                  playerData.m_weapons.m_primary &&
                  `bg-radar-primary`) ||
                `bg-radar-secondary`
              }`}
            />
          )}

          {playerData.m_weapons && playerData.m_weapons.m_secondary && (
            <MaskedIcon
              path={`./assets/icons/${playerData.m_weapons.m_secondary}.svg`}
              height={28}
              color={`${
                (playerData.m_weapons.m_active ==
                  playerData.m_weapons.m_secondary &&
                  `bg-radar-primary`) ||
                `bg-radar-secondary`
              }`}
            />
          )}

          {playerData.m_weapons &&
            playerData.m_weapons.m_melee &&
            playerData.m_weapons.m_melee.map((melee) => (
              <MaskedIcon
                key={melee}
                path={`./assets/icons/${melee}.svg`}
                height={28}
                color={`${
                  (playerData.m_weapons.m_active == melee &&
                    `bg-radar-primary`) ||
                  `bg-radar-secondary`
                }`}
              />
            ))}
        </div>

        <div className={`flex flex-col relative`}>
          <div
            className={`flex ${
              flipped && `flex-row-reverse`
            } gap-9 mt-3 items-center`}
          >
            {playerData.m_weapons &&
              playerData.m_weapons.m_utilities &&
              playerData.m_weapons.m_utilities.map((utility) => (
                <MaskedIcon
                  key={utility}
                  path={`./assets/icons/${utility}.svg`}
                  height={28}
                  color={`${
                    (playerData.m_weapons.m_active == utility &&
                      `bg-radar-primary`) ||
                    `bg-radar-secondary`
                  }`}
                />
              ))}

            {[
              ...Array(
                Math.max(
                  4 -
                    ((playerData.m_weapons &&
                      playerData.m_weapons.m_utilities &&
                      playerData.m_weapons.m_utilities.length) ||
                      0),
                  0
                )
              ),
            ].map((_, i) => (
              <div
                key={i}
                className="rounded-full w-[6px] h-[6px] bg-radar-primary"
              ></div>
            ))}

            {(playerData.m_team == teamEnum.counterTerrorist &&
              playerData.m_has_defuser && (
                <MaskedIcon
                  path={`./assets/icons/defuser.svg`}
                  height={28}
                  color={`bg-radar-secondary`}
                />
              )) ||
              (playerData.m_team == teamEnum.terrorist &&
                playerData.m_has_bomb && (
                  <MaskedIcon
                    path={`./assets/icons/c4.svg`}
                    height={28}
                    color={
                      ((playerData.m_weapons &&
                        playerData.m_weapons.m_active) == `c4` &&
                        `bg-radar-primary`) ||
                      `bg-radar-secondary`
                    }
                  />
                ))}
          </div>
        </div>
      </div>
    </li>
  );
};

export default PlayerCard;
