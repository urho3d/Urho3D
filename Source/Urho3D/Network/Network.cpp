//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Engine/EngineEvents.h"
#include "../IO/FileSystem.h"
#include "../Input/InputEvents.h"
#include "../IO/IOEvents.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Network/HttpRequest.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Network/NetworkPriority.h"
#include "../Network/Protocol.h"
#include "../Scene/Scene.h"

#include <SLikeNet/MessageIdentifiers.h>
#include <SLikeNet/NatPunchthroughClient.h>
#include <SLikeNet/peerinterface.h>
#include <SLikeNet/statistics.h>
#include <SLikeNet/FullyConnectedMesh2.h>
#include <SLikeNet/DS_List.h>
#include <SLikeNet/BitStream.h>
#include <SLikeNet/ReadyEvent.h>
#include <SLikeNet/ConnectionGraph2.h>

#ifdef SendMessage
#undef SendMessage
#endif

#include "../DebugNew.h"

namespace Urho3D
{

static const char* RAKNET_MESSAGEID_STRINGS[] = {
    "ID_CONNECTED_PING",
    "ID_UNCONNECTED_PING",
    "ID_UNCONNECTED_PING_OPEN_CONNECTIONS",
    "ID_CONNECTED_PONG",
    "ID_DETECT_LOST_CONNECTIONS",
    "ID_OPEN_CONNECTION_REQUEST_1",
    "ID_OPEN_CONNECTION_REPLY_1",
    "ID_OPEN_CONNECTION_REQUEST_2",
    "ID_OPEN_CONNECTION_REPLY_2",
    "ID_CONNECTION_REQUEST",
    "ID_REMOTE_SYSTEM_REQUIRES_PUBLIC_KEY",
    "ID_OUR_SYSTEM_REQUIRES_SECURITY",
    "ID_PUBLIC_KEY_MISMATCH",
    "ID_OUT_OF_BAND_INTERNAL",
    "ID_SND_RECEIPT_ACKED",
    "ID_SND_RECEIPT_LOSS",
    "ID_CONNECTION_REQUEST_ACCEPTED",
    "ID_CONNECTION_ATTEMPT_FAILED",
    "ID_ALREADY_CONNECTED",
    "ID_NEW_INCOMING_CONNECTION",
    "ID_NO_FREE_INCOMING_CONNECTIONS",
    "ID_DISCONNECTION_NOTIFICATION",
    "ID_CONNECTION_LOST",
    "ID_CONNECTION_BANNED",
    "ID_INVALID_PASSWORD",
    "ID_INCOMPATIBLE_PROTOCOL_VERSION",
    "ID_IP_RECENTLY_CONNECTED",
    "ID_TIMESTAMP",
    "ID_UNCONNECTED_PONG",
    "ID_ADVERTISE_SYSTEM",
    "ID_DOWNLOAD_PROGRESS",
    "ID_REMOTE_DISCONNECTION_NOTIFICATION",
    "ID_REMOTE_CONNECTION_LOST",
    "ID_REMOTE_NEW_INCOMING_CONNECTION",
    "ID_FILE_LIST_TRANSFER_HEADER",
    "ID_FILE_LIST_TRANSFER_FILE",
    "ID_FILE_LIST_REFERENCE_PUSH_ACK",
    "ID_DDT_DOWNLOAD_REQUEST",
    "ID_TRANSPORT_STRING",
    "ID_REPLICA_MANAGER_CONSTRUCTION",
    "ID_REPLICA_MANAGER_SCOPE_CHANGE",
    "ID_REPLICA_MANAGER_SERIALIZE",
    "ID_REPLICA_MANAGER_DOWNLOAD_STARTED",
    "ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE",
    "ID_RAKVOICE_OPEN_CHANNEL_REQUEST",
    "ID_RAKVOICE_OPEN_CHANNEL_REPLY",
    "ID_RAKVOICE_CLOSE_CHANNEL",
    "ID_RAKVOICE_DATA",
    "ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE",
    "ID_AUTOPATCHER_CREATION_LIST",
    "ID_AUTOPATCHER_DELETION_LIST",
    "ID_AUTOPATCHER_GET_PATCH",
    "ID_AUTOPATCHER_PATCH_LIST",
    "ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR",
    "ID_AUTOPATCHER_CANNOT_DOWNLOAD_ORIGINAL_UNMODIFIED_FILES",
    "ID_AUTOPATCHER_FINISHED_INTERNAL",
    "ID_AUTOPATCHER_FINISHED",
    "ID_AUTOPATCHER_RESTART_APPLICATION",
    "ID_NAT_PUNCHTHROUGH_REQUEST",
    "ID_NAT_CONNECT_AT_TIME",
    "ID_NAT_GET_MOST_RECENT_PORT",
    "ID_NAT_CLIENT_READY",
    "ID_NAT_TARGET_NOT_CONNECTED",
    "ID_NAT_TARGET_UNRESPONSIVE",
    "ID_NAT_CONNECTION_TO_TARGET_LOST",
    "ID_NAT_ALREADY_IN_PROGRESS",
    "ID_NAT_PUNCHTHROUGH_FAILED",
    "ID_NAT_PUNCHTHROUGH_SUCCEEDED",
    "ID_READY_EVENT_SET",
    "ID_READY_EVENT_UNSET",
    "ID_READY_EVENT_ALL_SET",
    "ID_READY_EVENT_QUERY",
    "ID_LOBBY_GENERAL",
    "ID_RPC_REMOTE_ERROR",
    "ID_RPC_PLUGIN",
    "ID_FILE_LIST_REFERENCE_PUSH",
    "ID_READY_EVENT_FORCE_ALL_SET",
    "ID_ROOMS_EXECUTE_FUNC",
    "ID_ROOMS_LOGON_STATUS",
    "ID_ROOMS_HANDLE_CHANGE",
    "ID_LOBBY2_SEND_MESSAGE",
    "ID_LOBBY2_SERVER_ERROR",
    "ID_FCM2_NEW_HOST",
    "ID_FCM2_REQUEST_FCMGUID",
    "ID_FCM2_RESPOND_CONNECTION_COUNT",
    "ID_FCM2_INFORM_FCMGUID",
    "ID_FCM2_UPDATE_MIN_TOTAL_CONNECTION_COUNT",
    "ID_FCM2_VERIFIED_JOIN_START",
    "ID_FCM2_VERIFIED_JOIN_CAPABLE",
    "ID_FCM2_VERIFIED_JOIN_FAILED",
    "ID_FCM2_VERIFIED_JOIN_ACCEPTED",
    "ID_FCM2_VERIFIED_JOIN_REJECTED",
    "ID_UDP_PROXY_GENERAL",
    "ID_SQLite3_EXEC",
    "ID_SQLite3_UNKNOWN_DB",
    "ID_SQLLITE_LOGGER",
    "ID_NAT_TYPE_DETECTION_REQUEST",
    "ID_NAT_TYPE_DETECTION_RESULT",
    "ID_ROUTER_2_INTERNAL",
    "ID_ROUTER_2_FORWARDING_NO_PATH",
    "ID_ROUTER_2_FORWARDING_ESTABLISHED",
    "ID_ROUTER_2_REROUTED",
    "ID_TEAM_BALANCER_INTERNAL",
    "ID_TEAM_BALANCER_REQUESTED_TEAM_FULL",
    "ID_TEAM_BALANCER_REQUESTED_TEAM_LOCKED",
    "ID_TEAM_BALANCER_TEAM_REQUESTED_CANCELLED",
    "ID_TEAM_BALANCER_TEAM_ASSIGNED",
    "ID_LIGHTSPEED_INTEGRATION",
    "ID_XBOX_LOBBY",
    "ID_TWO_WAY_AUTHENTICATION_INCOMING_CHALLENGE_SUCCESS",
    "ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_SUCCESS",
    "ID_TWO_WAY_AUTHENTICATION_INCOMING_CHALLENGE_FAILURE",
    "ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_FAILURE",
    "ID_TWO_WAY_AUTHENTICATION_OUTGOING_CHALLENGE_TIMEOUT",
    "ID_TWO_WAY_AUTHENTICATION_NEGOTIATION",
    "ID_CLOUD_POST_REQUEST",
    "ID_CLOUD_RELEASE_REQUEST",
    "ID_CLOUD_GET_REQUEST",
    "ID_CLOUD_GET_RESPONSE",
    "ID_CLOUD_UNSUBSCRIBE_REQUEST",
    "ID_CLOUD_SERVER_TO_SERVER_COMMAND",
    "ID_CLOUD_SUBSCRIPTION_NOTIFICATION",
    "ID_LIB_VOICE",
    "ID_RELAY_PLUGIN",
    "ID_NAT_REQUEST_BOUND_ADDRESSES",
    "ID_NAT_RESPOND_BOUND_ADDRESSES",
    "ID_FCM2_UPDATE_USER_CONTEXT",
    "ID_RESERVED_3",
    "ID_RESERVED_4",
    "ID_RESERVED_5",
    "ID_RESERVED_6",
    "ID_RESERVED_7",
    "ID_RESERVED_8",
    "ID_RESERVED_9",
    "ID_USER_PACKET_ENUM"
};

static const int DEFAULT_UPDATE_FPS = 30;
static const int SERVER_TIMEOUT_TIME = 5000;

Network::Network(Context* context) :
    Object(context),
    updateFps_(DEFAULT_UPDATE_FPS),
    simulatedLatency_(0),
    simulatedPacketLoss_(0.0f),
    updateInterval_(1.0f / (float)DEFAULT_UPDATE_FPS),
    updateAcc_(0.0f),
    isServer_(false),
    scene_(nullptr),
    natPunchServerAddress_(nullptr),
    remoteGUID_(nullptr),
    networkMode_(SERVER_CLIENT),
    natAutoReconnect_(true),
    allowedConnectionCount_(128),
    fullyConnectedMesh2_(nullptr),
    connectionGraph2_(nullptr),
    readyEvent_(nullptr)
{
    rakPeer_ = SLNet::RakPeerInterface::GetInstance();
    rakPeerClient_ = SLNet::RakPeerInterface::GetInstance();

    rakPeer_->SetTimeoutTime(SERVER_TIMEOUT_TIME, SLNet::UNASSIGNED_SYSTEM_ADDRESS);
    rakPeerClient_->SetTimeoutTime(SERVER_TIMEOUT_TIME, SLNet::UNASSIGNED_SYSTEM_ADDRESS);
//    rakPeer_->AttachPlugin(new PacketLogger());
    SetPassword("");
    SetDiscoveryBeacon(VariantMap());

    natPunchthroughClient_ = new SLNet::NatPunchthroughClient;
    natPunchthroughServerClient_ = new SLNet::NatPunchthroughClient;

    SetNATServerInfo("127.0.0.1", 61111);

    // Register Network library object factories
    RegisterNetworkLibrary(context_);

    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(Network, HandleBeginFrame));
    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Network, HandleRenderUpdate));

    // Blacklist remote events which are not to be allowed to be registered in any case
    blacklistedRemoteEvents_.Insert(E_CONSOLECOMMAND);
    blacklistedRemoteEvents_.Insert(E_LOGMESSAGE);
    blacklistedRemoteEvents_.Insert(E_BEGINFRAME);
    blacklistedRemoteEvents_.Insert(E_UPDATE);
    blacklistedRemoteEvents_.Insert(E_POSTUPDATE);
    blacklistedRemoteEvents_.Insert(E_RENDERUPDATE);
    blacklistedRemoteEvents_.Insert(E_ENDFRAME);
    blacklistedRemoteEvents_.Insert(E_MOUSEBUTTONDOWN);
    blacklistedRemoteEvents_.Insert(E_MOUSEBUTTONUP);
    blacklistedRemoteEvents_.Insert(E_MOUSEMOVE);
    blacklistedRemoteEvents_.Insert(E_MOUSEWHEEL);
    blacklistedRemoteEvents_.Insert(E_KEYDOWN);
    blacklistedRemoteEvents_.Insert(E_KEYUP);
    blacklistedRemoteEvents_.Insert(E_TEXTINPUT);
    blacklistedRemoteEvents_.Insert(E_JOYSTICKCONNECTED);
    blacklistedRemoteEvents_.Insert(E_JOYSTICKDISCONNECTED);
    blacklistedRemoteEvents_.Insert(E_JOYSTICKBUTTONDOWN);
    blacklistedRemoteEvents_.Insert(E_JOYSTICKBUTTONUP);
    blacklistedRemoteEvents_.Insert(E_JOYSTICKAXISMOVE);
    blacklistedRemoteEvents_.Insert(E_JOYSTICKHATMOVE);
    blacklistedRemoteEvents_.Insert(E_TOUCHBEGIN);
    blacklistedRemoteEvents_.Insert(E_TOUCHEND);
    blacklistedRemoteEvents_.Insert(E_TOUCHMOVE);
    blacklistedRemoteEvents_.Insert(E_GESTURERECORDED);
    blacklistedRemoteEvents_.Insert(E_GESTUREINPUT);
    blacklistedRemoteEvents_.Insert(E_MULTIGESTURE);
    blacklistedRemoteEvents_.Insert(E_DROPFILE);
    blacklistedRemoteEvents_.Insert(E_INPUTFOCUS);
    blacklistedRemoteEvents_.Insert(E_MOUSEVISIBLECHANGED);
    blacklistedRemoteEvents_.Insert(E_EXITREQUESTED);
    blacklistedRemoteEvents_.Insert(E_SERVERCONNECTED);
    blacklistedRemoteEvents_.Insert(E_SERVERDISCONNECTED);
    blacklistedRemoteEvents_.Insert(E_CONNECTFAILED);
    blacklistedRemoteEvents_.Insert(E_CLIENTCONNECTED);
    blacklistedRemoteEvents_.Insert(E_CLIENTDISCONNECTED);
    blacklistedRemoteEvents_.Insert(E_CLIENTIDENTITY);
    blacklistedRemoteEvents_.Insert(E_CLIENTSCENELOADED);
    blacklistedRemoteEvents_.Insert(E_NETWORKMESSAGE);
    blacklistedRemoteEvents_.Insert(E_NETWORKUPDATE);
    blacklistedRemoteEvents_.Insert(E_NETWORKUPDATESENT);
    blacklistedRemoteEvents_.Insert(E_NETWORKSCENELOADFAILED);
    blacklistedRemoteEvents_.Insert(E_NATMASTERCONNECTIONSUCCEEDED);
    blacklistedRemoteEvents_.Insert(E_NATMASTERCONNECTIONFAILED);
    blacklistedRemoteEvents_.Insert(E_NETWORKNATPUNCHTROUGHSUCCEEDED);
    blacklistedRemoteEvents_.Insert(E_NETWORKNATPUNCHTROUGHFAILED);
    blacklistedRemoteEvents_.Insert(E_P2PJOINREQUESTDENIED);
    blacklistedRemoteEvents_.Insert(E_P2PALLREADYCHANGED);
    blacklistedRemoteEvents_.Insert(E_NETWORKHOSTDISCOVERED);
    blacklistedRemoteEvents_.Insert(E_NETWORKINVALIDPASSWORD);
    blacklistedRemoteEvents_.Insert(E_P2PNEWHOST);
    blacklistedRemoteEvents_.Insert(E_P2PSESSIONSTARTED);

    RegisterRemoteEvent(E_NETWORKBANNED);
}

