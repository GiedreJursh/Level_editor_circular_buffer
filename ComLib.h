#ifndef COMLIB_H
#define COMLIB_H

#include <Windows.h>
#include <string>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ComLib
{
public:

	HANDLE hFileMap;
	HANDLE myMutex;
	DWORD ms;
	void* mData;
	bool exists = false;
	unsigned int mSize = 1 << 20;
	char* base;

	size_t totalSize;
	size_t failSafe;
	size_t* head;
	size_t* tail;
	size_t freeMemory;

	enum ROLE { PRODUCER, CONSUMER };
	enum MSG_TYPE { NORMAL, DUMMY };
	enum MSG_ROLE { MESH, CAMERA, LIGHT, TRANSFORM, MATERIAL, DELETE_INFO, NAME_CHANGED };
	enum LIGHT_TYPE { POINT, DIRECTIONAL, SPOT, AMBIENT };

	struct HEADER {
		MSG_TYPE type;
		size_t length;

		MSG_ROLE role;
		float variables[8];
	};

	struct INFO {
		MSG_ROLE role;		//	  Mesh:				Light:				Camera:			Transform:		Material:			DeleteInfo:			NameChanged:
		int variables[8];	// 0. Name				Name				Name			Name			Name				Name				Old Name
							// 1. Material Path		Type				Matrix			Matrix			Color				Object Type			New Name
							// 2. Vertex Count		Position			Type(0=P,1=O)					Texture Path							Object Typ
							// 3. Normal Count		Color												Bump Map
							// 4. UV Count			Intensty											Specular Color
							// 5. Polygon Count		Direction											Reflectivity
							// 6. Index Count		Angle
							// 7.					Ambient Intensity
	};

	struct MESH_V {
		char mashName[1024];
		char materialPath[1024];

		std::vector<std::vector<float>> vertex;
		std::vector<std::vector<float>> normals;
		std::vector<std::vector<float>> uvs;
		std::vector<int> trianglesPerPolygon;
		std::vector<int> index;
	};

	struct LIGHT_T {
		char lightName[1024];
		LIGHT_TYPE type;
		float position[3];
		float color[3];
		float intensity;

		// For Directional and Spot lights:
		float direction[4]; // Where last walue in Normal vector should be 1.0f

							// For Spot Light:
		float angle;

		// For Ambient Light only:
		float ambientIntensity;
	};

	struct CAMERA_T {
		char cameraName[1024];
		glm::mat4x4 cameraMatrix;
		double fovy;
		double aspectRatio;
		double nearPlane;
		double farPlane;
		double lrbt[4];
	};

	struct TRANSFORM_T {
		char transformName[1024];
		glm::mat4x4 transformMatrix;
	};

	struct MATERIAL_S {
		char materialName[1024];
		float color[3];
		char texturePath[1024];
		char BumpMapPath[1024];

		// For Phong and Blint:
		float specularColor[3];
		float reflectivity;
	};

	struct DELETE_INFO_MSG {
		char deleteInfoName[1024];
		MSG_ROLE role;
	};

	struct NAME_CHANGED_MSG {
		char oldName[1024];
		char newName[1024];
		MSG_ROLE role;
	};

	// Constructor
	// secret is the known name for the shared memory
	// buffSize is in MEGABYTES (multiple of 1<<20)
	// type is TYPE::PRODUCER or TYPE::CONSUMER
	ComLib(const std::string& secret, const size_t& buffSize, ROLE type);

	// returns "true" if data was sent successfully.
	// false if for ANY reason the data could not be sent.
	// we will not implement an "error handling" mechanism, so we will assume
	// that false means that there was no space in the buffer to put the message.
	// msg is a void pointer to the data.
	// length is the amount of bytes of the message to send.
	bool send(const INFO *msg, const MESH_V *mesh_v, const size_t length);
	bool send(const INFO *msg, const LIGHT_T *light_t, const size_t length);
	bool send(const INFO *msg, const CAMERA_T *camera_t, const size_t length);
	bool send(const INFO *msg, const TRANSFORM_T *transform_t, const size_t length);
	bool send(const INFO *msg, const MATERIAL_S *material_s, const size_t length);
	bool send(const INFO *msg, const DELETE_INFO_MSG *deleteInfo_msg, const size_t length);
	bool send(const INFO *msg, const NAME_CHANGED_MSG *nameChanged_msg, const size_t length);

	// returns: "true" if a message was received.
	// false if there was nothing to read.
	// "msg" is expected to have enough space for the message.
	// use "nextSize()" to check whether our temporary buffer is big enough
	// to hold the next message.
	// @length returns the size of the message just read.
	// @msg contains the actual message read.
	bool recv(INFO *msg, MESH_V *mesh_v, LIGHT_T *light_t, CAMERA_T *camera_t, TRANSFORM_T *transform_t, MATERIAL_S *material_s, DELETE_INFO_MSG *deleteInfo_msg, NAME_CHANGED_MSG *nameChanged_msg, size_t &length);
	// return the length of the next message
	// return 0 if no message is available.
	size_t nextSize();

	/* destroy all resources */
	~ComLib();
};

#endif
