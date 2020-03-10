#include "ComLib.h"
#include <iostream>

using namespace std;

ComLib::ComLib(const std::string & secret, const size_t & buffSize, ROLE type)
{
	ms = INT_MAX;
	hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		(DWORD)0,
		buffSize * mSize,
		(LPCSTR)secret.c_str());

	if (hFileMap == NULL)
	{
		exit(-1);
	}

	// Create the memory data buffer
	mData = MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	// Assign the pointers to the beginning of the buffer
	head = (size_t*)mData;
	tail = head + 1;
	base = (char*)(tail + 1);

	// If Producer is the first to run the program, then reset the value of head and tail
	if (type == PRODUCER)
	{
		*head = 0;
		*tail = 0;
	}

	totalSize = (buffSize * mSize) - (sizeof(head) + sizeof(tail));
	freeMemory = totalSize;
	failSafe = 0;
}

bool ComLib::send(const INFO *msg, const MESH_V *mesh_v, const size_t length)
{
	bool success = false;
	HEADER h;
	h.length = length;
	h.role = msg->role;

	for (int i = 0; i < 8; i++)
	{
		h.variables[i] = msg->variables[i];
	}

	size_t localTail = *tail;
	if (*head < localTail)
		freeMemory = localTail - *head;
	else if (*head >= localTail)
		freeMemory = totalSize - *head;

	failSafe = sizeof(HEADER) + h.length + (3 * sizeof(HEADER));

	if (freeMemory > failSafe)
	{
		h.type = NORMAL;
		// Copy size of HEADER h and add it to the base with head as offset
		memcpy(base + (*head), &h, sizeof(HEADER));

		// Now the consumer can first read if the message is a dummy or normal, and then the length of message. Copy the message into memory
		memcpy(base + (*head) + sizeof(HEADER), mesh_v->mashName, sizeof(char) * 1024);
		memcpy(base + (*head) + sizeof(HEADER) + (sizeof(char) * 1024), mesh_v->materialPath, sizeof(char) * 1024);

		size_t extramemo = sizeof(mesh_v->mashName) + sizeof(mesh_v->materialPath);

		for (int i = 0; i < msg->variables[2]; i++) {
			float x_y_z[3];
			x_y_z[0] = mesh_v->vertex[i].at(0);
			x_y_z[1] = mesh_v->vertex[i].at(1);
			x_y_z[2] = mesh_v->vertex[i].at(2);

			memcpy(base + (*head) + sizeof(HEADER) + extramemo, x_y_z, sizeof(float) * 3);

			extramemo += (3 * sizeof(float));
		}

		for (int i = 0; i < msg->variables[3]; i++) {
			float x_y_z[3];
			x_y_z[0] = mesh_v->normals[i].at(0);
			x_y_z[1] = mesh_v->normals[i].at(1);
			x_y_z[2] = mesh_v->normals[i].at(2);

			memcpy(base + (*head) + sizeof(HEADER) + extramemo, x_y_z, sizeof(float) * 3);

			extramemo += (3 * sizeof(float));
		}

		for (int i = 0; i < msg->variables[4]; i++) {
			float u_v[3];
			u_v[0] = mesh_v->uvs[i].at(0);
			u_v[1] = mesh_v->uvs[i].at(1);

			memcpy(base + (*head) + sizeof(HEADER) + extramemo, u_v, sizeof(float) * 2);

			extramemo += (2 * sizeof(float));
		}

		int* triangleArray = new int[mesh_v->trianglesPerPolygon.size()];

		for (int i = 0; i < msg->variables[5]; i++)
		{
			triangleArray[i] = mesh_v->trianglesPerPolygon.at(i);
		}

		memcpy(base + (*head) + sizeof(HEADER) + extramemo, triangleArray, sizeof(int) * mesh_v->trianglesPerPolygon.size());
		extramemo += (sizeof(int) * mesh_v->trianglesPerPolygon.size());

		int* indexArray = new int[mesh_v->index.size()];

		for (int i = 0; i < msg->variables[6]; i++)
		{
			indexArray[i] = mesh_v->index.at(i);
		}

		memcpy(base + (*head) + sizeof(HEADER) + extramemo, indexArray, sizeof(int) * mesh_v->index.size());
		extramemo += (sizeof(int) * mesh_v->index.size());

		// Increment the head with the size
		*head += sizeof(HEADER) + h.length;

		delete[] triangleArray;
		delete[] indexArray;

		success = true;
	}
	else
	{
		if (freeMemory > sizeof(HEADER) && (*head >= localTail))
		{
			if (localTail != 0)
			{
				freeMemory = totalSize;
				h.type = DUMMY;
				// Copy size of HEADER h and add it to the base with head as offset
				memcpy(base + (*head), &h, sizeof(HEADER));
				*head = 0;
			}
		}
	}
	return success;
}

