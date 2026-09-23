#include "surrealwidgets/core/resourcedata.h"
#include <fstream>
#include <stdexcept>
#include <memory>

// Android has no GTK/fontconfig to query for a system UI font (unlike
// resourcedata_unix.cpp, which this file replaces on Android - see
// SurrealWidgets/CMakeLists.txt's ANDROID branch). SurrealEngine's own
// ResourceLoaderPK3 (SurrealEngine/UI/WidgetResourceData.cpp) is installed via
// ResourceLoader::Set() before any of this is needed for the on-headset "big screen"
// menu/console quad, and always answers "system"/"monospace" font requests with its
// own bundled Noto fonts rather than asking the OS - so LoadSystemFont/
// LoadMonospaceSystemFont below are never actually reached in practice on this
// platform. They still need a body since ResourceData::LoadFont() calls them.
static std::vector<uint8_t> ReadAllBytes(const std::string& filename)
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file)
		throw std::runtime_error("Could not open: " + filename);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
		throw std::runtime_error("Could not read: " + filename);

	return buffer;
}

std::vector<SingleFontData> ResourceData::LoadSystemFont()
{
	throw std::runtime_error("No platform system font available on Android - a ResourceLoader must be installed via ResourceLoader::Set() before requesting the \"system\" font");
}

std::vector<SingleFontData> ResourceData::LoadMonospaceSystemFont()
{
	throw std::runtime_error("No platform monospace font available on Android - a ResourceLoader must be installed via ResourceLoader::Set() before requesting the \"monospace\" font");
}

double ResourceData::GetSystemFontSize()
{
	return 11.0;
}

class ResourceLoaderAndroid : public ResourceLoader
{
public:
	std::vector<SingleFontData> LoadFont(const std::string& name) override
	{
		if (name == "system")
			return ResourceData::LoadSystemFont();
		else if (name == "monospace")
			return ResourceData::LoadMonospaceSystemFont();
		else
			return { SingleFontData{ReadAllBytes(name + ".ttf"), ""} };
	}

	std::vector<uint8_t> ReadAllBytes(const std::string& filename) override
	{
		return ::ReadAllBytes(filename);
	}
};

static std::unique_ptr<ResourceLoader>& GetLoader()
{
	static std::unique_ptr<ResourceLoader> loader = std::make_unique<ResourceLoaderAndroid>();
	return loader;
}

ResourceLoader* ResourceLoader::Get()
{
	return GetLoader().get();
}

void ResourceLoader::Set(std::unique_ptr<ResourceLoader> instance)
{
	GetLoader() = std::move(instance);
}
