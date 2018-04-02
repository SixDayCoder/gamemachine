﻿#include "stdafx.h"
#include "gmmodel.h"
#include <linearmath.h>
#include <algorithm>
#include <iterator>
#include "foundation/gamemachine.h"

#define SAFE_ASSIGN(mb, df, pos) ((pos) >= (mb).size()) ? df : (mb[pos])
#define VERTEX_OFFSET(offset, idx) ((offset * GMModel::PositionDimension) + idx)
#define UV_OFFSET(offset, idx) ((offset << 1) + idx)

void GMModelPainter::packData(Vector<GMVertex>& packedData)
{
	D(d);
	GMVertex vd = { 0 };
	GMModel* model = getModel();
	// 按照position的size()/3来分配顶点
	GM_ASSERT(model->getMesh()->positions().size() % 3 == 0);
	for (GMuint i = 0; i < model->getMesh()->positions().size() / 3; ++i)
	{
		{
			auto& data_ref = model->getMesh()->positions();
			vd.vertices[0] = data_ref[i * 3];
			vd.vertices[1] = data_ref[i * 3 + 1];
			vd.vertices[2] = data_ref[i * 3 + 2];
		}

		{
			auto& data_ref = model->getMesh()->normals();
			vd.normals[0] = SAFE_ASSIGN(data_ref, 0, i * 3);
			vd.normals[1] = SAFE_ASSIGN(data_ref, 0, i * 3 + 1);
			vd.normals[2] = SAFE_ASSIGN(data_ref, 0, i * 3 + 2);
		}

		{
			auto& data_ref = model->getMesh()->texcoords();
			vd.texcoords[0] = SAFE_ASSIGN(data_ref, 0, i * 2);
			vd.texcoords[1] = SAFE_ASSIGN(data_ref, 0, i * 2 + 1);
		}

		{
			auto& data_ref = model->getMesh()->tangents();
			vd.tangents[0] = SAFE_ASSIGN(data_ref, 0, i * 3);
			vd.tangents[1] = SAFE_ASSIGN(data_ref, 0, i * 3 + 1);
			vd.tangents[2] = SAFE_ASSIGN(data_ref, 0, i * 3 + 2);
		}

		{
			auto& data_ref = model->getMesh()->bitangents();
			vd.bitangents[0] = SAFE_ASSIGN(data_ref, 0, i * 3);
			vd.bitangents[1] = SAFE_ASSIGN(data_ref, 0, i * 3 + 1);
			vd.bitangents[2] = SAFE_ASSIGN(data_ref, 0, i * 3 + 2);
		}

		{
			auto& data_ref = model->getMesh()->lightmaps();
			vd.lightmaps[0] = SAFE_ASSIGN(data_ref, 0, i * 2);
			vd.lightmaps[1] = SAFE_ASSIGN(data_ref, 0, i * 2 + 1);
		}

		{
			auto& data_ref = model->getMesh()->colors();
			vd.color[0] = SAFE_ASSIGN(data_ref, 0, i * 4);
			vd.color[1] = SAFE_ASSIGN(data_ref, 0, i * 4 + 1);
			vd.color[2] = SAFE_ASSIGN(data_ref, 0, i * 4 + 2);
			vd.color[3] = SAFE_ASSIGN(data_ref, 0, i * 4 + 3);
		}

		packedData.push_back(vd);
	}
}

GMModel::GMModel()
{
	D(d);
	d->mesh = new GMMesh();
}

