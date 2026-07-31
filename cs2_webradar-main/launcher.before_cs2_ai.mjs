import { spawn, spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const projectRoot = dirname(fileURLToPath(import.meta.url));
const webappDir = join(projectRoot, "webapp");
const usermodeDir = join(projectRoot, "usermode", "release");
const usermodeExe = join(usermodeDir, "usermode.exe");
const ecDir = join(projectRoot, "ec");
const ecExe = join(ecDir, "ec.exe");
const ecSdl = join(ecDir, "SDL3.dll");
const websocketEntry = join(webappDir, "ws", "app.js");
const viteEntry = join(webappDir, "node_modules", "vite", "bin", "vite.js");

const requiredFiles = [
  ["usermode.exe", usermodeExe],
  ["EC module", ecExe],
  ["EC SDL3 runtime", ecSdl],
  ["WebSocket server", websocketEntry],
  ["Vite", viteEntry],
];

for (const [name, path] of requiredFiles) {
  if (!existsSync(path)) {
    console.error(`[ERROR] ${name} was not found:`);
    console.error(path);
    console.error("Run npm install in the webapp folder if dependencies are missing.");
    process.exit(1);
  }
}

const children = [];
let stopping = false;

const terminate = (child) => {
  if (!child?.pid || child.exitCode !== null || child.signalCode !== null) return;

  if (process.platform === "win32") {
    spawnSync(
      "taskkill.exe",
      ["/PID", String(child.pid), "/T", "/F"],
      { stdio: "ignore", windowsHide: true }
    );
  } else {
    child.kill("SIGTERM");
  }
};

const stopAll = (reason, exitCode = 0) => {
  if (stopping) return;
  stopping = true;

  console.log(`\nStopping CS2 Web Radar (${reason})...`);
  for (const child of [...children].reverse()) terminate(child);
  console.log("All radar processes stopped.");
  process.exit(exitCode);
};

const startChild = (name, command, args, cwd) => {
  const child = spawn(command, args, {
    cwd,
    stdio: "inherit",
    windowsHide: true,
  });

  children.push(child);

  child.on("error", (error) => {
    console.error(`[ERROR] Failed to start ${name}: ${error.message}`);
    stopAll(`${name} failed`, 1);
  });

  child.on("exit", (code, signal) => {
    if (!stopping) {
      const result = signal ? `signal ${signal}` : `code ${code}`;
      console.log(`${name} stopped (${result}).`);
      stopAll("shutdown");
    }
  });

  return child;
};

process.on("SIGINT", () => stopAll("Ctrl+C"));
process.on("SIGTERM", () => stopAll("termination request"));
process.on("uncaughtException", (error) => {
  console.error(error);
  stopAll("launcher error", 1);
});

console.log("Starting CS2 Web Radar...");
console.log("EC is disabled by default. Press 0 to toggle it.");
console.log("Press Ctrl+C once to stop everything.\n");

startChild("WebSocket server", process.execPath, [websocketEntry], webappDir);
startChild("Vite", process.execPath, [viteEntry], webappDir);
startChild("usermode", usermodeExe, [], usermodeDir);
startChild("EC", ecExe, [], ecDir);
