#include "pch.hpp"

bool main()
{
    config_data_t config_data = {};
    INIT_STEP("config system", cfg::setup(config_data));
    INIT_STEP("memory", m_memory->setup());
    INIT_STEP("interfaces", i::setup());
    INIT_STEP("schema", schema::setup());

    ix::initNetSystem();
    LOG_INFO("winsock initialization completed");

    const auto formatted_address = std::format("ws://{}:22006/cs2_webradar", config_data.m_ip);

    static ix::WebSocket web_socket;
    bool connected = false;
    constexpr int max_connect_attempts = 5;
    for (int attempt = 1; attempt <= max_connect_attempts && !connected; ++attempt)
    {
        std::mutex handshake_mutex;
        std::condition_variable handshake_cv;
        bool attempt_connected = false;
        bool attempt_finished = false;

        web_socket.stop();
        web_socket.setUrl(formatted_address);
        web_socket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg)
        {
            if (msg->type == ix::WebSocketMessageType::Open)
            {
                {
                    std::lock_guard lock(handshake_mutex);
                    attempt_connected = true;
                    attempt_finished = true;
                }
                handshake_cv.notify_one();
            }
            else if (msg->type == ix::WebSocketMessageType::Error)
            {
                {
                    std::lock_guard lock(handshake_mutex);
                    attempt_finished = true;
                }
                handshake_cv.notify_one();
                LOG_WARNING(
                    "web socket connect attempt %d/%d failed ('%s')",
                    attempt,
                    max_connect_attempts,
                    formatted_address.c_str());
            }
        });
        web_socket.start();

        {
            std::unique_lock lock(handshake_mutex);
            handshake_cv.wait_for(
                lock,
                std::chrono::seconds(3),
                [&] { return attempt_finished; });
        }

        if (attempt_connected)
        {
            connected = true;
            LOG_INFO("connected to the web socket ('%s')", formatted_address.c_str());
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    if (!connected)
    {
        MessageBoxA(
            nullptr,
            "Cannot connect to websocket server.\n\n"
            "Make sure webapp is running and config.json contains a valid IP.\n"
            "Expected endpoint: ws://<ip>:22006/cs2_webradar",
            "cs2_webradar startup error",
            MB_OK | MB_ICONERROR | MB_TOPMOST);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        return {};
    }

    for (;;)
    {
        sdk::update();
        f::run();
        web_socket.send(f::m_data.dump());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return true;
}