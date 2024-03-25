#include "XASH_csm.h"
#include "util.h"
#include "log.h"

void CSMFile::parseMaterialsFromString(const std::string& materialsStr)
{
	std::istringstream ss(materialsStr);
	std::string material;
	while (ss.str().find('"', (size_t)ss.tellg()) != std::string::npos && ss >> std::quoted(material))
	{
		materials.push_back(material);
	}
}

std::string CSMFile::getStringFromMaterials()
{
	std::string ret{};
	for (auto& m : materials)
	{
		ret += "\"" + m + "\" ";
	}
	if (materials.size())
		ret.pop_back();
	return ret;
}

CSMFile::CSMFile()
{
	readed = false;
}

CSMFile::CSMFile(std::string path)
{
	readed = read(path);
}

bool CSMFile::validate()
{
	if (header.face_size != sizeof(csm_face))
	{
		print_log(PRINT_RED, "Error: Invalid CSM header face size {}!\n", header.face_size);
		return false;
	}
	if (header.vertex_size != sizeof(csm_vertex))
	{
		print_log(PRINT_RED, "Error: Invalid CSM header vertex size {}!\n", header.vertex_size);
		return false;
	}
	if (!faces.size() || !vertices.size())
	{
		print_log(PRINT_RED, "Error: Empty CSM structure!\n");
		return false;
	}
	for (auto& f : faces)
	{
		if (f.matIdx >= materials.size())
		{
			print_log(PRINT_RED, "Error: Invalid matIdx in face!\n");
			return false;
		}

		for (int i = 0; i < 3; i++)
		{
			if (f.vertIdx[i] >= vertices.size())
			{
				print_log(PRINT_RED, "Error: Invalid vertIdx[{}] in face = {}! Vertices: {}\n", i, f.vertIdx[i], vertices.size());
				return false;
			}
		}
	}
	return true;
}


bool CSMFile::read(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::binary);

	if (!file) {
		print_log(PRINT_RED, "Error: Failed to open file for reading: {}\n", filePath);
		return false;
	}

	if (!file.read(reinterpret_cast<char*>(&header), sizeof(header)))
	{
		print_log(PRINT_RED, "Error: Failed to read CSM header: {}\n", filePath);
		return false;
	}

	if (header.ident != IDCSMMODHEADER || header.version != IDCSM_VERSION)
	{
		print_log(PRINT_RED, "Error: Invalid CSM header struct version: {}\n", filePath);
		return false;
	}

	if (header.face_size != sizeof(csm_face) || header.vertex_size != sizeof(csm_vertex))
	{
		print_log(PRINT_RED, "Error: Invalid CSM header struct size: {}\n", filePath);
		return false;
	}

	std::string matstr;
	matstr.resize(header.mat_size);

	if (!file.seekg(header.mat_ofs))
	{
		print_log(PRINT_RED, "Error: Invalid CSM materials offset: {}\n", filePath);
		return false;
	}

	if (!file.read(reinterpret_cast<char*>(matstr.data()), header.mat_size))
	{
		print_log(PRINT_RED, "Error: Failed to read CSM materials: {}\n", filePath);
		return false;
	}

	if (!matstr.empty())
		parseMaterialsFromString(matstr);

	vertices.resize(header.vertex_count);

	if (!file.seekg(header.vertex_ofs))
	{
		print_log(PRINT_RED, "Error: Invalid CSM vertices offset: {}\n", filePath);
		return false;
	}

	if (!file.read(reinterpret_cast<char*>(vertices.data()), header.vertex_size * header.vertex_count))
	{
		print_log(PRINT_RED, "Error: Failed to read CSM vertices: {}\n", filePath);
		return false;
	}

	if (!file.seekg(header.faces_ofs))
	{
		print_log(PRINT_RED, "Error: Invalid CSM faces offset: {}\n", filePath);
		return false;
	}

	faces.resize(header.faces_count);

	if (!file.read(reinterpret_cast<char*>(faces.data()), header.face_size * header.faces_count))
	{
		print_log(PRINT_RED, "Error: Failed to read CSM faces: {}\n", filePath);
		return false;
	}

	file.close();

	readed = true;
	return true;
}

bool CSMFile::write(const std::string& filePath) {
	std::ofstream file(filePath, std::ios::binary);
	if (!file) {
		print_log(PRINT_RED, "Error: Failed to open file for writing: {}\n", filePath);
		return false;
	}

	header.vertex_count = (unsigned int)vertices.size();
	header.faces_count = (unsigned int)faces.size();

	if (!readed)
	{
		header.flags = 0;
	}

	header.ident = IDCSMMODHEADER;
	header.version = IDCSM_VERSION;

	std::string matstr = getStringFromMaterials();
	
	header.mat_size = (unsigned int)matstr.size() + 1;
	header.vertex_size = sizeof(csm_vertex);
	header.face_size = sizeof(csm_face);

	header.mat_ofs = sizeof(csm_header);
	header.vertex_ofs = header.mat_ofs + header.mat_size ;
	header.faces_ofs = header.vertex_ofs + (header.vertex_size * header.vertex_count);

	file.write(reinterpret_cast<char*>(&header), sizeof(header));

	matstr.push_back('\0');

	file.write(reinterpret_cast<const char*>(matstr.data()), header.mat_size);
	//matstr.pop_back(); -- if need do something with matstr

	file.write(reinterpret_cast<const char*>(vertices.data()), header.vertex_size * header.vertex_count);
	file.write(reinterpret_cast<const char*>(faces.data()), header.face_size * header.faces_count);

	file.close();
	return true;
}
