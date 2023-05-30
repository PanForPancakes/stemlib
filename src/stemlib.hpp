#pragma once

#include <Poco/Net/SocketStream.h>
#include <Poco/Net/StreamSocketImpl.h>

#include <functional>
#include <chrono>
#include <thread>
#include <string>
#include <set>
#include <map>

namespace stemlib
{
	template<typename T>
	using Shared = std::shared_ptr<T>;

	template<typename T>
	using Weak = std::weak_ptr<T>;

	typedef std::vector<uint8_t> ByteVector;
	typedef std::chrono::duration<float_t> TimeUnit;

	enum PacketKind : uint8_t { MESSAGE, SUBSCRIBE, UNSUBSCRIBE, PING, PONG };

	typedef std::string Channel;

	/// <summary>
	/// Type definition for channel listeners that should accept Channel object
	/// and ByteVector containing arbitrary data received and loopbacked
	/// </summary>
	typedef std::function<void(Channel, ByteVector)> Listener;

	typedef Shared<Listener> SharedListener;
	typedef Weak<Listener> WeakListener;

	class Client
	{
		std::map<Channel, std::set<SharedListener>> channels_and_listeners;

		void sendNotifications(Channel channel, ByteVector message);

		Poco::Net::SocketAddress endpoint;
		Poco::Net::StreamSocket socket;

		void receiveMessages();

		std::atomic_bool stop_receiving = false;
		std::thread server_receiver;

		void sendBasicData(PacketKind kind, ByteVector data);

		void sendSubscriptionPacket(Channel channel, bool subscribe);
		void sendMessagePacket(Channel channel, ByteVector message);
		void sendTennisPacket(ByteVector data, bool reply);

	public:
		Client(std::string host = "stem.fomalhaut.me", uint16_t port = 5733);
		~Client();

		void sendMessage(Channel channel, ByteVector message, bool loopback = false);
		void sendMessage(Channel channel, std::string message, bool loopback = false);

		bool subscribeListener(Channel channel, SharedListener listener);
		bool unsubscribeListener(Channel channel, SharedListener listener);
	};
}