GMModel::GMModel(GMModel& model)
{
	D(d);
	GMMesh* mesh = model.getMesh();

	if (!model.getPainter())
		GM.createModelPainterAndTransfer(&model);
	needNotTransferAnymore(); // 不需要再将顶点数据传输到显卡了

	d->mesh = new GMMesh();
	d->mesh->setMeshData(mesh->getMeshData());
	d->mesh->setArrangementMode(mesh->getArrangementMode());
	d->mesh->setName(mesh->getName());

	GM_COPY_VERTEX_PROPERTY(d->mesh, mesh, positions);
	GM_COPY_VERTEX_PROPERTY(d->mesh, mesh, normals);
	GM_COPY_VERTEX_PROPERTY(d->mesh, mesh, texcoords);
	GM_COPY_VERTEX_PROPERTY(d->mesh, mesh, tangents);
	GM_COPY_VERTEX_PROPERTY(d->mesh, mesh, bitangents);
	GM_COPY_VERTEX_PROPERTY(d->mesh, mesh, lightmaps);
	GM_COPY_VERTEX_PROPERTY(d->mesh, mesh, colors);

	// 开始拷贝components中的shaders部分
	auto& srcComponents = model.getMesh()->getComponents();
	for (auto& components : srcComponents)
	{
		GMComponent* c = new GMComponent(d->mesh);
		*c = *components;
		c->setParentMesh(d->mesh); //将mesh设置回来
	}
}

GMModel::~GMModel()
{
	D(d);
	GM_delete(d->mesh);
}

void GMModel::releaseMesh()
{
	D(d);
	d->mesh->releaseMeshData();
}

GMMeshData::GMMeshData()
{
	D(d);
	d->ref = 1;
	GM.getFactory()->createPainter(nullptr, nullptr, &d->painter);
}

GMMeshData::~GMMeshData()
{
	D(d);
	GM_delete(d->painter);
}

void GMMeshData::dispose()
{
	D(d);
	d->painter->dispose(this);
}

GMComponent::GMComponent(GMMesh* parent)
{
	D(d);
	d->parentMesh = parent;
	setVertexOffset(d->parentMesh->positions().size() / GMModel::PositionDimension);
	d->parentMesh->appendComponent(this);
}

void GMComponent::clear()
{
	D(d);
	d->primitiveCount = 0;
	d->primitiveVertices.clear();
	d->vertexOffsets.clear();
}

void GMComponent::setVertexOffset(GMuint offset)
{
	D(d);
	d->offset = offset;
}

void GMComponent::beginFace()
{
	D(d);
	d->currentFaceVerticesCount = 0;
}

void GMComponent::vertex(GMfloat x, GMfloat y, GMfloat z)
{
	D(d);
	auto& vertices = d->parentMesh->positions();
	vertices.push_back(x);
	vertices.push_back(y);
	vertices.push_back(z);
	d->currentFaceVerticesCount++;
}

void GMComponent::normal(GMfloat x, GMfloat y, GMfloat z)
{
	D(d);
	auto& normals = d->parentMesh->normals();
	normals.push_back(x);
	normals.push_back(y);
	normals.push_back(z);
}

void GMComponent::texcoord(GMfloat u, GMfloat v)
{
	D(d);
	auto& texcoords = d->parentMesh->texcoords();
	texcoords.push_back(u);
	texcoords.push_back(v);
}

void GMComponent::lightmap(GMfloat u, GMfloat v)
{
	D(d);
	auto& lightmaps = d->parentMesh->lightmaps();
	lightmaps.push_back(u);
	lightmaps.push_back(v);
}

void GMComponent::color(GMfloat r, GMfloat g, GMfloat b, GMfloat a)
{
	D(d);
	auto& colors = d->parentMesh->colors();
	colors.push_back(r);
	colors.push_back(g);
	colors.push_back(b);
	colors.push_back(a);
}

void GMComponent::endFace()
{
	D(d);
	d->vertexOffsets.push_back(d->primitiveVertices.empty() ?
		d->offset : d->vertexOffsets.back() + d->primitiveVertices.back()
	);
	d->primitiveVertices.push_back(d->currentFaceVerticesCount);
	d->primitiveCount++;
}

GMMesh::GMMesh()
{
	D(d);
	d->meshData = new GMMeshData();
}

GMMesh::~GMMesh()
{
	D(d);
	releaseMeshData();
	for (auto component : d->components)
	{
		GM_delete(component);
	}
}

void GMMesh::appendComponent(AUTORELEASE GMComponent* component)
{
	D(d);
	GM_ASSERT(std::find(d->components.begin(), d->components.end(), component) == d->components.end());
	d->components.push_back(component);
}