Network::~Network()
{
    // If server connection exists, disconnect, but do not send an event because we are shutting down
    Disconnect(1000);

    if (fullyConnectedMesh2_)
        fullyConnectedMesh2_->ResetHostCalculation();

    rakPeer_->DetachPlugin(natPunchthroughServerClient_);
    rakPeerClient_->DetachPlugin(natPunchthroughClient_);

    SLNet::NatPunchthroughClient::DestroyInstance(natPunchthroughServerClient_);
    natPunchthroughServerClient_ = nullptr;
    SLNet::NatPunchthroughClient::DestroyInstance(natPunchthroughClient_);
    natPunchthroughClient_ = nullptr;

    if (fullyConnectedMesh2_) {
        rakPeer_->DetachPlugin(fullyConnectedMesh2_);
        SLNet::FullyConnectedMesh2::DestroyInstance(fullyConnectedMesh2_);
        fullyConnectedMesh2_ = nullptr;
    }

    if (connectionGraph2_) {
        rakPeer_->DetachPlugin(connectionGraph2_);
        SLNet::ConnectionGraph2::DestroyInstance(connectionGraph2_);
        connectionGraph2_ = nullptr;
    }

    if (readyEvent_) {
        rakPeer_->DetachPlugin(readyEvent_);
        SLNet::ReadyEvent::DestroyInstance(readyEvent_);
        readyEvent_ = nullptr;
    }

    serverConnection_.Reset();

    clientConnections_.Clear();

    delete remoteGUID_;
    remoteGUID_ = nullptr;
    delete natPunchServerAddress_;
    natPunchServerAddress_ = nullptr;

    SLNet::RakPeerInterface::DestroyInstance(rakPeer_);
    SLNet::RakPeerInterface::DestroyInstance(rakPeerClient_);

    rakPeer_ = nullptr;
    rakPeerClient_ = nullptr;
}

