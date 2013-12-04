#ifndef CLIENT_LOBBY_ROOM_PROTOCOL_HPP
#define CLIENT_LOBBY_ROOM_PROTOCOL_HPP

#include "network/protocols/lobby_room_protocol.hpp"

class ClientLobbyRoomProtocol : public LobbyRoomProtocol
{
    public:
        ClientLobbyRoomProtocol(const TransportAddress& server_address);
        virtual ~ClientLobbyRoomProtocol();

        void requestKartSelection(std::string kart_name);
        void sendMessage(std::string message);
        void leave();

        virtual void notifyEvent(Event* event);
        virtual void setup();
        virtual void update();
        virtual void asynchronousUpdate() {}

    protected:
        void newPlayer(Event* event);
        void disconnectedPlayer(Event* event);
        void connectionAccepted(Event* event); //!< Callback function on connection acceptation
        void connectionRefused(Event* event); //!< Callback function on connection refusal
        void kartSelectionRefused(Event* event);
        void kartSelectionUpdate(Event* event);
        void startGame(Event* event);
        void startSelection(Event* event);

        TransportAddress m_server_address;
        STKPeer* m_server;

        enum STATE
        {
            NONE,
            LINKED,
            REQUESTING_CONNECTION,
            CONNECTED, // means in the lobby room
            KART_SELECTION,
            SELECTING_KARTS, // in the network kart selection screen
            PLAYING,
            DONE,
            EXITING
        };
        STATE m_state;
};

#endif // CLIENT_LOBBY_ROOM_PROTOCOL_HPP