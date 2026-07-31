import ReactDOM from "react-dom/client";
import { useEffect, useRef, useState } from "react";
import "./App.css";
import PlayerCard from "./components/PlayerCard";
import Radar from "./components/Radar";
import SettingsButton from "./components/settings";
import MaskedIcon from "./components/maskedicon";

const CONNECTION_TIMEOUT = 5000;
const ENTITY_GRACE_PERIOD = 700;

/* change this to '1' if you want to use offline (your own pc only) */
const USE_LOCALHOST = 0;

/* you can get your public ip from https://ipinfo.io/ip */
const PUBLIC_IP = "your ip goes here".trim();
const PORT = 22006;

const EFFECTIVE_IP = USE_LOCALHOST ? "localhost" : PUBLIC_IP.match(/[a-zA-Z]/) ? window.location.hostname : PUBLIC_IP;

const DEFAULT_SETTINGS = {
  dotSize: 1.5,
  bombSize: 0.5,
  radarSize: 1.15,
  mapRotation: 0,
  viewMode: "split",
};

const loadSettings = () => {
  try {
    const savedSettings = localStorage.getItem("radarSettings");
    if (!savedSettings)
      return DEFAULT_SETTINGS;

    const loadedSettings = { ...DEFAULT_SETTINGS, ...JSON.parse(savedSettings) };
    return {
      ...loadedSettings,
      mapRotation: Math.round((Number(loadedSettings.mapRotation) || 0) / 90) * 90,
    };
  } catch {
    return DEFAULT_SETTINGS;
  }
};

