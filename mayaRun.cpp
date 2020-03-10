#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <ComLib.cpp>

using namespace std;

static const char* panel1 = "modelPanel1";
static const char* panel2 = "modelPanel2";
static const char* panel3 = "modelPanel3";
static const char* panel4 = "modelPanel4";

MCallbackIdArray callbackIdArray;
vector<MString> objectCallbacks;
MObject m_node;
MStatus status = MS::kSuccess;
bool initBool = false;

vector<size_t> meshMessageLenght;

#pragma region INITIALIZE COMLIB
ComLib Producer("secretName", 200, ComLib::ROLE::PRODUCER);
ComLib::INFO* sendVrtInfo = new ComLib::INFO;

ComLib::INFO* sendCamInfo = new ComLib::INFO;
ComLib::CAMERA_T* sendCamData = new ComLib::CAMERA_T;

ComLib::INFO* sendMeshInfo = new ComLib::INFO;
ComLib::MESH_V* sendMeshData = new ComLib::MESH_V;

ComLib::INFO* sendLightInfo = new ComLib::INFO;
ComLib::LIGHT_T* sendLightData = new ComLib::LIGHT_T;

ComLib::INFO* sendTransformInfo = new ComLib::INFO;
ComLib::TRANSFORM_T* sendTransformData = new ComLib::TRANSFORM_T;

ComLib::INFO* sendMaterialInfo = new ComLib::INFO;
ComLib::MATERIAL_S* sendMaterialData = new ComLib::MATERIAL_S;

ComLib::INFO* sendDeleteInfo = new ComLib::INFO;
ComLib::DELETE_INFO_MSG* sendDeleteData = new ComLib::DELETE_INFO_MSG;

ComLib::INFO* sendNameChangedInfo = new ComLib::INFO;
ComLib::NAME_CHANGED_MSG* sendNameChanged = new ComLib::NAME_CHANGED_MSG;
#pragma endregion

// Vectors to keep track of changed Object values:
vector<ComLib::MESH_V> meshVec;
vector<ComLib::INFO> meshInfoVec;

vector<ComLib::LIGHT_T> lightVec;
vector<ComLib::INFO> lightInfoVec;

vector<ComLib::DELETE_INFO_MSG> deleteVec;
vector<ComLib::INFO> deleteInfoVec;

vector<ComLib::TRANSFORM_T> transformVec;
vector<ComLib::INFO> transformInfoVec;

enum NODE_TYPE { TRANSFORM, MESH };
MTimer gTimer;

// variables for mesh vertex iteration
vector<int> index;
vector<vector<float>> position;
vector<vector<float>> normal;
vector<vector<float>> uv;
vector<int> trisPerPoly;
vector<MObject> objectNodes;
vector<MObject> lightNodes;

// keep track of created meshes to maintain them
queue<MObject> newMeshes;
queue<MObject> newTransforms;
queue<MObject> newLight;
queue<MObject> newFileTexture;
queue<MObject> newCamera;
queue<MObject> newLambert;
std::vector<MDagPath> cameraPaths;

string getMeshName(MObject mesh);
MObject getHypershadeObject(string dependNodeName);

// Velues uppdated if any of the respective objects are changed.
bool camera_Changed = false;
bool light_Changed = false;
bool mesh_Changed = false;
bool transform_Changed = false;
bool material_Changed = false;
bool name_Changed = false;
bool delete_Changed = false;

// Sends info to Raylib. Here all updated object vectors are lopped through and snd sent to the Raylib.
// This function is called in Timer Callback so it wouldnt run 100 times every frame.
void infoSending(float lastTime)
{

	if (camera_Changed)
	{
		Producer.send(sendCamInfo, sendCamData, sizeof(ComLib::CAMERA_T));
	}
	if (meshVec.size() > 0)
	{
		// Loops through all meshes that were changed.
		for (int i = 0; i < meshVec.size(); i++)
		{
			sendMeshInfo->role = meshInfoVec.at(i).role;
			sendMeshInfo->variables[0] = meshInfoVec.at(i).variables[0];
			sendMeshInfo->variables[1] = meshInfoVec.at(i).variables[1];
			sendMeshInfo->variables[2] = meshInfoVec.at(i).variables[2];
			sendMeshInfo->variables[3] = meshInfoVec.at(i).variables[3];
			sendMeshInfo->variables[4] = meshInfoVec.at(i).variables[4];
			sendMeshInfo->variables[5] = meshInfoVec.at(i).variables[5];
			sendMeshInfo->variables[6] = meshInfoVec.at(i).variables[6];


			strcpy(sendMeshData->mashName, meshVec.at(i).mashName);

			sendMeshData->vertex = meshVec.at(i).vertex;
			sendMeshData->normals = meshVec.at(i).normals;
			sendMeshData->uvs = meshVec.at(i).uvs;
			sendMeshData->trianglesPerPolygon = meshVec.at(i).trianglesPerPolygon;
			sendMeshData->index = meshVec.at(i).index;

			Producer.send(sendMeshInfo, sendMeshData, meshMessageLenght.at(i));
		}
	}
	if (light_Changed)
	{
		// Loops through all lights that were changed.
		for (int i = 0; i < lightVec.size(); i++)
		{
			sendLightInfo->role = lightInfoVec.at(i).role;
			sendLightInfo->variables[0] = lightInfoVec.at(i).variables[0];
			sendLightInfo->variables[1] = lightInfoVec.at(i).variables[1];
			sendLightInfo->variables[2] = lightInfoVec.at(i).variables[2];
			sendLightInfo->variables[3] = lightInfoVec.at(i).variables[3];
			sendLightInfo->variables[4] = lightInfoVec.at(i).variables[4];

			strcpy(sendLightData->lightName, lightVec.at(i).lightName);
			sendLightData->position[0] = lightVec.at(i).position[0];
			sendLightData->position[1] = lightVec.at(i).position[1];
			sendLightData->position[2] = lightVec.at(i).position[2];
			sendLightData->position[3] = lightVec.at(i).position[3];
			sendLightData->color[0] = lightVec.at(i).color[0];
			sendLightData->color[1] = lightVec.at(i).color[1];
			sendLightData->color[2] = lightVec.at(i).color[2];
			sendLightData->color[3] = lightVec.at(i).color[3];
			sendLightData->intensity = lightVec.at(i).intensity;

			Producer.send(sendLightInfo, sendLightData, sizeof(ComLib::LIGHT_T));
		}
	}
	if (transformVec.size() > 0)
	{
		// Loops through all transforms that were changed.
		for (int i = 0; i < transformVec.size(); i++)
		{
			sendTransformInfo->role = transformInfoVec.at(i).role;
			sendTransformInfo->variables[0] = transformInfoVec.at(i).variables[0];
			sendTransformInfo->variables[1] = transformInfoVec.at(i).variables[1];


			for (int k = 0; k < 4; k++)
			{
				for (int j = 0; j < 4; j++)
				{
					sendTransformData->transformMatrix[k][j] = transformVec.at(i).transformMatrix[k][j];
				}
			}

			strcpy(sendTransformData->transformName, transformVec.at(i).transformName);

			Producer.send(sendTransformInfo, sendTransformData, sizeof(ComLib::TRANSFORM_T));
		}
	}

	if (delete_Changed)
	{
		// Loops through all delete messages that were changed.
		for (int i = 0; i < deleteVec.size(); i++)
		{
			sendDeleteInfo->role = deleteInfoVec.at(i).role;
			sendDeleteInfo->variables[0] = deleteInfoVec.at(i).variables[0];
			sendDeleteInfo->variables[1] = deleteInfoVec.at(i).variables[1];

			strcpy(sendDeleteData->deleteInfoName, deleteVec.at(i).deleteInfoName);
			sendDeleteData->role = deleteVec.at(i).role;

			Producer.send(sendDeleteInfo, sendDeleteData, sizeof(ComLib::DELETE_INFO_MSG));
		}
	}
	if (name_Changed)
	{
		Producer.send(sendNameChangedInfo, sendNameChanged, sizeof(ComLib::NAME_CHANGED_MSG));
	}

	// Reinitialize bools and clears vectors for next timer callback.
	camera_Changed = false;
	light_Changed = false;
	mesh_Changed = false;
	transform_Changed = false;
	material_Changed = false;
	name_Changed = false;
	delete_Changed = false;

	meshInfoVec.clear();
	meshVec.clear();
	meshMessageLenght.clear();

	lightInfoVec.clear();
	lightVec.clear();

	transformInfoVec.clear();
	transformVec.clear();

	deleteVec.clear();
	deleteInfoVec.clear();
}

