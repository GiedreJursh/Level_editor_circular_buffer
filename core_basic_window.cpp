#include <vector>
#include <stdio.h>
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <ComLib.cpp>
#include <Windows.h>

#include <chrono>
#include <ctime>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Target Engine main file (Raylib)
struct modelPos
{
	Model model;
	Matrix modelMatrix;
};

struct Light
{
	Vector3 color;
	Vector3 position;
	float intensity;
	Light() : color({ 1.0f, 1.0f, 1.0f }), position({ 0, 0, 0 }), intensity(1.0) {}
};

Light* newLight = new Light;
vector<Mesh> meshes;
vector<string> mayaNames;
vector<modelPos> flatScene;
double aspectRatio;
double nearPlane;
double farPlane;
double rLeft, rRight, rBottom, rTop;
float reflectivity = 0.01f;

void infoHandler(
	ComLib::INFO &info, ComLib::MESH_V &mesh, ComLib::LIGHT_T &light, ComLib::CAMERA_T &cam, ComLib::TRANSFORM_T &transform, ComLib::MATERIAL_S &material, ComLib::DELETE_INFO_MSG &cdelete, ComLib::NAME_CHANGED_MSG &nameChanged, 
	Camera* camera,
	Light* l,
	Shader shader,
	vector<modelPos> &mPos,
	vector<Mesh> &vMesh);

int main()
{
	SetTraceLog(LOG_WARNING);
	ComLib Consumer("secretName", 200, ComLib::ROLE::CONSUMER);
	size_t sizeBoi = 0;
	bool success = false;
	Matrix view = MatrixIdentity();
	Matrix proj = MatrixIdentity();

	ComLib::INFO comInfo;
	ComLib::MESH_V comMesh;
	ComLib::LIGHT_T comLight;
	ComLib::CAMERA_T comCamera;
	ComLib::TRANSFORM_T comTransform;
	ComLib::MATERIAL_S comMaterial;
	ComLib::DELETE_INFO_MSG comDelete;
	ComLib::NAME_CHANGED_MSG comNameChange;

	// Initialization
	//--------------------------------------------------------------------------------------
	int screenWidth = 1200;
	int screenHeight = 800;

	SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screenWidth, screenHeight, "demo");

	Vector3 mid = { 0 };

	// Define the camera to look into our 3d world
	Camera3D pCamera = { 0 };
	pCamera.position = { 4.0f, 4.0f, 4.0f };
	pCamera.target = { 0.0f, 1.0f, -1.0f };
	pCamera.up = { 0.0f, 1.0f, 0.0f };
	pCamera.fovy = 45.0f;
	pCamera.type = CAMERA_PERSPECTIVE;
	SetCameraMode(pCamera, CAMERA_PERSPECTIVE);

	// real models rendered each frame

	Shader shader1 = LoadShader(
		"resources/shaders/glsl330/phong.vs",
		"resources/shaders/glsl330/phong.fs");   // Load model shader
												 //material1.shader = shader1;

												 //Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
												 //SetCameraMode(pCamera, CAMERA_FREE);         // Set an orbital camera mode
	SetTargetFPS(60);                           // Set our game to run at 60 frames-per-second


	int modelLoc = GetShaderLocation(shader1, "model");
	int viewLoc = GetShaderLocation(shader1, "view");
	int projLoc = GetShaderLocation(shader1, "projection");

	int lightLoc = GetShaderLocation(shader1, "lightPos");
	int lightClrLoc = GetShaderLocation(shader1, "lightColor");
	int lightIntensity = GetShaderLocation(shader1, "lightIntensity");

	// Main game loop
	while (!WindowShouldClose()) { // Detect window close button or ESC key


		//std::clock_t c_start = std::clock();
		//auto t_start = std::chrono::high_resolution_clock::now();
		//cout << "Update!" << endl;
		for (int i = 0; i < 8; i++)
		{
			success = Consumer.recv(&comInfo, &comMesh, &comLight, &comCamera, &comTransform, &comMaterial, &comDelete, &comNameChange, sizeBoi);
			if (success) {
				infoHandler(comInfo, comMesh, comLight, comCamera, comTransform, comMaterial, comDelete, comNameChange, &pCamera, newLight, shader1, flatScene, meshes);
				if (comInfo.role == ComLib::MESH)
					comMesh.index.clear();
				comMesh.normals.clear();
				comMesh.trianglesPerPolygon.clear();
				comMesh.uvs.clear();
				comMesh.vertex.clear();
			}
		}
		view = MatrixLookAt(pCamera.position, pCamera.target, pCamera.up);
		if (pCamera.type == CAMERA_PERSPECTIVE)
		{
			proj = MatrixPerspective(pCamera.fovy, 1200.0 / 800, .1, 10000);
			SetCameraMode(pCamera, CAMERA_PERSPECTIVE);
		}
		else if (pCamera.type == CAMERA_ORTHOGRAPHIC)
		{
			proj = MatrixOrtho(rLeft, rRight, rBottom, rTop, .1, 10000);
			SetCameraMode(pCamera, CAMERA_ORTHOGRAPHIC);
		}



		BeginDrawing();
		ClearBackground(BLACK);

		SetMatrixProjection(proj);
		SetMatrixModelview(view);

		BeginMode3D(pCamera);

		DrawGrid(10, 1.0f);

		const float& lIntensity = newLight->intensity;
		const float& lReflectivity = reflectivity;
		for (int i = 0; i < flatScene.size(); i++)
		{
			auto m = flatScene[i];
			BeginShaderMode(m.model.material.shader);
			SetShaderValueMatrix(m.model.material.shader, viewLoc, view);
			SetShaderValueMatrix(m.model.material.shader, projLoc, proj);
			SetShaderValue(m.model.material.shader, lightLoc, Vector3ToFloat(newLight->position), 3);
			SetShaderValue(m.model.material.shader, lightClrLoc, Vector3ToFloat(newLight->color), 3);
			SetShaderValue(m.model.material.shader, lightIntensity, &lIntensity, 1);
			SetShaderValue(m.model.material.shader, reflectivity, &lReflectivity, 1);
			SetShaderValueMatrix(m.model.material.shader, modelLoc, m.modelMatrix);
			DrawModel(m.model, {}, 1.0, m.model.material.maps[MAP_DIFFUSE].color);
			EndShaderMode();


		}

		proj = MatrixIdentity();
		view = MatrixIdentity();
		SetMatrixProjection(proj);
		SetMatrixModelview(view);

		EndMode3D();
		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	UnloadShader(shader1);       // Unload shader
								 //UnloadTexture(texture1);     // Unload texture
	for (vector<modelPos>::iterator it = flatScene.begin(); it != flatScene.end(); ++it)
	{
		UnloadModel(it->model);
	}
	flatScene.clear();

	CloseWindowRL();              // Close window and OpenGL context
								  //--------------------------------------------------------------------------------------

	return 0;
}