const App = () => {
  const [playerArray, setPlayerArray] = useState([]);
  const [mapData, setMapData] = useState();
  const [localTeam, setLocalTeam] = useState();
  const [bombData, setBombData] = useState();
  const [settings, setSettings] = useState(loadSettings());
  const [bannerOpened, setBannerOpened] = useState(true)
  const playerCache = useRef(new Map());
  const bombCache = useRef({ data: undefined, seenAt: 0 });

  // Save settings to local storage whenever they change
  useEffect(() => {
    localStorage.setItem("radarSettings", JSON.stringify(settings));
  }, [settings]);

  useEffect(() => {
    let webSocket = null;
    let webSocketURL = null;
    let connectionTimeout = null;
    let currentMap = null;
    let disposed = false;

    const setRadarMessage = (message) => {
      const element = document.getElementsByClassName("radar_message")[0];
      if (element) element.textContent = message;
    };

    const connect = () => {

      if (PUBLIC_IP.startsWith("192.168")) {
        setRadarMessage(`A public IP address is required! Currently detected IP (${PUBLIC_IP}) is a private/local IP`);
        return;
      }

      if (!webSocket) {
        try {
          if (USE_LOCALHOST) {
            webSocketURL = `ws://localhost:${PORT}/cs2_webradar`;
          } else {
            webSocketURL = `ws://${EFFECTIVE_IP}:${PORT}/cs2_webradar`;
          }

          if (!webSocketURL) return;
          webSocket = new WebSocket(webSocketURL);
        } catch (error) {
          setRadarMessage(`${error}`);
        }
      }

      connectionTimeout = setTimeout(() => {
        webSocket.close();
      }, CONNECTION_TIMEOUT);

      webSocket.onopen = () => {
        clearTimeout(connectionTimeout);
        console.info("connected to the web socket");
      };

      webSocket.onclose = () => {
        clearTimeout(connectionTimeout);
        if (!disposed) console.error("disconnected from the web socket");
      };

      webSocket.onerror = (error) => {
        clearTimeout(connectionTimeout);
        setRadarMessage(`WebSocket connection to '${webSocketURL}' failed. Please check the IP address and try again`);
        console.error(error);
      };

      webSocket.onmessage = async (event) => {
        try {
          const message = typeof event.data === "string" ? event.data : await event.data.text();
          const parsedData = JSON.parse(message);
          if (disposed) return;

          const now = Date.now();
          const incomingPlayers = Array.isArray(parsedData.m_players)
            ? parsedData.m_players
            : [];

          incomingPlayers.forEach((incomingPlayer) => {
            const cachedPlayer = playerCache.current.get(incomingPlayer.m_idx);
            const position = incomingPlayer.m_position;
            const hasValidPosition =
              Number.isFinite(position?.x) &&
              Number.isFinite(position?.y) &&
              (position.x !== 0 || position.y !== 0);

            const player = !hasValidPosition && cachedPlayer
              ? {
                  ...incomingPlayer,
                  m_position: cachedPlayer.player.m_position,
                  m_eye_angle: cachedPlayer.player.m_eye_angle,
                }
              : incomingPlayer;

            playerCache.current.set(incomingPlayer.m_idx, {
              player,
              seenAt: now,
            });
          });

          playerCache.current.forEach((cachedPlayer, idx) => {
            if (now - cachedPlayer.seenAt > ENTITY_GRACE_PERIOD)
              playerCache.current.delete(idx);
          });

          setPlayerArray(
            Array.from(playerCache.current.values())
              .map(({ player }) => player)
              .sort((a, b) => a.m_idx - b.m_idx)
          );

          setLocalTeam(parsedData.m_local_team);

          if (parsedData.m_bomb) {
            bombCache.current = { data: parsedData.m_bomb, seenAt: now };
            setBombData(parsedData.m_bomb);
          } else if (now - bombCache.current.seenAt > ENTITY_GRACE_PERIOD) {
            bombCache.current = { data: undefined, seenAt: 0 };
            setBombData(undefined);
          }

          const map = parsedData.m_map;
          if (map !== "invalid" && map !== currentMap) {
            currentMap = map;
            const response = await fetch(`data/${map}/data.json`);
            if (!response.ok) throw new Error(`Unable to load map data for '${map}'`);

            const nextMapData = await response.json();
            if (disposed || currentMap !== map) return;

            setMapData({ ...nextMapData, name: map });
            document.body.style.backgroundImage = `url(./data/${map}/background.png)`;
          }
        } catch (error) {
          console.error("failed to process radar data", error);
        }
      };
    };

    connect();

    return () => {
      disposed = true;
      clearTimeout(connectionTimeout);
      if (webSocket && webSocket.readyState < WebSocket.CLOSING) webSocket.close();
    };
  }, []);

  return (
    <div className="w-screen h-screen flex flex-col"
      style={{
        background: `radial-gradient(50% 50% at 50% 50%, rgba(20, 40, 55, 0.95) 0%, rgba(7, 20, 30, 0.95) 100%)`,
        backdropFilter: `blur(7.5px)`,
      }}
    >
      {bannerOpened && (
        <section className="w-full flex items-center justify-between p-2 bg-radar-primary">
          <span className="w-full text-center text-[#1E3A54]">
            <span className="font-medium">€4.99</span> -
            HURRACAN - Plug & play feature rich shareable CS2 Web Radar
            <a className="ml-2 inline banner-link text-[#1E3A54]" href="https://hurracan.com">Learn more</a>
          </span>
          <button onClick={() => setBannerOpened(false)} className="hover:bg-[#9BC5E4]">
            <svg width="16" height="16" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32">
              <path fill="#4E799F" d="M 7.21875 5.78125 L 5.78125 7.21875 L 14.5625 16 L 5.78125 24.78125 L 7.21875 26.21875 L 16 17.4375 L 24.78125 26.21875 L 26.21875 24.78125 L 17.4375 16 L 26.21875 7.21875 L 24.78125 5.78125 L 16 14.5625 Z" />
            </svg>
          </button>
        </section>
      )}
      <div className={`w-full h-full flex flex-col justify-center overflow-hidden relative`}>
        <div className={`absolute right-2.5 top-2.5 z-50`}>
          <SettingsButton settings={settings} onSettingsChange={setSettings} />
        </div>

        {bombData && bombData.m_blow_time > 0 && !bombData.m_is_defused && (
          <div className={`absolute left-1/2 top-2 flex-col items-center gap-1 z-50`}>
            <div className={`flex justify-center items-center gap-1`}>
              <MaskedIcon
                path={`./assets/icons/c4_sml.png`}
                height={32}
                color={
                  (bombData.m_is_defusing &&
                    bombData.m_blow_time - bombData.m_defuse_time > 0 &&
                    `bg-radar-green`) ||
                  (bombData.m_blow_time - bombData.m_defuse_time < 0 &&
                    `bg-radar-red`) ||
                  `bg-radar-secondary`
                }
              />
              <span>{`${bombData.m_blow_time.toFixed(1)}s ${(bombData.m_is_defusing &&
                `(${bombData.m_defuse_time.toFixed(1)}s)`) ||
                ""
                }`}</span>
            </div>
          </div>
        )}

        {settings.viewMode === "models" ? (
          <div className="w-full h-full overflow-y-auto px-4 py-3">
            <div className="grid grid-cols-1 xl:grid-cols-2 gap-5">
              <section className="rounded-xl bg-radar-panel/60 border border-radar-secondary/20 p-3">
                <h2 className="text-radar-primary text-sm mb-3">Terrorists</h2>
                <ul className="flex flex-col gap-6 m-0 p-0">
                  {playerArray
                    .filter((player) => player.m_team == 2)
                    .map((player) => (
                      <PlayerCard
                        right={false}
                        key={player.m_idx}
                        playerData={player}
                        emphasizeModel={true}
                      />
                    ))}
                </ul>
              </section>

              <section className="rounded-xl bg-radar-panel/60 border border-radar-secondary/20 p-3">
                <h2 className="text-radar-primary text-sm mb-3 text-right">Counter-Terrorists</h2>
                <ul className="flex flex-col gap-6 m-0 p-0">
                  {playerArray
                    .filter((player) => player.m_team == 3)
                    .map((player) => (
                      <PlayerCard
                        right={true}
                        key={player.m_idx}
                        playerData={player}
                        emphasizeModel={true}
                      />
                    ))}
                </ul>
              </section>
            </div>
          </div>
        ) : (
          <div className="radar-layout">
            {settings.viewMode !== "radar" && (
              <ul id="terrorist" className="lg:flex hidden flex-col gap-7 m-0 p-0">
                {playerArray
                  .filter((player) => player.m_team == 2)
                  .map((player) => (
                    <PlayerCard
                      right={false}
                      key={player.m_idx}
                      playerData={player}
                    />
                  ))}
              </ul>
            )}

            {(playerArray.length > 0 && mapData && (
              <Radar
                playerArray={playerArray}
                radarImage={`./data/${mapData.name}/radar.png`}
                mapData={mapData}
                localTeam={localTeam}
                bombData={bombData}
                settings={settings}
              />
            )) || (
                <div id="radar" className={`relative overflow-hidden origin-center flex items-center justify-center p-6 text-center`}>
                  <h1 className="radar_message">
                    Connected! Waiting for data from usermode
                  </h1>
                </div>
              )}

            {settings.viewMode !== "radar" && (
              <ul
                id="counterTerrorist"
                className="lg:flex hidden flex-col gap-7 m-0 p-0"
              >
                {playerArray
                  .filter((player) => player.m_team == 3)
                  .map((player) => (
                    <PlayerCard
                      right={true}
                      key={player.m_idx}
                      playerData={player}
                      settings={settings}
                    />
                  ))}
              </ul>
            )}
          </div>
        )}
      </div>
    </div>
  );
};

export default App;