bool ComLib::send(const INFO *msg, const LIGHT_T *light_t, const size_t length)
{
	bool success = false;
	HEADER h;
	h.length = length;
	h.role = msg->role;

	for (int i = 0; i < 8; i++)
	{
		h.variables[i] = msg->variables[i];
	}

	size_t localTail = *tail;
	if (*head < localTail)
		freeMemory = localTail - *head;
	else if (*head >= localTail)
		freeMemory = totalSize - *head;

	failSafe = sizeof(HEADER) + h.length + (3 * sizeof(HEADER));

	if (freeMemory > failSafe)
	{
		h.type = NORMAL;
		// Copy size of HEADER h and add it to the base with head as offset
		memcpy(base + (*head), &h, sizeof(HEADER));

		// Now the consumer can first read if the message is a dummy or normal, and then the length of message. Copy the message into memory
		memcpy(base + (*head) + sizeof(HEADER), light_t, h.length);

		// Increment the head with the size
		*head += sizeof(HEADER) + h.length;

		success = true;
	}
	else
	{
		if (freeMemory > sizeof(HEADER) && (*head >= localTail))
		{
			if (localTail != 0)
			{
				freeMemory = totalSize;
				h.type = DUMMY;
				// Copy size of HEADER h and add it to the base with head as offset
				memcpy(base + (*head), &h, sizeof(HEADER));
				*head = 0;
			}
		}
	}
	return success;
}

bool ComLib::send(const INFO *msg, const CAMERA_T *camera_t, const size_t length)
{
	bool success = false;
	HEADER h;
	h.length = length;
	h.role = msg->role;

	for (int i = 0; i < 8; i++)
	{
		h.variables[i] = msg->variables[i];
	}


	size_t localTail = *tail;
	if (*head < localTail)
		freeMemory = localTail - *head;
	else if (*head >= localTail)
		freeMemory = totalSize - *head;

	failSafe = sizeof(HEADER) + h.length + (3 * sizeof(HEADER));

	if (freeMemory > failSafe)
	{
		h.type = NORMAL;
		// Copy size of HEADER h and add it to the base with head as offset
		memcpy(base + (*head), &h, sizeof(HEADER));

		// Now the consumer can first read if the message is a dummy or normal, and then the length of message. Copy the message into memory
		memcpy(base + (*head) + sizeof(HEADER), camera_t, h.length);

		// Increment the head with the size
		*head += sizeof(HEADER) + h.length;

		success = true;
	}
	else
	{
		if (freeMemory > sizeof(HEADER) && (*head >= localTail))
		{
			if (localTail != 0)
			{
				freeMemory = totalSize;
				h.type = DUMMY;
				// Copy size of HEADER h and add it to the base with head as offset
				memcpy(base + (*head), &h, sizeof(HEADER));
				*head = 0;
			}
		}
	}
	return success;
}

bool ComLib::send(const INFO *msg, const TRANSFORM_T *transform_t, const size_t length)
{
	bool success = false;
	HEADER h;
	h.length = length;
	h.role = msg->role;

	for (int i = 0; i < 8; i++)
	{
		h.variables[i] = msg->variables[i];
	}

	size_t localTail = *tail;
	if (*head < localTail)
		freeMemory = localTail - *head;
	else if (*head >= localTail)
		freeMemory = totalSize - *head;

	failSafe = sizeof(HEADER) + h.length + (3 * sizeof(HEADER));

	if (freeMemory > failSafe)
	{
		h.type = NORMAL;
		// Copy size of HEADER h and add it to the base with head as offset
		memcpy(base + (*head), &h, sizeof(HEADER));

		// Now the consumer can first read if the message is a dummy or normal, and then the length of message. Copy the message into memory
		memcpy(base + (*head) + sizeof(HEADER), transform_t, h.length);

		// Increment the head with the size
		*head += sizeof(HEADER) + h.length;

		success = true;
	}
	else
	{
		if (freeMemory > sizeof(HEADER) && (*head >= localTail))
		{
			if (localTail != 0)
			{
				freeMemory = totalSize;
				h.type = DUMMY;
				// Copy size of HEADER h and add it to the base with head as offset
				memcpy(base + (*head), &h, sizeof(HEADER));
				*head = 0;
			}
		}
	}
	return success;
}