void infoHandler(
	ComLib::INFO &info, ComLib::MESH_V &mesh, ComLib::LIGHT_T &light, ComLib::CAMERA_T &cam, ComLib::TRANSFORM_T &transform, ComLib::MATERIAL_S &material, ComLib::DELETE_INFO_MSG &cdelete, ComLib::NAME_CHANGED_MSG &nameChanged, 
	Camera* camera,
	Light* l,
	Shader shader,
	vector<modelPos> &mPos,
	vector<Mesh> &vMesh)
{
	switch (info.role) {

	case ComLib::DELETE_INFO:
	{
		string delName(cdelete.deleteInfoName);

		if (cdelete.role == ComLib::MSG_ROLE::MESH)
		{
			for (int i = 0; i < mPos.size(); i++)
			{
				if (mayaNames.at(i) == delName)
				{
					mayaNames.erase(mayaNames.begin() + i);
					vMesh.erase(meshes.begin() + i);
					mPos.erase(mPos.begin() + i);
				}
			}
		}
		else if (cdelete.role == ComLib::MSG_ROLE::LIGHT)
		{
			l->intensity = 0.0;
			l->position = { 0.0, 0.0, 0.0 };
			l->color = { 1.0, 1.0, 1.0 };
		}
		break;
	}
	case ComLib::MATERIAL:
	{
		string matName(material.materialName);
		string texture(material.texturePath);

		int index = 0;

		for (vector<string>::iterator i = mayaNames.begin(); i != mayaNames.end(); i++)
		{
			if (*i == matName)
			{
				if (info.variables[1] > 0)
				{
					Color fillClr{ 0 };
					float lerpIt[3] = {
						material.color[0],
						material.color[1],
						material.color[2]
					};
					fillClr.r = (unsigned char)(lerpIt[0] * 255);
					fillClr.g = (unsigned char)(lerpIt[1] * 255);
					fillClr.b = (unsigned char)(lerpIt[2] * 255);
					fillClr.a = 255;

					Material nMat = { 0 };
					nMat.shader = shader;
					nMat.maps[MAP_DIFFUSE].texture = GetTextureDefault();   // White texture (1x1 pixel)
					nMat.maps[MAP_DIFFUSE].color = fillClr;
					flatScene.at(index).model.material = nMat;
				}
				if (info.variables[2] > 0)
				{
					Color neutClr{ 255, 255, 255, 255 };
					
					Material nMat = { 0 };
					nMat.shader = shader;
					nMat.maps[MAP_DIFFUSE].texture = GetTextureDefault();   // White texture (1x1 pixel)
					nMat.maps[MAP_DIFFUSE].color = neutClr;
					flatScene.at(index).model.material = nMat;

					Texture2D alb = LoadTexture(texture.c_str());
					Image im = GetTextureData(alb);
					ImageFlipVertical(&im);
					Texture2D tex = LoadTextureFromImage(im);

					flatScene.at(index).model.material.maps[MAP_ALBEDO].texture = tex;
				}
				if (info.variables[3] > 0)
				{
					Color neutClr{ 255, 255, 255, 255 };
				}
				if (info.variables[4] > 0)
				{
					Color fillClr{ 0, 0, 0 };
					float lerpIt[3] = {
						material.specularColor[0],
						material.specularColor[1],
						material.specularColor[2]
					};
					fillClr.r = (unsigned char)(lerpIt[0] * 255);
					fillClr.g = (unsigned char)(lerpIt[1] * 255);
					fillClr.b = (unsigned char)(lerpIt[2] * 255);
					fillClr.a = 255;

					//Material nMat = { 0 };
					//nMat.shader = shader;
					//nMat.maps[MAP_SPECULAR].texture = GetTextureDefault();   // White texture (1x1 pixel)
					//nMat.maps[MAP_SPECULAR].color = fillClr;
					//flatScene.at(index).model.material = nMat;
				}
				if (info.variables[5] > 0)
				{
					reflectivity = material.reflectivity;
				}
			}
			index++;
		}
		break;
	}
	case ComLib::TRANSFORM:
	{
		string transName(transform.transformName);

		int index = 0;

		Matrix* updMatrix;

		for (vector<string>::iterator i = mayaNames.begin(); i != mayaNames.end(); i++)
		{
			updMatrix = new Matrix;

			if (*i == transName)
			{
				updMatrix->m0 = (float)transform.transformMatrix[0][0];
				updMatrix->m1 = (float)transform.transformMatrix[0][1];
				updMatrix->m2 = (float)transform.transformMatrix[0][2];
				updMatrix->m3 = (float)transform.transformMatrix[0][3];
				updMatrix->m4 = (float)transform.transformMatrix[1][0];
				updMatrix->m5 = (float)transform.transformMatrix[1][1];
				updMatrix->m6 = (float)transform.transformMatrix[1][2];
				updMatrix->m7 = (float)transform.transformMatrix[1][3];
				updMatrix->m8 = (float)transform.transformMatrix[2][0];
				updMatrix->m9 = (float)transform.transformMatrix[2][1];
				updMatrix->m10 = (float)transform.transformMatrix[2][2];
				updMatrix->m11 = (float)transform.transformMatrix[2][3];
				updMatrix->m12 = (float)transform.transformMatrix[3][0];
				updMatrix->m13 = (float)transform.transformMatrix[3][1];
				updMatrix->m14 = (float)transform.transformMatrix[3][2];
				updMatrix->m15 = (float)transform.transformMatrix[3][3];

				flatScene.at(index).modelMatrix = *updMatrix;
			}
			delete updMatrix;
			index++;
		}
		break;
	}
	case ComLib::CAMERA:
	{
		if (info.variables[2] == 0)
			camera->type = CAMERA_PERSPECTIVE;
		else
			camera->type = CAMERA_ORTHOGRAPHIC;


		camera->position = { cam.cameraMatrix[0][0], cam.cameraMatrix[0][1], cam.cameraMatrix[0][2] };
		camera->target = { cam.cameraMatrix[1][0], cam.cameraMatrix[1][1], cam.cameraMatrix[1][2] };
		camera->up = { cam.cameraMatrix[2][0], cam.cameraMatrix[2][1], cam.cameraMatrix[2][2] };
		camera->fovy = cam.fovy;
		aspectRatio = cam.aspectRatio;
		nearPlane = cam.nearPlane;
		farPlane = cam.farPlane;
		rLeft = cam.lrbt[0];
		rRight = cam.lrbt[1];
		rBottom = cam.lrbt[2];
		rTop = cam.lrbt[3];
		break;
	}
	case ComLib::LIGHT:
	{
		string recvString(light.lightName);

		if ((info.variables[2] == 0) || (info.variables[2] == 3 && info.variables[3] == 3))
		{
			newLight->color.x = light.color[0];
			newLight->color.y = light.color[1];
			newLight->color.z = light.color[2];
			newLight->intensity = light.intensity;
		}
		if ((info.variables[3] == 0) || (info.variables[2] == 3 && info.variables[3] == 3))
		{
			newLight->position.x = light.position[0];
			newLight->position.y = light.position[1];
			newLight->position.z = light.position[2];
		}
		break;
	}
	case ComLib::NAME_CHANGED:
	{
		string oldName = nameChanged.oldName;
		string newName = nameChanged.newName;

		for (int i = 0; i < flatScene.size(); i++)
		{
			if (mayaNames.at(i) == oldName)
			{
				mayaNames.at(i) = newName;
			}
		}
		break;
	}
	case ComLib::MESH:
	{
		int vtxCount = info.variables[2];
		int normalCount = info.variables[3];
		int uvCount = info.variables[4];
		int polyCount = info.variables[5];
		int indexCount = info.variables[6];

		if (vtxCount != 0)
		{
			float* vtxs = new float[vtxCount * 3];
			float* norms = new float[normalCount * 3];
			float* uvs = new float[uvCount * 2];
			unsigned short* index = new unsigned short[indexCount];
			int trisCount = 0;

			int vertCounter = 0;
			int uvCounter = 0;

			for (int i = 0; i < vtxCount; i++)
			{
				float xyz[3]{
					mesh.vertex.at(i)[0],
					mesh.vertex.at(i)[1],
					mesh.vertex.at(i)[2]
				};
				float nrm[3]{
					mesh.normals.at(i)[0],
					mesh.normals.at(i)[1],
					mesh.normals.at(i)[2]
				};
				float uv[2]{
					mesh.uvs.at(i)[0],
					mesh.uvs.at(i)[1]
				};
				for (int j = 0; j < 3; j++)
				{
					vtxs[vertCounter] = xyz[j];
					norms[vertCounter] = nrm[j];
					vertCounter++;
				}
				for (int k = 0; k < 2; k++)
				{
					uvs[uvCounter] = uv[k];
					uvCounter++;
				}
			}

			for (int i = 0; i < polyCount; i++)
			{
				trisCount += mesh.trianglesPerPolygon[i];
			}

			for (int i = 0; i < indexCount; i++)
			{
				index[i] = mesh.index[i];
			}
			bool meshExists = false;
			int counter = 0;
			for (vector<string>::iterator i = mayaNames.begin(); i != mayaNames.end(); i++)
			{
				if (*i == mesh.mashName)
				{
					meshExists = true;
					Mesh newMesh = {};
					newMesh.vertexCount = vtxCount;
					newMesh.triangleCount = trisCount;

					newMesh.vertices = new float[3 * vtxCount];
					newMesh.normals = new float[3 * vtxCount];
					newMesh.texcoords = new float[2 * vtxCount];
					newMesh.indices = new unsigned short[3 * trisCount];

					memcpy(newMesh.vertices, vtxs, sizeof(float) * (vtxCount * 3));
					memcpy(newMesh.normals, norms, sizeof(float) * (vtxCount * 3));
					memcpy(newMesh.texcoords, uvs, sizeof(float) * (vtxCount * 2));
					memcpy(newMesh.indices, index, sizeof(unsigned short) * (trisCount * 3));

					rlLoadMesh(&newMesh, false);

					flatScene.at(counter).model.mesh = newMesh;

				}
				counter++;
			}
			if (!meshExists)
			{
				Mesh newMesh = {};
				newMesh.vertexCount = vtxCount;
				newMesh.triangleCount = trisCount;

				newMesh.vertices = new float[3 * vtxCount];
				newMesh.normals = new float[3 * vtxCount];
				newMesh.texcoords = new float[2 * vtxCount];
				newMesh.indices = new unsigned short[3 * trisCount];

				memcpy(newMesh.vertices, vtxs, sizeof(float) * (vtxCount * 3));
				memcpy(newMesh.normals, norms, sizeof(float) * (vtxCount * 3));
				memcpy(newMesh.texcoords, uvs, sizeof(float) * (vtxCount * 2));
				memcpy(newMesh.indices, index, sizeof(unsigned short) * (trisCount * 3));

				rlLoadMesh(&newMesh, false);

				meshes.push_back(newMesh);
				mayaNames.push_back(mesh.mashName);

				Model newModel = LoadModelFromMesh(vMesh.back());
				Material nMat = { 0 };
				nMat = LoadMaterialDefault();
				newModel.material = nMat;
				newModel.material.shader = shader;
				mPos.push_back({ newModel, MatrixTranslate(0.0f, 0.0f, 0.0f) });
			}
			delete vtxs;
			delete norms;
			delete uvs;
		}
	}
	}
}
