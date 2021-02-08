#include "FrameWork/Logger_Import/loguru.hpp"
#include "FrameWork/EntryPoint.h"

#include "Netzwerk/message.h"
#include "Netzwerk/NetzwerkKernal.h"

/*int main(int argc, char* argv[])
{
	FrameWork_Logger_loguru::g_stderr_verbosity = 2;

	LOG_F(INFO, "Start");



	LOG_F(INFO, "Ende");
	return 0;
}*/

bool onConnect(uint32_t otherID, std::string otherIP, uint16_t port) {
	LOG_F(INFO, "onConnect");
	return true;
};

void onConnectionRemoved(uint32_t otherID) {
	LOG_F(INFO, "onDisconect ID: %d", otherID);
}
void onConnectionAdded(uint32_t otherID) {
	LOG_F(INFO, "onConnect ID: %d", otherID);
}

void onMessage(netzwerk::message<uint32_t> message) {
	LOG_F(INFO, "onMessage ID: %d soudceID: %d destinagionID: %d", message.header.id, message.header.sourceID, message.header.destinationID);
}

void onMessageDestinationNotFound(netzwerk::message<uint32_t> message) {
	LOG_F(WARNING, "onNotFound");
}

int main()
{
	FrameWork_Logger_loguru::g_stderr_verbosity = 2;

	netzwerk::NetzwerkKernal net = netzwerk::NetzwerkKernal(onMessage, onMessageDestinationNotFound, onConnectionRemoved, onConnectionAdded, onConnect);

	//net.Start("127.0.0.1", 60000, 60001);
	net.Start(60000);

	while (net.isRunning());


	//net::netzwerkBasik server(60000, onConnect, onDisconnect, onMessage);
	//server.Start();

	//while (1)
	//{
	//	server.Update(-1, true);
	//}

	return 0;
}