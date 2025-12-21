#include "[core_loader].hpp"
#include "logging.hpp"
#include "coroutine.hpp"

#include <thread>

[[noreturn]] int main()
{
    const auto log = Utils::Logging::Logger::Create("CORE");
    Utils::SetDefaultLogger(log);

    Core::BaseServiceContainer baseContainer{ log };

    auto & loader = Core::ServicesLoader();

    loader.PreInitialise(baseContainer);
    loader.Initialise();
    loader.OnAllServicesLoaded();
    loader.OnAllInterfacesLoaded();

    log->Msg("Core services initialised");

    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        loader.ProcessTick();
        Utils::GetTaskManager().ClearFinishedTasks();
    }
}

// #include <logging.hpp>
// #include <websocket.hpp>
//
// #include <thread>
// #include <chrono>
//
// #include <boost/json/serialize.hpp>
//
// using namespace Utils::Net::Websocket;
//
// class MyServerListener final : public Listener
// {
// public:
//     void OnSessionConnected(const Session::Shared& session) override
//     {
//         session->Log()->Debug("Client connected: {}:{}",
//                              session->RemoteAddress(),
//                              session->RemotePort());
//     }
//
//     void OnSessionDisconnected(const Session::Shared& session) override
//     {
//         session->Log()->Debug("Client disconnected: {}:{}",
//                              session->RemoteAddress(),
//                              session->RemotePort());
//     }
//
//     void OnMessage(const Session::Shared& session,
//                    const std::vector<std::uint8_t>& data) override
//     {
//         session->Log()->Debug("Bytes from {}: size={}",
//                              session->RemoteAddress(),
//                              data.size());
//     }
//
//     void OnMessage(const Session::Shared& session,
//                    std::string_view text) override
//     {
//         session->Log()->Debug("Text from {}: {}",
//                              session->RemoteAddress(),
//                              text);
//
//         session->Close();
//         // echo обратно клиенту
//         // session->Send(std::string("echo: ") + std::string(text));
//     }
//
//     void OnMessage(const Session::Shared& session,
//                    const boost::json::value& jsonValue) override
//     {
//         session->Log()->Debug("Json from {}: {}",
//                              session->RemoteAddress(),
//                              boost::json::serialize(jsonValue));
//     }
// };
//
// class MyClientListener final : public ClientListener
// {
// public:
//     explicit MyClientListener(const Utils::Logging::Logger::Shared& logger)
//         : logger_(logger)
//     {}
//
//     void OnConnected() override
//     {
//         logger_->Debug("[CLIENT] Connected");
//     }
//
//     void OnDisconnected() override
//     {
//         logger_->Debug("[CLIENT] Disconnected");
//     }
//
//     void OnMessage(const std::vector<std::uint8_t>& data) override
//     {
//         logger_->Debug("[CLIENT] Bytes message, size={}", data.size());
//     }
//
//     void OnMessage(std::string_view text) override
//     {
//         logger_->Debug("[CLIENT] Text message: {}", text);
//     }
//
//     void OnMessage(const boost::json::value& jsonValue) override
//     {
//         logger_->Debug("[CLIENT] Json message: {}",
//                        boost::json::serialize(jsonValue));
//     }
//
// private:
//     Utils::Logging::Logger::Shared logger_;
// };
//
// int main()
// {
//     // CORE логгер
//     auto coreLog = Utils::Logging::Logger::Create("CORE");
//     Utils::SetDefaultLogger(coreLog);
//
//     coreLog->Error("Hello error!");
//     coreLog->Warning("Hello warning!");
//     coreLog->Debug("Hello debug!");
//     coreLog->Msg("Hello msg!");
//
//     // ----- сервер -----
//     ServerConfig serverConfig;
//     serverConfig.address   = "0.0.0.0";
//     serverConfig.port      = 9002;
//     serverConfig.mode      = Mode::Text;
//     serverConfig.ioThreads = 4;
//     // serverConfig.useTls = true/false и т.п. по желанию
//
//     auto serverListener = std::make_shared<MyServerListener>();
//     auto server         = Server::Create(serverConfig, serverListener, coreLog);
//
//     // ----- клиент -----
//     ClientConfig clientConfig;
//     clientConfig.host            = "127.0.0.1";
//     clientConfig.port            = 9002;
//     clientConfig.path            = "/";          // если у тебя есть путь
//     clientConfig.mode            = Mode::Text;
//     clientConfig.ioThreads       = 1;
//     clientConfig.useTls          = false;        // или true + tls настройки
//     clientConfig.autoReconnect   = true;
//     clientConfig.reconnectDelayMs = 1000;
//
//     auto clientLogger   = coreLog->CreateChild("CLIENT-EVENTS");
//     auto clientListener = std::make_shared<MyClientListener>(clientLogger);
//     auto client         = Client::Create(clientConfig, clientListener, coreLog);
//
//     // ----- простой игровой цикл -----
//     std::uint64_t tick = 0;
//     bool sentTestMessage = false;
//
//     while (true)
//     {
//         server->ProcessTick();
//         client->ProcessTick();
//
//         // через ~1 секунду после старта попробуем отправить сообщение
//         if (!sentTestMessage && tick > 100)
//         {
//             client->Log()->Debug("[CLIENT] Sending test message");
//             client->Send(std::string_view("hello from client!"));
//             sentTestMessage = true;
//         }
//
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//         ++tick;
//     }
// }