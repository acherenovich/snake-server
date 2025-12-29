#pragma once

#include "interfaces/game_server.hpp"

#include "game_messages.hpp"
#include "udp.hpp"

#include <unordered_map>
#include <unordered_set>

namespace Core::App::Game
{
    using UdpServer   = Utils::Net::Udp::Server;
    using UdpListener = Utils::Net::Udp::Listener;
    using UdpSession  = Utils::Net::Udp::Session;

    using EntitySnake = Utils::Legacy::Game::Entity::Snake;
    using EntityFood  = Utils::Legacy::Game::Entity::Food;

    using namespace Utils::Legacy::Game::Math;

    class GameServer final :
        public Interface::GameServer,
        public Logic,
        public UdpListener,
        public std::enable_shared_from_this<GameServer>
    {
        UdpServer::Shared udpServer_;

        std::unordered_map<UdpSession::Shared, EntitySnake::Shared> sessions_;
        std::unordered_set<EntitySnake::Shared> killedSnakes_;

        std::unordered_set<UdpSession::Shared> fullUpdates_;

        struct PendingRemove
        {
            Utils::Legacy::Game::Net::EntityType type { Utils::Legacy::Game::Net::EntityType::Food };
            std::uint8_t retries { 0 };
        };

        struct SessionNetState
        {
            std::uint32_t updateSeq { 0 };

            std::unordered_set<std::uint32_t> lastVisible; // EntityID
            std::unordered_map<std::uint32_t, std::uint32_t> lastHash; // EntityID -> hash
            std::unordered_map<std::uint32_t, Utils::Legacy::Game::Net::EntityType> lastType; // EntityID -> type

            std::unordered_map<std::uint32_t, PendingRemove> pendingRemoves;

            bool fullUpdateAllSegmentsNext { false }; // kept for compatibility; FullUpdate now always sends full segments

            std::unordered_set<std::uint32_t> pendingSnakeSnapshots; // entityIDs to snapshot next tick
        };

        std::unordered_map<UdpSession::Shared, SessionNetState> netState_;
        std::uint32_t nextEntityID_ { 1 };

        float visibilityPaddingPercent_ { 0.20f };

    public:
        using Shared = std::shared_ptr<GameServer>;

        GameServer();

        void Initialise(std::uint8_t serverID);

        void ProcessTick() override;

    public:
        void OnSessionConnected(const UdpSession::Shared & session) override;

        void OnSessionDisconnected(const UdpSession::Shared & session) override;

        void OnMessage(const UdpSession::Shared & session, const std::vector<std::uint8_t>& data) override;

        void SendPartialUpdate(const UdpSession::Shared & session, const EntitySnake::Shared & snake);

        void SendFullUpdate(const UdpSession::Shared & session, const EntitySnake::Shared & snake);

        void SendSnakeSnapshot(const UdpSession::Shared& session, std::uint32_t entityID);

    public:
        void ProcessSnake(const EntitySnake::Shared & snake);

        void RespawnSnake(const EntitySnake::Shared & snake);

        void KillSnake(const EntitySnake::Shared & snake);

        void ProcessKills();

        void GenerateFoods();

        static Shared Create(const BaseServiceContainer * parent, std::uint8_t serverID);

    private:
        std::string GetServiceContainerName() const override
        {
            return "GameServer";
        }
    };

    // state hashes for delta filtering
    std::uint32_t HashBytes(const void* data, std::size_t size);

    std::vector<sf::Vector2f> GetSnakeFullSegments(const Utils::Legacy::Game::Entity::Snake::Shared& snake);

    std::vector<sf::Vector2f> SampleSnakeValidationPoints(const Utils::Legacy::Game::Entity::Snake::Shared& snake);

    bool IsSnakeVisibleByAnySegment(const EntitySnake::Shared& viewer, const EntitySnake::Shared& target, float radius);
}