#include <iostream>

#include "src/stemlib.hpp"

int main()
{
	std::cout << "Welcome to stemchat!\n";
	std::cout << "This is a simple app to show how to use stemlib.\n\n";

	std::cout << "Original stem server located at stem.fomalhaut.me and listens on 5733 port\n\n";

	std::cout << "Enter hostname of stem server: ";

	std::string hostname;
	std::cin >> hostname;

	std::cout << "Enter port of stem server: ";

	uint16_t port;
	std::cin >> port;

	std::cout << "Connecting...";

	static stemlib::Client client(hostname, port);

	std::cout << " Connected!\n\n";

	std::cout << "stemchat commands:\n";
	std::cout << "/listen <channel>\n";
	std::cout << "subscribes to server channel\n\n";

	std::cout << "/ignore [channel]\n";
	std::cout << "unsubs from channel or from all channels if particular one wasn't specified\n\n";

	std::cout << "/exit, /quit\n";
	std::cout << "leaves the application\n\n";

	std::cout << "* you write to the last subscribed channel *\n\n";

	stemlib::SharedListener listener = std::make_shared<stemlib::Listener>([](stemlib::Channel channel, stemlib::ByteVector message)
		{
			std::cout << channel;
			std::cout << " >> ";
			std::cout << std::string(message.begin(), message.end());
			std::cout << "\n";
		});

	stemlib::Channel last_one;
	std::set<stemlib::Channel> subs;

	while (true)
	{
		std::string user_input;
		std::getline(std::cin, user_input);

		if (user_input[0] == '/')
		{
			if (user_input.substr(0, 8) == "/listen ")
			{
				auto chan = user_input.substr(8);
				client.subscribeListener(chan, listener);
				last_one = chan;
				subs.insert(chan);

				std::cout << "Subbed to ";
				std::cout << (chan.size() ? chan : "[null]");
				std::cout << ".\n";
			}
			else if (user_input.substr(0, 7) == "/ignore")
			{
				auto chan = user_input.substr(8);
				if (chan.size() == 0)
				{
					for (auto& sub : subs)
						client.unsubscribeListener(sub, listener);

					subs.clear();
					std::cout << "Unsubbed from all.\n";
				}
				else if (subs.find(chan) != subs.end())
				{
					client.unsubscribeListener(chan, listener);

					subs.erase(chan);
					std::cout << "Unsubbed from channel " << chan << ".\n";

					if (chan == last_one)
						std::cout << "* you still write to just unsubbed channel *\n";
				}
				else
					std::cout << "Not subscribed to that one!\n";
			}
			else if ((user_input.substr(0, 5) == "/exit" || user_input.substr(0, 5) == "/quit"))
				return 0;
			else
				std::cout << "Unknown command!\n";
		}
		else if (!subs.empty())
		{
			client.sendMessage(last_one, user_input);
		}
		else
		{
			std::cout << "No channel!\n";
		}
	}

	return 0;
}