bool ComLib::send(const INFO *msg, const MATERIAL_S *material_s, const size_t length)
{
	bool success = false;
	HEADER h;
	h.length = length;
	h.role = msg->role;

	for (int i = 0; i < 8; i++)
	{
		h.variables[i] = msg->variables[i];
	}


	size_t localTail = *tail;
	if (*head < localTail)
		freeMemory = localTail - *head;
	else if (*head >= localTail)
		freeMemory = totalSize - *head;

	failSafe = sizeof(HEADER) + h.length + (3 * sizeof(HEADER));

	if (freeMemory > failSafe)
	{
		h.type = NORMAL;
		// Copy size of HEADER h and add it to the base with head as offset
		memcpy(base + (*head), &h, sizeof(HEADER));

		// Now the consumer can first read if the message is a dummy or normal, and then the length of message. Copy the message into memory
		memcpy(base + (*head) + sizeof(HEADER), material_s, h.length);

		// Increment the head with the size
		*head += sizeof(HEADER) + h.length;

		success = true;
	}
	else
	{
		if (freeMemory > sizeof(HEADER) && (*head >= localTail))
		{
			if (localTail != 0)
			{
				freeMemory = totalSize;
				h.type = DUMMY;
				// Copy size of HEADER h and add it to the base with head as offset
				memcpy(base + (*head), &h, sizeof(HEADER));
				*head = 0;
			}
		}
	}
	return success;
}

bool ComLib::send(const INFO *msg, const DELETE_INFO_MSG *deleteInfo_msg, const size_t length)
{
	bool success = false;
	HEADER h;
	h.length = length;
	h.role = msg->role;

	for (int i = 0; i < 8; i++)
	{
		h.variables[i] = msg->variables[i];
	}


	size_t localTail = *tail;
	if (*head < localTail)
		freeMemory = localTail - *head;
	else if (*head >= localTail)
		freeMemory = totalSize - *head;

	failSafe = sizeof(HEADER) + h.length + (3 * sizeof(HEADER));

	if (freeMemory > failSafe)
	{
		h.type = NORMAL;
		// Copy size of HEADER h and add it to the base with head as offset
		memcpy(base + (*head), &h, sizeof(HEADER));

		// Now the consumer can first read if the message is a dummy or normal, and then the length of message. Copy the message into memory
		memcpy(base + (*head) + sizeof(HEADER), deleteInfo_msg, h.length);

		// Increment the head with the size
		*head += sizeof(HEADER) + h.length;

		success = true;
	}
	else
	{
		if (freeMemory > sizeof(HEADER) && (*head >= localTail))
		{
			if (localTail != 0)
			{
				freeMemory = totalSize;
				h.type = DUMMY;
				// Copy size of HEADER h and add it to the base with head as offset
				memcpy(base + (*head), &h, sizeof(HEADER));
				*head = 0;
			}
		}
	}
	return success;
}

bool ComLib::send(const INFO *msg, const NAME_CHANGED_MSG *nameChanged_msg, const size_t length)
{
	bool success = false;
	HEADER h;
	h.length = length;
	h.role = msg->role;

	for (int i = 0; i < 8; i++)
	{
		h.variables[i] = msg->variables[i];
	}


	size_t localTail = *tail;
	if (*head < localTail)
		freeMemory = localTail - *head;
	else if (*head >= localTail)
		freeMemory = totalSize - *head;

	failSafe = sizeof(HEADER) + h.length + (3 * sizeof(HEADER));

	if (freeMemory > failSafe)
	{
		h.type = NORMAL;
		// Copy size of HEADER h and add it to the base with head as offset
		memcpy(base + (*head), &h, sizeof(HEADER));

		// Now the consumer can first read if the message is a dummy or normal, and then the length of message. Copy the message into memory
		memcpy(base + (*head) + sizeof(HEADER), nameChanged_msg, h.length);

		// Increment the head with the size
		*head += sizeof(HEADER) + h.length;

		success = true;
	}
	else
	{
		if (freeMemory > sizeof(HEADER) && (*head >= localTail))
		{
			if (localTail != 0)
			{
				freeMemory = totalSize;
				h.type = DUMMY;
				// Copy size of HEADER h and add it to the base with head as offset
				memcpy(base + (*head), &h, sizeof(HEADER));
				*head = 0;
			}
		}
	}
	return success;
}

