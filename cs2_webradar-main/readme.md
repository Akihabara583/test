# cs2_webradar

> [!IMPORTANT]
> This version is still supported, but **[hurracan](https://hurracan.com)** provides a plug & play experience with instant sharing, completely new and responsive interface, a custom overlay for seamless integration with the game, and kernel level security. **[Get your 7-day free trial today!](https://hurracan.com/account/products/web-radar/buy-plan)**

[![discord](https://img.shields.io/badge/discord-5865F2?style=flat&logo=discord&logoColor=white)](https://discord.gg/qEBpufFkXK)
[![en](https://img.shields.io/badge/lang-en-blue.svg)](https://github.com/clauadv/cs2_webradar/blob/master/readme.md)
[![en](https://img.shields.io/badge/lang-cn-blue.svg)](https://github.com/clauadv/cs2_webradar/blob/master/readme-CN.md) <br>
Undetected Counter-Strike 2 browser-based radar cheat

## Requirements
- [Node.js](https://nodejs.org/en/download)
- [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/)
- [vcpkg](https://vcpkg.io/en/)

## Usage
- In `webapp`, run `npm install` to install dependencies
- In `webapp`, run `npm run dev` to start the webapp
- In `usermode` project, run `cs2_webradar.sln`
- In visual studio's toolbar, hover on `Build` and press `Build Solution` or press `Ctrl + Shift + B`
- In `release` folder, run `usermode.exe` and, in your browser navigate to `localhost:5173` <br>

## Sharing
> [!TIP]
> For easier sharing, consider **[hurracan](https://hurracan.com)** which handles this automatically.
- After you built `usermode` project, open `config.json` and set `m_ip` to your public IP
- In `webapp` project, `app.jsx`, change `USE_LOCALHOST` to `0` and set `PUBLIC_IP` to your public IP
- In `cmd`, type `ipconfig`, find `Default Gateway` and navigate to it in your browser
- In your router configuration, find `Port Forwarding` tab and forward port `22006/tcp` and `5173/tcp`
- Now your friends can see the radar by navigating to `Your public IP:5173`

## Showcase
https://github.com/clauadv/cs2_webradar/assets/26169415/718d5969-004b-4300-b152-2051a75d490d

## License
This project is licensed under the [GPL-3.0 license](https://github.com/clauadv/cs2_webradar?tab=GPL-3.0-1-ov-file#readme)