void Network::HandleMessage(const SLNet::AddressOrGUID& source, int packetID, int msgID, const char* data, size_t numBytes)
{
    // Only process messages from known sources
    Connection* connection = GetConnection(source);
    if (connection)
    {
        MemoryBuffer msg(data, (unsigned)numBytes);
        if (connection->ProcessMessage((int)msgID, msg))
            return;
    }
    else {
        URHO3D_LOGWARNING("Discarding message from unknown MessageConnection " + String(source.ToString()) + " => " + source.rakNetGuid.ToString());
    }
}

void Network::NewConnectionEstablished(const SLNet::AddressOrGUID& connection, const char* address)
{
    if (allowedConnectionCount_ == 0 || GetClientConnections().Size() >= allowedConnectionCount_) {
        URHO3D_LOGWARNING("Server is full, dropping incoming connection!");
        rakPeer_->CloseConnection(connection, true);
        return;
    }

    ReadyStatusChanged();

    // It is possible that in P2P mode the incomming connection is already registered
    if (networkMode_ == PEER_TO_PEER && clientConnections_[connection]) {
        // TODO: proper scene state management
        clientConnections_[connection]->SetSceneLoaded(true);
        SendOutIdentityToPeers();
        return;
    }

    // Create a new client connection corresponding to this MessageConnection
    SharedPtr<Connection> newConnection(new Connection(context_, true, connection, rakPeer_));
    newConnection->ConfigureNetworkSimulator(simulatedLatency_, simulatedPacketLoss_);

    // All peers should share the same scene
    if (networkMode_ == PEER_TO_PEER && serverConnection_) {
        newConnection->SetScene(serverConnection_->GetScene());
        newConnection->SetSceneLoaded(true);
    }

    newConnection->SetIP(address);
    clientConnections_[connection] = newConnection;
    URHO3D_LOGINFO("Client " + newConnection->ToString() + " connected");

    using namespace ClientConnected;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = newConnection;
    newConnection->SendEvent(E_CLIENTCONNECTED, eventData);

    // Let all our peers know who we are
    if (networkMode_ == PEER_TO_PEER) {
        SendOutIdentityToPeers();
    }
}

void Network::SendOutIdentityToPeers()
{
    if (serverConnection_) {
        // Send our identity to all connected peers
        auto clients = GetClientConnections();
        for (auto it = clients.Begin(); it != clients.End(); ++it) {
            VectorBuffer msg;
            msg.WriteVariantMap(serverConnection_->GetIdentity());
            (*it)->SendMessage(MSG_IDENTITY, true, true, msg);
        }
    }
}

void Network::ClientDisconnected(const SLNet::AddressOrGUID& connection)
{
    // Remove the client connection that corresponds to this MessageConnection
    HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::Iterator i = clientConnections_.Find(connection);
    if (i != clientConnections_.End())
    {
        Connection* connection = i->second_;
        URHO3D_LOGINFO("Client " + connection->ToString() + " disconnected");

        using namespace ClientDisconnected;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_CONNECTION] = connection;
        connection->SendEvent(E_CLIENTDISCONNECTED, eventData);

        clientConnections_.Erase(i);
    }

    ReadyStatusChanged();
}

void Network::SetDiscoveryBeacon(const VariantMap& data)
{
    VectorBuffer buffer;
    buffer.WriteVariantMap(data);
    if (buffer.GetSize() > 400)
        URHO3D_LOGERROR("Discovery beacon of size: " + String(buffer.GetSize()) + " bytes is too large, modify MAX_OFFLINE_DATA_LENGTH in RakNet or reduce size");
    rakPeer_->SetOfflinePingResponse((const char*)buffer.GetData(), buffer.GetSize());
}

void Network::DiscoverHosts(unsigned port)
{
    // JSandusky: Contrary to the manual, we actually do have to perform Startup first before we can Ping
    if (!rakPeerClient_->IsActive())
    {
        SLNet::SocketDescriptor socket;
        // Startup local connection with max 1 incoming connection(first param) and 1 socket description (third param)
        rakPeerClient_->Startup(2, &socket, 1);
        rakPeerClient_->SetMaximumIncomingConnections(2);
    }
    rakPeerClient_->Ping("255.255.255.255", port, false);
}

void Network::SetPassword(const String& password)
{
    rakPeer_->SetIncomingPassword(password.CString(), password.Length());
    password_ = password;
}

bool Network::Connect(const String& address, unsigned short port, Scene* scene, const VariantMap& identity)
{
    URHO3D_PROFILE(Connect);

    if (!rakPeerClient_->IsActive())
    {
        SLNet::SocketDescriptor socket;
        // Startup local connection with max 2 incoming connections(first param) and 1 socket description (third param)
        rakPeerClient_->Startup(2, &socket, 1);
        rakPeerClient_->SetMaximumIncomingConnections(2);
    }

    //isServer_ = false;
    SLNet::ConnectionAttemptResult connectResult = rakPeerClient_->Connect(address.CString(), port, password_.CString(), password_.Length());
    if (connectResult == SLNet::CONNECTION_ATTEMPT_STARTED)
    {
        serverConnection_ = new Connection(context_, false, rakPeerClient_->GetMyGUID(), rakPeerClient_);
        serverConnection_->SetScene(scene);
        serverConnection_->SetIdentity(identity);
        serverConnection_->SetConnectPending(true);
        serverConnection_->ConfigureNetworkSimulator(simulatedLatency_, simulatedPacketLoss_);

        URHO3D_LOGINFO("Connecting to server " + address + ":" + String(port) + ", Client: " + serverConnection_->ToString());
        return true;
    }
    else if (connectResult == SLNet::ALREADY_CONNECTED_TO_ENDPOINT) {
        URHO3D_LOGWARNING("Already connected to server!");
        SendEvent(E_CONNECTIONINPROGRESS);
        return false;
    }
    else if (connectResult == SLNet::CONNECTION_ATTEMPT_ALREADY_IN_PROGRESS) {
        URHO3D_LOGWARNING("Connection attempt already in progress!");
        SendEvent(E_CONNECTIONINPROGRESS);
        return false;
    }
    else
    {
        URHO3D_LOGERROR("Failed to connect to server " + address + ":" + String(port) + ", error code: " + String((int)connectResult));
        SendEvent(E_CONNECTFAILED);
        return false;
    }
}

bool Network::ConnectNAT(const String& address, unsigned short port)
{
    URHO3D_PROFILE(ConnectNAT);

    if (!rakPeer_->IsActive())
    {
        SLNet::SocketDescriptor socket;
        // Startup local connection with max 2 incoming connections(first param) and 1 socket description (third param)
        rakPeer_->Startup(128, &socket, 1);
        rakPeer_->SetMaximumIncomingConnections(allowedConnectionCount_);
        rakPeer_->AttachPlugin(natPunchthroughServerClient_);
    }

    SLNet::ConnectionAttemptResult connectResult = rakPeer_->Connect(address.CString(), port, nullptr, 0);
    if (connectResult == SLNet::ALREADY_CONNECTED_TO_ENDPOINT) {
        SendEvent(E_NATMASTERCONNECTIONSUCCEEDED);
        URHO3D_LOGWARNING("Already connected to server " + address + ":" + String(port) + ", error code: " + String((int)connectResult));
        return false;
    }
    if (connectResult != SLNet::CONNECTION_ATTEMPT_STARTED)
    {
        URHO3D_LOGERROR("Failed to connect to server " + address + ":" + String(port) + ", error code: " + String((int)connectResult));
        SendEvent(E_CONNECTFAILED);
        return false;
    }
    else
    {
        URHO3D_LOGINFO("Connecting to server " + address + ":" + String(port));
        return true;
    }
}

void Network::Disconnect(int waitMSec)
{
    URHO3D_PROFILE(Disconnect);

    if (networkMode_ == PEER_TO_PEER) {
        rakPeer_->Shutdown(waitMSec);
        clientConnections_.Clear();
    }

    if (serverConnection_) {
        serverConnection_->Disconnect(waitMSec);
        serverConnection_.Reset();
    }

    if (rakPeerClient_->IsActive()) {
        rakPeerClient_->Shutdown(1000);
    }
    natPunchthroughClient_->Clear();

    isServer_ = false;
}