void GMMesh::calculateTangentSpace()
{
	D(d);
	if (d->texcoords.size() == 0)
		return;

	for (auto component : d->components)
	{
		for (GMuint i = 0; i < component->getPrimitiveCount(); i++)
		{
			// 每一个多边形拥有一个切线空间，这意味着多于3个点的多边形需要共面，否则切线空间会有问题
			GMint o = component->getOffsetPtr()[i];

			GMVec3 e0(d->positions[VERTEX_OFFSET(o, 0)], d->positions[VERTEX_OFFSET(o, 1)], d->positions[VERTEX_OFFSET(o, 2)]);
			GMVec3 e1(d->positions[VERTEX_OFFSET(o, 3)], d->positions[VERTEX_OFFSET(o, 4)], d->positions[VERTEX_OFFSET(o, 5)]);
			GMVec3 e2(d->positions[VERTEX_OFFSET(o, 6)], d->positions[VERTEX_OFFSET(o, 7)], d->positions[VERTEX_OFFSET(o, 8)]);

			GMVec2 uv0(d->texcoords[UV_OFFSET(o, 0)], d->texcoords[UV_OFFSET(o, 1)]);
			GMVec2 uv1(d->texcoords[UV_OFFSET(o, 2)], d->texcoords[UV_OFFSET(o, 3)]);
			GMVec2 uv2(d->texcoords[UV_OFFSET(o, 4)], d->texcoords[UV_OFFSET(o, 5)]);

			GMVec3 E1 = e1 - e0;
			GMVec3 E2 = e2 - e0;
			GMVec2 deltaUV1 = uv1 - uv0;
			GMVec2 deltaUV2 = uv2 - uv0;
			const GMfloat& t1 = deltaUV1.getX();
			const GMfloat& b1 = deltaUV1.getY();
			const GMfloat& t2 = deltaUV2.getX();
			const GMfloat& b2 = deltaUV2.getY();

			GMFloat4 f4_E1, f4_E2;
			E1.loadFloat4(f4_E1);
			E2.loadFloat4(f4_E2);

			GMfloat s = 1.0f / (t1*b2 - b1*t2);

			GMfloat tangents[3] = {
				s * (b2 * f4_E1[0] - b1 * f4_E2[0]),
				s * (b2 * f4_E1[1] - b1 * f4_E2[1]),
				s * (b2 * f4_E1[2] - b1 * f4_E2[2])
			};
			GMfloat bitangents[3] = {
				s * (-t2 * f4_E1[0] + t1 * f4_E2[0]),
				s * (-t2 * f4_E1[1] + t1 * f4_E2[1]),
				s * (-t2 * f4_E1[2] + t1 * f4_E2[2])
			};

			GMVec3 tangentVector = FastNormalize(GMVec3(tangents[0], tangents[1], tangents[2]));
			GMVec3 bitangentVector = FastNormalize(GMVec3(bitangents[0], bitangents[1], bitangents[2]));
			GMFloat4 f4_tangentVector, f4_bitangentVector;
			tangentVector.loadFloat4(f4_tangentVector);
			bitangentVector.loadFloat4(f4_bitangentVector);

			GMint verticesCount = component->getPrimitiveVerticesCountPtr()[i];
			for (GMint j = 0; j < verticesCount; j++)
			{
				d->tangents.push_back(f4_tangentVector[0]);
				d->tangents.push_back(f4_tangentVector[1]);
				d->tangents.push_back(f4_tangentVector[2]);

				d->bitangents.push_back(f4_bitangentVector[0]);
				d->bitangents.push_back(f4_bitangentVector[1]);
				d->bitangents.push_back(f4_bitangentVector[2]);
			}
		}
	}
}

void GMMesh::releaseMeshData()
{
	D(d);
	d->meshData->releaseRef();
	if (d->meshData->hasNoRef())
		GM_delete(d->meshData);
}

void GMMesh::setMeshData(GMMeshData* md)
{
	D(d);
	if (d->meshData != md)
	{
		GM_delete(d->meshData);
		d->meshData = md;
		md->addRef();
	}
}