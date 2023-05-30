#include "stemlib.hpp"

namespace stemlib
{
	template<typename T, std::endian to_endianness = std::endian::big>
	ByteVector toBytes(T value)
	{
		ByteVector buffer(sizeof(T));

		std::copy_n(reinterpret_cast<uint8_t*>(&value), sizeof(T), buffer.begin());

		if constexpr (std::endian::native != to_endianness)
			std::reverse(buffer.begin(), buffer.end());

		return buffer;
	}

	template<typename T, std::endian from_endianness = std::endian::big>
	T fromBytes(ByteVector bytes)
	{
		if constexpr (std::endian::native != from_endianness)
			std::reverse(bytes.begin(), bytes.end());

		T* buffer = reinterpret_cast<T*>(bytes.data());

		return *buffer;
	}

	void Client::sendNotifications(Channel channel, ByteVector message)
	{
		if (!channels_and_listeners.contains(channel))
			return;

		for (auto& listener : channels_and_listeners[channel])
			(*listener)(channel, message);

	}

	ByteVector receiveBuffer(Poco::Net::StreamSocket socket, size_t amount)
	{
		ByteVector buffer(amount, 0);
		socket.receiveBytes(buffer.data(), buffer.size());

		return buffer;
	}

	void Client::receiveMessages()
	{
		while (true)
		{
			uint16_t packet_size;
			ByteVector buffer;

		wait_for_packet:
			if (stop_receiving)
				return;
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(20));

			if (socket.available() < 2)
				goto wait_for_packet;

			packet_size = fromBytes<uint16_t>(receiveBuffer(socket, 2));

		wait_for_data:
			if (stop_receiving)
				return;
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(20));

			if (socket.available() < packet_size)
				goto wait_for_data;

			buffer = receiveBuffer(socket, packet_size);

			switch (buffer[0])
			{
			case MESSAGE:
			{
				uint16_t channel_size = *(buffer.begin() + 1);
				Channel channel = std::string(buffer.begin() + 2, buffer.begin() + 2 + channel_size);
				ByteVector message = ByteVector(buffer.begin() + 2 + channel_size, buffer.end());

				sendNotifications(channel, message);
			}
			case PING:
			{
				ByteVector data(buffer.begin() + 1, buffer.end());
				sendTennisPacket(data, true);
			}
			}
		}
	}

	void Client::sendBasicData(PacketKind kind, ByteVector data)
	{
		if (data.size() > (std::numeric_limits<uint16_t>::max)() - 1)
			throw std::exception("Data is too big.");

		uint16_t packet_size = 1 + data.size();

		ByteVector buffer(toBytes<uint16_t>(packet_size));
		buffer.reserve(packet_size);

		buffer.push_back(kind);
		buffer.insert(buffer.end(), data.begin(), data.end());

		socket.sendBytes(buffer.data(), buffer.size());
	}

	void Client::sendMessagePacket(Channel channel, ByteVector message)
	{
		if (channel.size() > (std::numeric_limits<uint8_t>::max)())
			throw std::exception("Channel is too big.");

		ByteVector buffer(toBytes<uint8_t>(channel.size()));
		buffer.insert(buffer.end(), channel.begin(), channel.end());
		buffer.insert(buffer.end(), message.begin(), message.end());

		sendBasicData(MESSAGE, buffer);
	}

	void Client::sendSubscriptionPacket(Channel channel, bool subscribe)
	{
		if (channel.size() > (std::numeric_limits<uint8_t>::max)())
			throw std::exception("Channel is too big.");

		ByteVector buffer(toBytes<uint8_t>(channel.size()));
		buffer.insert(buffer.end(), channel.begin(), channel.end());

		sendBasicData(subscribe ? SUBSCRIBE : UNSUBSCRIBE, buffer);
	}

	void Client::sendTennisPacket(ByteVector data, bool reply)
	{
		sendBasicData(reply ? PONG : PING, data);
	}

	Client::Client(std::string host, uint16_t port) : endpoint(host, port)
	{
		socket.connect(endpoint);
		server_receiver = std::thread([&]() { receiveMessages(); });
	}

	Client::~Client()
	{
		stop_receiving = true;
		server_receiver.join();
	}

	void Client::sendMessage(Channel channel, ByteVector message, bool loopback)
	{
		sendMessagePacket(channel, message);

		if (loopback)
			sendNotifications(channel, message);
	}

	void Client::sendMessage(Channel channel, std::string message, bool loopback)
	{
		sendMessage(channel, ByteVector(message.begin(), message.end()), loopback);
	}

	bool Client::subscribeListener(Channel channel, SharedListener listener)
	{
		if (channels_and_listeners.contains(channel))
		{
			if (channels_and_listeners[channel].contains(listener))
				return false;

			channels_and_listeners[channel].insert(listener);
			return true;
		}
		else
		{
			channels_and_listeners[channel] = { listener };
			sendSubscriptionPacket(channel, true);

			return true;
		}
	}

	bool Client::unsubscribeListener(Channel channel, SharedListener listener)
	{
		if (!channels_and_listeners.contains(channel))
			return false;

		if (!channels_and_listeners[channel].contains(listener))
			return false;

		channels_and_listeners[channel].erase(listener);

		if (channels_and_listeners[channel].empty())
		{
			channels_and_listeners.erase(channel);
			sendSubscriptionPacket(channel, false);
		}

		return true;
	}
}