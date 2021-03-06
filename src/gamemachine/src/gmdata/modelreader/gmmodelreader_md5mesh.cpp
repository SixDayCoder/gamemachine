﻿#include "stdafx.h"
#include "gmmodelreader_md5mesh.h"
#include "foundation/utilities/tools.h"
#include "foundation/gamemachine.h"
#include "../gamepackage/gmgamepackage.h"
#include "foundation/utilities/utilities.h"
#include "../gmskeleton.h"
#include "foundation/gmasync.h"
#include "foundation/gamemachine.h"

// Handlers
BEGIN_DECLARE_MD5_HANDLER(MD5Version, reader, scanner, GMModelReader_MD5Mesh*)
	GMint32 version;
	scanner.nextInt(version);
	reader->setMD5Version(version);
	return true;
END_DECLARE_MD5_HANDLER()

BEGIN_DECLARE_MD5_HANDLER(commandline, reader, scanner, GMModelReader_MD5Mesh*)
	GMString content;
	scanner.next(content);
	reader->setCommandline(content);
	return true;
END_DECLARE_MD5_HANDLER()

BEGIN_DECLARE_MD5_HANDLER(numJoints, reader, scanner, GMModelReader_MD5Mesh*)
	GMint32 numJoints;
	scanner.nextInt(numJoints);
	reader->setNumJoints(numJoints);
	reader->initJoints(numJoints);
	return true;
END_DECLARE_MD5_HANDLER()

BEGIN_DECLARE_MD5_HANDLER(numMeshes, reader, scanner, GMModelReader_MD5Mesh*)
	GMint32 numMeshes;
	scanner.nextInt(numMeshes);
	reader->setNumMeshes(numMeshes);
	reader->initMeshes(numMeshes);
	return true;
END_DECLARE_MD5_HANDLER()

/*
joints {
<string:name> <int:parentIndex> ( <vec3:position> ) ( <vec3:orientation> )
...
}
*/
BEGIN_DECLARE_MD5_HANDLER(joints, reader, scanner, GMModelReader_MD5Mesh*)
	GMString content;
	scanner.next(content);
	if (content != L"{")
		return false;
	IMd5MeshHandler* nextHandler = reader->findHandler(L"joints_inner");
	GM_ASSERT(nextHandler);
	reader->setNextHandler(nextHandler);
	return true;
END_DECLARE_MD5_HANDLER()

BEGIN_DECLARE_MD5_HANDLER(joints_inner, reader, scanner, GMModelReader_MD5Mesh*)
	GMString content;
	scanner.next(content);
	if (content == L"}")
	{
		reader->setNextHandler(nullptr);
		return true;
	}
	else if (content == L"")
	{
		// 不处理空行
		return true;
	}
	else
	{
		// <string:name> <int:parentIndex> ( <vec3:position> ) ( <vec3:orientation> )
		GMModelReader_MD5Mesh_Joint joint;
		joint.name = content;
		scanner.nextInt(joint.parentIndex);
		joint.position = GMMD5VectorParser::parseVector3(scanner);
		joint.orientation = GMMD5VectorParser::parseQuatFromVector3(scanner);

		scanner.next(content);
		if (content == "//")
		{
			scanner.next(joint.annotation);
		}

		reader->addJoint(std::move(joint));
	}
	return true;
END_DECLARE_MD5_HANDLER()

/*
mesh {
shader <string:texture>
numverts <int:numVerts>
vert <int:vertexIndex> ( <vec2:texCoords> ) <int:startWeight> <int:weightCount>
numtris <int:numTriangles>
tri <int:triangleIndex> <int:vertIndex0> <int:vertIndex1> <int:vertIndex2>
numweights <int:numWeights>
weight <int:weightIndex> <int:jointIndex> <float:weightBias> ( <vec3:weightPosition> )
...
}
*/
struct Handler_mesh_inner : IMd5MeshHandler
{
	virtual bool canHandle(const GMString& tag) override
	{
		return tag == L"mesh_inner";
	}