//Recursive Function:
bool hierarchyLooper(MObject &node)
{
	MFnDagNode dn(node);
	bool found = false;

	for (int i = 0; i < dn.childCount(); i++)
	{
		MFnDependencyNode fdn(dn.child(i));

		// If next Child is a Mesh or Light node the node values are returns to be sent in Raylib.
		if (dn.child(i).hasFn(MFn::kMesh))
		{
			objectNodes.push_back(node);
			found = true;
		}
		else if (dn.child(i).hasFn(MFn::kPointLight))
		{
			lightNodes.push_back(node);
			found = true;
		}
		else if (dn.child(i).isNull())
		{
			MGlobal::displayInfo(MString("It's Null!"));
		}

		// If the Children are nor of type mesh or light their children might be that. So hierarchyLooper is called again.
		if (MFnDagNode(dn.child(i)).childCount() > 0)
		{
			if (hierarchyLooper(dn.child(i)))
			{
				found = true;
			}
		}
	}

	return found;
}

void vertexIterator(MObject &node, vector<int> &trianglesPerPolygon, vector<int> &index, vector<vector<float>> &position, vector<vector<float>> &normal, vector<vector<float>> &uv)
{
	MItMeshVertex itvertex(node, NULL);

	MItMeshFaceVertex itFace(node, NULL);

	MStatus triangleCoutStatus = MStatus::kFailure;
	MStatus vertexArrStatus = MStatus::kFailure;
	MStatus vertexFoundStatus = MStatus::kFailure;
	MStatus normalFindStatus = MStatus::kFailure;
	MStatus uvAtpointStatus = MStatus::kFailure;
	MStatus normalIDstatus = MStatus::kFailure;
	MStatus triangelOffsetStatus = MStatus::kFailure;
	MStatus uvfromIDs = MStatus::kFailure;
	MStatus normalNewTest = MStatus::kFailure;

	MStatus vertexReturn = MStatus::kFailure;

	MIntArray triangleCounts, triangleVertices;
	MIntArray vertexCount, vertexList;
	MIntArray normalCount, normalIds;
	MFnDependencyNode fn(node);

	vector<vector<float>> pPositions;
	vector<vector<float>> pNormals;
	vector<vector<float>> pUVs;
	vector<int> triIndex;

	// This returns the actual Mesh node alowing to acces its values in MSpace::kWorld.
	MDagPath path;
	MFnDagNode(node).getPath(path);
	MFnMesh mesh(path, NULL);
	MItMeshPolygon itpolygon(path, MObject::kNullObj, NULL);
	
	MIntArray triangleCounts1, triangleIndeces;
	int polynum = mesh.numPolygons(NULL);

	if (triangleCoutStatus = mesh.getTriangles(triangleCounts, triangleVertices))
	{
		for (int i = 0; i < triangleCounts.length(); i++) 
		{
			trianglesPerPolygon.push_back(triangleCounts[i]);
		}
	}

	if (triangelOffsetStatus = mesh.getTriangleOffsets(triangleCounts1, triangleIndeces))
	{
		for (int i = 0; i < triangleIndeces.length(); i++)
		{
			index.push_back(triangleIndeces[i]);
		}
	}

	MPointArray vertexArr;

	if (vertexReturn = mesh.getVertices(vertexCount, vertexList))
	{
		vector<int> uvIndex;
		for (int i = 0; i < vertexCount.length(); i++)
		{
			for (int j = 0; j < int(vertexCount[i]); j++)
			{
				int uvID;
				// Returns UV id per face, per vertex.
				if (uvAtpointStatus = mesh.getPolygonUVid(i, j, uvID, NULL))
				{
					uvIndex.push_back(uvID);
				}

				MVector normaltest;
				// Returns Polygon normals per face. 
				if (normalNewTest = mesh.getPolygonNormal(i, normaltest, MSpace::kWorld))
				{
					float nx, ny, nz;
					nx = (float)normaltest.x;
					ny = (float)normaltest.y;
					nz = (float)normaltest.z;

					vector<float> norms;
					norms.push_back(nx);
					norms.push_back(ny);
					norms.push_back(nz);
					norms.push_back(1.0f);

					normal.push_back(norms);
				}
			}
		}
		for (int i = 0; i < vertexList.length(); i++)
		{
			MPoint point;
			// Returns Vertex positions.
			float2 uvPoint;

			if (vertexFoundStatus = mesh.getPoint(vertexList[i], point, MSpace::kObject))
			{
				float x, y, z;
				x = (float)point.x;
				y = (float)point.y;
				z = (float)point.z;

				vector<float> posit;
				posit.push_back(x);
				posit.push_back(y);
				posit.push_back(z);

				position.push_back(posit);
			}
		}
		for (int i = 0; i < uvIndex.size(); i++)
		{
			float u, v;
			// Returns U and V values for uv index.
			if (uvfromIDs = mesh.getUV(uvIndex.at(i), u, v, NULL))
			{
				vector<float> uvs;
				uvs.push_back(u);
				uvs.push_back(v);

				uv.push_back(uvs);
			}
		}
	}
}

void nodeAboutToDelete(MObject &node, void *clientData)
{
	MFnDagNode dn(node);
	string nodeName = MFnDependencyNode(dn.parent(0)).name().asChar();

	if (node.hasFn(MFn::kMesh))
	{
		ComLib::DELETE_INFO_MSG tempDelete;
		ComLib::INFO tempInfo;

		tempInfo.role = ComLib::MSG_ROLE::DELETE_INFO;
		tempInfo.variables[0] = 1;
		tempInfo.variables[1] = 1;

		const char* nName = nodeName.c_str();
		strcpy(tempDelete.deleteInfoName, nName);
		tempDelete.role = ComLib::MSG_ROLE::MESH;

		deleteInfoVec.push_back(tempInfo);
		deleteVec.push_back(tempDelete);

		delete_Changed = true;
	}
	else if (node.hasFn(MFn::kLight))
	{
		ComLib::DELETE_INFO_MSG tempDelete;
		ComLib::INFO tempInfo;

		tempInfo.role = ComLib::MSG_ROLE::DELETE_INFO;
		tempInfo.variables[0] = 1;
		tempInfo.variables[1] = 1;

		const char* nName = nodeName.c_str();
		strcpy(tempDelete.deleteInfoName, nName);
		tempDelete.role = ComLib::MSG_ROLE::LIGHT;

		deleteInfoVec.push_back(tempInfo);
		deleteVec.push_back(tempDelete);

		delete_Changed = true;
	}

	MCallbackIdArray callbackIds;
	MCallbackId callbackIDid = MMessage::nodeCallbacks(node, callbackIds);
	MCallbackId removedCallbacksid = MMessage::removeCallbacks(callbackIds);
}

