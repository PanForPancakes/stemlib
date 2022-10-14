#include "stemlib.hpp"

int main()
{
	try
	{
		stem::client client("stem.fomalhaut.me", 5733);

		client.send_message("asdf", std::string(65500, 'F'));

		std::thread runner([&] { try { client.runner(); } catch (std::exception& e) { std::cerr << e.what() << std::endl; } });

		stem::client::callback_ptr setup_listener = client.register_listener([&](std::string channel, std::string data) { std::cout << data; });
		client.subscribe_listener(setup_listener, "asdf");

		std::cout << client.ping();

		runner.join();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}