#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include "ComLib.h"

using namespace std;

string role = "";
string random = "";
int numMsgs = 0;
int delay = 0;
int memSize = 0;
int msgSize = 0;
bool toBeRand = false;

void produce(int half);
void consume(int half);

int main(int argc, char **argv)
{
	role = argv[1];
	delay = stoi(argv[2]);
	memSize = stoi(argv[3]);
	numMsgs = stoi(argv[4]);
	if (argv[5][0] == 'r')
		toBeRand = true;
	else
		msgSize = stoi(argv[5]);

	std::cout << numMsgs << std::endl;

	int halfMem = memSize * (1 << 20) / 2;

	if (role[0] == 'p')
		produce(halfMem);
	else
		consume(halfMem);
}


void produce(int half)
{
	ComLib Producer("secretName", memSize, ComLib::ROLE::PRODUCER);
	int count = 0;
	ComLib::INFO newVertex;
	newVertex.role = ComLib::MSG_ROLE::VERTEX;
	newVertex.info.push_back(1.0f);
	bool success = false;
	while (count < numMsgs)
	{
		Sleep(delay);

		success = Producer.send(&newVertex, sizeof(newVertex));
		if (success)
		{
			count++;
		}
	}
	getchar();
}


void consume(int half)
{
	ComLib Consumer("secretName", memSize, ComLib::ROLE::CONSUMER);

	ComLib::INFO* recvVertex = 0;
	size_t sizeOfBoi;
	int count = 0;
	bool success = false;
	std::cout << numMsgs << std::endl;
	while (count < numMsgs)
	{
		Sleep(delay);

		success = Consumer.recv(recvVertex, sizeOfBoi);
		if (success)
		{
			count++;
		}
	}
	getchar();
}