bool Network::StartServer(unsigned short port, unsigned int maxConnections)
{
    if (IsServerRunning())
        return true;

    URHO3D_PROFILE(StartServer);

    SLNet::SocketDescriptor socket;//(port, AF_INET);
    socket.port = port;
    socket.socketFamily = AF_INET;
    // Startup local connection with max 128 incoming connection(first param) and 1 socket description (third param)
    SLNet::StartupResult startResult = rakPeer_->Startup(maxConnections, &socket, 1);
    allowedConnectionCount_ = maxConnections;

    if (startResult == SLNet::RAKNET_STARTED)
    {
        URHO3D_LOGINFO("Started server on port " + String(port));
        rakPeer_->SetMaximumIncomingConnections(maxConnections);
        isServer_ = true;
        rakPeer_->SetOccasionalPing(true);
        rakPeer_->SetUnreliableTimeout(1000);
        //rakPeer_->SetIncomingPassword("Parole", (int)strlen("Parole"));
        return true;
    }
    else
    {
        URHO3D_LOGINFO("Failed to start server on port " + String(port) + ", error code: " + String((int)startResult));
        return false;
    }
}

void Network::StopServer()
{
    clientConnections_.Clear();

    if (!rakPeer_)
        return;

    if (!IsServerRunning())
        return;
    // Provide 300 ms to notify
    rakPeer_->Shutdown(300);

    URHO3D_PROFILE(StopServer);

    URHO3D_LOGINFO("Stopped server");
}

void Network::SetNATServerInfo(const String& address, unsigned short port)
{
    if (!natPunchServerAddress_)
        natPunchServerAddress_ = new SLNet::SystemAddress;

    natPunchServerAddress_->FromStringExplicitPort(address.CString(), port);
}