	virtual bool handle(GMModelReader_MD5* r, GMScanner& scanner) override
	{
		GMModelReader_MD5Mesh* reader = gm_cast<GMModelReader_MD5Mesh*>(r);
		GMString content;
		scanner.next(content);

		if (content == L"shader")
		{
			scanner.next(content);
			m_cacheMesh->shader = GMModelReader_MD5::removeQuotes(content);
		}
		else if (content == L"numverts")
		{
			GMint32 n;
			scanner.nextInt(n);
			m_cacheMesh->numVertices = n;
			m_cacheMesh->vertices.resize(n);
		}
		else if (content == L"")
		{
			// 不处理空行
			return true;
		}
		else if (content == L"}")
		{
			if (m_cacheMesh)
				reader->addMesh(std::move(*m_cacheMesh));
			reader->setNextHandler(nullptr);
			return true;
		}
		else if (content == L"vert")
		{
			// vert <int:vertexIndex> ( <vec2:texCoords> ) <int:startWeight> <int:weightCount>
			GMint32 index, i;
			GMModelReader_MD5Mesh_Vertex v;
			scanner.nextInt(index);
			v.texCoords = GMMD5VectorParser::parseVector2(scanner);
			scanner.nextInt(i);
			v.startWeight = i;
			scanner.nextInt(i);
			v.weightCount = i;
			m_cacheMesh->vertices[index] = std::move(v);
		}
		else if (content == L"numtris")
		{
			GMint32 n;
			scanner.nextInt(n);
			m_cacheMesh->numTriangles = n;
			m_cacheMesh->triangleIndices.resize(n);
		}
		else if (content == L"tri")
		{
			// tri <int:triangleIndex> <int:vertIndex0> <int:vertIndex1> <int:vertIndex2>
			GMint32 index, indices[3];
			scanner.nextInt(index);
			scanner.nextInt(indices[0]);
			scanner.nextInt(indices[1]);
			scanner.nextInt(indices[2]);
			m_cacheMesh->triangleIndices[index] = { indices[0], indices[1], indices[2] };
		}
		else if (content == L"numweights")
		{
			GMint32 n;
			scanner.nextInt(n);
			m_cacheMesh->numWeights = n;
			m_cacheMesh->weights.resize(n);
		}
		else if (content == L"weight")
		{
			// weight <int:weightIndex> <int:jointIndex> <float:weightBias> ( <vec3:weightPosition> )
			GMModelReader_MD5Mesh_Weight weight;
			GMint32 index, i;
			GMfloat f;
			scanner.nextInt(index);
			scanner.nextInt(i);
			weight.jointIndex = i;
			scanner.nextFloat(f);
			weight.weightBias = f;
			weight.weightPosition = GMMD5VectorParser::parseVector3(scanner);
			m_cacheMesh->weights[index] = std::move(weight);
		}

		return true;
	}

	void reset()
	{
		m_cacheMesh.reset(new GMModelReader_MD5Mesh_Mesh());
	}

private:
	GMOwnedPtr<GMModelReader_MD5Mesh_Mesh> m_cacheMesh;
};

BEGIN_DECLARE_MD5_HANDLER(mesh, reader, scanner, GMModelReader_MD5Mesh*)
	GMString content;
	scanner.next(content);
	if (content != L"{")
		return false;
	Handler_mesh_inner* nextHandler = gm_cast<Handler_mesh_inner*>(reader->findHandler(L"mesh_inner"));
	GM_ASSERT(nextHandler);
	nextHandler->reset();
	reader->setNextHandler(nextHandler);
	return true;
END_DECLARE_MD5_HANDLER()

//////////////////////////////////////////////////////////////////////////
bool GMModelReader_MD5Mesh::load(const GMModelLoadSettings& settings, GMBuffer& buffer, REF GMAsset& asset)
{
	D(d);
	buffer.convertToStringBuffer();
	GMString text = GMString(reinterpret_cast<char*>(buffer.buffer));
	StringReader<GMwchar> sr(text.c_str());

	GMString line;
	GMString tag, content;
	line.reserve(LINE_MAX);
	while (sr.readLine(line))
	{
		GMScanner s(line);
		if (!d->nextHandler)
		{
			s.next(tag);
			IMd5MeshHandler* handler = findHandler(tag);
			if (handler)
			{
				if (!handler->handle(this, s))
					return false;
			}
		}
		else
		{
			if (!d->nextHandler->handle(this, s))
				return false;
		}
	}

	GMModels* targetModels = nullptr;
	if (asset.isEmpty())
	{
		targetModels = new GMModels();
		asset = GMAsset(GMAssetType::Models, targetModels);
	}
	else
	{
		targetModels = asset.getModels();
	}
	buildModel(settings, targetModels);
	return true;
}

bool GMModelReader_MD5Mesh::test(const GMBuffer& buffer)
{
	if (buffer.size > 13)
	{
		char head[14] = { 0 };
		GMString::stringCopyN(head, 14, reinterpret_cast<const char*>(buffer.buffer), 13);
		if (GMString::stringEquals(head, "MD5Version 10"))
			return true;
	}

	return false;
}

Vector<GMOwnedPtr<IMd5MeshHandler>>& GMModelReader_MD5Mesh::getHandlers()
{
	D(d);
	if (d->handlers.empty())
	{
		d->handlers.push_back(NEW_MD5_HANDLER(MD5Version));
		d->handlers.push_back(NEW_MD5_HANDLER(commandline));
		d->handlers.push_back(NEW_MD5_HANDLER(numJoints));
		d->handlers.push_back(NEW_MD5_HANDLER(numMeshes));
		d->handlers.push_back(NEW_MD5_HANDLER(joints));
		d->handlers.push_back(NEW_MD5_HANDLER(joints_inner));
		d->handlers.push_back(NEW_MD5_HANDLER(mesh));
		d->handlers.push_back(NEW_MD5_HANDLER(mesh_inner));
	}
	return d->handlers;
}

