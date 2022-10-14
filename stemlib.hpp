#pragma once

#include <Poco/Net/SocketStream.h>
#include <Poco/Net/StreamSocketImpl.h>

#include <iostream>
#include <functional>
#include <chrono>
#include <thread>
#include <string>
#include <unordered_set>
#include <unordered_map>

using namespace std::chrono;

namespace stem
{
	class client
	{
	public:
		typedef std::function<void(std::string /*channel*/, std::string /*data*/)> callback;
		typedef std::weak_ptr<std::function<void(std::string /*channel*/, std::string /*data*/)>> callback_ptr;

	private:
		enum packet_type { __message__, __subscribe__, __unsubscribe__, __ping__, __pong__ };

		struct packet
		{
			uint8_t type;

			std::string channel;
			std::string data;
		};

		Poco::Net::StreamSocket socket;
		Poco::Net::SocketAddress server;

		std::unordered_map<std::string /*channel*/, uint16_t /*subscribers*/> subscribed_channels;
		std::unordered_map<std::shared_ptr<callback> /*listener*/, std::unordered_set<std::string> /*subscriptions*/> message_listeners;

		struct
		{
			std::mutex pong_mutex;
			std::condition_variable pong_cv;
			std::pair<std::string, steady_clock::time_point> pong_response;
		} pong_data;

		void write(packet pack)
		{
			uint16_t size = 1 + pack.data.size() + (pack.type == __ping__ || pack.type == __pong__ ? 0 : pack.channel.size() + 1);

			std::vector<uint8_t> buffer;

			buffer.insert(buffer.end(), {static_cast<uint8_t>(size >> 8), static_cast<uint8_t>(size & 0xFF), pack.type});

			if (!(pack.type == __ping__ || pack.type == __pong__))
			{
				buffer.insert(buffer.end(), { static_cast<uint8_t>(pack.channel.size() & 0xFF) });
				buffer.insert(buffer.end(), pack.channel.begin(), pack.channel.end());
			}

			buffer.insert(buffer.end(), pack.data.begin(), pack.data.end());

			socket.sendBytes(buffer.data(), buffer.size());
		}

		packet read()
		{
			uint8_t buff_size[2];
			socket.receiveBytes(&buff_size, 2);

			uint16_t data_size = buff_size[0] << 8 | buff_size[1];

			std::vector<uint8_t> buffer(data_size);

			socket.receiveBytes(buffer.data(), data_size);

			packet p;
			p.type = buffer[0];

			if (!(p.type == __ping__ || p.type == __pong__))
			{
				uint8_t id_size = buffer[1];
				p.channel.assign(buffer.begin() + 2, buffer.begin() + id_size + 2);
				p.data.assign(buffer.begin() + id_size + 2, buffer.end());
			}
			else
			{
				p.data.assign(buffer.begin() + 1, buffer.end());
			}

			return p;
		}

		void on_packet(packet pack)
		{
			switch (pack.type)
			{
			case __message__:
				for (auto& listener : message_listeners)
				{
					if (listener.second.contains(pack.channel))
						(*listener.first)(pack.channel, pack.data);
				}
				break;
			case __ping__:
				pack.type = __pong__;
				write(pack);
				break;
			case __pong__:
				std::lock_guard<std::mutex> lock(pong_data.pong_mutex);
				pong_data.pong_response.first = pack.data;
				pong_data.pong_response.second = high_resolution_clock::now();
				pong_data.pong_cv.notify_one();
				break;
			}
		}

		bool is_subscribed(std::string channel)
		{
			return subscribed_channels.contains(channel);
		}

		void subscribe(std::string channel)
		{
			auto subscriptions_search = subscribed_channels.find(channel);

			if (subscriptions_search != subscribed_channels.end())
			{
				subscribed_channels[subscriptions_search->first]++;
				return;
			}
			else
			{
				subscribed_channels.emplace(channel, 1);
			}

			packet request;
			request.type = __subscribe__;
			request.channel = channel;

			write(request);
		}

		void unsubscribe(std::string channel)
		{
			if (!is_subscribed(channel))
				return;

			auto subscriptions_search = subscribed_channels.find(channel);

			if (subscriptions_search->second > 1)
				subscribed_channels[subscriptions_search->first]--;
			else
				subscribed_channels.erase(channel);

			packet request;
			request.type = __unsubscribe__;
			request.channel = channel;

			write(request);
		}

	public:
		client(std::string address, uint16_t port, bool connect = true) : server(address, port)
		{
			if (connect)
				this->connect();
		}

		~client()
		{
			disconnect();
		}

		//TODO: redo
		void runner()
		{
			try
			{
				while (true)
					on_packet(read());
			}
			catch (std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}

		void connect()
		{
			socket.connect(server);
		}

		void disconnect()
		{
			if (is_connected())
				socket.close();

			subscribed_channels.clear();
		}

		//TODO: auto listen
		void reconnect()
		{
			disconnect();
			connect();
		}

		//TODO: fix (spoiler alert: it doesn't work at all)
		bool is_connected()
		{
			return true;
		}

		int64_t ping(milliseconds timeout = 5000ms)
		{
			packet request;
			request.type = __ping__;

			int64_t time = duration_cast<seconds>(high_resolution_clock::now().time_since_epoch()).count();
			uint8_t* value = reinterpret_cast<uint8_t*>(&time);

			for (size_t byte = 0; byte < sizeof(int64_t); byte++)
				request.data.push_back(value[byte]);

			steady_clock::time_point start = high_resolution_clock::now();
			write(request);

			std::unique_lock<std::mutex> lock(pong_data.pong_mutex);
			if (!pong_data.pong_cv.wait_for(lock, timeout, [&] { return pong_data.pong_response.first == request.data; }))
				return -1;

			return duration_cast<milliseconds>(pong_data.pong_response.second - start).count();
		}

		void send_message(std::string channel, std::string data)
		{
			packet request;
			request.type = __message__;
			request.channel = channel;
			request.data = data;

			write(request);
		}

		callback_ptr register_listener(callback listener)
		{
			auto& reference = *message_listeners.insert(std::make_pair(std::make_shared<callback>(listener), std::unordered_set<std::string>())).first;

			return reference.first;
		}

		void unregister_listener(callback_ptr id)
		{
			auto search = message_listeners.find(id.lock());

			if (search != message_listeners.end())
			{
				clear_listener_subscriptions(id);

				message_listeners.erase(search);
			}
		}

		void clear_listeners()
		{
			for (std::pair<std::string, uint16_t> subscription : subscribed_channels)
				unsubscribe(subscription.first);

			message_listeners.clear();
		}

		void subscribe_listener(callback_ptr id, std::string channel)
		{
			auto listener_search = message_listeners.find(id.lock());

			listener_search->second.insert(channel);

			subscribe(channel);
		}

		void unsubscribe_listener(callback_ptr id, std::string channel)
		{
			auto listener_search = message_listeners.find(id.lock());

			listener_search->second.erase(channel);

			unsubscribe(channel);
		}

		void clear_listener_subscriptions(callback_ptr id)
		{
			auto listener_search = message_listeners.find(id.lock());

			for (std::string subscription : listener_search->second)
				unsubscribe_listener(id, subscription);
		}
	};
}