void Network::StartNATClient()
{
    if (!rakPeer_) {
        URHO3D_LOGERROR("Unable to start NAT client, client not initialized!");
        return;
    }
    if (natPunchServerAddress_->GetPort() == 0) {
        URHO3D_LOGERROR("NAT master server address incorrect!");
        return;
    }

    rakPeer_->AttachPlugin(natPunchthroughServerClient_);
    guid_ = String(rakPeer_->GetGuidFromSystemAddress(SLNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());
    URHO3D_LOGINFO("GUID: " + guid_);
    rakPeer_->Connect(natPunchServerAddress_->ToString(false), natPunchServerAddress_->GetPort(), nullptr, 0);
}

void Network::AttemptNATPunchtrough(const String& guid, Scene* scene, const VariantMap& identity)
{
    scene_ = scene;
    identity_ = identity;
    if (!remoteGUID_)
        remoteGUID_ = new SLNet::RakNetGUID;

    remoteGUID_->FromString(guid.CString());

    if (rakPeerClient_->IsActive()) {
        rakPeerClient_->Shutdown(1000);
    }
    natPunchthroughClient_->Clear();
    rakPeerClient_->AttachPlugin(natPunchthroughClient_);

    SLNet::SocketDescriptor socket;
    // Startup local connection with max 2 incoming connections(first param) and 1 socket description (third param)
    rakPeerClient_->Startup(2, &socket, 1);
    rakPeerClient_->SetMaximumIncomingConnections(2);

    SLNet::ConnectionAttemptResult connectResult = rakPeerClient_->Connect(natPunchServerAddress_->ToString(false), natPunchServerAddress_->GetPort(), nullptr, 0);
    URHO3D_LOGINFO("AttemptNATPunchtrough status " + String((int) connectResult));
}

void Network::BroadcastMessage(int msgID, bool reliable, bool inOrder, const VectorBuffer& msg, unsigned contentID)
{
    BroadcastMessage(msgID, reliable, inOrder, msg.GetData(), msg.GetSize(), contentID);
}

void Network::BroadcastMessage(int msgID, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes,
    unsigned contentID)
{
    if (!rakPeer_)
        return;

    VectorBuffer msgData;
    msgData.WriteUByte((unsigned char)ID_USER_PACKET_ENUM);
    msgData.WriteUInt((unsigned int)msgID);
    msgData.Write(data, numBytes);

    if (isServer_ || networkMode_ == PEER_TO_PEER) {
        rakPeer_->Send((const char *) msgData.GetData(), (int) msgData.GetSize(), HIGH_PRIORITY, RELIABLE, (char) 0, SLNet::UNASSIGNED_RAKNET_GUID, true);
    }
    else
        URHO3D_LOGERROR("Server not running, can not broadcast messages");
}

void Network::BroadcastRemoteEvent(StringHash eventType, bool inOrder, const VariantMap& eventData)
{
    for (HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::Iterator i = clientConnections_.Begin(); i != clientConnections_.End(); ++i)
        i->second_->SendRemoteEvent(eventType, inOrder, eventData);
}

void Network::BroadcastRemoteEvent(Scene* scene, StringHash eventType, bool inOrder, const VariantMap& eventData)
{
    for (HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::Iterator i = clientConnections_.Begin();
         i != clientConnections_.End(); ++i)
    {
        if (i->second_->GetScene() == scene)
            i->second_->SendRemoteEvent(eventType, inOrder, eventData);
    }
}

void Network::BroadcastRemoteEvent(Node* node, StringHash eventType, bool inOrder, const VariantMap& eventData)
{
    if (!node)
    {
        URHO3D_LOGERROR("Null sender node for remote node event");
        return;
    }
    if (!node->IsReplicated())
    {
        URHO3D_LOGERROR("Sender node has a local ID, can not send remote node event");
        return;
    }

    Scene* scene = node->GetScene();
    for (HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::Iterator i = clientConnections_.Begin();
         i != clientConnections_.End(); ++i)
    {
        if (i->second_->GetScene() == scene)
            i->second_->SendRemoteEvent(node, eventType, inOrder, eventData);
    }
}

void Network::SetUpdateFps(int fps)
{
    updateFps_ = Max(fps, 1);
    updateInterval_ = 1.0f / (float)updateFps_;
    updateAcc_ = 0.0f;
}

void Network::SetSimulatedLatency(int ms)
{
    simulatedLatency_ = Max(ms, 0);
    ConfigureNetworkSimulator();
}

void Network::SetSimulatedPacketLoss(float probability)
{
    simulatedPacketLoss_ = Clamp(probability, 0.0f, 1.0f);
    ConfigureNetworkSimulator();
}

void Network::RegisterRemoteEvent(StringHash eventType)
{
    if (blacklistedRemoteEvents_.Find(eventType) != blacklistedRemoteEvents_.End())
    {
        URHO3D_LOGERROR("Attempted to register blacklisted remote event type " + String(eventType));
        return;
    }

    allowedRemoteEvents_.Insert(eventType);
}

void Network::UnregisterRemoteEvent(StringHash eventType)
{
    allowedRemoteEvents_.Erase(eventType);
}

void Network::UnregisterAllRemoteEvents()
{
    allowedRemoteEvents_.Clear();
}

void Network::SetPackageCacheDir(const String& path)
{
    packageCacheDir_ = AddTrailingSlash(path);
}

void Network::SendPackageToClients(Scene* scene, PackageFile* package)
{
    if (!scene)
    {
        URHO3D_LOGERROR("Null scene specified for SendPackageToClients");
        return;
    }
    if (!package)
    {
        URHO3D_LOGERROR("Null package specified for SendPackageToClients");
        return;
    }

    for (HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::Iterator i = clientConnections_.Begin();
         i != clientConnections_.End(); ++i)
    {
        if (i->second_->GetScene() == scene)
            i->second_->SendPackageToClient(package);
    }
}

SharedPtr<HttpRequest> Network::MakeHttpRequest(const String& url, const String& verb, const Vector<String>& headers,
    const String& postData)
{
    URHO3D_PROFILE(MakeHttpRequest);

    // The initialization of the request will take time, can not know at this point if it has an error or not
    SharedPtr<HttpRequest> request(new HttpRequest(url, verb, headers, postData));
    return request;
}

void Network::BanAddress(const String& address)
{
    URHO3D_LOGWARNING("Banning address: " + address);
    rakPeer_->AddToBanList(address.CString(), 0);
}

void Network::BanConnection(Connection* connection, const String& reason)
{
    using namespace NetworkBanned;
    VariantMap data = GetEventDataMap();
    data[P_REASON] = reason;
    connection->SendRemoteEvent(E_NETWORKBANNED, false, data);
    BanAddress(connection->GetIP());
}

Connection* Network::GetConnection(const SLNet::AddressOrGUID& connection) const
{
    if (serverConnection_ && serverConnection_->GetAddressOrGUID() == connection) {
        return serverConnection_;
    }
    else
    {
        HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::ConstIterator i = clientConnections_.Find(connection);
        if (i != clientConnections_.End())
            return i->second_;
        else
            return nullptr;
    }
}

Connection* Network::GetClientConnection(const SLNet::AddressOrGUID& connection) const
{
    HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::ConstIterator i = clientConnections_.Find(connection);
    if (i != clientConnections_.End())
        return i->second_;

    return nullptr;
}

Connection* Network::GetServerConnection() const
{
    return serverConnection_;
}

Vector<SharedPtr<Connection> > Network::GetClientConnections() const
{
    Vector<SharedPtr<Connection> > ret;
    for (HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::ConstIterator i = clientConnections_.Begin();
         i != clientConnections_.End(); ++i)
        ret.Push(i->second_);

    return ret;
}

bool Network::IsServerRunning() const
{
    if (!rakPeer_)
        return false;

    if (networkMode_ == PEER_TO_PEER && !isServer_) {
        return false;
    }
    return rakPeer_->IsActive() && isServer_;
}

bool Network::CheckRemoteEvent(StringHash eventType) const
{
    return allowedRemoteEvents_.Contains(eventType);
}

void Network::HandleIncomingPacket(SLNet::Packet* packet, bool isServer)
{
    unsigned char packetID = packet->data[0];

//    if (packetID < ID_USER_PACKET_ENUM) {
//        URHO3D_LOGERROR(">> " + String((int)packetID) + "  HandleIncomingPacket: " + String(RAKNET_MESSAGEID_STRINGS[packetID]));
//    }

    bool packetHandled = false;

    // Deal with timestamped backents
    unsigned dataStart = sizeof(char);
    if (packetID == ID_TIMESTAMP)
    {
        dataStart += sizeof(SLNet::Time);
        packetID = packet->data[dataStart];
        dataStart += sizeof(char);
    }

    if (packetID == ID_NEW_INCOMING_CONNECTION)
    {
        URHO3D_LOGINFOF("ID_NEW_INCOMING_CONNECTION from %s. guid=%s.", packet->systemAddress.ToString(true), packet->guid.ToString());
        if (isServer || networkMode_ == PEER_TO_PEER)
        {
            NewConnectionEstablished(packet->guid, packet->systemAddress.ToString(false));
        }

        packetHandled = true;
    }
    if (packetID == ID_REMOTE_NEW_INCOMING_CONNECTION)
    {
        URHO3D_LOGINFOF("ID_REMOTE_NEW_INCOMING_CONNECTION from %s. guid=%s.", packet->systemAddress.ToString(true), packet->guid.ToString());
        if (networkMode_ == PEER_TO_PEER)
        {
            unsigned int count;
            SLNet::BitStream bsIn(packet->data, packet->length, false);
            bsIn.IgnoreBytes(sizeof(SLNet::MessageID));
            bsIn.Read(count);
            SLNet::SystemAddress remoteAddress;
            SLNet::RakNetGUID remoteGuid;
            NewConnectionEstablished(packet->guid, packet->systemAddress.ToString(false));

            for (unsigned int i=0; i < count; i++)
            {
                bsIn.Read(remoteAddress);
                bsIn.Read(remoteGuid);
                if (!remoteAddress.EqualsExcludingPort(*natPunchServerAddress_)) {
                    URHO3D_LOGINFO("BBBB GUID " + String(remoteGuid.ToString()));
                    NewConnectionEstablished(remoteGuid, remoteAddress.ToString(false));

                }
            }
        }

        packetHandled = true;
    }
    else if (packetID == ID_REMOTE_CONNECTION_LOST || packetID == ID_REMOTE_DISCONNECTION_NOTIFICATION)
    {
        // TODO find out who's really sending out this message
        packetHandled = true;
    }
    else if (packetID == ID_ALREADY_CONNECTED)
    {
        if (natPunchServerAddress_ && packet->systemAddress == *natPunchServerAddress_) {
            URHO3D_LOGINFO("Already connected to NAT server! ");
            SendEvent(E_NATMASTERCONNECTIONSUCCEEDED);
            if (!isServer && networkMode_ == SERVER_CLIENT)
            {
                bool connecting = natPunchthroughClient_->OpenNAT(*remoteGUID_, *natPunchServerAddress_);
                if (!connecting) {
                    URHO3D_LOGERROR("Unable to do NAT punchtrough!");
                }
            }
        }
        packetHandled = true;
    }
    else if (packetID == ID_CONNECTION_REQUEST_ACCEPTED) // We're a client, our connection has been accepted
    {
        if(natPunchServerAddress_ && packet->systemAddress == *natPunchServerAddress_) {
            URHO3D_LOGINFO("Succesfully connected to NAT punchtrough server! ");
            SendEvent(E_NATMASTERCONNECTIONSUCCEEDED);
            if (!isServer && remoteGUID_ && networkMode_ == SERVER_CLIENT)
            {
                natPunchthroughClient_->OpenNAT(*remoteGUID_, *natPunchServerAddress_);
            }
        } else {
            if (networkMode_ == SERVER_CLIENT) {
                OnServerConnected(packet->guid);
            } else {
                URHO3D_LOGINFOF("ID_CONNECTION_REQUEST_ACCEPTED from %s,guid=%s", packet->systemAddress.ToString(true), packet->guid.ToString());
                // Assume that we're connecting to the P2P host
//                serverConnection_->SetAddressOrGUID(packet->guid);
//                SLNet::BitStream bsOut;
//                bsOut.Write((unsigned char) MSG_P2P_JOIN_REQUEST);
//                rakPeer_->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->guid, false);

            }
        }
        packetHandled = true;
    }
    else if (packetID == ID_NAT_TARGET_NOT_CONNECTED)
    {
        URHO3D_LOGERROR("Target server not connected to NAT master server!");
        packetHandled = true;
    }
    else if (packetID == ID_CONNECTION_LOST || packetID == ID_DISCONNECTION_NOTIFICATION) // We've lost connectivity with the packet source
    {
        if(natPunchServerAddress_ && packet->systemAddress == *natPunchServerAddress_) {
            URHO3D_LOGERROR("Connection to NAT server lost!");
            if (natAutoReconnect_) {
                URHO3D_LOGINFO("Reconnecting to NAT server");
                if (networkMode_ == PEER_TO_PEER) {
                    ConnectNAT(natPunchServerAddress_->ToString(false), natPunchServerAddress_->GetPort());
                } else {
                    rakPeer_->Connect(natPunchServerAddress_->ToString(false), natPunchServerAddress_->GetPort(), nullptr, 0);
                }
            }
        }
        else if (isServer)
        {
            ClientDisconnected(packet->guid);
        }
        else
        {
            if (networkMode_ == SERVER_CLIENT) {
                OnServerDisconnected(packet->systemAddress);
            } else {
                ClientDisconnected(packet->guid);
            }
        }
        packetHandled = true;
    }
    else if (packetID == ID_CONNECTION_ATTEMPT_FAILED) // We've failed to connect to the server/peer
    {
        if (natPunchServerAddress_ && packet->systemAddress == *natPunchServerAddress_) {
            URHO3D_LOGERROR("Connection to NAT punchtrough server failed!");
            SendEvent(E_NATMASTERCONNECTIONFAILED);

        } else {

            if (!isServer)
            {
                OnServerDisconnected(packet->systemAddress);
            }
        }
        packetHandled = true;
    }
    else if (packetID == ID_NAT_PUNCHTHROUGH_SUCCEEDED)
    {
        SLNet::SystemAddress remotePeer = packet->systemAddress;
        URHO3D_LOGINFO("NAT punchtrough succeeded! Remote peer: " + String(remotePeer.ToString()));
        if (!isServer && networkMode_ == SERVER_CLIENT)
        {
            using namespace NetworkNatPunchtroughSucceeded;
            VariantMap eventMap;
            eventMap[P_ADDRESS] = remotePeer.ToString(false);
            eventMap[P_PORT] = remotePeer.GetPort();
            SendEvent(E_NETWORKNATPUNCHTROUGHSUCCEEDED, eventMap);
            URHO3D_LOGINFO("Connecting to server behind NAT: " + String(remotePeer.ToString()));
            Connect(String(remotePeer.ToString(false)), remotePeer.GetPort(), scene_, identity_);
        } else if (networkMode_ == PEER_TO_PEER){
            SLNet::ConnectionAttemptResult car = rakPeer_->Connect(packet->systemAddress.ToString(false), packet->systemAddress.GetPort(), password_.CString(), password_.Length());
        }
        packetHandled = true;
    }
    else if (packetID == ID_NAT_PUNCHTHROUGH_FAILED || packetID == ID_NAT_TARGET_NOT_CONNECTED || packetID == ID_NAT_TARGET_UNRESPONSIVE || packetID == ID_NAT_CONNECTION_TO_TARGET_LOST)
    {
        URHO3D_LOGERROR("NAT punchtrough failed!");
        SLNet::SystemAddress remotePeer = packet->systemAddress;
        using namespace NetworkNatPunchtroughFailed;
        VariantMap eventMap;
        eventMap[P_ADDRESS] = remotePeer.ToString(false);
        eventMap[P_PORT] = remotePeer.GetPort();
        SendEvent(E_NETWORKNATPUNCHTROUGHFAILED, eventMap);
        packetHandled = true;
    }
    else if (packetID == ID_CONNECTION_BANNED) // We're a client and we're on the ban list
    {
        URHO3D_LOGERROR("Connection failed, you're banned!");
        SendEvent(E_NETWORKBANNED);
        packetHandled = true;
    }
    else if (packetID == ID_INVALID_PASSWORD) // We're a client, and we gave an invalid password
    {
        URHO3D_LOGERROR("Invalid password provided for connection!");
        SendEvent(E_NETWORKINVALIDPASSWORD);
        packetHandled = true;
    }
    else if (packetID == ID_DOWNLOAD_PROGRESS) // Part of a file transfer
    {
        //URHO3D_LOGINFO("101010");
    }
    else if (packetID == ID_UNCONNECTED_PING)
    {
        packetHandled = true;
    }
    else if (packetID == ID_READY_EVENT_SET || packetID == ID_READY_EVENT_UNSET || packetID == ID_READY_EVENT_ALL_SET || packetID == ID_READY_EVENT_QUERY || packetID == ID_READY_EVENT_FORCE_ALL_SET)
    {
        ReadyStatusChanged();
        packetHandled = true;
    }
    else if (packetID == ID_UNCONNECTED_PONG) // Host discovery response
    {
        if (!isServer)
        {
            using namespace NetworkHostDiscovered;

            dataStart += sizeof(SLNet::TimeMS);
            VariantMap& eventMap = context_->GetEventDataMap();
            if (packet->length > packet->length - dataStart) {
                VectorBuffer buffer(packet->data + dataStart, packet->length - dataStart);
                VariantMap srcData = buffer.ReadVariantMap();
                eventMap[P_BEACON] = srcData;
            }
            else {
                eventMap[P_BEACON] = VariantMap();
            }

            eventMap[P_ADDRESS] = String(packet->systemAddress.ToString(false));
            eventMap[P_PORT] = (int)packet->systemAddress.GetPort();
            SendEvent(E_NETWORKHOSTDISCOVERED, eventMap);
        }
        packetHandled = true;
    }
    else if (packetID == ID_FCM2_REQUEST_FCMGUID)
    {
        if (networkMode_ == SERVER_CLIENT) {
            URHO3D_LOGERROR("Network mode mismatch, disconnecting!");
            OnServerDisconnected(packet->systemAddress);
            SendEvent(E_NETWORKMODEMISMATCH);
            packetHandled = true;
        }
    }
    else if (packetID == ID_FCM2_NEW_HOST)
    {
        SLNet::BitStream bs(packet->data,packet->length,false);
        bs.IgnoreBytes(1);
        SLNet::RakNetGUID oldHost;
        bs.Read(oldHost);

        if (serverConnection_) {
            serverConnection_->SetAddressOrGUID(packet->guid);
        }
        hostGuid_ = packet->guid.ToString();
        if (packet->guid == rakPeer_->GetMyGUID())
        {
            isServer_ = true;
            if (oldHost != SLNet::UNASSIGNED_RAKNET_GUID)
            {
                URHO3D_LOGINFOF("ID_FCM2_NEW_HOST: Taking over as host from the old host [%s].", oldHost.ToString());
            }
            else
            {
                // Room not hosted if we become host the first time since this was done in CreateRoom() already
                URHO3D_LOGINFO("ID_FCM2_NEW_HOST: We have become host for the first time");
            }

            for (auto it = clientConnections_.Begin(); it != clientConnections_.End(); ++it) {
                URHO3D_LOGINFO("Setting new scene for clients");
//                (*it).second_->SetScene(serverConnection_->GetScene());
                //TODO decide what to do when we take ownership as the host, should the scene needs to be reloaded?
                (*it).second_->SetSceneLoaded(true);
                //(*it).second_->SetScene(serverConnection_->GetScene());
            }
        }
        else
        {
            isServer_ = false;
            if (oldHost != SLNet::UNASSIGNED_RAKNET_GUID) {
                URHO3D_LOGINFOF("ID_FCM2_NEW_HOST: A new system %s has become host, GUID=%s", packet->systemAddress.ToString(true), packet->guid.ToString());
            }
            else {
                URHO3D_LOGINFOF("ID_FCM2_NEW_HOST: System %s is host, GUID=%s", packet->systemAddress.ToString(true), packet->guid.ToString());
            }

//            // Send the identity map now to our new host
            VectorBuffer msg;
            msg.WriteVariantMap(serverConnection_->GetIdentity());
            serverConnection_->SendMessage(MSG_IDENTITY, true, true, msg);
        }

        using namespace P2PNewHost;
        VariantMap data = GetEventDataMap();
        data[P_ADDRESS] = packet->systemAddress.ToString(false);
        data[P_PORT] = packet->systemAddress.GetPort();
        data[P_GUID] = packet->guid.ToString();
        SendEvent(E_P2PNEWHOST, data);
        ReadyStatusChanged();

        packetHandled = true;
    }
    else if (packetID == ID_FCM2_VERIFIED_JOIN_START) {

        URHO3D_LOGINFO("ID_FCM2_VERIFIED_JOIN_START");
        packetHandled = true;
    }
    else if (packetID == ID_NO_FREE_INCOMING_CONNECTIONS)
    {
        URHO3D_LOGERROR("Server is full!");
        Disconnect(1000);
        SendEvent(E_SERVERFULL);
        packetHandled = true;
    }
    else if (packetID == ID_FCM2_VERIFIED_JOIN_CAPABLE)
    {
        URHO3D_LOGINFO("ID_FCM2_VERIFIED_JOIN_CAPABLE");
//        fullyConnectedMesh2_->RespondOnVerifiedJoinCapable(packet, true, nullptr);
        packetHandled = true;
    }
    else if (packetID == ID_FCM2_VERIFIED_JOIN_ACCEPTED)
    {
//        DataStructures::List<SLNet::RakNetGUID> systemsAccepted;
//        bool thisSystemAccepted;
//        fullyConnectedMesh2_->GetVerifiedJoinAcceptedAdditionalData(packet, &thisSystemAccepted, systemsAccepted, nullptr);
//        if (thisSystemAccepted) {
//            URHO3D_LOGINFO("Game join request accepted");
//        }
        URHO3D_LOGINFO("ID_FCM2_VERIFIED_JOIN_ACCEPTED");
        packetHandled = true;
    }
    else if (packetID == ID_FCM2_VERIFIED_JOIN_REJECTED)
    {
        URHO3D_LOGINFO("ID_FCM2_VERIFIED_JOIN_REJECTED");
    }
    else if (packetID == ID_FCM2_REQUEST_FCMGUID)
    {
        URHO3D_LOGINFO("ID_FCM2_REQUEST_FCMGUID");
    }
    else if (packetID == ID_FCM2_RESPOND_CONNECTION_COUNT)
    {
        URHO3D_LOGINFO("ID_FCM2_RESPOND_CONNECTION_COUNT");
    }
    else if (packetID == ID_FCM2_INFORM_FCMGUID)
    {
        URHO3D_LOGINFO("ID_FCM2_INFORM_FCMGUID");
    }
    else if (packetID == ID_FCM2_UPDATE_MIN_TOTAL_CONNECTION_COUNT)
    {
        URHO3D_LOGINFO("ID_FCM2_UPDATE_MIN_TOTAL_CONNECTION_COUNT");
    }
    else if (packetID == ID_FCM2_UPDATE_USER_CONTEXT)
    {
        URHO3D_LOGINFO("ID_FCM2_UPDATE_USER_CONTEXT");
    }
    // Urho3D messages
    if (packetID >= ID_USER_PACKET_ENUM)
    {
        unsigned int messageID = *(unsigned int*)(packet->data + dataStart);
        dataStart += sizeof(unsigned int);
        //URHO3D_LOGINFOF("ID_USER_PACKET_ENUM %i", packetID);
        if (packetID == MSG_P2P_JOIN_REQUEST) {
            URHO3D_LOGINFO("MSG_P2P_JOIN_REQUEST");
//            if (allowedConnectionCount_ > 0 && GetClientConnections().Size() < allowedConnectionCount_) {
////                fullyConnectedMesh2_->StartVerifiedJoin(packet->guid);
//                URHO3D_LOGWARNING("StartVerifiedJoin SUCCESS");
//            } else {
//                VectorBuffer msg;
//                GetConnection(packet->guid)->SendMessage(MSG_P2P_JOIN_REQUEST_DENIED, true, true, msg);
//                ClientDisconnected(packet->guid);
//                URHO3D_LOGWARNING("StartVerifiedJoin FAILED");
//            }
        }
        else if (networkMode_ == PEER_TO_PEER && IsHostSystem())
        {
            // We are the host in the P2P server, parse the message accordingly
            HandleMessage(packet->guid, 0, messageID, (const char*)(packet->data + dataStart), packet->length - dataStart);
        }
        else if (networkMode_ == SERVER_CLIENT && isServer_) {
            // We are the server in the server-client connection
            HandleMessage(packet->systemAddress, 0, messageID, (const char*)(packet->data + dataStart), packet->length - dataStart);
        }
        else {
            // we are client in either P2P or server-client mode
            MemoryBuffer buffer(packet->data + dataStart, packet->length - dataStart);
            bool processed = serverConnection_ && serverConnection_->ProcessMessage(messageID, buffer);
            if (!processed)
            {
                HandleMessage(packet->systemAddress, 0, messageID, (const char*)(packet->data + dataStart), packet->length - dataStart);
            }
        }
        packetHandled = true;
    }

    if (!packetHandled && packetID < sizeof(RAKNET_MESSAGEID_STRINGS))
        URHO3D_LOGERROR("Unhandled network packet: " + String(RAKNET_MESSAGEID_STRINGS[packetID]));
    else if (!packetHandled)
        URHO3D_LOGERRORF("Unhandled network packet: %i", packetID);

}

void Network::Update(float timeStep)
{
    URHO3D_PROFILE(UpdateNetwork);

    //Process all incoming messages for the server
    if (rakPeer_->IsActive())
    {
        while (SLNet::Packet* packet = rakPeer_->Receive())
        {
            if (IsHostSystem() || networkMode_ == SERVER_CLIENT) {
                // Process all messages as server/host
                HandleIncomingPacket(packet, true);
            }
            else {
                // Process messages as a client/peer
                HandleIncomingPacket(packet, false);
            }
            // Check if interface is still active, ban could happen while the message was processing
            if (rakPeer_->IsActive()) {
                rakPeer_->DeallocatePacket(packet);
            }
        }
    }

    // Process all incoming messages for the client, server-client mode only
    if (rakPeerClient_->IsActive())
    {
        while (SLNet::Packet* packet = rakPeerClient_->Receive())
        {
            HandleIncomingPacket(packet, false);
            if (rakPeerClient_->IsActive()) {
                rakPeerClient_->DeallocatePacket(packet);
            }
        }
    }
}

void Network::PostUpdate(float timeStep)
{
    URHO3D_PROFILE(PostUpdateNetwork);

    // Check if periodic update should happen now
    updateAcc_ += timeStep;
    bool updateNow = updateAcc_ >= updateInterval_;
    if (updateNow)
    {
        // Notify of the impending update to allow for example updated client controls to be set
        SendEvent(E_NETWORKUPDATE);
        updateAcc_ = fmodf(updateAcc_, updateInterval_);

        if (IsServerRunning())
        {
            // Collect and prepare all networked scenes
            {
                URHO3D_PROFILE(PrepareServerUpdate);

                networkScenes_.Clear();
                for (HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::Iterator i = clientConnections_.Begin();
                     i != clientConnections_.End(); ++i)
                {
                    Scene* scene = i->second_->GetScene();
                    if (scene)
                        networkScenes_.Insert(scene);
                }

                for (HashSet<Scene*>::ConstIterator i = networkScenes_.Begin(); i != networkScenes_.End(); ++i)
                    (*i)->PrepareNetworkUpdate();
            }

            {
                URHO3D_PROFILE(SendServerUpdate);

                // Then send server updates for each client connection
                for (HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::Iterator i = clientConnections_.Begin();
                     i != clientConnections_.End(); ++i)
                {
                    i->second_->SendServerUpdate();
                    i->second_->SendRemoteEvents();
                    i->second_->SendPackages();
                    i->second_->SendAllBuffers();
                }
            }
        }

        if (serverConnection_)
        {
            // No need to send out updates if we're the host system
            if (networkMode_ == PEER_TO_PEER && !IsHostSystem()) {
                // Send the client update
                serverConnection_->SendClientUpdate();
                serverConnection_->SendRemoteEvents();
                serverConnection_->SendAllBuffers();
            } else if (networkMode_ == SERVER_CLIENT) {
                // Send the client update
                serverConnection_->SendClientUpdate();
                serverConnection_->SendRemoteEvents();
                serverConnection_->SendAllBuffers();
            }
        }

        // Notify that the update was sent
        SendEvent(E_NETWORKUPDATESENT);
    }
}

void Network::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    using namespace BeginFrame;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void Network::HandleRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace RenderUpdate;

    PostUpdate(eventData[P_TIMESTEP].GetFloat());
}

