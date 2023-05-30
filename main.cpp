#include <iostream>

#include "src/stemlib.hpp"

int main()
{
	try
	{
		static stemlib::Client better;

		stemlib::SharedListener listener = std::make_shared<stemlib::Listener>([](stemlib::Channel channel, stemlib::ByteVector message)
			{
				std::cout << std::string(message.begin(), message.end());
			});

		better.subscribeListener("wassup", listener);

		better.sendMessage("wassup", "zxcv");

		std::this_thread::sleep_for(std::chrono::milliseconds(50000));
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}