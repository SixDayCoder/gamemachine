﻿#include "stdafx.h"
#include "gmassets.h"

#define RESERVED 1024

GMAssets::GMAssets()
{
	D(d);
	d->root = new GMAssetsNode();
	d->root->name = "root";
	d->orphans.reserve(RESERVED);

	// 创建默认目录
	createNodeFromPath(GM_ASSET_MODELS);
	createNodeFromPath(GM_ASSET_TEXTURES);
	createNodeFromPath(GM_ASSET_LIGHTMAPS);
}

GMAssets::~GMAssets()
{
	D(d);
	clearChildNode(d->root);
	clearOrphans();
}

void GMAssets::clearOrphans()
{
	D(d);
	for (auto& node : d->orphans)
	{
		deleteAsset(node);
		delete node;
	}
}

GMAsset GMAssets::insertAsset(GMAssetType type, void* asset)
{
	D(d);
	// 使用匿名的asset，意味着它不需要被第二次找到，直接放入vector
	GMAssetsNode* node = new GMAssetsNode();
	node->asset.type = type;
	node->asset.asset = asset;
	d->orphans.push_back(node);
	return node->asset;
}

GMAsset GMAssets::insertAsset(const char* path, const GMAssetName& name, GMAssetType type, void* asset)
{
	GMAssetsNode* node = nullptr;
	// 把(a/b, c/d)字符串类型的参数改写为(a/b/c, d)
	char newPath[GMAssetName::NAME_MAX], newName[GMAssetName::NAME_MAX];
	combinePath({ path, name.name.data() }, newPath, newName);
	node = getNodeFromPath(newPath, true);
	node = makeChild(node, newName);
	GM_ASSERT(node);
	node->name = newName;
	node->asset.type = type;
	node->asset.asset = asset;
	return node->asset;
}

inline bool splash(char in)
{
	return in == '/' || in == '\\';
}

GMAssetsNode* GMAssets::getNodeFromPath(const char* path, bool createIfNotExists)
{
	D(d);
	GMAssetsNode* node = d->root;
	return getNodeFromPath(node, path, createIfNotExists);
}

void GMAssets::createNodeFromPath(const char* path)
{
	getNodeFromPath(path, true);
}

GMAsset GMAssets::createIsolatedAsset(GMAssetType type, void* data)
{
	GMAsset ret;
	ret.type = type;
	ret.asset = data;
	return ret;
}

GMAssetsNode* GMAssets::findChild(GMAssetsNode* parentNode, const GMAssetName& name, bool createIfNotExists)
{
	if (!parentNode)
		return nullptr;

	auto iter = parentNode->childs.find(name);
	if (iter != parentNode->childs.end())
		return iter->second;

	GM_ASSERT(iter == parentNode->childs.end());
	if (!createIfNotExists)
		return nullptr;

	GMAssetsNode* node = new GMAssetsNode();
	node->name = name;
	parentNode->childs.insert({ name, node });
	return node;
}

GMAssetsNode* GMAssets::makeChild(GMAssetsNode* parentNode, const GMAssetName& name)
{
	GMAssetsNode* node = new GMAssetsNode();
	node->name = name;
	parentNode->childs.insert({ name, node });
	return node;
}

GMString GMAssets::combinePath(std::initializer_list<GMString> args, REF char* path, REF char* lastPart)
{
	GMString result;
	char name[GMAssetName::NAME_MAX];
	for (auto arg : args)
	{
		std::string l = arg.toStdString();
		Scanner s(l.c_str(), splash);
		s.next(name);
		while (*name)
		{
			if (lastPart)
				strcpy_s(lastPart, GMAssetName::NAME_MAX, name);
			if (path)
				strcpy_s(path, GMAssetName::NAME_MAX, result.toStdString().c_str());
			
			result += name;
			result += "/";
			s.next(name);
		}
	}
	return result;
}

GMAssetsNode* GMAssets::getNodeFromPath(GMAssetsNode* beginNode, const char* path, bool createIfNotExists)
{
	Scanner s(path, splash);
	GMAssetsNode* node = beginNode;
	if (!node)
		return node;

	char sep[LINE_MAX];
	s.next(sep);
	while (*sep)
	{
		node = findChild(node, sep, createIfNotExists);
		if (!node)
			return nullptr;
		s.next(sep);
	}
	return node;
}

void GMAssets::clearChildNode(GMAssetsNode* self)
{
	auto& childs = self->childs;
	for (auto& node : childs)
	{
		// 递归清理子对象
		clearChildNode(node.second);
	}

	// 清理自身
	deleteAsset(self);
	delete self;
}

void GMAssets::deleteAsset(GMAssetsNode* node)
{
	switch (node->asset.type)
	{
	case GMAssetType::None:
		break;
	case GMAssetType::Texture:
		delete getTexture(node->asset);
		break;
	case GMAssetType::Model:
		delete getModel(node->asset);
		break;
	default:
		GM_ASSERT(false);
		break;
	}
}