void Network::OnServerConnected(const SLNet::AddressOrGUID& address)
{
    serverConnection_->SetConnectPending(false);
    serverConnection_->SetAddressOrGUID(address);
    URHO3D_LOGINFO("Connected to server!");

    // Send the identity map now
    VectorBuffer msg;
    msg.WriteVariantMap(serverConnection_->GetIdentity());
    serverConnection_->SendMessage(MSG_IDENTITY, true, true, msg);

    SendEvent(E_SERVERCONNECTED);
}

void Network::OnServerDisconnected(const SLNet::AddressOrGUID& address)
{
    if (networkMode_ == PEER_TO_PEER) {
        return;
    }

    if (!serverConnection_) {
        return;
    }

    if (natPunchServerAddress_ && *natPunchServerAddress_ == address.systemAddress) {
        SendEvent(E_NATMASTERDISCONNECTED);
        return;
    }

    // Differentiate between failed connection, and disconnection
    bool failedConnect = serverConnection_ && serverConnection_->IsConnectPending();
    serverConnection_.Reset();

    if (!failedConnect)
    {
        URHO3D_LOGINFO("Disconnected from server");
        SendEvent(E_SERVERDISCONNECTED);
    }
    else
    {
        URHO3D_LOGERROR("Failed to connect to server");
        SendEvent(E_CONNECTFAILED);
    }

//    if (rakPeerClient_->IsActive()) {
        rakPeerClient_->Shutdown(0);
//    }

    natPunchthroughClient_->Clear();
}