void GMModelReader_MD5Mesh::buildModel(const GMModelLoadSettings& settings, GMModels* models)
{
	D(d);
	if (!models)
		return;
	
	static GMint32 numberOfProcessors = GM.getRunningStates().systemInfo.numberOfProcessors;
	// 临时结构，用于缓存顶点、法线
	struct Vertex
	{
		GMVec3 position = Zero<GMVec3>();
		GMVec3 normal = Zero<GMVec3>();
		GMVec2 texcoord = Zero<GMVec2>();
	};

	for (auto& mesh : d->meshes)
	{
		GMModel* model = new GMModel();
		model->setUsageHint(GMUsageHint::DynamicDraw);
		model->getShader().setFrontFace(GMS_FrontFace::COUNTER_CLOCKWISE); //MD5采用OpenGL右手坐标系，因此向前方向是逆时针的
		models->push_back(GMAsset(GMAssetType::Model, model));
		mesh.targetModel = model;

		GMMesh* m = new GMMesh(model);
		if (!mesh.shader.isEmpty())
		{
			GMTextureAsset tex = d->shaders[mesh.shader];
			if (tex.isEmpty())
			{
				GMString imgPath = GMPath::fullname(GM.getGamePackageManager()->pathOf(GMPackageIndex::Models, settings.directory), mesh.shader);
				GMToolUtil::createTextureFromFullPath(settings.context, imgPath, tex);
			}
			if (!tex.isEmpty())
			{
				d->shaders[mesh.shader] = tex;
				GMToolUtil::addTextureToShader(model->getShader(), tex, GMTextureType::Ambient);
			}
		}

		AlignedVector<Vertex> vertices;
		vertices.resize(mesh.vertices.size());
		GMAsync::blockedAsync(
			GMAsync::Async,
			numberOfProcessors,
			mesh.vertices.begin(),
			mesh.vertices.end(),
			[&vertices, &mesh, &d](auto begin, auto end) {
				GMint32 index = begin - mesh.vertices.begin();
				for (auto iter = begin; iter != end; ++iter)
				{
					auto& vert = *iter;
					Vertex vertex;
					GMVec3 pos = Zero<GMVec3>();
					// 每个顶点的坐标由结点的权重累计计算得到
					for (GMint32 i = 0; i < vert.weightCount; ++i)
					{
						const auto& weight = mesh.weights[vert.startWeight + i];
						const auto& joint = d->joints[weight.jointIndex];
						GMVec3 rotationPos = weight.weightPosition * joint.orientation;
						pos += (joint.position + rotationPos) * weight.weightBias;
					}
					vertex.position = pos;
					vertex.texcoord = vert.texCoords;
					vertices[index++] = vertex;
				}
			}
		);

		GMAsync::blockedAsync(
			GMAsync::Async,
			numberOfProcessors,
			mesh.triangleIndices.begin(),
			mesh.triangleIndices.end(),
			[&vertices](auto begin, auto end) {
				for (auto iter = begin; iter != end; ++iter)
				{
					const auto& triIdx = *iter;
					GMVec3 v0 = vertices[triIdx[0]].position;
					GMVec3 v1 = vertices[triIdx[1]].position;
					GMVec3 v2 = vertices[triIdx[2]].position;
					GMVec3 normal = Cross(v2 - v0, v1 - v0);

					// 计算法线
					vertices[triIdx[0]].normal += normal;
					vertices[triIdx[1]].normal += normal;
					vertices[triIdx[2]].normal += normal;
				}
			}
		);

		GMAsync::blockedAsync(
			GMAsync::Async,
			numberOfProcessors,
			vertices.begin(),
			vertices.end(),
			[](auto begin, auto end) {
				for (auto iter = begin; iter != end; ++iter)
				{
					iter->normal = Normalize(iter->normal);
				}
			}
		);

		// 组装Vertex
		for (const auto& triIdx : mesh.triangleIndices)
		{
			for (GMint32 i = 0; i < 3; ++i)
			{
				GMVertex v = { 0 };
				const Vertex& vertexTemp = vertices[triIdx[i]];
				v.positions = { vertexTemp.position.getX(), vertexTemp.position.getY(), vertexTemp.position.getZ() };
				v.normals = { vertexTemp.normal.getX(), vertexTemp.normal.getY(), vertexTemp.normal.getZ() };
				v.texcoords = { vertexTemp.texcoord.getX(), vertexTemp.texcoord.getY() };
				m->vertex(v);
			}
		}
	}

	swapAll(models);
	d->shaders.clear();
}

void GMModelReader_MD5Mesh::swapAll(GMModels* models)
{
	// 将mesh数据交换到Model中
	D(d);
	GMSkeleton* skeleton = nullptr;
	if (!(skeleton = models->getSkeleton()))
	{
		skeleton = new GMSkeleton();
		models->setSkeleton(skeleton);
	}

	skeleton->getMeshes().swap(d->meshes);
}