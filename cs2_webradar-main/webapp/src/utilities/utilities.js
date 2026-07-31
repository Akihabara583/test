export const getRadarPosition = (mapData, entityCoords) => {
  if (!Number.isFinite(entityCoords?.x) || !Number.isFinite(entityCoords?.y)) {
    return { x: 0, y: 0 };
  }

  if (!Number.isFinite(mapData?.x) || !Number.isFinite(mapData?.y) || !Number.isFinite(mapData?.scale) || mapData.scale === 0) {
    return { x: 0, y: 0 };
  }

  const position = {
    x: (entityCoords.x - mapData.x) / mapData.scale / 1024,
    y: (((entityCoords.y - mapData.y) / mapData.scale) * -1.0) / 1024,
  };

  return position;
};

export const playerColors = [
  // blue
  "#84c8ed",

  // green
  "#009a7d",

  // yellow
  "#eadd40",

  // orange
  "#df7d29",

  // purple
  "#b72b92",

  // white
  "#ffffff",
];

export const teamEnum = {
  none: 0,
  spectator: 1,
  terrorist: 2,
  counterTerrorist: 3,
};
