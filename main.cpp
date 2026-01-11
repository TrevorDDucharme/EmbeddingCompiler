#include "ArgParser.hpp"
#include <zstd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>

//use zstd to compress the crap out of the input buffer
std::vector<unsigned char> create_zip_in_memory(std::vector<unsigned char> &inputBuffer, int level = 19)
{
	std::vector<unsigned char> zipBuffer;
	size_t const cBuffOutSize = ZSTD_compressBound(inputBuffer.size());
	zipBuffer.resize(cBuffOutSize);
	size_t const cSize = ZSTD_compress(zipBuffer.data(), cBuffOutSize, inputBuffer.data(), inputBuffer.size(), level);
	zipBuffer.resize(cSize);
	return zipBuffer;
}

int main(int argc, char *argv[])
{
	argumentParser parser;
	Flag help("help", false, [&]()
			  {
		parser.printUsage();
		exit(0); });
	parser.addFlag(&help);
	Parameter output("outputDirectory", true);
	parser.addParameter(&output);
	Parameter file_name("fileName", false);
	parser.addParameter(&file_name);
	if (!parser.parse(argc, argv))
	{
		exit(1);
	}
	if (help.getValue())
	{
		parser.printUsage();
		exit(0);
	}

	printf("Output Directory: %s\n", output.getValue().c_str());
	printf("File Name: %s\n", file_name.getValue().c_str());

	std::vector<unsigned char> inputBuffer;
	std::istreambuf_iterator<char> begin(std::cin), end;
	inputBuffer = std::vector<unsigned char>(begin, end);


	std::vector<unsigned char> zipBuffer = create_zip_in_memory(inputBuffer);
	printf("Creating source files\n");
	std::ofstream header(output.getValue()+"/"+file_name.getValue()+".hpp");
	header << "#pragma once\n";
	header << "unsigned char *get" << file_name.getValue() << "MemoryFile();\n";
	header << "unsigned int get" << file_name.getValue() << "MemoryFileSize();\n";
	header.close();

	header.open(output.getValue()+"/"+file_name.getValue()+"EmbeddedVFS.hpp");
	header << "#pragma once\n";
	header << "#include \"" << file_name.getValue() << ".hpp\"\n";
	header << "#include <iostream>\n";
	header << "#include <stdlib.h>\n";
	header << "#include <string.h>\n";
	header << "#include <zstd.h>\n";
	header << "#include <physfs.h>\n";
	header << "#include <vector>\n";
	header << "#include <string>\n";
	header << "bool init" << file_name.getValue() << "EmbeddedVFS(char* programName);\n";
	header << "unsigned char* decompress"<<file_name.getValue()<<"ZipInMemory(unsigned char* inputBuffer, unsigned int inputBufferSize);\n";
	header << "bool mount" << file_name.getValue() << "EmbeddedVFS();\n";
	header << "std::vector<std::string> list" << file_name.getValue() << "EmbeddedFiles(const char* path, bool fullPath = false);\n";
	header << "std::vector<std::string> list" << file_name.getValue() << "EmbeddedFilesRelativeTo(const char* path, const char* relativeTo);\n";
	header << "std::vector<unsigned char> load" << file_name.getValue() << "EmbeddedFile(const char* path);\n";
	header << "std::string load" << file_name.getValue() << "EmbeddedFileAsString(const char* path);\n";
	header << "bool exists" << file_name.getValue() << "EmbeddedFile(const char* path);\n";
	header << "bool is" << file_name.getValue() << "EmbeddedFolder(const char* path);\n";
	header << "bool is" << file_name.getValue() << "EmbeddedFile(const char* path);\n";
	header << "bool extract" << file_name.getValue() << "To(const char* embeddedPath, const char* realPath);\n";


	std::ofstream source(output.getValue()+"/"+file_name.getValue()+".cpp");
	source << "#include \"" << file_name.getValue() << ".hpp\"\n";
	source << "unsigned char " << file_name.getValue() << "MemoryFile[] = {\n";
	source << std::hex;
	for (int i = 0; i < zipBuffer.size(); i++)
	{
		source << "0x" << (int)zipBuffer[i];
		if (i != zipBuffer.size() - 1)
		{
			source << ",";
		}
		if (i % 16 == 0&&i!=0)
		{
			source << "\n";
		}
	}
	source << std::dec;
	source << "};\n";

	//set back to normal
	source << std::dec;

	source << "unsigned int " << file_name.getValue() << "MemoryFileSize = " << zipBuffer.size() << ";\n";

	source << "unsigned char* get" << file_name.getValue() << "MemoryFile()\n";
	source << "{\n";
	source << "\treturn " << file_name.getValue() << "MemoryFile;\n";
	source << "}\n";

	source << "unsigned int get" << file_name.getValue() << "MemoryFileSize()\n";
	source << "{\n";
	source << "\treturn " << file_name.getValue() << "MemoryFileSize;\n";
	source << "}\n";
	source.close();

	source.open(output.getValue()+"/"+file_name.getValue()+"EmbeddedVFS.cpp");
	source << "#include \"" << file_name.getValue() << "EmbeddedVFS.hpp\"\n";
	
	source << "static unsigned char* inMemFilesystem = nullptr;\n";
	source << "static unsigned int inMemFilesystemSize = 0;\n\n";
	
	source << "unsigned char* decompress"<<file_name.getValue()<<"ZipInMemory(unsigned char* inputBuffer, unsigned int inputBufferSize, unsigned int &outputSize)" << "\n";
	source << "{\n";
	source << "\tsize_t const cBuffOutSize = ZSTD_getFrameContentSize(inputBuffer, inputBufferSize);\n";
	source << "\tunsigned char* zipBuffer = (unsigned char*)malloc(cBuffOutSize);\n";
	source << "\tsize_t const cSize = ZSTD_decompress(zipBuffer, cBuffOutSize, inputBuffer, inputBufferSize);\n";
	source << "\tzipBuffer = (unsigned char*)realloc(zipBuffer, cSize);\n";
	source << "\toutputSize = cSize;\n";
	source << "\treturn zipBuffer;\n";
	source << "}\n\n";

	source << "bool init" << file_name.getValue() << "EmbeddedVFS(char* programName)\n";
	source << "{\n";
	source << "\tstatic bool isInitialized = false;\n";
	source << "\tif (isInitialized)\n";
	source << "\t{\n";
	source << "\t    return true;\n";
	source << "\t}\n";
	source << "\tif (PHYSFS_init(programName) == 0)\n";
	source << "\t{\n";
	source << "\t    std::cerr << \"Failed to initialize PhysFS: \" << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) << std::endl;\n";
	source << "\t    isInitialized = false;\n";
	source << "\t}\n";
	source << "\telse\n";
	source << "\t{\n";
	source << "\t    isInitialized = true;\n";
	source << "\t}\n";
	source << "\treturn isInitialized;\n";
	source << "}\n\n";

	source << "bool mount" << file_name.getValue() << "EmbeddedVFS()\n";
	source << "{\n";
	source << "\tunsigned int outputSize = 0;\n";
	source << "\tinMemFilesystem = decompress"<<file_name.getValue()<<"ZipInMemory(get"<<file_name.getValue()<<"MemoryFile(), get"<<file_name.getValue()<<"MemoryFileSize(), outputSize);\n";
	source << "\tinMemFilesystemSize = outputSize;\n";
	source << "\tif (PHYSFS_mountMemory(inMemFilesystem, inMemFilesystemSize, nullptr, \"/\", \"/\", 0) == 0)\n";
	source << "\t{\n";
	source << "\t\tstd::cerr << \"Failed to mount memory filesystem: \" << PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()) << std::endl;\n";
	source << "\t\treturn false;\n";
	source << "\t}\n";
	source << "\treturn true;\n";
	source << "}\n\n";

	source << "std::vector<std::string> list" << file_name.getValue() << "EmbeddedFiles(const char* path, bool fullPath)\n";
	source << "{\n";
	source << "\tstd::vector<std::string> files;\n";
	source << "\tchar **rc = PHYSFS_enumerateFiles(path);\n";
	source << "\tchar **i;\n";
	source << "\tfor (i = rc; *i != nullptr; i++)\n";
	source << "\t{\n";
	source << "\t\tif (fullPath) {\n";
	source << "\t\t\tstd::string full = std::string(path);\n";
	source << "\t\t\tif (full.back() != '/' && full.back() != '\\0') full += '/';\n";
	source << "\t\t\tfull += *i;\n";
	source << "\t\t\tfiles.push_back(full);\n";
	source << "\t\t} else {\n";
	source << "\t\t\tfiles.push_back(*i);\n";
	source << "\t\t}\n";
	source << "\t}\n";
	source << "\tPHYSFS_freeList(rc);\n";
	source << "\treturn files;\n";
	source << "}\n\n";

	source << "std::vector<std::string> list" << file_name.getValue() << "EmbeddedFilesRelativeTo(const char* path, const char* relativeTo)\n";
	source << "{\n";
	source << "\tstd::vector<std::string> files;\n";
	source << "\tchar **rc = PHYSFS_enumerateFiles(path);\n";
	source << "\tchar **i;\n";
	source << "\tstd::string relTo = std::string(relativeTo);\n";
	source << "\tif (!relTo.empty() && relTo.back() != '/' && relTo.back() != '\\0') relTo += '/';\n";
	source << "\tfor (i = rc; *i != nullptr; i++)\n";
	source << "\t{\n";
	source << "\t\tstd::string full = std::string(path);\n";
	source << "\t\tif (full.back() != '/' && full.back() != '\\0') full += '/';\n";
	source << "\t\tfull += *i;\n";
	source << "\t\tif (full.find(relTo) == 0) {\n";
	source << "\t\t\tfiles.push_back(full.substr(relTo.length()));\n";
	source << "\t\t} else {\n";
	source << "\t\t\tfiles.push_back(full);\n";
	source << "\t\t}\n";
	source << "\t}\n";
	source << "\tPHYSFS_freeList(rc);\n";
	source << "\treturn files;\n";
	source << "}\n\n";

	source << "std::vector<unsigned char> load" << file_name.getValue() << "EmbeddedFile(const char* path)\n";
	source << "{\n";
	source << "\tPHYSFS_file* file = PHYSFS_openRead(path);\n";
	source << "\tif (file == nullptr)\n";
	source << "\t{\n";
	source << "\t\tthrow std::runtime_error(\"Failed to open file: \" + std::string(path) + \": \" + PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));\n";
	source << "\t}\n";
	source << "\tPHYSFS_sint64 fileSize = PHYSFS_fileLength(file);\n";
	source << "\tstd::vector<unsigned char> buffer(fileSize);\n";
	source << "\tPHYSFS_read(file, buffer.data(), 1, fileSize);\n";
	source << "\tPHYSFS_close(file);\n";
	source << "\treturn buffer;\n";
	source << "}\n\n";

	source << "std::string load" << file_name.getValue() << "EmbeddedFileAsString(const char* path)\n";
	source << "{\n";
	source << "\tPHYSFS_file* file = PHYSFS_openRead(path);\n";
	source << "\tif (file == nullptr)\n";
	source << "\t{\n";
	source << "\t\tthrow std::runtime_error(\"Failed to open file: \" + std::string(path) + \": \" + PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));\n";
	source << "\t}\n";
	source << "\tPHYSFS_sint64 fileSize = PHYSFS_fileLength(file);\n";
	source << "\tstd::vector<char> buffer(fileSize);\n";
	source << "\tPHYSFS_read(file, buffer.data(), 1, fileSize);\n";
	source << "\tPHYSFS_close(file);\n";
	source << "\treturn std::string(buffer.begin(), buffer.end());\n";
	source << "}\n\n";


	source << "bool exists" << file_name.getValue() << "EmbeddedFile(const char* path)\n";
	source << "{\n";
	source << "\treturn PHYSFS_exists(path) != 0;\n";
	source << "}\n\n";

	source << "bool is" << file_name.getValue() << "EmbeddedFolder(const char* path)\n";
	source << "{\n";
	source << "\treturn PHYSFS_isDirectory(path) != 0;\n";
	source << "}\n\n";


	source << "bool is" << file_name.getValue() << "EmbeddedFile(const char* path)\n";
	source << "{\n";
	source << "\treturn PHYSFS_isDirectory(path) == 0 && PHYSFS_exists(path) != 0;\n";
	source << "}\n\n";

	// extractTo function
	source << "#include <sys/stat.h>\n";
	source << "#include <sys/types.h>\n";
	source << "#include <fstream>\n";
	source << "#include <vector>\n";
	source << "#include <string>\n";
	source << "#include <cstring>\n";
	source << "#include <cerrno>\n";
	source << "#include <filesystem>\n";
	source << "bool extract" << file_name.getValue() << "To(const char* embeddedPath, const char* realPath) {\n";
	source << "    if (PHYSFS_isDirectory(embeddedPath)) {\n";
	source << "        std::filesystem::create_directories(realPath);\n";
	source << "        char **rc = PHYSFS_enumerateFiles(embeddedPath);\n";
	source << "        char **i;\n";
	source << "        for (i = rc; *i != nullptr; i++) {\n";
	source << "            std::string subEmbedded = std::string(embeddedPath);\n";
	source << "            if (!subEmbedded.empty() && subEmbedded.back() != '/') subEmbedded += '/';\n";
	source << "            subEmbedded += *i;\n";
	source << "            std::string subReal = std::string(realPath);\n";
	source << "            if (!subReal.empty() && subReal.back() != '/') subReal += '/';\n";
	source << "            subReal += *i;\n";
	source << "            if (!extract" << file_name.getValue() << "To(subEmbedded.c_str(), subReal.c_str())) {\n";
	source << "                PHYSFS_freeList(rc);\n";
	source << "                return false;\n";
	source << "            }\n";
	source << "        }\n";
	source << "        PHYSFS_freeList(rc);\n";
	source << "        return true;\n";
	source << "    } else if (PHYSFS_exists(embeddedPath)) {\n";
	source << "        PHYSFS_file* file = PHYSFS_openRead(embeddedPath);\n";
	source << "        if (!file) return false;\n";
	source << "        PHYSFS_sint64 fileSize = PHYSFS_fileLength(file);\n";
	source << "        std::vector<unsigned char> buffer(fileSize);\n";
	source << "        PHYSFS_read(file, buffer.data(), 1, fileSize);\n";
	source << "        PHYSFS_close(file);\n";
	source << "        std::ofstream out(realPath, std::ios::binary);\n";
	source << "        if (!out) return false;\n";
	source << "        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());\n";
	source << "        out.close();\n";
	source << "        return true;\n";
	source << "    }\n";
	source << "    return false;\n";
	source << "}\n";

	source.close();


	printf("Done\n");
	printf("Zip buffer size: %d\n", zipBuffer.size());
	return 0;
}