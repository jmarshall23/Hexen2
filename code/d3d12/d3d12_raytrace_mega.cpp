// d3d12_raytrace_mega.cpp
//

#include "d3d12_local.h"
#include "../libs/xml/tinyxml2.h"

struct MegaEntry_t {
	char name[512];
	int x;
	int y;
	int w;
	int h;
};

std::vector<MegaEntry_t> megaEntries;

void Tileset_ParseTile(tinyxml2::XMLNode* tile) {
//	tinyxml2::XMLNode* KeyNode = tile->FirstChildElement("Key");
//	tinyxml2::XMLNode* NameNode = KeyNode->FirstChildElement("Name");
//	tinyxml2::XMLNode* ShapeNode = KeyNode->FirstChildElement("Shape");
//	tinyxml2::XMLNode* ValueNode = tile->FirstChildElement("Value");
//	tinyxml2::XMLNode* FramesNode = ValueNode->FirstChildElement("Frames");
//	tinyxml2::XMLNode* FrameNode = FramesNode->FirstChildElement("Frame");
//
//	tileRule.name = NameNode->FirstChild()->ToText()->Value();
//	tileRule.hash = generateHashValue(tileRule.name.c_str(), tileRule.name.size());
//	tileRule.shape = atoi(ShapeNode->FirstChild()->ToText()->Value());
//
//	while (FrameNode != NULL) {
//		tileRule.frames.push_back(FrameNode->FirstChild()->ToText()->Value());
//		FrameNode = FrameNode->NextSiblingElement("Frame");
//	}

	tinyxml2::XMLElement* elem = (tinyxml2::XMLElement*)tile;
	const tinyxml2::XMLAttribute* attribute = elem->FirstAttribute();

	MegaEntry_t entry;
	strcpy(entry.name, attribute->Value());
	attribute = attribute->Next();
	entry.x = atoi(attribute->Value());
	attribute = attribute->Next();
	entry.y = atoi(attribute->Value());
	attribute = attribute->Next();
	entry.w = atoi(attribute->Value());
	attribute = attribute->Next();
	entry.h = atoi(attribute->Value());
	megaEntries.push_back(entry);
}

void GL_LoadMegaXML(const char *path) {
	tinyxml2::XMLDocument doc;
	doc.LoadFile(path);

	Con_Printf("Loading Mega XML %s...\n");

	tinyxml2::XMLElement* root = doc.FirstChildElement();
	if (root == NULL) {
		Sys_Error("Failed to load mega XML");
		return;
	}

	tinyxml2::XMLNode* tilesetTypeClassNode = root->FirstChild();
	tinyxml2::XMLNode* tile = tilesetTypeClassNode;
	while (tile != NULL) {
		Tileset_ParseTile(tile);
		tile = tile->NextSiblingElement("sprite");
	}

	Con_Printf("%d mega entries...\n", megaEntries.size());
}