void Network::ConfigureNetworkSimulator()
{
    if (serverConnection_)
        serverConnection_->ConfigureNetworkSimulator(simulatedLatency_, simulatedPacketLoss_);

    for (HashMap<SLNet::AddressOrGUID, SharedPtr<Connection> >::Iterator i = clientConnections_.Begin();
         i != clientConnections_.End(); ++i)
        i->second_->ConfigureNetworkSimulator(simulatedLatency_, simulatedPacketLoss_);
}

bool Network::StartSession(Scene* scene, const VariantMap& identity)
{
    if (networkMode_ == SERVER_CLIENT) {
        URHO3D_LOGERROR("P2P sessions are not available for SERVER_CLIENT mode!");
        return false;
    }

    if (!natPunchServerAddress_) {
        URHO3D_LOGERROR("Set the NAT server info first!");
        return false;
    }

    UnsubscribeFromEvent(E_NATMASTERCONNECTIONSUCCEEDED);
    SubscribeToEvent(E_NATMASTERCONNECTIONSUCCEEDED, URHO3D_HANDLER(Network, HandleNATStartSession));

    scene_ = scene;
    identity_ = identity;

    ConnectNAT(natPunchServerAddress_->ToString(false), natPunchServerAddress_->GetPort());

    guid_ = String(rakPeer_->GetGuidFromSystemAddress(SLNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());

    return true;

}

void Network::HandleNATStartSession(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(E_NATMASTERCONNECTIONSUCCEEDED);

    if (serverConnection_) {
        serverConnection_.Reset();
    }

    serverConnection_ = new Connection(context_, false, rakPeer_->GetMyGUID(), rakPeer_);
    serverConnection_->SetScene(scene_);
    serverConnection_->SetSceneLoaded(true);
    serverConnection_->SetIdentity(identity_);
    serverConnection_->SetConnectPending(true);
    serverConnection_->ConfigureNetworkSimulator(simulatedLatency_, simulatedPacketLoss_);

    rakPeer_->SetOccasionalPing(true);
    fullyConnectedMesh2_->Clear();
    fullyConnectedMesh2_->ResetHostCalculation();

    hostGuid_ = GetGUID();
    isServer_ = true;
    SetReady(false);

    SendEvent(E_P2PSESSIONSTARTED);
}

void Network::JoinSession(const String& guid, Scene* scene, const VariantMap& identity)
{
    if (networkMode_ == SERVER_CLIENT) {
        URHO3D_LOGERROR("P2P sessions are not available for SERVER_CLIENT mode!");
        return;
    }

    if (!natPunchServerAddress_) {
        URHO3D_LOGERROR("Set the NAT server info first!");
        return;
    }

    UnsubscribeFromEvent(E_NATMASTERCONNECTIONSUCCEEDED);
    SubscribeToEvent(E_NATMASTERCONNECTIONSUCCEEDED, URHO3D_HANDLER(Network, HandleNATJoinSession));

    if (remoteGUID_) {
        delete remoteGUID_;
        remoteGUID_ = nullptr;
    }

    remoteGUID_ = new SLNet::RakNetGUID;
    remoteGUID_->FromString(guid.CString());
    hostGuid_ = guid;

    scene_ = scene;
    identity_ = identity;

    ConnectNAT(natPunchServerAddress_->ToString(false), natPunchServerAddress_->GetPort());

    guid_ = String(rakPeer_->GetGuidFromSystemAddress(SLNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());
}

void Network::HandleNATJoinSession(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(E_NATMASTERCONNECTIONSUCCEEDED);
    SetReady(false);

    if (serverConnection_) {
        serverConnection_.Reset();
    }

    serverConnection_ = new Connection(context_, false, rakPeer_->GetMyBoundAddress(), rakPeer_);
    serverConnection_->SetScene(scene_);
    serverConnection_->SetSceneLoaded(true);
    serverConnection_->SetIdentity(identity_);
    serverConnection_->SetConnectPending(true);
    serverConnection_->SetAddressOrGUID(*remoteGUID_);
    serverConnection_->ConfigureNetworkSimulator(simulatedLatency_, simulatedPacketLoss_);

    rakPeer_->SetOccasionalPing(true);
    fullyConnectedMesh2_->ResetHostCalculation();
    fullyConnectedMesh2_->Clear();

    if (natPunchthroughServerClient_->OpenNAT(*remoteGUID_, *natPunchServerAddress_)) {
        SendEvent(E_P2PSESSIONJOINED);
    } else {
        URHO3D_LOGERROR("P2P join attempt failed!");
        SendEvent(E_P2PSESSIONJOINFAILED);
    }

    ReadyStatusChanged();
}

int Network::GetParticipantCount()
{
    if (networkMode_ == SERVER_CLIENT) {
        return GetClientConnections().Size();
    }
    if (fullyConnectedMesh2_) {
        return fullyConnectedMesh2_->GetParticipantCount();
    }
    return 0;
}

bool Network::IsConnectedHost()
{
    if (networkMode_ == SERVER_CLIENT) {
        return false;
    }
    return fullyConnectedMesh2_->IsConnectedHost();
}

bool Network::IsHostSystem()
{
    if (networkMode_ == SERVER_CLIENT) {
        return IsServerRunning();
    }
    return fullyConnectedMesh2_->IsHostSystem();
}

const String Network::GetHostAddress()
{
    if (networkMode_ == SERVER_CLIENT) {
        return rakPeer_->GetMyGUID().ToString();
    }
    return hostGuid_;
}

void Network::SetReady(bool value)
{
    if (networkMode_ == SERVER_CLIENT) {
        return;
    }

    readyEvent_->SetEvent(0, value);
    ReadyStatusChanged();
}

bool Network::GetReady()
{
    if (networkMode_ == SERVER_CLIENT) {
        return false;
    }
    return readyEvent_->IsEventSet(0);
}

void Network::ReadyStatusChanged()
{
    if (networkMode_ == SERVER_CLIENT) {
        return;
    }
    DataStructures::List<SLNet::RakNetGUID> participantList;
    fullyConnectedMesh2_->GetParticipantList(participantList);
    for (unsigned int i = 0; i < participantList.Size(); i++) {
        if (participantList[i] != rakPeerClient_->GetMyGUID()) {
            readyEvent_->AddToWaitList(0, participantList[i]);
        }
    }

    fullyConnectedMesh2_->GetParticipantList(participantList);
    bool allReady = true;
    for (unsigned int i = 0; i < participantList.Size(); i++) {
        if (participantList[i] != rakPeer_->GetMyGUID()) {
            int ready = readyEvent_->GetReadyStatus(0, participantList[i]);
            Connection* connection = GetClientConnection(participantList[i]);
            if (connection) {
                if (ready != SLNet::RES_ALL_READY && ready != SLNet::RES_READY) {
                    if (connection) {
                        connection->SetReady(false);
                    }
                    allReady = false;
                } else {
                    if (connection) {
                        connection->SetReady(true);
                    }
                }
            }
        }
    }

    VariantMap data = GetEventDataMap();
    if (serverConnection_) {
        if (readyEvent_->IsEventSet(0)) {
            serverConnection_->SetReady(true);
        }
        else {
            serverConnection_->SetReady(false);
        }
    }
    if (allReady && readyEvent_->IsEventSet(0)) {
        data[P2PAllReadyChanged::P_READY] = true;
    } else {
        data[P2PAllReadyChanged::P_READY] = false;
    }

    SendEvent(E_P2PALLREADYCHANGED, data);

}

void Network::ResetHost()
{
    if (networkMode_ == SERVER_CLIENT) {
        return;
    }
    fullyConnectedMesh2_->ResetHostCalculation();
}

void Network::SetMode(NetworkMode mode, bool force)
{
    if (rakPeer_->IsActive() || rakPeerClient_->IsActive()) {
        URHO3D_LOGERROR("Failed to change network mode! Shutdown networking system first or use forced mode changing!");
        return;
    }

    if (force) {
        if (rakPeer_->IsActive()) {
            rakPeer_->Shutdown(100);
        }
        if (rakPeerClient_->IsActive()) {
            rakPeerClient_->Shutdown(100);
        }
        clientConnections_.Clear();
        serverConnection_.Reset();
    }
    networkMode_ = mode;

    if (networkMode_ == PEER_TO_PEER) {
        if (fullyConnectedMesh2_) {
            rakPeer_->DetachPlugin(fullyConnectedMesh2_);
            SLNet::FullyConnectedMesh2::DestroyInstance(fullyConnectedMesh2_);
        }
        fullyConnectedMesh2_ = SLNet::FullyConnectedMesh2::GetInstance();
        rakPeer_->AttachPlugin(fullyConnectedMesh2_);
        fullyConnectedMesh2_->SetAutoparticipateConnections(true);
        fullyConnectedMesh2_->SetConnectOnNewRemoteConnection(true, password_.CString());

        if (readyEvent_) {
            rakPeer_->DetachPlugin(readyEvent_);
            SLNet::ReadyEvent::DestroyInstance(readyEvent_);
        }
        readyEvent_ = SLNet::ReadyEvent::GetInstance();
        rakPeer_->AttachPlugin(readyEvent_);

        if (connectionGraph2_) {
            rakPeer_->DetachPlugin(connectionGraph2_);
            SLNet::ConnectionGraph2::DestroyInstance(connectionGraph2_);
        }
        connectionGraph2_ = SLNet::ConnectionGraph2::GetInstance();
        rakPeer_->AttachPlugin(connectionGraph2_);

    } else {
        if (fullyConnectedMesh2_) {
            rakPeer_->DetachPlugin(fullyConnectedMesh2_);
            SLNet::FullyConnectedMesh2::DestroyInstance(fullyConnectedMesh2_);
            fullyConnectedMesh2_ = nullptr;
        }
        if (readyEvent_) {
            rakPeer_->DetachPlugin(readyEvent_);
            SLNet::ReadyEvent::DestroyInstance(readyEvent_);
            readyEvent_ = nullptr;
        }
        if (connectionGraph2_) {
            rakPeer_->DetachPlugin(connectionGraph2_);
            SLNet::ConnectionGraph2::DestroyInstance(connectionGraph2_);
            connectionGraph2_ = nullptr;
        }
    }
}

void Network::SetNATAutoReconnect(bool retry)
{
    natAutoReconnect_ = retry;
}

void Network::SetAllowedConnections(unsigned short connectionCount)
{
    URHO3D_LOGINFO("Max incoming connection changed to " + String(connectionCount));
    rakPeer_->SetMaximumIncomingConnections(connectionCount);
    allowedConnectionCount_ = connectionCount;
}

void RegisterNetworkLibrary(Context* context)
{
    NetworkPriority::RegisterObject(context);
}

}