bool ComLib::recv(INFO *msg, MESH_V *mesh_v, LIGHT_T *light_t, CAMERA_T *camera_t, TRANSFORM_T *transform_t, MATERIAL_S *material_s, DELETE_INFO_MSG *deleteInfo_msg, NAME_CHANGED_MSG *nameChanged_msg, size_t &length)
{
	bool success = false;
	HEADER h;
	size_t localHead = *head;
	if (*tail != localHead)
	{
		memcpy(&h, base + (*tail), sizeof(HEADER));
		length = h.length;

		if (h.type == NORMAL)
		{
			msg->role = h.role;
			for (int i = 0; i < 8; i++)
			{
				msg->variables[i] = h.variables[i];
			}

			if (h.role == MESH) {
				int nrOfVertex = h.variables[2];
				int nrOfVertextNormalValues = 3;
				int nrOfUVValues = 2;

				memcpy(&mesh_v->mashName, base + (*tail) + sizeof(HEADER), sizeof(char) * 1024);
				memcpy(&mesh_v->materialPath, base + (*tail) + sizeof(HEADER) + sizeof(mesh_v->mashName), sizeof(char) * 1024);


				size_t extraSize = sizeof(mesh_v->mashName) + sizeof(mesh_v->materialPath);

				for (int i = 0; i < nrOfVertex; i++)
				{

					std::vector<float> vertex;
					float x_y_z[3];

					memcpy(&x_y_z, base + (*tail) + sizeof(HEADER) + extraSize, sizeof(float) * 3);

					vertex.push_back(x_y_z[0]);
					vertex.push_back(x_y_z[1]);
					vertex.push_back(x_y_z[2]);
					mesh_v->vertex.push_back(vertex);

					extraSize += (3 * sizeof(float));
				}

				for (int i = 0; i < nrOfVertex; i++)
				{
					std::vector<float> normals;
					float x_y_z[3];

					memcpy(&x_y_z, base + (*tail) + sizeof(HEADER) + extraSize, sizeof(float) * 3);

					normals.push_back(x_y_z[0]);
					normals.push_back(x_y_z[1]);
					normals.push_back(x_y_z[2]);
					mesh_v->normals.push_back(normals);

					extraSize += (3 * sizeof(float));
				}

				for (int i = 0; i < nrOfVertex; i++)
				{
					std::vector<float> uvs;
					float u_v[2];

					memcpy(&u_v, base + (*tail) + sizeof(HEADER) + extraSize, sizeof(float) * 2);

					uvs.push_back(u_v[0]);
					uvs.push_back(u_v[1]);
					mesh_v->uvs.push_back(uvs);

					extraSize += (2 * sizeof(float));
				}

				int* triangleArray = new int[msg->variables[5]];
				memcpy(triangleArray, base + (*tail) + sizeof(HEADER) + extraSize, sizeof(int) * msg->variables[5]);

				for (int i = 0; i < msg->variables[5]; i++)
				{
					mesh_v->trianglesPerPolygon.push_back(triangleArray[i]);
				}

				extraSize += (sizeof(int) * mesh_v->trianglesPerPolygon.size());

				int* indexArray = new int[msg->variables[6]];
				memcpy(indexArray, base + (*tail) + sizeof(HEADER) + extraSize, sizeof(int) * msg->variables[6]);
				for (int i = 0; i < msg->variables[6]; i++)
				{
					mesh_v->index.push_back(indexArray[i]);
				}

				extraSize += (sizeof(int) * mesh_v->index.size());

				delete[] triangleArray;
				delete[] indexArray;

				*tail += sizeof(HEADER) + length;
				success = true;
			}
			else if (h.role == LIGHT) {
				memcpy(light_t, base + (*tail) + sizeof(HEADER), h.length);

				*tail += sizeof(HEADER) + h.length;
				success = true;
			}
			else if (h.role == CAMERA) {
				memcpy(camera_t, base + (*tail) + sizeof(HEADER), length);

				*tail += sizeof(HEADER) + length;
				success = true;
			}
			else if (h.role == TRANSFORM) {
				memcpy(transform_t, base + (*tail) + sizeof(HEADER), length);

				*tail += sizeof(HEADER) + length;
				success = true;
			}
			else if (h.role == MATERIAL) {
				memcpy(material_s, base + (*tail) + sizeof(HEADER), length);

				*tail += sizeof(HEADER) + length;
				success = true;
			}
			else if (h.role == DELETE_INFO) {
				memcpy(deleteInfo_msg, base + (*tail) + sizeof(HEADER), length);

				*tail += sizeof(HEADER) + length;
				success = true;
			}
			else if (h.role == NAME_CHANGED) {
			memcpy(nameChanged_msg, base + (*tail) + sizeof(HEADER), length);

			*tail += sizeof(HEADER) + length;
			success = true;
			}
		}
		else
			*tail = 0;
	}
	return success;
}

size_t ComLib::nextSize()
{
	HEADER temp;
	memcpy(&temp, base + (*head), sizeof(HEADER));

	return temp.length;
}

ComLib::~ComLib()
{
	UnmapViewOfFile((LPCVOID)mData);
	CloseHandle(hFileMap);
}