void cameraUpdate(const MString &str, void *clientData)
{
	MStatus result;
	M3dView view;
	result = M3dView::getM3dViewFromModelPanel((char*)clientData, view);
	MDagPath pathToCam;
	view.getCamera(pathToCam);
	MFnCamera activeCam(pathToCam);
	MFnTransform camTransform(pathToCam);

	double left, right, top, bottom;

	if (result == MS::kSuccess)
	{
		activeCam.getViewingFrustum(activeCam.aspectRatio(), left, right, bottom, top);

		MVector viewDir = activeCam.viewDirection(MSpace::kWorld, &result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get view direction for ") + activeCam.name());

		MPoint upDir = activeCam.upDirection(MSpace::kWorld, &result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get up direction for ") + activeCam.name());

		float fovy = activeCam.verticalFilmAperture(&result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get horizontal fov for ") + activeCam.name());

		double nearPlane = activeCam.nearClippingPlane(&result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get near clipping plane for ") + activeCam.name());

		double farPlane = activeCam.farClippingPlane(&result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get far clipping plane for ") + activeCam.name());

		double aspectRatio = activeCam.aspectRatio(&result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get aspect ratio for ") + activeCam.name());

		double focalLength = activeCam.focalLength(&result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get aspect ratio for ") + activeCam.name());

		double orthoWidth = activeCam.orthoWidth(&result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get ortho width for ") + activeCam.name());

		MPoint target = activeCam.centerOfInterestPoint(MSpace::kWorld, &result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get center of interest for ") + activeCam.name());

		MPoint eyePoint = activeCam.eyePoint(MSpace::kWorld, &result);
		if (result == MS::kFailure)
			MGlobal::displayInfo(MString("Failed to get eye point for ") + activeCam.name());

		sendCamInfo->role = ComLib::MSG_ROLE::CAMERA;
		sendCamInfo->variables[0] = 1;
		sendCamInfo->variables[1] = 1;


		if (activeCam.name().asChar()[0] == 112)
		{
			sendCamInfo->variables[2] = 0;
			sendCamData->fovy = fovy * 25.4f;		// verticalFilmAperture returns in inch. Convert to mm: 1inch = 25.4mm.
		}
		else
		{
			sendCamInfo->variables[2] = 1;
			sendCamData->fovy = orthoWidth;
		}
		sendCamData->cameraMatrix[0][0] = eyePoint.x;
		sendCamData->cameraMatrix[0][1] = eyePoint.y;
		sendCamData->cameraMatrix[0][2] = eyePoint.z;
		sendCamData->cameraMatrix[1][0] = target.x;
		sendCamData->cameraMatrix[1][1] = target.y;
		sendCamData->cameraMatrix[1][2] = target.z;
		sendCamData->cameraMatrix[2][0] = upDir.x;
		sendCamData->cameraMatrix[2][1] = upDir.y;
		sendCamData->cameraMatrix[2][2] = upDir.z;
		sendCamData->aspectRatio = aspectRatio;
		sendCamData->nearPlane = nearPlane;
		sendCamData->farPlane = farPlane;
		sendCamData->lrbt[0] = left;
		sendCamData->lrbt[1] = right;
		sendCamData->lrbt[2] = bottom;
		sendCamData->lrbt[3] = top;

		const char* camName = activeCam.name().asChar();
		strcpy(sendCamData->cameraName, camName);

		camera_Changed = true;

	}
}

void WorldMatrixModifiedFunction(MObject &node, MDagMessage::MatrixModifiedFlags &modified, void *clientData)
{
	MFnDependencyNode fn(node);
	MDagPath path;
	MFnDagNode(node).getPath(path);
	MFnTransform transform(path);
	MMatrix lockalTransform = transform.transformationMatrix();
	MMatrix worldTransform = path.inclusiveMatrix();

	const char* transformName = fn.name().asChar();
	char tempName[1024];
	strcpy(tempName, transformName);
	bool found = false;

	for (int i = 0; i < transformVec.size(); i++)
	{
		if (strcmp(transformName, transformVec.at(i).transformName) == 0)
		{
			transformInfoVec.at(i).role = ComLib::MSG_ROLE::TRANSFORM;
			transformInfoVec.at(i).variables[0] = 1;
			transformInfoVec.at(i).variables[1] = 1;

			for (int k = 0; k < 4; k++)
			{
				for (int j = 0; j < 4; j++)
				{
					transformVec.at(i).transformMatrix[k][j] = worldTransform[k][j];
				}
			}
			found = true;
		}
	}
	if (!found)
	{
		ComLib::TRANSFORM_T tempTransform;
		ComLib::INFO tempInfo;

		tempInfo.role = ComLib::MSG_ROLE::TRANSFORM;
		tempInfo.variables[0] = 1;
		tempInfo.variables[1] = 1;

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				tempTransform.transformMatrix[i][j] = worldTransform[i][j];
			}
		}

		const char* transformName = fn.name().asChar();
		strcpy(tempTransform.transformName, transformName);

		transformInfoVec.push_back(tempInfo);
		transformVec.push_back(tempTransform);

		transform_Changed = true;
	}
}


void nodeNameAttributeChanged(MObject &node, const MString &str, void *clientData)
{
	MFnDependencyNode fn(node);

	const char* oldName = str.asChar();
	const char* newName = fn.name().asChar();
	ComLib::MSG_ROLE role;

	if (node.hasFn(MFn::kTransform))
	{
		role = ComLib::MSG_ROLE::TRANSFORM;
	}
	else if (node.hasFn(MFn::kMesh))
	{
		role = ComLib::MSG_ROLE::MESH;
	}
	else if (node.hasFn(MFn::kLambert) || node.hasFn(MFn::kPhong) || node.hasFn(MFn::kBlinn))
	{
		role = ComLib::MSG_ROLE::MATERIAL;
	}
	else if (node.hasFn(MFn::kPointLight) || node.hasFn(MFn::kAmbientLight) || node.hasFn(MFn::kDirectionalLight) || node.hasFn(MFn::kSpotLight))
	{
		role = ComLib::MSG_ROLE::LIGHT;
	}

	sendNameChangedInfo->role = ComLib::MSG_ROLE::NAME_CHANGED;
	sendNameChangedInfo->variables[0] = 1;
	sendNameChangedInfo->variables[1] = 1;
	sendNameChangedInfo->variables[2] = 1;

	sendNameChanged->role = role;
	strcpy(sendNameChanged->oldName, oldName);
	strcpy(sendNameChanged->newName, newName);

	name_Changed = true;
}

void nodeAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	MObject node = plug.node();
	MFnDependencyNode fn(node);
	std::string nodeName = fn.name().asChar();

	float tx = 0.0f, ty = 0.0f, tz = 0.0f;

	if (MNodeMessage::AttributeMessage::kAttributeSet & msg)
	{
		if (node.hasFn(MFn::kPointLight))
		{
			MFnLight light(node, NULL);

			MColor lightColor = light.color(NULL);
			float lightIntensity = light.intensity();

			string mName = getMeshName(node);
			const char* clightName = mName.c_str();
			char tempname[1024];
			strcpy(tempname, clightName);
			bool found = false;

			for (int i = 0; i < lightVec.size(); i++)
			{
				if (strcmp(clightName, lightVec.at(i).lightName) == 0)
				{
					lightInfoVec.at(i).role = ComLib::LIGHT;
					lightInfoVec.at(i).variables[0] = 1;
					lightInfoVec.at(i).variables[1] = 1;
					//lightInfoVec.at(i).variables[2] = 0; // Color chnges doesnt mean that position does.
					lightInfoVec.at(i).variables[3] = 3;
					lightInfoVec.at(i).variables[4] = 1;

					strcpy(sendLightData->lightName, clightName);

					lightVec.at(i).type = ComLib::LIGHT_TYPE::POINT;
					lightVec.at(i).color[0] = lightColor.r;
					lightVec.at(i).color[1] = lightColor.g;
					lightVec.at(i).color[2] = lightColor.b;
					lightVec.at(i).intensity = lightIntensity;
					found = true;
				}
			}
			if (!found)
			{
				ComLib::INFO tempInfo;
				ComLib::LIGHT_T tempLight;

				tempInfo.role = ComLib::LIGHT;
				tempInfo.variables[0] = 1;
				tempInfo.variables[1] = 1;
				tempInfo.variables[2] = 0;
				tempInfo.variables[3] = 3;
				tempInfo.variables[4] = 1;

				strcpy(tempLight.lightName, clightName);

				tempLight.type = ComLib::LIGHT_TYPE::POINT;
				tempLight.color[0] = lightColor.r;
				tempLight.color[1] = lightColor.g;
				tempLight.color[2] = lightColor.b;
				tempLight.intensity = lightIntensity;

				lightInfoVec.push_back(tempInfo);
				lightVec.push_back(tempLight);

				light_Changed = true;
			}
		}
		else if (node.hasFn(MFn::kTransform))
		{

			objectNodes.clear();
			lightNodes.clear();

			MFnDagNode dagNode(node);


			if (dagNode.childCount() > 0)
			{
				if (hierarchyLooper(node))
				{
					MGlobal::displayInfo(MString("Sucsess!"));

				}
			}

			if (lightNodes.size() > 0)
			{
				for (int i = 0; i < lightNodes.size(); i++)
				{
					MDagPath lightpath;
					MFnDagNode lightDagNode(lightNodes.at(i));
					lightDagNode.getPath(lightpath);
					MFnDependencyNode newFn(lightNodes.at(i));

					MMatrix worldTransform = lightpath.inclusiveMatrix();

					std::string lightnodeName = newFn.name().asChar();
					const char* cLightName = lightnodeName.c_str();
					char tempName[1024];
					strcpy(tempName, cLightName);
					bool found = false;

					for (int i = 0; i < lightVec.size(); i++)
					{
						if (strcmp(cLightName, lightVec.at(i).lightName) == 0)
						{
							lightInfoVec.at(i).role = ComLib::LIGHT;
							lightInfoVec.at(i).variables[0] = 1;
							lightInfoVec.at(i).variables[1] = 1;
							lightInfoVec.at(i).variables[2] = 3;
							//lightInfoVec.at(i).variables[3] = 0; // If Position changed doesnt mean that color changed.
							lightInfoVec.at(i).variables[4] = 1;

							lightVec.at(i).type = ComLib::LIGHT_TYPE::POINT;
							lightVec.at(i).position[0] = worldTransform[3][0];
							lightVec.at(i).position[1] = worldTransform[3][1];
							lightVec.at(i).position[2] = worldTransform[3][2];

							strcpy(lightVec.at(i).lightName, cLightName);
							found = true;
						}
					}

					if (!found)
					{
						ComLib::INFO tempInfo;
						ComLib::LIGHT_T tempLight;

						tempInfo.role = ComLib::LIGHT;
						tempInfo.variables[0] = 1;
						tempInfo.variables[1] = 1;
						tempInfo.variables[2] = 3;
						tempInfo.variables[3] = 0;
						tempInfo.variables[4] = 1;

						tempLight.type = ComLib::LIGHT_TYPE::POINT;
						tempLight.position[0] = worldTransform[3][0];
						tempLight.position[1] = worldTransform[3][1];
						tempLight.position[2] = worldTransform[3][2];

						strcpy(tempLight.lightName, cLightName);

						lightInfoVec.push_back(tempInfo);
						lightVec.push_back(tempLight);

						light_Changed = true;
					}
				}
			}
			if (objectNodes.size() > 0)
			{
				for (int i = 0; i < objectNodes.size(); i++)
				{
					MDagPath objectpath;
					MFnDagNode ojectDagNode(objectNodes.at(i));
					ojectDagNode.getPath(objectpath);
					MFnDependencyNode newFn(objectNodes.at(i));

					MMatrix transformMMatrix = objectpath.inclusiveMatrix();

					const char* transformName = newFn.name().asChar();
					char tempName[1024];
					strcpy(tempName, transformName);
					bool found = false;

					for (int l = 0; l < transformVec.size(); l++)
					{
						if (strcmp(transformName, transformVec.at(l).transformName) == 0)
						{
							transformInfoVec.at(l).role = ComLib::MSG_ROLE::TRANSFORM;
							transformInfoVec.at(l).variables[0] = 1;
							transformInfoVec.at(l).variables[1] = 1;

							for (int k = 0; k < 4; k++)
							{
								for (int j = 0; j < 4; j++)
								{
									transformVec.at(l).transformMatrix[k][j] = transformMMatrix[k][j];
								}
							}
							found = true;
						}
					}
					if (!found)
					{
						ComLib::TRANSFORM_T tempTransform;
						ComLib::INFO tempInfo;

						tempInfo.role = ComLib::MSG_ROLE::TRANSFORM;
						tempInfo.variables[0] = 1;
						tempInfo.variables[1] = 1;

						for (int k = 0; k < 4; k++)
						{
							for (int j = 0; j < 4; j++)
							{
								tempTransform.transformMatrix[k][j] = transformMMatrix[k][j];
							}
						}
						strcpy(tempTransform.transformName, transformName);

						transformInfoVec.push_back(tempInfo);
						transformVec.push_back(tempTransform);

						transform_Changed = true;
					}
				}
			}
		}
		else if (node.hasFn(MFn::kLambert) || node.hasFn(MFn::kPhong) || node.hasFn(MFn::kBlinn))
		{
			MObject lambObj = getHypershadeObject(fn.name().asChar());
			string mName = getMeshName(lambObj);

			if (plug.partialName() == "c")
			{
				if (plug.isCompound())
				{
					float r, g, b;
					MPlug plug_x = plug.child(0);
					MPlug plug_y = plug.child(1);
					MPlug plug_z = plug.child(2);

					// get the values from the child plugs
					plug_x.getValue(r);
					plug_y.getValue(g);
					plug_z.getValue(b);

					sendMaterialInfo->role = ComLib::MSG_ROLE::MATERIAL;
					sendMaterialInfo->variables[0] = 1;
					sendMaterialInfo->variables[1] = 3;
					sendMaterialInfo->variables[2] = 0;

					const char* name = mName.c_str();
					strcpy(sendMaterialData->materialName, name);
					sendMaterialData->color[0] = r;
					sendMaterialData->color[1] = g;
					sendMaterialData->color[2] = b;
				}
			}
			if (node.hasFn(MFn::kPhong) || node.hasFn(MFn::kBlinn))
			{
				MPlug specPlug = fn.findPlug("specularColor");
				MPlug refPlug = fn.findPlug("reflectivity");

				float r, g, b;
				MPlug plug_x = specPlug.child(0);
				MPlug plug_y = specPlug.child(1);
				MPlug plug_z = specPlug.child(2);

				// get the values from the child plugs
				plug_x.getValue(r);
				plug_y.getValue(g);
				plug_z.getValue(b);

				sendMaterialInfo->variables[4] = 1;
				sendMaterialInfo->variables[5] = 1;

				sendMaterialData->specularColor[0] = r;
				sendMaterialData->specularColor[1] = g;
				sendMaterialData->specularColor[2] = b;

				float ref;
				refPlug.getValue(ref);

				sendMaterialData->reflectivity = ref;
			}
			else
			{
				sendMaterialInfo->variables[4] = 0;
				sendMaterialInfo->variables[5] = 0;
			}

			material_Changed = true;
			newLambert.push(node);
		}
		else if (node.hasFn(MFn::kFileTexture))
		{
			if (plug.partialName() == "ftn") // pratial name is shortes name posible. ftn = file texture.
			{
				if (fn.findPlug("outColor").isConnected())
				{
					MPlug outAlpha = fn.findPlug("outColor");
					MPlugArray connections;
					outAlpha.connectedTo(connections, false, true);
					
					if (connections.length() > 0)
					{
						string pathName = connections[0].name().asChar();
						int indexs = pathName.find_last_of(".");
						string mName = pathName.substr(indexs, pathName.length() - 1);

						if (mName == ".specularColor")
						{
							MGlobal::displayInfo(MString("Specular map"));
						}
						else if (mName == ".color")
						{
							MFnDependencyNode hunting(connections[0].node());

							MPlugArray shaderPlugs;
							if (hunting.findPlug("outColor").connectedTo(shaderPlugs, false, true))
							{
								for (int i = 0; i < shaderPlugs.length(); i++)
								{
									MFnDependencyNode shaderNode(shaderPlugs[i].node());
									MObject mesh = getHypershadeObject(shaderNode.name().asChar());

									if (mesh.apiType() > 0)
									{
										string nName = getMeshName(mesh);

										sendMaterialInfo->role = ComLib::MSG_ROLE::MATERIAL;
										sendMaterialInfo->variables[0] = 1;
										sendMaterialInfo->variables[1] = 0;
										sendMaterialInfo->variables[2] = 1;

										MString value;
										plug.getValue(value);

										MObject lambertObj = connections[0].node();
										strcpy(sendMaterialData->materialName, nName.c_str());
										strcpy(sendMaterialData->texturePath, value.asChar());

										newLambert.push(lambertObj);

										material_Changed = true;
									}
								}
							}
						}
					}
				}
				if (fn.findPlug("outAlpha").isConnected())
				{
					MPlug outAlpha(fn.findPlug("outAlpha"));
					MPlugArray connections;
					outAlpha.connectedTo(connections, false, true);

					if (connections.length() > 0)
					{
						string pathName = connections[0].name().asChar();
						int indexs = pathName.find_last_of(".");
						string mName = pathName.substr(indexs, pathName.length() - 1);

						MGlobal::displayInfo(mName.c_str());

						if (mName == ".diffuse")
						{
							MGlobal::displayInfo(MString("Diffuse map"));
						}
						else if (mName == ".bumpValue")
						{
							MGlobal::displayInfo(MString("Normal map"));
						}
					}
				}

				MString value;
				plug.getValue(value);
			}
		}
	}
	// Final changes to mesh are after all calculations are preformed ar found in outMesh uppdate.
	else if (MNodeMessage::AttributeMessage::kAttributeEval & msg && (plug == MFnDagNode(plug.node()).findPlug("outMesh")))
	{
		if (node.hasFn(MFn::kMesh))
		{
			index.clear();
			position.clear();
			normal.clear();
			uv.clear();
			trisPerPoly.clear();

			vertexIterator(node, trisPerPoly, index, position, normal, uv);

			string mName = getMeshName(node);
			char tempName[1024];
			strcpy(tempName, mName.c_str());
			bool found = false;
			for (int i = 0; i < meshVec.size(); i++)
			{
				if (strcmp(tempName, meshVec.at(i).mashName) == 0)
				{
					meshInfoVec.at(i).role = ComLib::MSG_ROLE::MESH;
					meshInfoVec.at(i).variables[0] = 1;
					meshInfoVec.at(i).variables[1] = 1;
					meshInfoVec.at(i).variables[2] = position.size();
					meshInfoVec.at(i).variables[3] = normal.size();
					meshInfoVec.at(i).variables[4] = uv.size();
					meshInfoVec.at(i).variables[5] = trisPerPoly.size();
					meshInfoVec.at(i).variables[6] = index.size();

					strcpy(meshVec.at(i).mashName, mName.c_str());
					meshVec.at(i).vertex = position;
					meshVec.at(i).normals = normal;
					meshVec.at(i).uvs = uv;
					meshVec.at(i).trianglesPerPolygon = trisPerPoly;
					meshVec.at(i).index = index;

					size_t extraLength = (2 * (1024 * sizeof(char))) +
						(position.size() * 3 * sizeof(float)) +
						(normal.size() * 3 * sizeof(float)) +
						(uv.size() * 2 * sizeof(float)) +
						(trisPerPoly.size() * sizeof(int)) +
						(index.size() * sizeof(int));


					meshMessageLenght.at(i) = extraLength;
					found = true;
				}
			}
			if (!found)
			{
				ComLib::INFO tempInfo;

				tempInfo.role = ComLib::MSG_ROLE::MESH;
				tempInfo.variables[0] = 1;
				tempInfo.variables[1] = 1;
				tempInfo.variables[2] = position.size();
				tempInfo.variables[3] = normal.size();
				tempInfo.variables[4] = uv.size();
				tempInfo.variables[5] = trisPerPoly.size();
				tempInfo.variables[6] = index.size();

				meshInfoVec.push_back(tempInfo);

				string mName = getMeshName(node);


				ComLib::MESH_V tempMesh;

				strcpy(tempMesh.mashName, mName.c_str());

				tempMesh.vertex = position;
				tempMesh.normals = normal;
				tempMesh.uvs = uv;
				tempMesh.trianglesPerPolygon = trisPerPoly;
				tempMesh.index = index;

				meshVec.push_back(tempMesh);

				size_t extraLength = (2 * (1024 * sizeof(char))) +
					(position.size() * 3 * sizeof(float)) +
					(normal.size() * 3 * sizeof(float)) +
					(uv.size() * 2 * sizeof(float)) +
					(trisPerPoly.size() * sizeof(int)) +
					(index.size() * sizeof(int));


				meshMessageLenght.push_back(extraLength);
				mesh_Changed = true;
			}
		}
	}
	else if (MNodeMessage::AttributeMessage::kConnectionMade & msg)
	{
		if (node.hasFn(MFn::kLambert) || node.hasFn(MFn::kPhong) || node.hasFn(MFn::kBlinn))
		{
			if (plug == MFnDependencyNode(plug.node()).findPlug("color"))
			{
				if (MFnDependencyNode(plug.node()).findPlug("color").isConnected())
				{
					MPlugArray plugArr;
					if (plug.connectedTo(plugArr, true, false, NULL))
					{
						MObject conectedNode = plugArr[0].node();
						MFnDependencyNode dnNode(conectedNode);

						MStatus statusForChange = MS::kSuccess;
						MCallbackId changId = MNodeMessage::addAttributeChangedCallback(conectedNode, nodeAttributeChanged, NULL, &statusForChange);

						if (statusForChange == MS::kSuccess)
							callbackIdArray.append(changId);

						MStatus statusForNameChange = MS::kSuccess;
						MCallbackId nameId = MNodeMessage::addNameChangedCallback(conectedNode, nodeNameAttributeChanged, NULL, &statusForNameChange);

						if (statusForNameChange == MS::kSuccess)
							callbackIdArray.append(nameId);

					}
				}
			}
			MObject lambObj = getHypershadeObject(fn.name().asChar());
			string mName = getMeshName(lambObj);

		}
		else if (node.hasFn(MFn::kMesh))
		{
			MFnMesh meshForShade(node);
			MObjectArray shaders;
			unsigned int instanceNumber = meshForShade.instanceCount(false, NULL);
			MIntArray shade_indices;
			meshForShade.getConnectedShaders(0, shaders, shade_indices);
			int lLength = shaders.length();
			MPlug surfaceShade;
			MPlugArray connectedPlugs;
			MObject connectedPlug;

			switch (shaders.length())
			{
			case 0:
				break;

			case 1:
			{
				surfaceShade = MFnDependencyNode(shaders[0]).findPlug("surfaceShader", &status);

				if (surfaceShade.isNull())
					return;

				surfaceShade.connectedTo(connectedPlugs, true, false);

				if (connectedPlugs.length() != 1)
					return;

				connectedPlug = connectedPlugs[0].node();

				// Check if other meshes had the same material and now have to be updated.
				newLambert.push(connectedPlug);
			}
			break;
			default:
			{
				break;
			}
			}
		}
	}
}


/*
* how Maya calls this method when a node is added.
* new POLY mesh: kPolyXXX, kTransform, kMesh
* new MATERIAL : kBlinn, kShadingEngine, kMaterialInfo
* new LIGHT    : kTransform, [kPointLight, kDirLight, kAmbientLight]
* new JOINT    : kJoint
*/
void nodeAdded(MObject &node, void * clientData)
{
	MFnDependencyNode fn(node);

	if (node.hasFn(MFn::kMesh))
	{
		if ((newMeshes.size() == 0) || (node != newMeshes.front()))
			newMeshes.push(node);
	}
	else if (node.hasFn(MFn::kTransform))
	{
		if ((newTransforms.size() == 0) || (node != newTransforms.front()))
			newTransforms.push(node);
	}
	else if (node.hasFn(MFn::kLambert) || node.hasFn(MFn::kPhong) || node.hasFn(MFn::kBlinn))
	{
		if ((newLambert.size() == 0) || (node != newLambert.front()))
			newLambert.push(node);
	}
	else if (node.hasFn(MFn::kCamera))
	{
		if ((newCamera.size() == 0) || (node != newCamera.front()))
			newCamera.push(node);
	}
	else if (node.hasFn(MFn::kFileTexture))
	{
		if ((newFileTexture.size() == 0) || (node != newFileTexture.front()))
			newFileTexture.push(node);
	}
	else if (node.hasFn(MFn::kPointLight))
	{
		if ((newLight.size() == 0) || (node != newLight.front()))
			newLight.push(node);
	}
}

void timerCallback(float elapsedTime, float lastTime, void*clientData)
{
	if (newMeshes.size() != 0)
	{
		MStatus statusForChange = MS::kSuccess;
		MCallbackId changId = MNodeMessage::addAttributeChangedCallback(newMeshes.front(), nodeAttributeChanged, NULL, &statusForChange);

		if (statusForChange == MS::kSuccess)
			callbackIdArray.append(changId);

		MStatus statusForNameChange = MS::kSuccess;
		MCallbackId nameId = MNodeMessage::addNameChangedCallback(newMeshes.front(), nodeNameAttributeChanged, NULL, &statusForNameChange);

		if (statusForNameChange == MS::kSuccess)
			callbackIdArray.append(nameId);

		MStatus statusForDelitingNodeId = MS::kSuccess;
		MCallbackId aboutToRemove = MNodeMessage::addNodePreRemovalCallback(newMeshes.front(), nodeAboutToDelete, NULL, &statusForDelitingNodeId);

		if (statusForDelitingNodeId == MS::kSuccess)
			callbackIdArray.append(statusForDelitingNodeId);

		MDagPath path;
		MFnDagNode meshNode(newMeshes.front());
		meshNode.getPath(path);

		MStatus statusForMatrix = MS::kSuccess;
		MCallbackId matrixId = MDagMessage::addWorldMatrixModifiedCallback(path, WorldMatrixModifiedFunction, NULL, &statusForMatrix);

		if (statusForMatrix == MS::kSuccess)
			callbackIdArray.append(matrixId);

		// Clears vectors before sending them to be filled:
		trisPerPoly.swap(vector<int>());
		index.swap(vector<int>());
		position.swap(vector<vector<float>>());
		normal.swap(vector<vector<float>>());
		uv.swap(vector<vector<float>>());

		// This is done the first time to get the first Mesh that is created in LoadPlugin.py:		
		vertexIterator(newMeshes.front(), trisPerPoly, index, position, normal, uv);

		string mName = getMeshName(newMeshes.front());
		char tempName[1024];
		strcpy(tempName, mName.c_str());
		bool found = false;
		for (int i = 0; i < meshVec.size(); i++)
		{
			if (strcmp(tempName, meshVec.at(i).mashName) == 0 )
			{
				meshInfoVec.at(i).role = ComLib::MSG_ROLE::MESH;
				meshInfoVec.at(i).variables[0] = 1;
				meshInfoVec.at(i).variables[1] = 1;
				meshInfoVec.at(i).variables[2] = position.size();
				meshInfoVec.at(i).variables[3] = normal.size();
				meshInfoVec.at(i).variables[4] = uv.size();
				meshInfoVec.at(i).variables[5] = trisPerPoly.size();
				meshInfoVec.at(i).variables[6] = index.size();

				strcpy(meshVec.at(i).mashName, mName.c_str());
				meshVec.at(i).vertex = position;
				meshVec.at(i).normals = normal;
				meshVec.at(i).uvs = uv;
				meshVec.at(i).trianglesPerPolygon = trisPerPoly;
				meshVec.at(i).index = index;

				size_t extraLength = (2 * (1024 * sizeof(char))) +
					(position.size() * 3 * sizeof(float)) +
					(normal.size() * 3 * sizeof(float)) +
					(uv.size() * 2 * sizeof(float)) +
					(trisPerPoly.size() * sizeof(int)) +
					(index.size() * sizeof(int));


				meshMessageLenght.at(i) = extraLength;
				found = true;
			}
		}
		if (!found)
		{
			ComLib::INFO tempInfo;

			tempInfo.role = ComLib::MSG_ROLE::MESH;
			tempInfo.variables[0] = 1;
			tempInfo.variables[1] = 1;
			tempInfo.variables[2] = position.size();
			tempInfo.variables[3] = normal.size();
			tempInfo.variables[4] = uv.size();
			tempInfo.variables[5] = trisPerPoly.size();
			tempInfo.variables[6] = index.size();

			meshInfoVec.push_back(tempInfo);

			string mName = getMeshName(newMeshes.front());

			ComLib::MESH_V tempMesh;

			strcpy(tempMesh.mashName, mName.c_str());

			tempMesh.vertex = position;
			tempMesh.normals = normal;
			tempMesh.uvs = uv;
			tempMesh.trianglesPerPolygon = trisPerPoly;
			tempMesh.index = index;

			meshVec.push_back(tempMesh);

			size_t extraLength = (2 * (1024 * sizeof(char))) +
				(position.size() * 3 * sizeof(float)) +
				(normal.size() * 3 * sizeof(float)) +
				(uv.size() * 2 * sizeof(float)) +
				(trisPerPoly.size() * sizeof(int)) +
				(index.size() * sizeof(int));


			meshMessageLenght.push_back(extraLength);
			mesh_Changed = true;
		}
		

		MFnMesh meshForShade(newMeshes.front());
		MObjectArray shaders;
		unsigned int instanceNumber = meshForShade.instanceCount(false, NULL);
		MIntArray shade_indices;
		meshForShade.getConnectedShaders(0, shaders, shade_indices);
		int lLength = shaders.length();
		MPlug surfaceShade;
		MPlugArray connectedPlugs;
		MObject connectedPlug;

		switch (shaders.length())
		{
		case 0:
			break;

		case 1:
		{
			surfaceShade = MFnDependencyNode(shaders[0]).findPlug("surfaceShader", &status);

			if (surfaceShade.isNull())
				return;

			surfaceShade.connectedTo(connectedPlugs, true, false);

			if (connectedPlugs.length() != 1)
				return;

			connectedPlug = connectedPlugs[0].node();

			newLambert.push(connectedPlug);
		}
		break;
		default:
		{
			break;
		}
		}
		newMeshes.pop();
	}
	else if (newTransforms.size() != 0)
	{
		MStatus statusForChange = MS::kSuccess;
		MCallbackId changId = MNodeMessage::addAttributeChangedCallback(newTransforms.front(), nodeAttributeChanged, NULL, &statusForChange);

		if (statusForChange == MS::kSuccess)
			callbackIdArray.append(changId);

		MStatus statusForNameChange = MS::kSuccess;
		MCallbackId nameId = MNodeMessage::addNameChangedCallback(newTransforms.front(), nodeNameAttributeChanged, NULL, &statusForNameChange);

		if (statusForNameChange == MS::kSuccess)
			callbackIdArray.append(nameId);

		newTransforms.pop();
	}
	else if (newCamera.size() != 0)
	{

		MStatus statusForChange = MS::kSuccess;
		MCallbackId changId = MNodeMessage::addAttributeChangedCallback(newCamera.front(), nodeAttributeChanged, NULL, &statusForChange);

		if (statusForChange == MS::kSuccess)
			callbackIdArray.append(changId);

		MStatus statusForNameChange = MS::kSuccess;
		MCallbackId nameId = MNodeMessage::addNameChangedCallback(newCamera.front(), nodeNameAttributeChanged, NULL, &statusForNameChange);

		if (statusForNameChange == MS::kSuccess)
			callbackIdArray.append(nameId);

		newCamera.pop();
	}
	else if (newLambert.size() != 0)
	{
		MFnDependencyNode frontLamb(newLambert.front());
		bool sameObjectFound = false;
		for (int i = 0; i < objectCallbacks.size(); i++)
		{
			if (objectCallbacks.at(i) == frontLamb.name())
			{
				sameObjectFound = true;
			}
		}
		if (!sameObjectFound)
		{
			MStatus statusForChange = MS::kSuccess;
			MCallbackId changId = MNodeMessage::addAttributeChangedCallback(newLambert.front(), nodeAttributeChanged, NULL, &statusForChange);

			if (statusForChange == MS::kSuccess)
				callbackIdArray.append(changId);

			MStatus statusForNameChange = MS::kSuccess;
			MCallbackId nameId = MNodeMessage::addNameChangedCallback(newLambert.front(), nodeNameAttributeChanged, NULL, &statusForNameChange);

			if (statusForNameChange == MS::kSuccess)
				callbackIdArray.append(nameId);

			objectCallbacks.push_back(frontLamb.name());
		}

		// Updates material to all meshes in scene right away:
		MString mayaCommand = "hyperShade -o "; // o stands for objects connected to that shader.
		mayaCommand += MFnDependencyNode(newLambert.front()).name().asChar(); // Sends material name that the objects are conected to.
		MGlobal::executeCommand(mayaCommand); // This comand selects all those objects
		MSelectionList selected; // Creates list for selections.
		MGlobal::getActiveSelectionList(selected); // Puts selections into list.

		MObject mesh;
		
		for (int i = 0; i < selected.length(); i++)
		{
			selected.getDependNode(i, mesh);

			MGlobal::select(mesh, MGlobal::kReplaceList);

			string mName = getMeshName(mesh);

			MPlug plug = MFnDependencyNode(newLambert.front()).findPlug("color");

			float r, g, b;
			MPlug plug_x = plug.child(0);
			MPlug plug_y = plug.child(1);
			MPlug plug_z = plug.child(2);

			// get the values from the child plugs
			plug_x.getValue(r);
			plug_y.getValue(g);
			plug_z.getValue(b);

			sendMaterialInfo->role = ComLib::MSG_ROLE::MATERIAL;
			sendMaterialInfo->variables[0] = 1;
			sendMaterialInfo->variables[1] = 3;
			sendMaterialInfo->variables[2] = 0;

			const char* name = mName.c_str();
			strcpy(sendMaterialData->materialName, name);
			sendMaterialData->color[0] = r;
			sendMaterialData->color[1] = g;
			sendMaterialData->color[2] = b;

			if (newLambert.front().hasFn(MFn::kPhong) || newLambert.front().hasFn(MFn::kBlinn))
			{
				MPlug specPlug = frontLamb.findPlug("specularColor");
				MPlug refPlug = frontLamb.findPlug("reflectivity");

				float r, g, b;
				MPlug plug_x = specPlug.child(0);
				MPlug plug_y = specPlug.child(1);
				MPlug plug_z = specPlug.child(2);

				// get the values from the child plugs
				plug_x.getValue(r);
				plug_y.getValue(g);
				plug_z.getValue(b);

				sendMaterialInfo->variables[4] = 1;
				sendMaterialInfo->variables[5] = 1;

				sendMaterialData->specularColor[0] = r;
				sendMaterialData->specularColor[1] = g;
				sendMaterialData->specularColor[2] = b;

				float ref;
				refPlug.getValue(ref);

				sendMaterialData->reflectivity = ref;
			}
			else
			{
				sendMaterialInfo->variables[4] = 0;
				sendMaterialInfo->variables[5] = 0;
			}

			if (plug.isConnected())
			{
				MPlugArray plugsFromColor;
				plug.connectedTo(plugsFromColor, true, false, NULL);

				MFnDependencyNode textDependNode(plugsFromColor[0].node());
				MPlug spacePlug = textDependNode.findPlug("fileTextureName");
				MPlug outAlpha = textDependNode.findPlug("outColor");
				MPlugArray connections;
				outAlpha.connectedTo(connections, false, true);

				if (connections.length() > 0)
				{
					string pathName = connections[0].name().asChar();
					int indexs = pathName.find_last_of(".");
					string mName = pathName.substr(indexs, pathName.length() - 1);

					if (mName == ".specularColor")
					{
						MGlobal::displayInfo(MString("Specular map"));
					}
					else if (mName == ".color")
					{
						MFnDependencyNode hunting(connections[0].node());

						MPlugArray shaderPlugs;
						if (hunting.findPlug("outColor").connectedTo(shaderPlugs, false, true))
						{
							for (int i = 0; i < shaderPlugs.length(); i++)
							{
								MFnDependencyNode shaderNode(shaderPlugs[i].node());
								MObject mesh = getHypershadeObject(shaderNode.name().asChar());

								if (mesh.apiType() > 0)
								{
									string nName = getMeshName(mesh);

									MString value;
									spacePlug.getValue(value);

									sendMaterialInfo->variables[2] = 1;

									strcpy(sendMaterialData->texturePath, value.asChar());
								}
							}

						}
					}
				}
			}
			if (frontLamb.findPlug("outAlpha").isConnected())
			{
				MPlug outAlpha(frontLamb.findPlug("outAlpha"));
				MPlugArray connections;
				outAlpha.connectedTo(connections, false, true);

				if (connections.length() > 0)
				{
					string pathName = connections[0].name().asChar();
					int indexs = pathName.find_last_of(".");
					string mName = pathName.substr(indexs, pathName.length() - 1);

					if (mName == ".diffuse")
					{
						MGlobal::displayInfo(MString("Diffuse map"));
					}
					else if (mName == ".bumpValue")
					{
						MGlobal::displayInfo(MString("Normal map"));
					}
				}
			}
			// Un-selects everything.
			MGlobal::clearSelectionList();
			Producer.send(sendMaterialInfo, sendMaterialData, sizeof(ComLib::MATERIAL_S));
		}

		newLambert.pop();
	}
	else if (newLight.size() != 0)
	{
		MStatus statusForChange = MS::kSuccess;
		MCallbackId changId = MNodeMessage::addAttributeChangedCallback(newLight.front(), nodeAttributeChanged, NULL, &statusForChange);

		if (statusForChange == MS::kSuccess)
			callbackIdArray.append(changId);

		MStatus statusForNameChange = MS::kSuccess;
		MCallbackId nameId = MNodeMessage::addNameChangedCallback(newLight.front(), nodeNameAttributeChanged, NULL, &statusForNameChange);

		if (statusForNameChange == MS::kSuccess)
			callbackIdArray.append(nameId);

		MStatus statusForDelitingNodeId = MS::kSuccess;
		MCallbackId aboutToRemove = MNodeMessage::addNodePreRemovalCallback(newLight.front(), nodeAboutToDelete, NULL, &statusForDelitingNodeId);

		if (statusForDelitingNodeId == MS::kSuccess)
			callbackIdArray.append(statusForDelitingNodeId);

		string lName = getMeshName(newLight.front());
		char tempName[1024];
		strcpy(tempName, lName.c_str());
		bool lightFound = false;

		for (int i = 0; i < lightVec.size(); i++)
		{
			if (strcmp(tempName, lightVec.at(i).lightName) == 0)
			{
				lightInfoVec.at(i).role = ComLib::MSG_ROLE::LIGHT;
				lightInfoVec.at(i).variables[0] = 1;
				lightInfoVec.at(i).variables[1] = 1;
				lightInfoVec.at(i).variables[2] = 3;
				lightInfoVec.at(i).variables[3] = 3;
				lightInfoVec.at(i).variables[4] = 1;

				strcpy(lightVec.at(i).lightName, lName.c_str());
				lightVec.at(i).position[0] = 0;
				lightVec.at(i).position[1] = 0;
				lightVec.at(i).position[2] = 0;
				lightVec.at(i).position[3] = 0;
				lightVec.at(i).color[0] = 1;
				lightVec.at(i).color[1] = 1;
				lightVec.at(i).color[2] = 1;
				lightVec.at(i).color[3] = 1;
				lightVec.at(i).intensity = 1;
				lightFound = true;
			}
		}
		if (!lightFound)
		{
			ComLib::LIGHT_T tempLight;
			ComLib::INFO tempLightInfo;

			tempLightInfo.role = ComLib::MSG_ROLE::LIGHT;
			tempLightInfo.variables[0] = 1;
			tempLightInfo.variables[1] = 1;
			tempLightInfo.variables[2] = 3;
			tempLightInfo.variables[3] = 3;
			tempLightInfo.variables[4] = 1;

			strcpy(tempLight.lightName, lName.c_str());
			tempLight.position[0] = 0;
			tempLight.position[1] = 0;
			tempLight.position[2] = 0;
			tempLight.position[3] = 0;
			tempLight.color[0] = 1;
			tempLight.color[1] = 1;
			tempLight.color[2] = 1;
			tempLight.color[3] = 1;
			tempLight.intensity = 1;

			lightInfoVec.push_back(tempLightInfo);
			lightVec.push_back(tempLight);

			light_Changed = true;
		}

		newLight.pop();
	}

	infoSending(lastTime);
}

/*
* Plugin entry point
* For remote control of maya
* open command port: commandPort -name ":1234"
* close command port: commandPort -cl -name ":1234"
* send command: see loadPlugin.py and unloadPlugin.py
*/
EXPORT MStatus initializePlugin(MObject obj) {
	MStatus res = MS::kSuccess;
	MFnPlugin myPlugin(obj, "level editor", "1.0", "Any", &res);
	if (MFAIL(res)) {
		CHECK_MSTATUS(res);
	}

	MGlobal::displayInfo("Maya plugin loaded!");
	MStatus status = MS::kSuccess;

	MCallbackId id = MTimerMessage::addTimerCallback(
		0.03,
		timerCallback,
		NULL,
		&status
	);

	MFnPlugin plugin(obj, "X", "0.1", "Any", &status);
	if (status == MS::kSuccess)
		callbackIdArray.append(id);

	MStatus status2 = MS::kSuccess;

	MCallbackId nodID = MDGMessage::addNodeAddedCallback(
		nodeAdded,
		"dependNode",
		NULL,
		&status2
	);

	if (status2 == MS::kSuccess)
		callbackIdArray.append(nodID);

	MStatus exist = MS::kSuccess;
	MItDag iterator(MItDag::kDepthFirst, MFn::kCamera, &exist);
	if (exist == MS::kSuccess)
	{
		while (!iterator.isDone())
		{
			newCamera.push(iterator.item());

			MDagPath path;
			MFnDagNode dg(iterator.item());
			dg.getPath(path);
			cameraPaths.push_back(path);
			MFnCamera fn(newCamera.front());
			MObject fnParent = fn.parent(0);

			// Because we use only one camera and only have suport for one we asignt the atribute chainged here.
			id = MNodeMessage::addAttributeChangedCallback(fnParent, nodeAttributeChanged, NULL, &exist);
			if (status == MS::kSuccess)
				callbackIdArray.append(id);

			MStatus statusForCamera = MS::kSuccess;
			MCallbackId cameraID = MNodeMessage::addAttributeChangedCallback(newCamera.front(), nodeAttributeChanged, NULL, &statusForCamera);

			if (statusForCamera == MS::kSuccess)
				callbackIdArray.append(cameraID);

			newCamera.pop();
			iterator.next();
		}
	}

	// Create an iterator, only for type kMesh, that traverses in Breadth First order
	MItDag meshIt(MItDag::kBreadthFirst, MFn::kMesh, &res);
	// use the iterator in a for loop.
	for (; !meshIt.isDone(); meshIt.next()) {
		MFnMesh myMeshFn(meshIt.currentItem());
		//MGlobal::displayInfo(MString("This is MeshIt: ") + meshIt.currentItem());
		//. . . 
	}

	// redirect cout to cerr, so that when we do cout goes to cerr
	// in the maya output window (not the scripting output!)
	cout.rdbuf(cerr.rdbuf());
	cout << "Plugin loaded ===========================" << endl;


	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(MString(panel1), cameraUpdate, (void*)panel1));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(MString(panel2), cameraUpdate, (void*)panel2));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(MString(panel3), cameraUpdate, (void*)panel3));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback(MString(panel4), cameraUpdate, (void*)panel4));


	gTimer.clear();
	gTimer.beginTimer();

	return res;
}


EXPORT MStatus uninitializePlugin(MObject obj) {
	MFnPlugin plugin(obj);
	//Free resources here:
	MMessage::removeCallbacks(callbackIdArray);
	MGlobal::displayInfo(MString("Plugin unloaded ========================="));
	return MS::kSuccess;
}

string getMeshName(MObject mesh)
{
	MDagPath path;
	MFnDagNode meshNodes(mesh);
	meshNodes.getPath(path);
	string pathName = path.fullPathName().asChar();
	int index = pathName.find_last_of("|");
	string meshName = pathName.substr(1, index - 1);

	return meshName;
}

MObject getHypershadeObject(string dependNodeName)
{
	MString mayaCommand = "hyperShade -o ";
	mayaCommand += dependNodeName.c_str();
	MGlobal::executeCommand(mayaCommand);
	MSelectionList selected;
	MGlobal::getActiveSelectionList(selected);

	MObject mesh;
	selected.getDependNode(0, mesh);

	return mesh;
}