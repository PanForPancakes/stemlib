#pragma once

#include <functional>
#include <chrono>
#include <thread>
#include <string>
#include <map>
#include <set>

#include <Poco/Net/SocketStream.h>
#include <Poco/Net/StreamSocketImpl.h>

namespace stemlib
{
	/// <summary>
	/// Alias for std::shared_ptr
	/// </summary>
	template<typename T>
	using Shared = std::shared_ptr<T>;

	/// <summary>
	/// Alias for std::weak_ptr
	/// </summary>
	template<typename T>
	using Weak = std::weak_ptr<T>;

	/// <summary>
	/// Alias for std::vector&lt;uint8_t&gt;
	/// </summary>
	typedef std::vector<uint8_t> ByteVector;

	/// <summary>
	/// Alias for std::chrono::duration&lt;float_t&gt;
	/// </summary>
	typedef std::chrono::duration<float_t> TimeUnit;

	/// <summary>
	/// Packet types available in Stem protocol.
	/// This enum could be converted to uint8_t to get third Stem packet byte.
	/// </summary>
	enum PacketKind : uint8_t { MESSAGE, SUBSCRIBE, UNSUBSCRIBE, PING, PONG };

	/// <summary>
	/// Currently an alias for std::string.
	/// If used as "Stem Channel" string object should be no longer than 255 bytes.
	/// TODO: replace with struct/class.
	/// </summary>
	typedef std::string Channel;

	/// <summary>
	/// Type definition for channel listeners that should accept Channel object
	/// and ByteVector containing arbitrary data received and loopbacked.
	/// </summary>
	typedef std::function<void(Channel, ByteVector)> Listener;

	/// <summary>
	/// Alias for Shared&lt;Listener&gt;
	/// </summary>
	typedef Shared<Listener> SharedListener;

	/// <summary>
	/// Alias for Weak&lt;Listener&gt;
	/// </summary>
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

		/// <summary>
		/// Sends a byte vector message to the server and can loopback message
		/// to notify listeners that use this client instance and listen to
		/// specified channel
		/// </summary>
		void sendMessage(Channel channel, ByteVector message, bool loopback = false);

		/// <summary>
		/// Sends a string message to the server and can loopback message
		/// to notify listeners that use this client instance and listen to
		/// specified channel
		/// </summary>
		void sendMessage(Channel channel, std::string message, bool loopback = false);

		/// <summary>
		/// Subscribes shared listener to the specific channel.
		/// One shared listener can listen to many channels at the same time.
		/// </summary>
		bool subscribeListener(Channel channel, SharedListener listener);

		/// <summary>
		/// Unsubscribes shared listener from specific channel.
		/// </summary>
		bool unsubscribeListener(Channel channel, SharedListener listener);
	};
}