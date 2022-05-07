#include "stemlib.hpp"

//disclaimer: i wrote everything down here being drunk

int main()
{
	try
	{
		asio::io_context io_context;

		stem::client client(io_context, "stem.fomalhaut.me", 5733);
		std::thread runner([&] { try { client.runner(); } catch (std::exception& e) { std::cerr << e.what() << std::endl; } });

		stem::client::callback_id setup_listener = client.register_listener([&](std::string channel, std::string data) {});
		client.subscribe_listener(setup_listener, "asdf");

		std::vector<std::thread> io_threads;
		for (int i = 0; i < 4; i++)
			io_threads.emplace_back(std::thread([&] { io_context.run(); }));

		for (std::thread& thread : io_threads)
			thread.join();

		runner.join();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}